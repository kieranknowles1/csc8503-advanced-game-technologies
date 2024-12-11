# CSC8503 Advanced Game Technologies

My coursework for the Advanced Game Technologies module at Newcastle University.

## Usage

### Controls

As any player:
- `WASD` - Move
- `Space` - Jump
- `Mouse` - Look
- `Scroll` - Zoom
- `F2` - Toggle free camera
- `F3` - Toggle debug drawing

As the server:
- `F11` - Restart the game

### Command Line Arguments

The game will launch as a server by default. To launch as a client, pass the
`--client` argument. For a complete list of arguments, run with `--help`.

## Included Scripts

`test-server.sh` and `test-server.bat` are included to launch a server and a
client in one click on Linux and Windows respectively. The Linux version
requires `tmux` to be installed and is more capable, building the project before
launching.

## Tools Used

[keesiemeijer/maze-generator](https://keesiemeijer.github.io/maze-generator/) - Maze generation

## Building

The project uses CMake to build. As always, an out-of-source build is recommended.
Windows builds will use the Windows API for input and windowing, so SDL2 is not
required.

### Linux Support

Linux is supported via SDL2, a `flake.nix` is included for NixOS users. Building has
only been tested using Clang, and issues may arise due to its differences from MSVC.

Please consult `flake.nix` for a list of dependencies that must be installed on other
distributions.
