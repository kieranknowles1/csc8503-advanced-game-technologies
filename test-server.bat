@REM Windows equivalent of `test-server.sh`
@REM We don't have tmux on Windows, so we can't
@REM show both outputs in the same window.

set EXE_PATH=./build/CSC8503/Debug/CSC8503.exe

@REM Players are cats, so give them Khajiit names
start %EXE_PATH% --no-capture --name "J'zargo"
@REM Sleep for a bit to allow the server to start
timeout /t 1
@REM Put the client on a different monitor
start %EXE_PATH% --client --window 2000 0 --name "M'aiq the Liar"
