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
