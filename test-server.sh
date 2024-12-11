#!/usr/bin/env bash
set -euo pipefail

EXE_PATH=./build/CSC8503/CSC8503
# Run a multithreaded build, leaving two threads to avoid overloading the system
THREADS=$(( $(nproc) - 2 ))

echo "Building with $THREADS threads"
make --directory build -j$THREADS

# Run two instances - one as server and one as client
# Display both outputs through tmux
# Position windows on separate monitors
tmux new-session -d -s "server" $EXE_PATH --no-capture --name "J'zargo"
tmux split-window -h $EXE_PATH --client --window 2000 0 --name "M'aiq the Liar"

# Display outputs of both instances
tmux attach-session -t "server"
