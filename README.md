## bgfx-minimal-rgfw-example

This is a fork of [bgfx-minimal-example](https://github.com/funatsufumiya/bgfx-minimal-example) which uses [RGFW](https://github.com/ColleagueRiley/RGFW) for windowing instead of GLFW.

### Build and Run

#### for example: macOS arm64

```bash
$ premake5 --arch=arm64 xcode4

# and then open the generated project file with Xcode, or:
$ xcodebuild -project build/xcode4/helloworld.xcodeproj -configuration Debug
```

#### for example: Windows

```bash
$ premake5.exe vs2022
$ start build\vs2022\bgfx-minimal-rgfw-example.sln
```

## bgfx-minimal-example

[![License](https://img.shields.io/badge/license-BSD--2%20clause-blue.svg)](https://bkaradzic.github.io/bgfx/license.html)

Minimal [bgfx](https://github.com/bkaradzic/bgfx/) "hello world" example.

This doesn't use the [bgfx example framework](https://github.com/bkaradzic/bgfx/tree/master/examples/common). [GLFW](https://www.glfw.org/) is used for windowing. There are separate single and multithreaded examples.

[Premake 5](https://premake.github.io/) is used instead of [GENie](https://github.com/bkaradzic/GENie), so this also serves as an example of how to build [bgfx](https://github.com/bkaradzic/bgfx/), [bimg](https://github.com/bkaradzic/bimg/) and [bx](https://github.com/bkaradzic/bx/) with a different build system.

### Related links

[Using the bgfx library with C++ on Ubuntu](https://www.sandeepnambiar.com/getting-started-with-bgfx/) and the associated [repository](https://github.com/gamedolphin/bgfx-sample).

[Hello, bgfx!](https://dev.to/pperon/hello-bgfx-4dka)
