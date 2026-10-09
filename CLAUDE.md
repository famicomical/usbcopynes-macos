# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

USB CopyNES Blue: Win32 GUI client (C, Win32 API dialogs, no framework) for the retrousb.com USB CopyNES cartridge dumper. Fork of the original retrousb 4.0.0 source. Talks to the device over FTDI D2XX (`ftd2xx.dll`) or legacy parallel port (`inpout32.dll`).

## Build

Windows only, MSVC, 32-bit. No tests, no lint.

```
msbuild src/CopyNESW_VC17.sln -t:rebuild -property:Configuration=Release -property:Platform=Win32
```

- `CopyNESW_VC17.sln` is the maintained solution (VS 2017, CI uses `windows-2019`). `_VC9`/`_VC10` solutions exist but are unmaintained.
- Output goes to `bin/` (`USBCopyNESBlue.exe` release, `USBCopyNESBlueD.exe` debug, both gitignored). Intermediates in `src/tmp/`.
- Precompiled header: every `.c` starts with `#include "StdAfx.h"`, which pulls in `CopyNESW.h`, `resource.h`, `ftd2xx.h`, `inpout32.h`, `version.h`.
- Adding a source file means editing `CopyNESW_VC17.vcxproj` (and `.filters`) by hand.
- Version string lives only in `src/version.h`.
- CI (`.github/workflows/build.yml`) rebuilds Release on push/PR to master and uploads `bin/` as artifact.

### macOS command-line build (`src/mac/`)

```
brew install libftdi
make -C src/mac          # -> bin/copynes
bin/copynes              # usage; e.g. bin/copynes dump NES-NROM mygame
```

- Same `c_*.c`, `misc.c`, `config.c`, `crc32.c` compiled unchanged. `src/mac/StdAfx.h` is force-included and defines the Windows header's include guard, so `#include "StdAfx.h"` in the originals becomes a no-op. Needs `-fcommon` because `CopyNESW.h` has tentative definitions.
- `shim.c` replaces `miscdialogs.c` and the few Win32 calls (MessageBox, Sleep, ini read, `_fullpath`) with console I/O. Prompts consume trailing argv answers in order, then stdin. Ini is read-only on macOS.
- `io_libftdi.c` replaces the USB half of `io.cpp` (FT2232: interface A data, interface B DTR/RTS = reset/play). Parallel port not ported. Verified against a real unit (BIOS v4) with Homebrew libftdi, no driver changes needed.
- Not ported: `c_options`, `c_bankwatch`, `c_microbug`, `c_vrc7regs`, `c_playnsf` (own Win32 dialogs).
- Shared-code rule: path separator checks use `PATHSEP` from `CopyNESW.h`, never a literal backslash.

`bin/` is also the runtime tree: `PLUGINS/`, `MAPPERS.DAT`, `USB CopyNES.ini`, and output dirs `PRG/ CHR/ WRAM/ NES/ CRC/ NSF/`. The copies of `MAPPERS.DAT` and `USB CopyNES.ini` under `src/` are stale originals; edit the ones in `bin/`.

## Architecture

### PC side (`src/`)

- `CopyNESW.c`: `WinMain`, main dialog proc. Each button `IDC_MAIN_*` calls one `CMD_*()` function and reports success.
- `c_*.c`: one file per command (`CMD_DUMPCART`, `CMD_PLAYNSF`, `CMD_RUNPLUG`, ...). Each defines `CMD_NAME` before anything else because `MSGBOX_TITLE` in `CopyNESW.h` expands it. Typical flow: `PromptPlugin()` / `Prompt()` for input, `OpenStatus()`, `ResetNES(RESET_COPYMODE)`, `LoadPlugin("clear.bin")` + `RunCode()`, then `LoadPlugin(plugin->file)` + `RunCode()`, then `ReadByte`/`WriteByte` loop driven by the plugin's protocol, write files via `WriteNES`/`WriteUNIF` in `misc.c`.
- `io.cpp`: the only hardware layer. `OpenPort(port,...)` with `port == -1` means USB (opens FTDI channels "USB CopyNES A" = data, "B" = control); otherwise LPT via inpout32. `WriteCommand(a,b,c,d,e)` is the 5-byte BIOS command; `LoadPlugin` skips the plugin file's 128-byte header, sends 1024 bytes of 6502 code to `$0400`, and reads the trailing text as `ROMstring`. `RunCode` jumps to it. Everything is `extern "C"`.
- `config.c`: reads/writes `USB CopyNES.ini` (paths relative to exe dir, `ParPort=-1` = USB), parses `MAPPERS.DAT` into `Plugins[]` categories (`PCategory`/`PPlugin` in `CopyNESW.h`), `IsValidPlugin` requires file size >= 1152 (128 header + 1024 code).
- `miscdialogs.c`: status window with percent bar and cancel button, prompt dialogs, plugin picker. `resource.h`/`CopyNESW.rc` hold dialog IDs.
- `HWVer` (from `FindVersion()`) gates features by BIOS version; check it before adding commands that need newer firmware.

### NES side (`src/PLUGIN SRC/`)

Plugins are 6502 programs the PC uploads and runs on the CopyNES. Binaries ship in `bin/PLUGINS/*.BIN`; `MAPPERS.DAT` maps name/mapper number to a `.bin` (format documented in `readme.txt`).

- Two toolchains. Old: TASM, `ASM.BAT name` builds `name.ASM` -> `name.BIN` with `TEMPLATE.ASM` as the starting point. New: cc65, `ccasm.bat name` assembles `name.s` + `pluginlib/bioscalls.s` and links with `pluginlib/plugin.cfg` (code at `$0400`, 128-byte TITLE block at `$0380`). `template.s` is the cc65 starting point. `movebin.bat` moves the result into `bin/PLUGINS/`.
- Plugins call BIOS routines through fixed vectors at `$0200+` (`send_byte`, `read_byte`, `chk_vram`, `chk_wram`, `do_crc`, ...). BIOS source is in `BIOS/`. Byte-level handshake protocol is in `DOCS/INTERFACE.TXT`; many board notes in `DOCS/`.
- `DONE/` holds finished plugin sources; some shipped `.BIN`s have no source (see git history, e.g. GTROM).
- PC command and plugin must agree on the byte stream. Changing one side without the other breaks dumps silently.

### Misc

- `bin/PRG/*.vrc7` + `patches.txt`: VRC7 instrument patch sets used by the VRC7 register tool; `vrc7_compare.py` compares them.
