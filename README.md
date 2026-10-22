<p align="center"><img src="logo.webp" alt="Silent Hill 4: The Room" width="600"></p>

# Silent Hill 4: The Room — Nintendo Switch port (work in progress)

A native Nintendo Switch port of the PC version of **Silent Hill 4: The Room**, made the same way as
[sh2-nx](https://github.com/cutarev/sh2-nx) and [sh3-nx](https://github.com/cutarev/sh3-nx).

It's not an emulator. The game's x86 executable is statically recompiled to C ahead of time and the C is compiled
for the Switch's ARM64 CPU. The Windows layer the game expects (Direct3D 8, DirectSound, DirectInput, kernel32...)
is reimplemented on top of SDL2 and OpenGL; the movies (DirectShow MPEG-1) play through FFmpeg.

No game files are included: you need your own copy of the PC game (the GOG release is what this is built against).

## How to install

1. Download `sh4-nx.nro` from the [releases](https://github.com/cutarev/sh4-nx/releases).
2. Put it in `/switch/sh4-nx/` on the SD card, together with `SILENT HILL 4.exe` and the `data`, `demo`, `movie` and
   `sound` folders from your PC copy of the game, and an empty `save` folder.
3. Launch it from the homebrew menu in title takeover mode (hold R while starting an installed game), not from the
   album.

Controls: the left stick moves, the right stick turns the camera; A confirms, B cancels. If the game crashes,
`sh4-crash.log` is written next to the NRO: please attach it to an issue.

## Building

Put the PC game folder (or a link to it) at `game/` in the repo root, then:

```sh
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
tools/regen.sh                      # "SILENT HILL 4.exe" -> src/recomp/gen (a few minutes)
cmake -S . -B build/linux && cmake --build build/linux -j   # Linux build, needs SDL2 and FFmpeg
tools/build-switch.sh               # build/switch/sh4-nx.nro, in the devkitPro container (podman)
```

Run the Linux build from the game folder: `build/linux/sh4 <game folder>`.

Before the game comes a warning screen, then a launcher with Play and Options (which edits `sh4.ini`). Its images are
built into the binary from `assets/`; `tools/launcher_assets.py <warning.png>` remakes them.

---

Silent Hill is a trademark of Konami. This project is not affiliated with or endorsed by Konami.
