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
target_link_libraries( gxruntime PUBLIC config stdutil fmod freeimage )
target_link_libraries( gxruntime PRIVATE ddraw dinput8 dsound winmm quartz ole32 uuid dxguid )
