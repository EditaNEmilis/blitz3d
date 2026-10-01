
// Stand-in for the DirectX .x model reader.
//
// blitz3d/loader_x.cpp is the original implementation and is left untouched.
// It needs <dxfile.h>, <rmxftmpl.h> and <rmxfguid.h> from the DirectX SDK, and
// d3dxof.dll at run time. Microsoft retired the full DirectX SDK download in
// favour of the End-User Runtime, which carries no headers and no d3dxof, so
// that file cannot be compiled on a current toolchain.
//
// LoadModel() still dispatches the ".x" extension here (bbruntime/bbblitz3d.cpp),
// so the symbol has to exist; it reports the file as unloadable. Put the DirectX
// SDK headers and d3dxof.dll back, and list loader_x.cpp instead of this file in
// blitz3d/CMakeLists.txt, to get .x support again.

#include "std.h"
#include "loader_x.h"
#include "meshmodel.h"

MeshModel *Loader_X::load( const string &filename,const Transform &t,int hint ){
	return 0;
}
