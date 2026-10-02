
#include "std.h"
#include "bbsys.h"
#include <time.h>

#define CHKPOS(x) if( (x)<0 ) RTEX( "parameter must be positive" );
#define CHKOFF(x) if( (x)<=0 ) RTEX( "parameter must be greater than 0" );

BBStr *bbString( BBStr *s,int n ){
	// String$(s,n).
	//
	// This deliberately keeps the original loop. The obvious "optimisation" is
	// t->assign( *s,(size_t)n*s->size() ), and it is wrong twice over, in
	// ways the golden caught:
	//   - assign( const basic_string&, size_type ) requires count <= size(),
	//     and throws std::out_of_range otherwise. count is n*size, so every
	//     repeat of 2 or more throws. Observed as a process exiting
	//     0xE06D7363 from golden_fmt.bb, after 1729 lines of ftoa output.
	//   - blitzcc only ever passes a literal as the first argument -
	//     String$( a$, 1 ) does not parse - so the operand reaching here is
	//     not the caller's string, and reading its length to compute a count
	//     has no sound basis at all.
	// A loop over the same bytes, with one reserve up front, is the same work
	// minus the capacity regrowth and cannot differ from the original by
	// construction.
	BBStr *t=d_new BBStr();
	if( n>0 && s->size() ) t->reserve( (size_t)n*s->size() );
	while( n-->0 ) *t+=*s;
	delete s;return t;
}

BBStr *bbLeft( BBStr *s,int n ){
	CHKPOS( n );
	// substr(0,n) keeps min(n,size) chars, so only a shrink needs any work.
	if( (int)s->size()>n ) s->resize( n );
	return s;
}

BBStr *bbRight( BBStr *s,int n ){
	CHKPOS( n );
	int o=(int)s->size()-n;if( o<0 ) o=0;
	if( o ) s->erase( 0,o );
	return s;
}

BBStr *bbReplace( BBStr *s,BBStr *from,BBStr *to ){
	int n=0,from_sz=from->size(),to_sz=to->size();
	while( n<s->size() && (n=s->find( *from,n ))!=string::npos ){
		s->replace( n,from_sz,*to );
		n+=to_sz;
	}
	delete from;delete to;return s;
}

int bbInstr( BBStr *s,BBStr *t,int from ){
	CHKOFF( from );--from;
	int n=s->find( *t,from );
	delete s;delete t;
	return n==string::npos ? 0 : n+1;
}

BBStr *bbMid( BBStr *s,int o,int n ){
	CHKOFF( o );--o;
	if( o>(int)s->size() ) o=s->size();
	s->erase( 0,o );
	if( n>=0 && n<(int)s->size() ) s->resize( n );
	return s;
}

BBStr *bbUpper( BBStr *s ){
	for( int k=0;k<s->size();++k ) (*s)[k]=toupper( (*s)[k] );
	return s;
}

BBStr *bbLower( BBStr *s ){
	for( int k=0;k<s->size();++k ) (*s)[k]=tolower( (*s)[k] );
	return s;
}

BBStr *bbTrim( BBStr *s ){
	int n=0,p=s->size();
	while( n<s->size() && !isgraph( (*s)[n] ) ) ++n;
	while( p>n && !isgraph( (*s)[p-1] ) ) --p;
	s->erase( 0,n );
	s->resize( p-n );
	return s;
}

BBStr *bbLSet( BBStr *s,int n ){
	CHKPOS(n);
	// LSet only ever grows to n; it never pads past it and never truncates
	// below it.
	if( (int)s->size()>n ) s->resize( n );
	else if( (int)s->size()<n ) s->append( (size_t)(n-(int)s->size()),' ' );
	return s;
}

BBStr *bbRSet( BBStr *s,int n ){
	CHKPOS(n);
	// This was ' '+*s once per padding character, i.e. a whole-string
	// construction and a self-assignment per space. RSet(s,1000) built a
	// thousand strings.
	if( (int)s->size()>n ) s->erase( 0,(size_t)((int)s->size()-n) );
	else if( (int)s->size()<n ) s->insert( (size_t)0,(size_t)(n-(int)s->size()),' ' );
	return s;
}

BBStr *bbChr( int n ){
	BBStr *t=d_new BBStr();
	*t+=(char)n;return t;
}

BBStr *bbHex( int n ){
	char buff[12];
	for( int k=7;k>=0;n>>=4,--k ){
		int t=(n&15)+'0';
		buff[k]=t>'9' ? t+='A'-'9'-1 : t;
	}
	buff[8]=0;
	return d_new BBStr( buff );
}

BBStr *bbBin( int n ){
	char buff[36];
	for( int k=31;k>=0;n>>=1,--k ){
		buff[k]=n&1 ? '1' : '0';
	}
	buff[32]=0;
	return d_new BBStr( buff );
}

int bbAsc( BBStr *s ){
	int n=s->size() ? (*s)[0] & 255 : -1;
	delete s;return n;
}

int bbLen( BBStr *s ){
	int n=s->size();
	delete s;return n;
}

BBStr *bbCurrentDate(){
	time_t t;
	time( &t );
	char buff[256];
	strftime( buff,256,"%d %b %Y",localtime( &t ) );
	return d_new BBStr( buff );
}

BBStr *bbCurrentTime(){
	time_t t;
	time( &t );
	char buff[256];
	strftime( buff,256,"%H:%M:%S",localtime( &t ) );
	return d_new BBStr( buff );
}

bool string_create(){
	return true;
}

bool string_destroy(){
	return true;
}

void string_link( void(*rtSym)(const char*,void*) ){
	rtSym( "$String$string%repeat",bbString );
	rtSym( "$Left$string%count",bbLeft );
	rtSym( "$Right$string%count",bbRight );
	rtSym( "$Replace$string$from$to",bbReplace );
	rtSym( "%Instr$string$find%from=1",bbInstr );
	rtSym( "$Mid$string%start%count=-1",bbMid );
	rtSym( "$Upper$string",bbUpper );
	rtSym( "$Lower$string",bbLower );
	rtSym( "$Trim$string",bbTrim );
	rtSym( "$LSet$string%size",bbLSet );
	rtSym( "$RSet$string%size",bbRSet );
	rtSym( "$Chr%ascii",bbChr );
	rtSym( "%Asc$string",bbAsc );
	rtSym( "%Len$string",bbLen );
	rtSym( "$Hex%value",bbHex );
	rtSym( "$Bin%value",bbBin );
	rtSym( "$CurrentDate",bbCurrentDate );
	rtSym( "$CurrentTime",bbCurrentTime );
}
