
#ifndef DDUTIL_H
#define DDUTIL_H

#include <ddraw.h>

class gxGraphics;
typedef IDirectDrawSurface7 ddSurf;

struct ddUtil{

	static void buildMipMaps( ddSurf *surf );
	static void copy( ddSurf *dest,int dx,int dy,int dw,int dh,ddSurf *src,int sx,int sy,int sw,int sh );
	static ddSurf *loadSurface( const std::string &f,int flags,gxGraphics *gfx );
	static ddSurf *createSurface( int width,int height,int flags,gxGraphics *gfx );
};

class PixelFormat{
	int depth,pitch;
	unsigned amask,rmask,gmask,bmask,argbfill;
	unsigned char ashr,ashl,rshr,rshl,gshr,gshl,bshr,bshl;
	typedef void (_fastcall *Plot)(void *pix,unsigned argb);
	typedef unsigned (_fastcall *Point)(void *pix);
	Plot plot;
	Point point;

	char *plot_code,*point_code;

public:
	PixelFormat():plot_code(0){
	}

	PixelFormat( const DDPIXELFORMAT &pf ):plot_code(0){
		setFormat( pf );
	}

	~PixelFormat();

	void setFormat( const DDPIXELFORMAT &pf );

	int getDepth()const{
		return depth; 
	}
	int getPitch()const{ 
		return pitch; 
	}
	unsigned fromARGB( unsigned n )const{
		return ( (n>>ashr<<ashl)&amask ) | ( (n>>rshr<<rshl)&rmask ) | ( (n>>gshr<<gshl)&gmask ) | ( (n>>bshr<<bshl)&bmask );
	}
	unsigned toARGB( unsigned n )const{
		return ( (n&amask)>>ashl<<ashr ) | ( (n&rmask)>>rshl<<rshr ) | ( (n&gmask)>>gshl<<gshr ) | ( (n&bmask)>>bshl<<bshr ) | argbfill;
	}
	void setPixel( void *p,unsigned n )const{ plot(p,n); }
	unsigned getPixel( void *p )const{ return point(p); }

	// True when this is a 32bpp format whose point() thunk takes its no-repack
	// fast path, i.e. a plain 32-bit load. Only then is
	// (getPixel(p)&0xffffff) the same number as (*(unsigned*)p&0xffffff), which
	// is what lets callers compare raw pixels instead of calling through the
	// thunk. The condition is copied verbatim from AsmCoder::CodePoint; it is
	// deliberately not widened, because the repack path shifts channel values
	// rather than replicating them and is therefore lossy - e.g. a 5-bit blue
	// of 1 and a blue of 0 both expand to 0.
	bool isPlain32()const{
		return pitch==4 &&
			rmask==0xff0000 && gmask==0xff00 && bmask==0xff &&
			( amask==0 || amask==0xff000000 );
	}

	unsigned getAMask()const{ return amask; }
	unsigned getRMask()const{ return rmask; }
	unsigned getGMask()const{ return gmask; }
	unsigned getBMask()const{ return bmask; }
};

#endif