# gxruntime: the device layer. DirectDraw 7 surfaces, DirectInput 8, FMOD Ex
# audio, DirectShow playback, FreeImage texture loading and the x86 software
# rasteriser.
b3_lib( gxruntime gxruntime
	ddutil.cpp ddutil.h
	gxaudio.cpp gxaudio.h
	gxcanvas.cpp gxcanvas.h
	gxchannel.cpp gxchannel.h
	gxdevice.cpp gxdevice.h
	gxdir.cpp gxdir.h
	gxfilesystem.cpp gxfilesystem.h
	gxfont.cpp gxfont.h
	gxgraphics.cpp gxgraphics.h
	gxinput.cpp gxinput.h
	gxlight.cpp gxlight.h
	gxmesh.cpp gxmesh.h
	gxmovie.cpp gxmovie.h
	gxmusic.cpp gxmusic.h
	gxruntime.cpp gxruntime.h
	gxscene.cpp gxscene.h
	gxsound.cpp gxsound.h
	gxtimer.cpp gxtimer.h
	std.cpp std.h
	asmcoder.h )

target_compile_definitions( gxruntime PRIVATE _LIB )

# Bounds checks inside gxCanvas's collision bitmask loops. They sit in the inner
# loop of CollideImage, once per 32 pixels and once per collide word, so they
# are not free. cmake/b3.cmake defines NDEBUG for every configuration of every
# target, so keying these off NDEBUG would silently drop them from debug builds
# as well; this is an explicit opt-in instead. Default OFF.
option( GX_BITMASK_DEBUG "Bounds-check the gxCanvas collision bitmask loops" OFF )
if( GX_BITMASK_DEBUG )
	target_compile_definitions( gxruntime PRIVATE GX_BITMASK_DEBUG )
endif()

target_link_libraries( gxruntime PUBLIC config stdutil fmod freeimage )
target_link_libraries( gxruntime PRIVATE ddraw dinput8 dsound winmm quartz ole32 uuid dxguid )
