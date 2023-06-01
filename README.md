# fuse

This is the begining of my project "BPF based optimization for Remote Fuse filesystem"

# My notes
I have two designs at the moment.

my first design is based on looping on the internal splicing implementation while
updating the respective splice offsets.

The limitation of this is that one of either the receiver or sender has to be a pipe,
in my case particulary it the receiver that has to refer to a pipe.

Why is this a limitation: 1.Each pipe has its own set of pipe buffers .
Essentially, a pipe buffer is a page frame that contains data written into the pipe
and yet to be read. Each pipe makes use of 16 pipe buffers. 

From my observation the splice system call is not good when i want to transfer a data range
continuously. the data yet to be read can be spread among several partially filled pipe buffers:
in fact, each write operation may copy the data in a fresh, empty pipe buffer if the previous
pipe buffer has not enough free space to store the new data. Hence I may end up blocked and writing
very small amount of used data.

Another limitation is that from our filesystem , we splice from the socket to the pipe. The socket is a special
file system hence it can not be used with copy_file_range(). Also the socket is not seekable. So the solution
i suggest is to copy the data to a temporary file and perform the operations from there.
