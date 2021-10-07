## afid
the asynchronous file-based interprocess data transfer library

### Purpose
Afid is an implementation of asynchronous IPC through shared files with locks. Afid exists for situations where communication is needed between two(-or-more) interacting processes, when speed and security are *not* high priorities. For the vast majority of small hobbyist projects (basically my entire code output) the results are completely acceptable, and with barebones dependencies and code overhead.

Afid is only as fast as file access on your machine (as it is *literally* writing to the filesystem every time a message is sent), so using files hosted on hard disk will be significantly slower than other forms of IPC, though for sending very large files between processes (on the order of gigabytes) this might be the only solution. By default, afid will prioritize storing its files in tmpfs (when running on UNIX-like systems). This will *typically* be hosted in RAM and thus significantly faster; however, the details of your particular filesystem may differ.

Afid is highly insecure (by design). All files are available to be viewed and edited by any process on the machine. This makes it extremely simple to negotiate connections between processes, but it should *not* be used for security software or any situation where delicate information is passed between processes. If you insist on using afid for such software, remember that it provides no encryption or security, so please add some in userspace. For your own sake.

## Building
On Unix-compatible systems with GCC, simply open the source folder and issue `make` to create a linkable object file `afid.o`.

## Usage
To use afid in your project, copy the `afid.h` and `afid.o` files to your source tree, then link `afid.o` during compilation. This can be done in GCC as `gcc yourfile.c afid.o -o yourexecutable`. Also make sure to add

`#include "afid.o"`

to any source file which uses the library.

## Data Types
```
struct l_file {
     char *name;  \\ The file's path
     char *lock;  \\ The lock file's path
     FILE *ptr;   \\ A file descriptor, if the file is open
};

struct afid_hub {
     l_file file; \\ The hub file
     l_file out;  \\ The file used for output
     l_file in;   \\ The file used for input
};
```
Afid's functions create and manipulate the `afid_hub` struct. This is abstracted away; for essential functions, the actual structure of the hub object does not matter. It is, however, included in this documentation for completeness.

## Functions
`afid_hub *afid_create(char *hubfile, char *outfile, char *infile);`
When successful, this creates a new hub file, with the current process acting as the server. The arguments accepted are filenames for the hub file, the out file (where output will be written), and the in file (where input will be read), respectively.

Any or all of these arguments can be `NULL`, in which case an automatically generated file in `/tmp/` will be used instead. When the hub file is unspecified, this means that the server must be given some alternative method of communicating to other processes where the hub file is located.

`afid_hub *afid_destroy(afid_hub *hub)`
The counterpart to `afid_create()`, this should be used by the server (but only by the server) to clean up files before exiting. Failure to do so may cause unavoidable errors when trying to create a new hub at the same location.

`afid_hub *afid_connect(char *hubfile)`
This allows client processes to connect to an existing hub and begin communicating with the server process. When successful, this returns a pointer to the hub struct; when unsuccessful this returns NULL.

`afid_hub *afid_disconnect(afid_hub *hub)`
The counterpart to `afid_connect()`, this should be used by client processes to clean up before exiting. Failure to do may leave lock files dangling and unusable memory allocated.

`bool afid_msg(afid_hub *hub, size_t *size)`
This function checks if the hub has a message on it waiting to be read. Returns `true` if a message exists and can be accessed; otherwise returns `false`. If `size` is specified, the size of the waiting message is stored in the location specified upon success.

If `size` is `NULL`, the size of the message is ignored; the process calling `afid_lock` may have to determine this size separately.

`FILE *afid_lock(afid_hub *hub, char *mode)`
Returns a file descriptor ready to be read from/written to. This function abstracts away the task of determining the correct file stream and setting appropriate locks. `mode` is any of the modes useable by `fopen()`; afid will determine the correct path and locks based on this mode.

`void afid_unlock(afid_hub *hub)`
Finalizes any file access and removes file locks to allow the files to be read from/written to by other processes.

## MIT License

Copyright (c) 2021 auul

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
