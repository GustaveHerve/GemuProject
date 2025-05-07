# GemuProject
GemuProject is a simple work-in-progress emulator for the GameBoy game system. (Most specifically emulating the DMG black and white model).

This is a personal project that I started in the process of learning more about C programming, low level programming, assembly and video game system emulation in general. As such, it is not aimed to be an alternative for already existing GameBoy emulators and is only targeting a moderate level of accuracy (for now it is at best M-cycle level).

Currently the following is implemented:
- Opcodes intepreter and full CPU emulation
- PPU state machine using pixel FIFOs
- APU emulation
- Basic keyboard input support (B: Z key, A: X key, Space: Select, Enter: Start, Arrow keys for D-Pad)
- Support for no mapper, MBC1, MBC3 (no RTC support for now) and MBC5, including exporting external RAM in a .sav file if the mapper has a battery

## Usage
### Building
To build the project, you can currently run CMake (ideally in a separate build directory)

### Usage
`./gemu [-b BOOT_ROM_PATH] ROM_PATH`

## Credits
### Documentation
This emulator was made using the following documentation:
- [Pandocs](https://gbdev.io/pandocs/)
- [The Cycle-Accurate Game Boy Docs](https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf)
- [Gekkio's Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf)
- [The Ultimate Game Boy Talk](https://www.youtube.com/watch?v=HyzD8pNlpwI)
- [The Gameboy Emulator Development Guide](https://hacktix.github.io/GBEDG/)
- [NightShade's Game Boy Sound Emulation Guide](https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html)


### Frameworks
I used the following technologies and frameworks to make GemuProject:
- C language
- [SDL3](https://www.libsdl.org/) for rendering, input and audio handling
