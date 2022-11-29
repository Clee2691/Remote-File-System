# Practicum #2

A simple remote file system supporting mutliple requests from multiple clients using multiprocessing.

# Design
- Implemented 5 commands, GET, INFO, PUT, MD, RM.
- Implemented mirroring of PUT, MD, and RM requests.
- Servicing multiple clients simultaneously with multiprocessing.
- The server and client runs on an infinite loop.
- Client downloads files into the `Downloads` folder.
- Shared functions are located in `filefuncs.c`.
- Since I use WSL to develop, I use 2 USB drives with drive letters of `D:/` and `E:/`. I have to mount them into the environment before they are seen.
- Can use the command `exit` to exit the client. 
- More explanations are in the code.

# Building & Running
1. Compile `client.c` & `server.c` with the `Makefile`.
```
$ make
```
2. Run server with `make ser`.
```
$ make ser
```
3. Run client with `make cli`.
```
$ make cli
```
4. Clean up with `make clean`.
```
$ make clean
```