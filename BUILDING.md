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

### The Windows SDK

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

**DirectPlay 4 networking.** `dplayx.dll` and `dplobby.dll` are not present on
Windows 10 or 11 and `CLSID_DirectPlay` is unregistered, so `CoCreateInstance`
fails before any interface pointer is obtained. `compat/dplay.h` and
`compat/dplobby.h` declare just enough of the DirectPlay 4 surface for
`bbruntime`'s multiplayer code to compile, with the interfaces left abstract so
no vtable layout can be got wrong, and `DirectPlayLobbyCreate` made an inline
stub that returns `DPERR_UNSUPPORTED`. The runtime reports multiplayer as
unavailable rather than pretending. Restoring it means dropping `compat/` from
the include path and putting the genuine DirectX SDK headers back.

**`.x` model loading.** `blitz3d/loader_x.cpp` needs `<dxfile.h>`,
`<rmxftmpl.h>` and `<rmxfguid.h>` from the DirectX SDK, plus `d3dxof.dll` at run
time. Microsoft retired the full SDK download in favour of the End-User
Runtime, which carries neither headers nor `d3dxof`, and the End-User Runtime
is all that is still offered at the SDK's download page. The original file is
left untouched; `blitz3d/loader_x_stub.cpp` provides the `Loader_X::load` symbol
that `LoadModel`'s `.x` dispatch needs, and returns 0, so `LoadMesh` reports
failure. Swap the two entries in `blitz3d/CMakeLists.txt` if you restore the
SDK.

Two of the bundled birdie samples, `dolphin` and `LodMesh`, load `dolphin.x` and
then use the result without checking it, so they fault on the null entity. Every
other game in `_release` runs.

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
tools/build.cmd         configure + build + stage
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
