
#ifndef GXFONT_H
#define GXFONT_H

class gxCanvas;
class gxGraphics;

typedef IDirectDrawSurface7 ddSurf;

class gxFont{
public:
	gxFont(
		gxGraphics *graphics,gxCanvas *canvas,
		int width,int height,int begin_char,int end_char,int def_char,
		int *offs,int *widths );
	~gxFont();

	int charWidth( int c )const;
	void render( gxCanvas *dest,unsigned color_argb,int x,int y,const std::string &t,int from,int to );

private:
	gxGraphics *graphics;
	gxCanvas *canvas,*t_canvas;
	int width,height,begin_char,end_char,def_char;
	int *offs,*widths;

	// One-entry memo of the last thing rendered into t_canvas. Redrawing a
	// HUD line every frame is the common case, and without this it pays a
	// background fill plus one colorkeyed blit per character plus a final blit
	// every single time, to produce a bitmap that already exists. t_canvas is
	// only ever written by render(), so "t_canvas already holds this string in
	// this colour" is a complete and cheap validity test.
	std::string last_text;
	unsigned last_color;
	int last_width;

	/***** GX INTERFACE *****/
public:
	enum{
		FONT_BOLD=1,
		FONT_ITALIC=2,
		FONT_UNDERLINE=4
	};

	//ACCESSORS
	int getWidth()const;							//width of widest char
	int getHeight()const;							//height of font
	int getWidth( const std::string &text )const;	//width of string
	int getWidth( const std::string &text,int from,int to )const;	//width of text[from,to)
	bool isPrintable( int chr )const;				//printable char?
};

#endif