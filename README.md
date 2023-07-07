# Fuse
This is the begining of my project "BPF based optimization for Remote Fuse filesystem"
# SECTION 1

# Splice Support in FUSE.

- Splice is a system call which enables userspce program to transfer data between two in kernel memory buffers without copying the data to userspace.

- Splicing was introduced to FUSE to avoid exessive context switching when transfering data between the fuse driver and the data source.

- In its operation, splice requires that one end of the data transfer be a pipe. Hence to fully transer the data from one file source to file destination a minimum of two splice call is required. For example Fuse use a minimum of 3 splice calls to fully transfer the data from data source to device file.
	1. VMSPLICE - to put the header to the write end of the pipe.
	2. SPLICE - to write the actual data to the write end  of the pipe.
	3. SPLICE - to do the transer from the read end of the pipe to the device file.  

# Fuse buffers 

To seamlessy support splicing , Fuse represents its buffer in two forms:

	1. a regular memory region identified by a pointer in the user daemon's address space.
	2. a kerne memory pointed by a file descriptor

# Fuse buffers Structures.

There are two structures associated.


1. Single data buffer

	Generic data buffer for I/O, extended attributes, etc. 		Data may be supplied as a memory pointer or as a file 		descriptor.

```
struct fuse_buf {
	size_t size; 		- Size of data in bytes	
	enum fuse_buf_flags flags;	-Buffer flags
	void *mem;			- Memory pointer Used unless FUSE_BUF_IS_FD flag is set.
	int fd;			- File descriptor, Used if FUSE_BUF_IS_FD flag is set.
	off_t pos;			- File position, Used if FUSE_BUF_FD_SEEK flag is set.
};
```
2. Data buffer vector

	An array of data buffers, each containing a memory pointer or a file descriptor. Allocate dynamically to add more than one buffer.

```
struct fuse_bufvec {
	size_t count;		-Number of buffers in the array	
	size_t idx;			-Index of current buffer within the array
	size_t off;			-Current offset within the current buffer
	struct fuse_buf buf[1];	-Array of buffers
};
```
# Payload Spliting for Network storage.
- Network storage often involves different protocols that govern its operations, resulting in data packets with a specific format, typically consisting of a header, payload, and footer.

- While the objective is to make the requesting application access the payload part of data, the FUSE user daemon may  require the header and footer to perform essential  operations, such as permission enforcement, access control, or data integrity verification.

- The problem may become even complicated for usecases where FUSE is used to create a Virtual File (eg. Merging of different Network files).

# Support for in-kernel Payload Spliting in Fuse daemon.

- To support Payload spliting, the fuse_buf structure is modified to include the sizes for header and footer. 

```
struct fuse_buf {	
	size_t size; 			- Size of data in bytes	
	size_t header; 			- Size of header in bytes, useless unless FUSE_BUF_FD_SECTION is set.	
	size_t footer; 			- Size of footer in bytes, useless unless FUSE_BUF_FD_SECTION is set.		
	enum fuse_buf_flags flags;	- Buffer flags
	void *mem;			- Memory pointer Used unless FUSE_BUF_IS_FD flag is set. If FUSE_BUF_FD_SECTION is set, it is used to hold the header and footer of the data when operation completes. 
	int fd;				- File descriptor, Used if FUSE_BUF_IS_FD flag is set.
	off_t pos;			- File position, Used if FUSE_BUF_FD_SEEK flag is set.
};

```

# Note
- FUSE is implemented to support two APIs, lowlevel API which provide more controls including handling the file to file mapping and Highlevel API built on top of lowlevel.

- To make most of splicing in FUSE, callbacks are to be implemented for the respective API. For both low and highlevel, write_buf is to be implemented as splice_write. However, for splice_read, lowlevel and highlevel API have different ways to implement. Lowlevel API requires the operation be put in the read callback it self with the direct call to fuse_reply_data function, while highlevel API provides the read_buf API to just provide the file descriptor.

- Moreover while splice_write it enabled by default , splice_read is not hence respective SPLICE_READ capability flag have to be enabled durind filesytem initialization to support this.

Example Code for Highlevel API to support Splice

```
int stackfs__read_buf(const char *path, struct fuse_bufvec **bufp,size_t size, off_t off, struct fuse_file_info *fi)
{ 
	// Initial Code here is removed

struct fuse_bufvec *buf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
*buf = FUSE_BUFVEC_INIT(response.size);

// Size of Metadata in bytes
buf->buf[0].header = 102;
buf->buf[0].footer = 102;

// Define FUSE_BUF_FD_SECTION and Pass the FD to splice
buf->buf[0].flags |= FUSE_BUF_IS_FD | FUSE_BUF_FD_SECTION;
buf->buf[0].fd = data->sockfd;
*bufp = buf;
}

```

# FUSE driver Communication.

- FUSE design utilized the device file /dev/fuse as a means for Usespace daemon to communicate with the kernel. Moreover ,the communication is supporteed by the use of specific protol which contain headers for the respective direction of the communication i.e to userspace daemon(IN) or to kernel (OUT).

- The IN header is refered as fuse_in_header and out header as fuse_out_header. As a consequence of using pipe buffers for its communications FUSE reserve a whole Page (4k) to be used for headers. That is for data transfer between the driver and userspace , the first page will contain only the Fuse respective header.

	```C
	struct fuse_in_header {
		uint32_t len;
		uint32_t opcode;
		uint64_t unique;
		uint64_t nodeid;
		uint32_t uid;
		uint32_t gid;
		uint32_t pid;
		uint32_t padding;
	};
	```

	```
	struct fuse_out_header {
		uint32_t len;
		int32_t error;
		uint64_t unique;
		uint32_t header; \\ Added for payload Spliting
		uint32_t footer; \\ Added for payload Spliting
		uintptr_t mem;
	};
	
	```

- To support splice the device file /dev/fuse operation include the callback functions for splice read and splice write which are invoked when splice system call is called it.

# In focus splice_write dev operation.










# SECTION 2. ExtFUSE updates

The user space program gets the ebpf context.

The ebpf context contains the following:
	
	1. ebpf program fd to be loaded
	2. Array of fds representing the created maps.


There are three maps available i.e

	1. entry_map,	
	2. attr_map 
	3. handler - contains the associated handler programs to be executed for a specified OPCODE

Example of Kernel Requests Execution Sequence. 

**Case study 1. Lookup Requests.**

	1. Fetches the lookup cached entry from entry_map 
	2. The handler returns UPCALL(-ENOSYS) if there is no entry
	3. If the entry is available it prepares for output
	4. For positive entries the attributes are fetched from attr_map, if not present the handler returns UPCALL(-ENOSYS).
	5. For negative entries are expected to have no attributes
	6. When the out is populated with the required parameters and inode information  bpf_extfuse_write_args is used to return the 				result to the FUSE DRIVER.
	7. Appropriate entry count is atomically incremented.
	
**Case Study 2. Read Requests.**

	1. Feches the attribute entry from attr_map.
	2. If present continue , otherwise returns UPCALL(-ENOSYS) if not
	3. Checks if its passthrough, returns PASSTHRU(1) otherwise returns UPCALL(-ENOSYS)
	


**Hint:**

	1. bpf_extfuse_write_args wraps probe_kernel_write  
	2. bpf_extfuse_read_args wraps probe_kernel_read

# My Other notes notes
I have two designs at the moment.

1. Based on looping on the internal splicing implementation while updating the respective splice offsets.

	The limitation of this is that one of either the receiver or sender has to be a pipe, in my case particulary it the receiver that has to refer to a pipe. Why is this a limitation: 

	Each pipe has its own set of pipe buffers . Essentially, a pipe buffer is a page frame that contains data written into the pipe and yet to be read. Each pipe makes use of 16 pipe buffers. 

	From my observation the splice system call is not good when i want to transfer a data range continuously. the data yet to be read can be spread among several partially filled pipe buffers:
in fact, each write operation may copy the data in a fresh, empty pipe buffer if the previous
pipe buffer has not enough free space to store the new data. Hence I may end up blocked and writing
very small amount of used data.

Another limitation is that from our filesystem , we splice from the socket to the pipe. The socket is a special
file system hence it can not be used with copy_file_range(). Also the socket is not seekable. So the solution
i suggest is to copy the data to a temporary file and perform the operations from there.

