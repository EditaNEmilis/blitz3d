
#ifndef XFILE_H
#define XFILE_H

#include "std.h"

//A reader for the DirectX 3D file format, the ".x" model files.
//
//The format has two encodings and this reads both of them: the text
//encoding, which is what most exporters write, and the binary encoding,
//which is a token stream carrying the same tree.
//
//Neither encoding needs the DirectX SDK. The SDK's headers are gone and the
//library that used to parse these files, d3dxof.dll, is not installed on
//any current Windows, so a file that needs a model has no way in. The
//format itself is documented, and the tree it holds is small: a template
//instance, its members in a fixed order, and its child templates.
//
//A template's members are read back in template order, so the reader has to
//be told what to expect. The text encoding does not record whether a number
//was written as an integer or as a float, and an exporter is free to write
//1 where a float is meant, so the type comes from the template and not from
//the file.

//One member value of a template.
struct XValue{
	enum{ INT=0,FLOAT=1,STRING=2 };

	int type;
	int i;
	float f;
	string s;

	XValue():type(INT),i(0),f(0){}
};

//A template instance, a child template, or a reference to one.
class XNode{
public:
	string name;      //the template's name, or the name a reference points at
	string inst;      //the instance's name, empty if it has none
	bool reference;   //a "{ name }" reference rather than an object
	vector<XValue> values;
	vector<XNode*> children;

	XNode( const string &n ):name(n),reference(false){}
	~XNode();

	//the first child with the given template name, 0 if there is none
	XNode *findChild( const string &n )const;
};

//The contents of a .x file.
class XFile{
public:
	//reads a file, returns 0 if it cannot be read or is not a .x file
	static XFile *load( const string &filename );
	~XFile();

	const vector<XNode*> &getRoots()const{ return roots; }

	//an object by instance name, 0 if the file has none
	XNode *find( const string &inst )const;

private:
	vector<XNode*> roots;
	map<string,XNode*> by_inst;

	void index( XNode *node );
};

//Reads a template's members in template order.
class XCursor{
public:
	XCursor( XNode *n ):node(n),pos(0){}

	bool atEnd()const{ return pos>=(int)node->values.size(); }

	int readInt();
	float readFloat();
	string readString();

private:
	XNode *node;
	int pos;
};

#endif
