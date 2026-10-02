
#include "std.h"
#include "gxfont.h"
#include "gxcanvas.h"
#include "gxgraphics.h"

gxFont::gxFont( gxGraphics *g,gxCanvas *c,int w,int h,int b,int e,int d,int *os,int *ws ):
graphics(g),canvas(c),
width(w),height(h),begin_char(b),end_char(e),def_char(d),
offs(os),widths(ws),last_color(0),last_width(0){
	canvas->setMask( 0xffffff );
	t_canvas=graphics->createCanvas( graphics->getWidth(),height,0 );
}

gxFont::~gxFont(){
	delete[] offs;
	delete[] widths;
	graphics->freeCanvas( t_canvas );
	graphics->freeCanvas( canvas );
}

int gxFont::charWidth( int c )const{
	if( c<begin_char || c>=end_char ) c=def_char;
	return widths[c-begin_char];
}

void gxFont::render( gxCanvas *dest,unsigned color_argb,int x,int y,const string &t,int from,int to ){

	// Hit: same colour, same characters, in the order they were last rendered.
	// t_canvas still holds that bitmap, so go straight to the one blit.
	if( last_width && color_argb==last_color &&
		(int)last_text.size()==to-from &&
		t.compare( from,to-from,last_text )==0 ){
		dest->blit( x,y,t_canvas,0,0,last_width,height,false );
		return;
	}

	int w=getWidth( t,from,to );
	if( w>t_canvas->getWidth() ){
		graphics->freeCanvas( t_canvas );
		t_canvas=graphics->createCanvas( w,height,0 );
		last_text.clear();			//the old bitmap went with the old canvas
	}

	t_canvas->setColor( color_argb );
	if( !(t_canvas->getColor()&0xffffff) ) t_canvas->setColor( 0x10 );
	t_canvas->rect( 0,0,w,height,true );

	int t_x=0;
	for( int k=from;k<to;++k ){
		int c=t[k]&0xff;
		if( c<begin_char || c>=end_char ) c=def_char;
		c-=begin_char;
		int sx=(offs[c]>>16)&0xffff,sy=offs[c]&0xffff;
		t_canvas->blit( t_x,0,canvas,sx,sy,widths[c],height,false );
		t_x+=widths[c];
	}

	dest->blit( x,y,t_canvas,0,0,w,height,false );

	last_text.assign( t,from,to-from );
	last_color=color_argb;
	last_width=w;
}

int gxFont::getWidth()const{
	return width;
}

int gxFont::getHeight()const{
	return height;
}

int gxFont::getWidth( const string &t )const{
	return getWidth( t,0,t.size() );
}

int gxFont::getWidth( const string &t,int from,int to )const{
	int w=0;
	for( int k=from;k<to;++k ){
		int c=t[k]&0xff;
		if( c<begin_char || c>=end_char ) c=def_char;
		w+=widths[c-begin_char];
	}
	return w;
}

bool gxFont::isPrintable( int chr )const{
	return chr>=begin_char && chr<end_char;
}
