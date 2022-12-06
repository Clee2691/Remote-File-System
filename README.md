# Practicum #2

A simple remote file system supporting mutliple requests from multiple clients using multiprocessing.

# Design
- Implemented 5 commands, GET, INFO, PUT, MD, RM.
- Implemented mirroring of PUT, MD, and RM requests.
- Servicing multiple clients simultaneously with multiprocessing.
- I made the max path length of any request to be 10 names including the file.
- The way files are downloaded is by opening the file from the server and streaming the bytes of the file through to the client. The client then makes a new file and writes the byte stream to the new file.
- The directories are made in a recursive fashion so a path of up to 9 folders can be made.
- The server and client runs on an infinite loop.
- Client downloads files into the `clientRoot/downloads` folder.
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

# Supported Commands & Examples

## 1. `GET`
### Command Example
```
$ Enter Request: GET home/neu/file.txt
``` 
### Example Response
```
File Size: 19
Server's response: Successful request!
```

### Expected Result
If the file is found, it is downloaded into `clientRoot/downloads` folder. Otherwise, it will throw a file not found error.

## 2. `INFO`
### Command Example
```
$ Enter Request: INFO home/neu
```

### Example Response
```
Information:

Type: Directory
Size: 4096 bytes
Permissions: drwxrwxrwx
Last status change: Tue Dec  6 15:07:04 2022
Last file access: Tue Dec  6 15:07:04 2022
Last file modification: Tue Dec  6 15:07:04 2022
```
### Expected Result
Returns information about the directory or file if file is present on the server. It will return the `type`, `size`, `permissions`, and `file access dates`. If file or directiory is not found, it will say so and no information is returned.

## 3. `PUT`
### Command Example
1. Enter the `PUT` request. Must have a file extension. Can be any extension.
```
$ Enter Request: PUT home/neu/file.txt
```
2. Enter the contents to write to the file.
```
$ Enter file contents (MAX: 2000 characters):
Some stuff to put into the file.
```

### Example Response
```
Server's response: Successful request!
```

### Expected Result

## 4. `MD`
### Command Example
1. The directory must **NOT** have an extension! 
```
$ Enter Request: MD home/neu
```

### Example Response
```
Successfully created the directory
Server's response: Successful request!
```

### Expected Result
The new directories will be made according to the path if they are not found so in this example `home` will be created and then `neu` after.

## 5. `RM`
### Command Example
```
$ Enter Request: RM home/neu
```
### Example Response
```
Successfully removed the directory or file!
Server's response: Successful request!
```
### Expected Result
The file or directory will be removed. The directory must be empty in order for it to be removed.