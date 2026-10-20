
#include "std.h"
#include "xfile.h"

//The reader for the DirectX 3D file format.
//
//A file holds a tree of template instances. The text encoding writes that
//tree as text:
//
//  Mesh name {
//    16;
//    -6.0;6.25;6.125;, ...
//    MeshMaterialList { 1; 8; 0;0;0; ... }
//  }
//
//and the binary encoding writes the same tree as a token stream, where the
//tokens carry the types: a TOKEN_INTEGER_LIST is a run of DWORDs and a
//TOKEN_FLOAT_LIST a run of floats.
//
//Both encodings start with a sixteen byte header of four four byte fields:
//"xof ", the version, "txt " or "bin ", and the size of a float in bits.
//
//The members of a template come in a fixed order and the reader has to know
//that order, because the text encoding does not record whether a number was
//meant as an integer or as a float.

//The tokens of the binary encoding.
static const int XTOKEN_NAME=1;
static const int XTOKEN_STRING=2;
static const int XTOKEN_INTEGER=3;
static const int XTOKEN_GUID=5;
static const int XTOKEN_INTEGER_LIST=6;
static const int XTOKEN_FLOAT_LIST=7;
static const int XTOKEN_OBRACE=10;
static const int XTOKEN_CBRACE=11;
static const int XTOKEN_COMMA=19;
static const int XTOKEN_SEMICOLON=20;
static const int XTOKEN_TEMPLATE=31;
static const int XTOKEN_FLOAT=42;
static const int XTOKEN_DOUBLE=43;

//The tokens of the text encoding.
enum{
	XT_EOF=0,XT_NUM,XT_STR,XT_ID,XT_OBRACE,XT_CBRACE,XT_ANGLE,XT_OBRACKET,XT_CBRACKET
};

struct XToken{
	int type;
	double num;
	bool is_int;
	string str;

	XToken():type(XT_EOF),num(0),is_int(false){}
};

class XParser{
public:
	XParser( const string &filename );
	~XParser();

	bool parse();
	vector<XNode*> roots;

private:
	vector<char> data;
	int pos;
	bool binary;
	int float_bits;
	bool failed;

	//pushback for the text lexer
	int push_char;
	bool has_push_char;
	XToken pending;
	bool has_pending;

	//the file
	int lexChar();
	void unlexChar( int c );
	int readWord();
	int readDword();
	float readFloat();
	double readDouble();
	string readName();
	void skipBytes( int n );

	//the binary encoding
	int nextToken();
	void pushToken( int tok );
	XNode *parseBinaryObject();
	XNode *finishBinaryObject( const string &name );
	bool parseBinaryBody( XNode *node );
	bool skipBinaryTemplate();

	//the text encoding
	int lex( XToken &t );
	XNode *parseTextObject( const string &name );
	bool skipTextTemplate();
};

XParser::XParser( const string &filename ):
	pos(0),binary(false),float_bits(32),failed(false),
	push_char(-1),has_push_char(false),has_pending(false){
	//the C runtime's file functions, like the other loaders use, rather
	//than the streams
	FILE *f=fopen( filename.c_str(),"rb" );
	if( !f ) return;
	fseek( f,0,SEEK_END );
	long size=ftell( f );
	fseek( f,0,SEEK_SET );
	if( size<=0 ){ fclose( f ); return; }
	data.resize( (size_t)size );
	if( fread( &data[0],1,(size_t)size,f )!=(size_t)size ){
		data.clear();
	}
	fclose( f );
}

XParser::~XParser(){
	for( int k=0;k<(int)roots.size();++k ) delete roots[k];
}

//--- the file -------------------------------------------------------------

int XParser::lexChar(){
	if( has_push_char ){
		has_push_char=false;
		return push_char;
	}
	if( pos>=(int)data.size() ) return -1;
	return (unsigned char)data[pos++];
}

void XParser::unlexChar( int c ){
	if( c<0 ) return;
	push_char=c;
	has_push_char=true;
}

void XParser::skipBytes( int n ){
	pos+=n;
	if( pos>(int)data.size() ) pos=data.size();
}

int XParser::readWord(){
	if( pos+2>(int)data.size() ){ failed=true; return 0; }
	int v=(unsigned char)data[pos]|(unsigned char)data[pos+1]<<8;
	pos+=2;
	return v;
}

int XParser::readDword(){
	if( pos+4>(int)data.size() ){ failed=true; return 0; }
	int v=(unsigned char)data[pos]|(unsigned char)data[pos+1]<<8|
		(unsigned char)data[pos+2]<<16|(unsigned char)data[pos+3]<<24;
	pos+=4;
	return v;
}

float XParser::readFloat(){
	if( pos+4>(int)data.size() ){ failed=true; return 0; }
	float v;
	memcpy( &v,&data[pos],4 );
	pos+=4;
	return v;
}

double XParser::readDouble(){
	if( pos+8>(int)data.size() ){ failed=true; return 0; }
	double v;
	memcpy( &v,&data[pos],8 );
	pos+=8;
	return v;
}

string XParser::readName(){
	int n=readDword();
	if( n<0 || pos+n>(int)data.size() ){ failed=true; return ""; }
	string s( &data[pos],n );
	pos+=n;
	return s;
}

//--- the binary encoding -------------------------------------------------

int XParser::nextToken(){
	if( has_pending ){ has_pending=false; return pending.type; }
	return readWord();
}

void XParser::pushToken( int tok ){
	pending.type=tok;
	has_pending=true;
}

//A template definition in the binary encoding. It is a block of type
//declarations that says how the data of an object is laid out, and this
//reader does not need it: the templates it knows are the standard ones and
//their layout is fixed. So the block is counted through and dropped.
bool XParser::skipBinaryTemplate(){
	int tok=nextToken();
	if( tok!=XTOKEN_NAME ) return false;
	readName();
	tok=nextToken();
	if( tok!=XTOKEN_OBRACE ) return false;

	int depth=1;
	while( depth>0 ){
		if( pos+2>(int)data.size() ) return false;
		tok=nextToken();
		if( tok==XTOKEN_OBRACE ) ++depth;
		else if( tok==XTOKEN_CBRACE ) --depth;
		else if( tok==XTOKEN_NAME ) readName();
		else if( tok==XTOKEN_INTEGER ) readDword();
		else if( tok==XTOKEN_GUID ) skipBytes( 16 );
		else if( tok==XTOKEN_TEMPLATE ){
			if( !skipBinaryTemplate() ) return false;
		}
	}
	return true;
}

XNode *XParser::finishBinaryObject( const string &name ){
	XNode *node=d_new XNode( name );

	//an instance name may follow the template name
	int tok=nextToken();
	if( tok==XTOKEN_NAME ) node->inst=readName();
	else pushToken( tok );

	tok=nextToken();
	if( tok!=XTOKEN_OBRACE ){
		delete node;
		failed=true;
		return 0;
	}

	if( !parseBinaryBody( node ) ){
		delete node;
		return 0;
	}
	return node;
}

bool XParser::parseBinaryBody( XNode *node ){
	for( ;; ){
		int tok=nextToken();
		if( tok==XTOKEN_CBRACE ) return true;

		if( tok==XTOKEN_NAME ){
			//a child object
			string name=readName();
			XNode *child=finishBinaryObject( name );
			if( !child ) return false;
			node->children.push_back( child );
			continue;
		}

		if( tok==XTOKEN_OBRACE ){
			//a reference to an object named earlier in the file
			int t2=nextToken();
			if( t2!=XTOKEN_NAME ){ failed=true; return false; }
			XNode *ref=d_new XNode( readName() );
			ref->reference=true;
			int t3=nextToken();
			if( t3==XTOKEN_GUID ) skipBytes( 16 );
			else pushToken( t3 );
			if( nextToken()!=XTOKEN_CBRACE ){
				delete ref;
				failed=true;
				return false;
			}
			node->children.push_back( ref );
			continue;
		}

		if( tok==XTOKEN_INTEGER_LIST ){
			int n=readDword();
			for( int k=0;k<n;++k ){
				XValue v;
				v.type=XValue::INT;
				v.i=readDword();
				node->values.push_back( v );
			}
			continue;
		}

		if( tok==XTOKEN_FLOAT_LIST ){
			int n=readDword();
			for( int k=0;k<n;++k ){
				XValue v;
				v.type=XValue::FLOAT;
				v.f=float_bits==64 ? (float)readDouble() : readFloat();
				node->values.push_back( v );
			}
			continue;
		}

		if( tok==XTOKEN_STRING ){
			string s=readName();
			nextToken();//the terminator
			XValue v;
			v.type=XValue::STRING;
			v.s=s;
			node->values.push_back( v );
			continue;
		}

		if( tok==XTOKEN_INTEGER ){
			XValue v;
			v.type=XValue::INT;
			v.i=readDword();
			node->values.push_back( v );
			continue;
		}

		if( tok==XTOKEN_FLOAT ){
			XValue v;
			v.type=XValue::FLOAT;
			v.f=readFloat();
			node->values.push_back( v );
			continue;
		}

		if( tok==XTOKEN_GUID ){
			skipBytes( 16 );
			continue;
		}

		if( tok==XTOKEN_TEMPLATE ){
			if( !skipBinaryTemplate() ){ failed=true; return false; }
			continue;
		}

		if( tok==XTOKEN_COMMA || tok==XTOKEN_SEMICOLON ) continue;

		failed=true;
		return false;
	}
}

XNode *XParser::parseBinaryObject(){
	int tok=nextToken();
	if( tok==XTOKEN_TEMPLATE ){
		if( !skipBinaryTemplate() ) failed=true;
		return 0;
	}
	if( tok!=XTOKEN_NAME ){ failed=true; return 0; }
	return finishBinaryObject( readName() );
}

//--- the text encoding ----------------------------------------------------

int XParser::lex( XToken &t ){
	if( has_pending ){
		t=pending;
		has_pending=false;
		return t.type;
	}

	for( ;; ){
		int c=lexChar();
		if( c<0 ){ t.type=XT_EOF; return t.type; }

		if( c==' ' || c=='\t' || c=='\r' || c=='\n' || c==',' || c==';' ) continue;

		if( c=='/' ){
			int c2=lexChar();
			if( c2=='/' ){
				while( (c=lexChar())>=0 && c!='\n' ){}
				continue;
			}
			if( c2=='*' ){
				int prev=0;
				for( ;; ){
					int c3=lexChar();
					if( c3<0 ) break;
					if( prev=='*' && c3=='/' ) break;
					prev=c3;
				}
				continue;
			}
			unlexChar( c2 );
			failed=true;
			t.type=XT_EOF;
			return t.type;
		}

		if( c=='{' ){ t.type=XT_OBRACE; return t.type; }
		if( c=='}' ){ t.type=XT_CBRACE; return t.type; }

		//the brackets of a template's optional member list, which is
		//"[...]" and only appears in a template definition
		if( c=='[' ){ t.type=XT_OBRACKET; return t.type; }
		if( c==']' ){ t.type=XT_CBRACKET; return t.type; }

		if( c=='<' ){
			//a class id, which is not needed
			int c2;
			while( (c2=lexChar())>=0 && c2!='>' ){}
			t.type=XT_ANGLE;
			return t.type;
		}

		if( c=='"' ){
			string s;
			int c2;
			while( (c2=lexChar())>=0 && c2!='"' ) s+=char( c2 );
			t.type=XT_STR;
			t.str=s;
			return t.type;
		}

		if( isdigit(c) || c=='-' || c=='+' || c=='.' ){
			string s;
			s+=char( c );
			for( ;; ){
				int c2=lexChar();
				if( c2<0 ) break;
				if( isdigit(c2) || c2=='.' || c2=='e' || c2=='E' || c2=='+' || c2=='-' ){
					s+=char( c2 );
				}else{
					unlexChar( c2 );
					break;
				}
			}
			t.type=XT_NUM;
			t.num=atof( s.c_str() );
			t.is_int=s.find('.')==string::npos && s.find('e')==string::npos && s.find('E')==string::npos;
			return t.type;
		}

		if( isalpha(c) || c=='_' ){
			string s;
			s+=char( c );
			for( ;; ){
				int c2=lexChar();
				if( c2<0 ) break;
				if( isalnum(c2) || c2=='_' || c2=='-' ){
					s+=char( c2 );
				}else{
					unlexChar( c2 );
					break;
				}
			}
			t.type=XT_ID;
			t.str=s;
			return t.type;
		}

		failed=true;
		t.type=XT_EOF;
		return t.type;
	}
}

//A template definition in the text encoding. Same as the binary one: a
//block of type declarations, dropped.
bool XParser::skipTextTemplate(){
	XToken t;
	int type=lex( t );
	if( type!=XT_ID ) return false;
	type=lex( t );
	if( type!=XT_OBRACE ) return false;

	int depth=1;
	while( depth>0 ){
		type=lex( t );
		if( type==XT_EOF ) return false;
		if( type==XT_OBRACE ) ++depth;
		else if( type==XT_CBRACE ) --depth;
	}
	return true;
}

XNode *XParser::parseTextObject( const string &name ){
	XNode *node=d_new XNode( name );

	XToken t;
	int type=lex( t );
	if( type==XT_ID ){
		//an instance name
		node->inst=t.str;
		type=lex( t );
	}
	if( type!=XT_OBRACE ){
		delete node;
		failed=true;
		return 0;
	}

	for( ;; ){
		type=lex( t );
		if( type==XT_EOF ){
			delete node;
			failed=true;
			return 0;
		}
		if( type==XT_CBRACE ) break;

		if( type==XT_OBRACE ){
			//a reference to an object named earlier in the file
			int t2=lex( t );
			if( t2!=XT_ID ){
				delete node;
				failed=true;
				return 0;
			}
			XNode *ref=d_new XNode( t.str );
			ref->reference=true;
			int t3=lex( t );
			if( t3==XT_ANGLE ) t3=lex( t );
			if( t3!=XT_CBRACE ){
				delete ref;
				delete node;
				failed=true;
				return 0;
			}
			node->children.push_back( ref );
			continue;
		}

		if( type==XT_ID ){
			//a child object
			XNode *child=parseTextObject( t.str );
			if( !child ){
				delete node;
				return 0;
			}
			node->children.push_back( child );
			continue;
		}

		if( type==XT_NUM ){
			XValue v;
			if( t.is_int ){
				v.type=XValue::INT;
				v.i=(int)t.num;
			}else{
				v.type=XValue::FLOAT;
				v.f=(float)t.num;
			}
			node->values.push_back( v );
			continue;
		}

		if( type==XT_STR ){
			XValue v;
			v.type=XValue::STRING;
			v.s=t.str;
			node->values.push_back( v );
			continue;
		}

		if( type==XT_ANGLE ) continue;//a class id
		if( type==XT_OBRACKET || type==XT_CBRACKET ) continue;//an optional member list

		delete node;
		failed=true;
		return 0;
	}

	return node;
}

//--- the file as a whole --------------------------------------------------

bool XParser::parse(){
	if( data.size()<16 ) return false;

	//the header: "xof ", the version, "txt " or "bin ", the float size
	if( !(data[0]=='x' && data[1]=='o' && data[2]=='f' && data[3]==' ') ) return false;
	string fmt( &data[8],4 );
	string bits( &data[12],4 );
	float_bits=atoi( bits.c_str() );
	if( float_bits!=32 && float_bits!=64 ) float_bits=32;
	pos=16;

	if( fmt.substr(0,3)=="bin" ){
		binary=true;
		for( ;; ){
			if( pos>=(int)data.size() ) break;
			XNode *node=parseBinaryObject();
			if( node ){
				roots.push_back( node );
			}else if( failed ){
				break;
			}
		}
	}else{
		binary=false;
		for( ;; ){
			XToken t;
			int type=lex( t );
			if( type==XT_EOF ) break;
			if( type==XT_CBRACE ){
				//the file closes more braces than it opens. Some exporters
				//write one too many, and everything up to here parsed.
				break;
			}
			if( type!=XT_ID ){ failed=true; break; }
			if( t.str=="template" ){
				if( !skipTextTemplate() ){ failed=true; break; }
				continue;
			}
			XNode *node=parseTextObject( t.str );
			if( !node ){ failed=true; break; }
			roots.push_back( node );
		}
	}

	return !roots.empty();
}

//--- the file's contents --------------------------------------------------

XNode::~XNode(){
	for( int k=0;k<(int)children.size();++k ) delete children[k];
}

XNode *XNode::findChild( const string &n )const{
	for( int k=0;k<(int)children.size();++k ){
		if( children[k]->name==n ) return children[k];
	}
	return 0;
}

XFile *XFile::load( const string &filename ){
	XParser parser( filename );
	if( !parser.parse() ) return 0;

	XFile *file=d_new XFile;
	file->roots.swap( parser.roots );
	for( int k=0;k<(int)file->roots.size();++k ) file->index( file->roots[k] );
	return file;
}

XFile::~XFile(){
	for( int k=0;k<(int)roots.size();++k ) delete roots[k];
}

void XFile::index( XNode *node ){
	if( node->inst.size() ) by_inst[node->inst]=node;
	for( int k=0;k<(int)node->children.size();++k ) index( node->children[k] );
}

XNode *XFile::find( const string &inst )const{
	map<string,XNode*>::const_iterator it=by_inst.find( inst );
	if( it==by_inst.end() ) return 0;
	return it->second;
}

int XCursor::readInt(){
	if( pos>=(int)node->values.size() ) return 0;
	const XValue &v=node->values[pos++];
	if( v.type==XValue::FLOAT ) return (int)v.f;
	if( v.type==XValue::STRING ) return 0;
	return v.i;
}

float XCursor::readFloat(){
	if( pos>=(int)node->values.size() ) return 0;
	const XValue &v=node->values[pos++];
	if( v.type==XValue::INT ) return (float)v.i;
	if( v.type==XValue::STRING ) return 0;
	return v.f;
}

string XCursor::readString(){
	if( pos>=(int)node->values.size() ) return "";
	const XValue &v=node->values[pos++];
	return v.s;
}
