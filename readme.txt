USB CopyNES Blue for macOS
==========================
Command-line client only. No GUI.

This is a fork of USB CopyNES Blue (https://github.com/bbbradsmith/usbcopynesblue),
itself a fork of USB CopyNES from retrousb.com. Original source and the USB CopyNES
device: http://www.retrousb.com/product_info.php?products_id=36

The Windows GUI client still builds from src/CopyNESW_VC17.sln (Visual Studio 2017)
and is otherwise unchanged. This fork adds a macOS command-line client that reuses
the same command code over libftdi.


Requirements
------------
  Homebrew, then:
    brew install libftdi          # USB driver library (libusb based)
    brew install cc65             # only if you want to build plugins

  No kernel extension or FTDI D2XX driver is needed. Tested on macOS 15 with a
  USB CopyNES running BIOS v4.


Build
-----
    make -C src/mac               # produces bin/copynes

  bin/ is the runtime tree. The executable must stay next to MAPPERS.DAT,
  "USB CopyNES.ini" and the PLUGINS/ PRG/ CHR/ NES/ WRAM/ CRC/ NSF/ folders.


Usage
-----
    bin/copynes                   # prints the command list
    bin/copynes <command> [answers...]

  Every command asks the same questions as the Windows client, in the same
  order, on the terminal. Any answers given on the command line are consumed
  by those prompts in order; when they run out, prompts read from stdin.
  Yes/no questions take y or n. Run a command with no answers to see what it
  asks. Each consumed answer is echoed as "prompt: answer".

  Commands needing the CopyNES:
    dump       [plugin] [romname]   dump cartridge to PRG/, CHR/ and NES/
    playcart                        reset into play mode
    runplug    [plugin]             run a utility plugin
    writewram  [plugin] [file.sav]  upload WRAM
    ramcart    [plugin] [file.nes]  program a RAM/flash cart
    fixgar                          fix a garbled Game Genie
    nesinfo                         show CopyNES BIOS version string
    playlog    [file.txt]           play a register log

  Offline commands:
    makenes    [file.prg] ...       build .nes from PRG/CHR (written next to the PRG)
    makeunif   [file.prg] ...       build .unf from PRG/CHR
    splitnes   [file.nes]           split .nes into PRG/CHR
    splitunif  [file.unf]           split .unf into PRG/CHR
    breakbank  [file] ...           split a file into banks
    disasm     [file] ...           disassemble 6502
    list                            list plugins from MAPPERS.DAT
    config                          show resolved paths and BIOS version

  [plugin] may be:
    - a MAPPERS.DAT name, e.g. NES-MMC3
    - its .bin filename, e.g. txrom1.bin
    - any other .bin in PLUGINS/, which then asks for an iNES mapper number
      (-1 skips .nes, 9999 skips .nes and .unif) and a UNIF board name

  Examples:
    bin/copynes dump NES-NROM smb
    bin/copynes dump NES-MMC3-256K game3      # fixed 256K PRG + 256K CHR, see below
    bin/copynes dump 189A.BIN game 4 ""       # unlisted plugin, mapper 4, no board name
    bin/copynes list | grep -i mmc3

  The NES is reset into play mode when any command exits, as on Windows.


Plugins
-------
  MMC3 (txrom1.bin) and most other plugins detect ROM size by comparing banks
  to find where addressing wraps. A cart whose ROM holds duplicated banks is
  undersized by that method. NES-MMC3-256K (txrom256.bin) skips detection and
  always dumps 256K PRG and 256K CHR. Source: src/PLUGIN SRC/DONE/TXROM256.ASM.

  Plugins in src/PLUGIN SRC/DONE/*.ASM are written for TASM. To build one on
  macOS with cc65:
    src/PLUGIN\ SRC/mac/build.sh TXROM256   # -> bin/PLUGINS/txrom256.bin
  (tasm2ca65.py converts the syntax, plugin.cfg lays out the 1152-byte image.)
  Rebuilding TXROM1.ASM this way matches the shipped txrom1.bin except for two
  source changes that postdate the shipped binary.


Not ported
----------
  Options, Bank Watch, MicroBug, VRC7 Registers and Play NSF (they have their
  own Windows dialogs). Parallel port CopyNES. "USB CopyNES.ini" is read but
  never written on macOS; edit it by hand.


Layout
------
src/            Windows client source (CopyNESW_VC17.sln) and shared command code
src/mac/        macOS client: Makefile, main.c, shim.c (console I/O), io_libftdi.c
src/PLUGIN SRC/ 6502 plugin sources; DONE/ finished plugins; mac/ cc65 build script
bin/            output executables and runtime data


MAPPERS.DAT format
------------------
  The first line is 5 integers:
    = column of plugin name
    = column of plugin filename
    = mapper number
    = column of plugin description
    = column of end of line
  Each category header should have: (aligned to the specified columns)
    * instead of plugin name
    empty filename (spaces)
    0 as mapper number
    title of category instead of plugin description
  Each plugin entry in a category should have: (aligned to the specified columns)
    plugin name
    filename
    mapper number
    description
  If a line begins with ****, the rest of the line is completely ignored.
