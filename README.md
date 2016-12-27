# PSF Information Tool

This is a small command-line utility that parses and displays the header of a
[PSF file](https://en.wikipedia.org/wiki/Portable_Sound_Format), i.e. game
console music files. Those files are compressed and usually contain a raw
binary, which produces the game music when run on the target platform (or in an
emulator).

It's written in C and has minimal dependencies. Initially this was meant as a
library to parse PSF files, extract the binary and maybe even convert them to
regular sound files, but that hasn't happened yet ;). The parser can be easily
used as a library though and is quite thorough in checking the header (i.e. it
verifies the checksum, can load all libraries and there's a wide range of error
codes when things go wrong).

## Build

Install zlib development headers (e.g. on Fedora `sudo dnf install
zlib-devel`) then just run `make`.

## Usage

```
./psf-dump <file>
```

Example output:

```
PSF information (for '/[...]/Pokemon Emerald - Littleroot Town.minigsf'):

Version: GameBoy Advance (GSF) (0x22)
Data Size: Reserved area:         0 Byte
           Compressed:           22 Byte
           Uncompressed:         14 Byte
Number of tags: 6
Number of libraries: 1
Hierarchy index: 0

Tags:
      _lib = trm-pmeu.gsflib
     gsfby = Saptapper
     title = Littleroot Town
      game = Pokemon Emerald
    length = 1:48.270
      fade = 5

Libraries:

PSF information (for '/[...]/trm-pmeu.gsflib'):

Version: GameBoy Advance (GSF) (0x22)
Data Size: Reserved area:         0 Byte
           Compressed:       761787 Byte
           Uncompressed:   16777228 Byte
Number of tags: 0
Number of libraries: 0
Hierarchy index: 1
```
