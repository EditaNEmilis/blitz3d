# Building Blitz3D with a modern toolchain

The original build in `README.TXT` is an MSVC 6.0 workspace. That compiler is
long gone, and the IDE plus debugger need MFC, which no longer pairs with the
free Visual Studio SKUs. This tree builds the same fourteen projects with
Visual Studio 2019/2022 through CMake and Ninja, with no source-level change
beyond the fixes listed further down.

## What you need

| Requirement | Notes |
|---|---|
| Visual Studio 2019 or 2022 | Desktop development with C++ |
| MFC | The `atlmfc` component. The build fails with a clear message if it is absent. |
| Windows 10/11 SDK | Comes with Visual Studio |
| CMake 3.20+ | |
| Ninja | Bundled with VS, or `pip install ninja` |
| `freeimage241/` | See below |
| `fmodapi375win/` | See below |

## The two third-party trees

Both belong *inside* this directory, next to the project folders, exactly as
`README.TXT` step 1 and 2 describe.

**`freeimage241/`** is a git clone of Blitz Research's FreeImage fork. It ships
the prebuilt `Source/FreeImageLib/Release/FreeImage.lib`, so nothing needs to be
compiled. One header was fixed (see `FreeImage.h` below).

**`fmodapi375win/`** is no longer downloadable. The URL in `README.TXT`,
`http://www.fmod.org/files/public/fmodapi375win.zip`, now returns the fmod.com
single-page app as a 453-byte HTML file, and the `fmod/fmodex` GitHub repository
returns 404. The genuine archive is preserved by the Internet Archive:

```
https://web.archive.org/web/2018id_/http://www.fmod.org/files/public/fmodapi375win.zip
```

That is 2,005,928 bytes and expands to the directory layout the build expects
(`api/inc/fmod.h`, `api/lib/fmodvc.lib`, `api/fmod.dll`). It is the only FMOD Ex
3.75 build that still exists anywhere, so the import library is 32-bit, MBCS and
`__stdcall`, which is what the generated Blitz code expects.

## Build

```
tools\build.cmd
```

That sets up a 32-bit developer shell, configures into `build\` and installs the
binaries into `_release\`. Useful variations:

```
tools\build.cmd --clean     wipe build\ first
tools\build.cmd --debug     debug build
```

By hand:

```
call "%VSINSTALLDIR%VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build -G Ninja
cmake --build build
```

The x86 environment is mandatory. Blitz3D is a 32-bit application end to end,
from the generated x86 code to the prebuilt FreeImage and FMOD import libraries,
and the build refuses to configure for x64.

## Output

```
_release\Blitz3D.exe      the launcher
_release\bin\ide.exe      the IDE
_release\bin\blitzcc.exe  the command line compiler
_release\bin\runtime.dll  the runtime loaded by compiled programs
_release\bin\linker.dll   the linker's DLL wrapper
_release\bin\debugger.dll the debugger front end
_release\bin\fmod.dll     audio, staged for the runtime to load
```

## Verifying

Four scripts check the result. Two of them answer "does it still work", two
answer "did it get faster" and "is it still correct".

### Does it still work

`tools\smoketest.ps1` compiles and runs thirteen small Blitz programs and
checks the exit code of each. Twelve must exit 0; the thirteenth asserts a
documented environment limit (see below).

`tools\rungame.ps1 <game-dir>` compiles and runs a real game from `_release`,
which exercises texture loading, mesh loading, the world renderer and input.
Games that pass:

```
tools\rungame.ps1 _release\Games\bb3d_asteroids
tools\rungame.ps1 _release\Games\TunnelRun
tools\rungame.ps1 _release\samples\birdie\dominos
```

### Is it still correct

`tools\smoketest.ps1` and `tools\rungame.ps1` both answer the same question:
does it run and exit cleanly. That is the wrong question for most changes to
this engine, because the expensive rewrites are exactly the ones whose failure
mode is a plausible-looking wrong number rather than a crash. Rewriting float
formatting, rewriting the string library and replacing the `Matrix`/`Transform`
scratch rings all either work or produce garbage that looks fine.

`tools\verify.ps1` is the gate for those. It runs three programs in
`tools\golden\` and compares their output byte for byte against
`tools\golden\expected\`:

| program | pins |
|---|---|
| `golden_core.bb` | entity/transform math, `TFormPoint`/`TFormVector`/`TFormNormal`, `EntityDistance`, matrix elements, mesh fitting. 768 lines. This is the only gate for `geom.h`. |
| `golden_fmt.bb` | `ftoa` over a dense sweep plus every documented edge case of `Left`/`Right`/`Mid`/`Trim`/`LSet`/`RSet`/`String$`. 1936 lines. |
| `golden_collide.bb` | sphere/box/ellipsoid/polygon collision, every response mode, contact points and normals. 1241 lines. This is the gate for any collision broadphase. |
| `golden_x.bb` | the `.x` model loader: a cube it writes itself, in the text encoding, with normals and texture coordinates, and a real model from the release tree with a frame hierarchy, a material list and a texture. 24 lines. This is the gate for the `.x` reader. |

```
tools\verify.ps1                  check
tools\verify.ps1 -Record          re-record, only when the current behaviour is known-good
tools\verify.ps1 -Only collide    one case
```

Do not `-Record` to make a diff go away. A diff is a bug until proven
otherwise.

### Did it get faster

`tools\bench.ps1` runs the programs in `tools\bench\`, which cover Blitz BASIC
execution, the 2D canvas, the scene graph and collision, and blitzcc's own
compile time, then reports operations per second against
`tools\bench-baseline.txt`.

```
tools\bench.ps1                                  report
tools\bench.ps1 -Baseline tools\bench-baseline.txt   report with deltas
tools\bench.ps1 -SaveBaseline <file>            re-record
tools\bench.ps1 -Only bb_exec                   one case
```

Every case reports the **minimum** of several internal repetitions, and that
is not a stylistic choice. Blitz3D's register allocator emits no loop
alignment, so the measured cost of a small hot loop depends on the address it
happens to land at: adding one unused function ahead of a benchmark moved a
1M-iteration eight-argument call loop from 8ms to 156ms on the same machine,
same runtime, same source. A mean of a few runs measures code layout as much
as it measures code. The consequence is that a change which alters generated
code layout can move a microbenchmark by an order of magnitude for reasons
unrelated to the optimisation, so those cases are marked layout-sensitive in
the baseline file and any result for them has to be confirmed against a
whole-program frame time.

### B3_ALLOW_NO_3D_DEVICE

`tools\bench.ps1` and `tools\verify.ps1` both set this.

`gxGraphics::createScene()` asks DirectDraw 7 for `IID_IDirect3D7` and
enumerates devices; every Windows since 8 dropped the D3D7 HAL, so it returns
0 on any current machine and `Graphics3D` raises "Unable to create 3D Scene".
That used to leave `World` null too, and the first `UpdateWorld` then
dereferenced it - `bbUpdateWorld`'s `debug3d()` only reports that under the
debugger, so a release build access-violated instead of raising "3D Graphics
mode not set".

`blitz3d_open()` now builds the `World` before it asks for a device, since
entities, transforms, collision, animation and picking are all engine-side and
need no device. With this variable set, a failed `createScene` is not fatal,
so the engine core can be measured and golden-tested on a machine that cannot
render 3D at all. Games never set it and still get the original error.

Note what this does *not* give you: the D3D7 submission path in
`gxruntime/gxscene.cpp` is still unmeasurable here, and `gxScene::clear`,
`render` and `end` still dereference `dir3dDev` without a null check.

## Quick reference

```
tools\build.cmd                        configure, build, stage into _release\
tools\build.cmd --clean                wipe build\ first
tools\gate.ps1                         build + verify + smoke + bench, one verdict
tools\smoketest.ps1                    13 small programs, exit codes
tools\rungame.ps1 <dir>                one real game
tools\verify.ps1                       golden transcripts
tools\bench.ps1                        benchmarks against the baseline
tools\probe.ps1 -Dir <project>         compile one project with cl, no relink
tools\why.ps1 <exe>                    run it, dump the text of its windows
```

`tools\gate.ps1` is the one to run after any change. It builds, checks the
golden transcripts, runs the smoke test and re-runs the benchmarks against the
baseline, and prints a single pass or fail. Anything short of that is a
change nobody has checked.

`tools\probe.ps1 -Dir <project>` compiles a single project with `cl`
directly and summarises the errors. It was the porting aid; it is kept
because it is the quickest way to see whether a source change breaks one
project without relinking the world.

## Why the build flags matter

`/Gz` is the flag that decides whether the port works. Every `.dsp` in the
original workspace carried it, and it is not in the README because it was
invisible in VC6's project UI.

MSVC's default calling convention for C++ on x86 is `__cdecl`, but blitzcc's
code generator emits bare `call _bbStrConst` with no stack cleanup, because it
assumes every runtime function is `__stdcall`. Build the runtime as `__cdecl`
and nothing fails to compile. Instead, every nested call leaves its arguments
on the stack, the frame a call's argument list was built in slides away, and
`__bbStrConcat` ends up handed the integer `5` where the first string operand
belongs:

```blitz
Local n = 5
Print "a=" + n    ' access violation, __bbStrConcat(s1=0x00000005, ...)
```

Constant folding hides this, which is why `Print "a=" + 5` works and looks like
the feature is fine. The fix is one flag, in `cmake/b3.cmake`:

```cmake
target_compile_options( ${target} PRIVATE /W3 /Oi /Gy /Gz )
```

## Source changes

Everything below is a genuine incompatibility with a current toolchain, not
tidying.

### Calling conventions and the compiler

* `compiler/stmtnode.cpp`, `blitz/libs.cpp` — `for( int k=... )` followed by
  use of `k` outside the loop. VC6 leaked the for-init declaration into the
  enclosing block; that is not standard C++ and MSVC now rejects it. Hoisted.
* `blitz/main.cpp` — `#undef environ`. `<stdlib.h>` maps `environ` to
  `(*__p__environ())`, so the local `Environ *environ` in `main` parsed as a
  redeclaration of that CRT accessor.
* `stdutil/stdutil.cpp`, `debugger/stdutil.cpp` — the file-local `_finite` and
  `_isnan` helpers collide with the CRT's `__cdecl` versions now that the
  default is `__stdcall`. Renamed to `b3finite` / `b3isnan`.

### The DirectPlay 4 interface

The DirectX SDK is not redistributable and Microsoft retired the download in
favour of the End-User Runtime, which carries neither headers nor `d3dxof`, so
`<dplay.h>` and `<dplobby.h>` cannot be had. `compat/dplay.h` and
`compat/dplobby.h` are those two headers written out from the published
interface, and they replace shims that declared the interfaces abstract so
that no vtable layout could be got wrong. Nothing was ever called through
those shims, so nothing caught the fact that their structures and constants
were invented: `DPSESSIONDESC2` had the wrong field order, `DPNAME` had the
wrong field order, `DPID_SYSMSG` was the wrong value, and the `DPSESSION_*`
flags were the wrong values. A program that used the multiplayer commands
would have talked to the wrong fields.

The vtable order is the part that has to be exactly right, and it is not the
order the methods are usually listed in. Each version of the interface added
its methods in one block, and each block is in name order: `IDirectPlay4` is
the IUnknown methods, then the methods `IDirectPlay2` added, then
`IDirectPlay3`'s, then `IDirectPlay4`'s. `IDirectPlayLobby3` is laid out the
same way, with the lobby's own methods, then `IDirectPlayLobby2`'s, then
`IDirectPlayLobby3`'s. A call one slot out lands in the middle of somebody
else's function, so the order in the header is the whole point of it.

The library is still here. `dplayx.dll` ships with Windows, the 32-bit one in
`SysWOW64`, and it exports `DirectPlayCreate`, `DirectPlayEnumerate`,
`DirectPlayLobbyCreateA` and the COM entry points. What is gone is the
registration: `CLSID_DirectPlay` is not in the registry, so
`CoCreateInstance` cannot be used.

Two consequences shape `bbruntime/multiplay_setup.cpp`:

* The interface comes from the library's own class factory,
  `DllGetClassObject` and `IClassFactory::CreateInstance`, rather than from
  `DirectPlayCreate`. `DirectPlayCreate` binds the object to a service
  provider as it creates it, and a bound object refuses
  `InitializeConnection`, which is how an address is attached to a session.
  The class factory hands back an object with no provider bound to it, which
  is what the code expects.
* The library is loaded by name with `LoadLibrary`, so a machine without it
  still starts and the feature reports itself unavailable.

`DirectPlayLobbyCreateA` is called with the signature it actually has, which
takes no interface identifier: it hands back `IDirectPlayLobby`, and
`IDirectPlayLobby3` is reached from there with `QueryInterface`. The lobby
builds the TCP/IP address that `InitializeConnection` is given.

`tools\smoketest.ps1` has a case that hosts a session and makes a player, so
the path is run on every gate.

### The .x model format

`blitz3d/loader_x.cpp` is the original reader. It needs `<dxfile.h>`,
`<rmxftmpl.h>` and `<rmxfguid.h>` from the DirectX SDK, plus `d3dxof.dll` at
run time. Microsoft retired the full SDK download in favour of the End-User
Runtime, which carries neither headers nor `d3dxof`, and the End-User Runtime
is all that is still offered at the SDK's download page. The original file is
left untouched.

The format itself is documented, and the tree it holds is small: a template
instance, its members in a fixed order, and its child templates. So
`blitz3d/xfile.cpp` reads the file and `blitz3d/loader_x_native.cpp` reads the
templates out of what it read, in the same order the SDK's templates describe.
Neither needs the SDK or a Microsoft binary at run time.

Both encodings are read. The text encoding is what most exporters write: the
tree as text, with the members of a template in a flat run of numbers. The
binary encoding is the same tree as a token stream, where the tokens carry the
types, so a run of DWORDs and a run of floats are told apart by the token
rather than guessed at.

Two things the format does not tell you, and the reader has to supply:

* The text encoding does not record whether a number was meant as an integer
  or as a float, and an exporter is free to write `1` where a float is meant.
  The template decides, so the reader reads the members of a template in
  template order rather than by their syntax.
* A material can be written inside the mesh's material list or as a reference
  to a material named earlier in the file, so the reader keeps the instance
  names and resolves the references.

The templates that are read are `Mesh`, `MeshNormals`, `MeshTextureCoords`,
`MeshVertexColors`, `MeshMaterialList`, `Material`, `TextureFilename`, `Frame`,
`FrameTransformMatrix`, `AnimationSet`, `Animation` and `AnimationKey`. A
rotation key is a quaternion with the scalar part first, which is the order the
format writes and the order the engine's own quaternion uses.

`tools\golden\golden_x.bb` is the gate: a cube it writes itself, so the text
encoding is pinned against a file whose every byte is known there, and a real
model from the release tree, so the frame hierarchy, the material list and the
texture are covered too. Every `.x` file in `_release` loads with it: 45 of
them, both encodings.

### The Windows SDK

* `gxruntime/ddutil.cpp` — `adjustTexSize` asked the Direct3D 7 device for its
  caps without checking that there was a device. There is not one on any
  current Windows, so every texture load - `LoadTexture`, and with it every
  model that names a texture - dereferenced a null device and took the program
  down with it. With no device there are no caps to fit the texture to, so the
  size is left alone.
* `gxruntime/gxinput.{h,cpp}`, `gxruntime/gxruntime.cpp`, `gxruntime/std.h` —
  DirectInput 7 to 8. `dinput8.dll` on Windows 10 exports only
  `DirectInput8Create`; `DirectInputCreateEx`, which the original code calls, is
  gone, so there is no way to create an `IDirectInput7`. Migrated to
  `DirectInput8Create` / `IID_IDirectInput8W` / `IDirectInputDevice8`, switched
  `CreateDeviceEx` to `CreateDevice`, and translated the device type check:
  DI7 reported gamepads and joysticks under one `DIDEVTYPE_JOYSTICK` with a
  subtype byte, DI8 gives each its own type byte, so both are now enumerated.
* `blitzide/htmlhelp.h` — two collisions. The file's include guard was
  `HTMLHELP_H`, which is also the SDK's `<htmlhelp.h>` guard, so the whole file
  vanished once MFC pulled that header in; and `<htmlhelp.h>` does
  `#define HtmlHelp HtmlHelpA`, which renamed the class out from under itself.
  Guard renamed, `#undef HtmlHelp` added.
* `bblaunch/checkdx.cpp` — dropped the DirectMusic 6.1 probe. `dmusici.h` is no
  longer in the SDK and `dmusic.dll` is not installed, so `CoCreateInstance`
  always failed, which pinned the reported DirectX version at 0x600 and made
  `bblaunch` treat the machine as lacking DirectX 7 and refuse to start. The
  DDraw7 check below it is the real evidence, so the probe is skipped and the
  comment explains why.
* `debugger/mainframe.cpp` — `strrchr` on a `const char*` now selects the
  `const` overload, so the `char *` result variables had to become `const char *`.
* `blitz3d/q3bsprep.cpp` — `static log( const string &t )` with no return type.
  VC6 defaulted it to `int`; MSVC will not. Added `void`.
* `blitz3d/loader_3ds.cpp` — `Box box( Vector(),Vector() )` is a most vexing
  parse; MSVC reads it as a function declaration, after which `box.a` is an
  error. Spelled out with a named `Vector`.
* `blitz3d/geom.h` — `#undef INFINITY`. `<math.h>` defines `INFINITY` as a C99
  float literal macro, which collides with the constant of the same name.
* `blitz3d/md2rep.cpp` — `vector::begin()` used as a `char*` / `const T*`.
  VC6's iterator was a raw pointer; a standard one is not. Replaced with
  `.data()`.
* `bbruntime/bbsockets.cpp` — same, for the UDP send and receive buffers.
* `blitz3d/entity.cpp` — removed `#include "stats.h"`. The per-entity profiling
  header is not part of the open source release and nothing in the file used it.
* `freeimage241/Source/FreeImage.h` — `FI_AllocateProc` declared default
  arguments on a function *pointer typedef*, which is invalid C++. The defaults
  on the real `FreeImage_Allocate` declaration are left alone.

### Dependency paths

The `.dsp` files and the two headers that reach into the third-party trees
assume the dependencies sit one level *above* this directory, which contradicts
`README.TXT` steps 1 and 2. Since CMake supplies the include directories, the
hardcoded paths became plain header names:

* `gxruntime/std.h` — `#include <fmod.h>`
* `gxruntime/ddutil.cpp` — `#include "FreeImage.h"`

## What does not work, and why

**DirectPlay 4 networking.** This works, and it is described under *Source
changes* rather than here. The short version: `dplayx.dll` is still shipped
with Windows and still exports everything the runtime needs, only the COM
registration is gone, so the runtime asks the library's own class factory for
the interface instead of going through `CoCreateInstance`, and
`compat/dplay.h` and `compat/dplobby.h` are the DirectPlay 4 interface
written out from the published definition rather than the SDK's headers.

What is still missing is the lobby *server*, which was `dplobby.dll` and went
away with the SDK. The lobby *client* is in `dplayx.dll` and is used, so
building an address and listing the sessions on the network both work. A
machine that has no firewall rule for `dplayx.dll` will not exchange sessions
with another machine, because the session list exchange is inbound traffic;
`tools\smoketest.ps1` runs the multiplayer path, and on a machine where the
library or the network is not there the path reports failure rather than
crashing.

**`.x` model loading.** This used to be impossible and is not any more, so it
is described under *Source changes* rather than here. The short version:
`blitz3d/xfile.cpp` reads the format and `blitz3d/loader_x_native.cpp` turns
what it reads into meshes, with no DirectX SDK and no `d3dxof.dll`.

**320x240 in exclusive fullscreen.** `Graphics 320,240,32` with no mode
argument asks for exclusive fullscreen, not a window; mode 2 is windowed. Modern
display drivers no longer offer 320x240 to a DirectDraw 7 mode change, so that
form fails with the runtime's "Unable to set graphics mode". 640x480 and 800x600
exclusive both work, as does 320x240 windowed. `tools\smoketest.ps1` asserts this
so it stays visible.

**3D acceleration.** gxruntime still asks for `IDirect3D7`, and `d3d.h` is still
in the Windows SDK, so the code is intact, but the Direct3D 7 HAL that it needs
has not existed since Windows 9x. Expect software rendering, as the original
Blitz3D does on any modern Windows.

## Layout

```
CMakeLists.txt          workspace, dependency order, third-party and MFC discovery
cmake/b3.cmake          the four helpers every project file uses
compat/                 dplay.h and dplobby.h for the SDK's DirectPlay gap
blitz3d/xfile.{h,cpp}   the .x file reader, both encodings
blitz3d/loader_x_native.cpp  the .x loader, on top of that reader
tools/build.cmd         configure + build + stage
tools/gate.ps1          build + verify + smoke + bench, one verdict
tools/verify.ps1        golden transcripts
tools/golden/           the golden programs and their expected output
tools/bench.ps1         benchmarks against the baseline
tools/probe.ps1         per-project compile probe
tools/smoketest.ps1     compile and run a matrix of small programs
tools/rungame.ps1       compile and run a real game
tools/why.ps1           run a program and dump the text of any window it owns
```

The original `blitz3d.dsw` and the `.dsp` files are untouched, so the MSVC 6.0
route in `README.TXT` still describes what the sources were written for. The
CMake files mirror each `.dsp`'s source list, defines, libraries and output path.

## Licence

Unchanged: zlib/libpng, as stated in `README.TXT` and `LICENSE.TXT`.
