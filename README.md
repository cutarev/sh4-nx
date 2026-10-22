<p align="center"><img src="logo.webp" alt="Silent Hill 4: The Room" width="600"></p>

# Silent Hill 4: The Room — Nintendo Switch port (work in progress)

A native Nintendo Switch port of the PC version of **Silent Hill 4: The Room**, made the same way as
[sh2-nx](https://github.com/cutarev/sh2-nx) and [sh3-nx](https://github.com/cutarev/sh3-nx).

It's not an emulator. The game's x86 executable is statically recompiled to C ahead of time and the C is compiled
for the Switch's ARM64 CPU. The Windows layer the game expects (Direct3D 8, DirectSound, DirectInput, kernel32...)
is reimplemented on top of SDL2 and OpenGL; the movies (DirectShow MPEG-1) play through FFmpeg.

No game files are included: you need your own copy of the PC game (the GOG release is what this is built against).

## Building

Put the PC game folder (or a link to it) at `game/` in the repo root, then:

```sh
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
tools/regen.sh                      # "SILENT HILL 4.exe" -> src/recomp/gen (a few minutes)
cmake -S . -B build/linux && cmake --build build/linux -j   # Linux build, needs SDL2 and FFmpeg
tools/build-switch.sh               # build/switch/sh4-nx.nro, in the devkitPro container (podman)
```

Run the Linux build from the game folder: `build/linux/sh4 <game folder>`.

---

Silent Hill is a trademark of Konami. This project is not affiliated with or endorsed by Konami.
