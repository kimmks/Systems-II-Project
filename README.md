CS 361: Computer Systems II				        C. Taylor   
Request for Comments:					       E. Trafton
Obsoletes:						        A. Malone
Category: System Specification				       B. Stoller
								  K. Kimm
                                                      February - May 2019

        System Specification for Concurrent Database 
         Server and Client with a Network Interface


Table of Contents:

1.0 System Overview
2.0 Client Overview
  2.1 Command-line Execution
  2.2 Stress-Testing Suggestions
3.0 Server Overview
  3.1 Command-line Execution
  3.2 Components (Back-end Utilities)
  3.3 Summary of Inter-process Communication Techniques
  3.4 Multithreaded Implementation
  3.5 Performance Requirements
4.0 Data Sources
5.0 Data Indexing
  5.1 Tree Implementation: AVL
  5.2 Data Storage
  5.3 Data Parsing
  5.4 Server-Tree Interface
6.0 Data Compression and Decompression
7.0 Data Transfer Protocols
  7.1 Data Set Load/Unload
  7.2 Data Index Build/Destroy
  7.3 Data Search
  7.4 Error Message Response
8.0 Team Contract and Discussion of Responsibilities
  8.1 Team Rules
  8.2 Clarifications
9.0 Suggestions for Future Improvement

1.0 System Overview
-------------------------------------

2.0 Client Overview
-------------------

2.1 Command-line Execution

A client must be able to run the executable "net" from a command-line 
interface. In a terminal, a client must be able to send five types of 
requests, as specified in section 7 (Data Transfer Protocols). The 
command-line statements for these protocols are as follows.

Data Set Load: ./net -l <filename.csv>
Data Set Unload: ./net -u
Data Search: ./net -S <search-terms>
Index Build: ./net -b <field-name>
Index Destroy: ./net -d <field-name>

In addition, a client should provide confirmation messages displayed in 
the terminal to indicate actions to the user of the client.

2.2 Stress-testing Suggestions

Stress-testing is an important part of any system development process. 
The system is required to run at a reasonable speed, even when building 
indexes off of CSV files with more than 100,000 records. If 
stress-testing is implemented, a client should send a special request 
to the server to indicate that stress-testing should begin. The request 
should be indicated by a unique command-line flag. For example, 
"./net -x" as the command-line statement.

2.3 Client Logic

The logic that creates a new client must be located in the run_client() 
function in the server class (server.c). Other parts of server.c are specified 
in Section 3.4.

[put run_client() specification here]

3.0 Server Overview
-------------------

3.1 Command-line Execution
  
The server must be able to be run from a command-line interface. It is 
recommended to organize the server's files in a way such that all 
relevant files can be compiled with a single call to gcc. The server 
must be run in a terminal using the command "./net -s". Once the server 
is active it will wait for requests from a client.

When a request is received from a client, various log messages are 
displayed on the server side to illustrate actions and track errors. The 
specification for these log messages is below.

"<Request-Type> RECEIVED"
  
3.2 Components (Back-end Utilities)

The components of the server are the so-called "Back-end Utilities" used
for processing of the data file, indexing the file, executing operations
on indexed data, and preparing the data for transfer to the client over 
a network connection. For efficiency, the back-end utilities should use 
multiple processes to handle various tasks.

The components are: compression/decompression utilities (Section 6), 
SHA256SUM hashing algorithm (Section 7), a tree data structure that 
allows for efficient searching and insertion (Section 5), CSV parsing 
and processing algorithms (Section 5), and a controller to manage the 
system as a whole. In the original implementation of the system, the 
server itself acted as the controller.

3.3 Summary of Inter-process Communication (IPC) Techniques

IPC techniques that must be used in the system include memory-mapped 
files, pipes, and network sockets. 

All back-end utilities share access to a mapped region of memory 
containing the entire CSV data file. Memory mapping occurs as part of 
the data set loading process and uses the mmap system function to achieve
this. Memory-mapping the file allows back-end utilities running in 
different processes to access the same information. An important note is
that none of the algorithms in the server modify the original CSV file in 
any way and only read data from it. CSV parsing and processing algorithms
are specified in Section 5.

Pipes are used in the back-end utilities to communicate the hash value 
produced by the SHA256SUM command-line utility to the process responsible
for sending the data packet (Section 7.3) to the client.

Network sockets are used to allow multiple clients to send requests to 
and receive responses from the server. The server uses a multithreaded 
queue system to handle incoming client requests and allows up to four 
worker threads to work with a different client socket connection.

3.4 Multithreaded Implementation

  3.4.1 Main Driver
  
  The main driver (main.c) of the server includes command-line parsing for the commands
  detailed in Sections 2.1 and 3.1. If the -c flag is passed, it initiates the
  run_client() function in the server class (server.c), which runs this version of the program
  as a client that can send requests to the server. If the -s flag is passed, it 
  initiates the run_server() function in the server class, which uses the domain as
  a server on localhost port number 4025. Only after one type of start-up state is
  specified can a client send the load, search, build, destroy, and unload requests to the running server.

  Note: Our implementation runs both the clients and the server on JMU's stu 
  shared network. So, an appropriate and available port number should be chosen 
  based on the network on which the system will be run.

  The driver can also be responsible for some user-related error checking from the 
  command-line input if the specific CSV files are being used. For example, our 
  implementation identifies which of the two CSV files were loaded and checks any
  given field names against a list of searchable fields.

  3.4.2 Server Design and Logic

  The logic of the server is centrally housed in the run_server() function in the 
  server class. The client logic (run_client()) is discussed in Section 2.3. The server
  class also contains functions necessary to handle other requests from clients. The 
  server uses a circular queue to manage incoming connections from clients by 
  passing them to four worker threads that will accept requests from the given
  client. 

  The run_server() function must first initialize the semaphores and locks necessary
  to synchronize thread access to the circular queue.

  3.4.3 Server Functions 
  
  char *run_server ()
  
  The run_server() function creates a TCP server socket on localhost for port number 4025.
  This server is permanent and can be only be shut down remotely. It reads from a socket 
  and then writes back what was in it. The server has a 10 second timeout for connections.
  It can hold up to 10 pending connections at once. The server will exit when the string 
  "exit" is sent to it. It will then close the socket and return.

  request_t read_data_request ( char *buffer, size_t req_len, int sockfd )

  The read_data_request function reads in a data request and stores it for later parsing.
  The buffer will store the data from the socket. The request length (req_len) will be used
  to read from the socket. The socket to read from will be identified by the given file 
  descriptor (sockfd). This function checks the type of request based on the first 
  character of the buffer data (search, load, build, unload, destroy) and returns the type as
  a request_t (an enumerated struct with the different request types).

  void write_ack_message ( int sockfd, request_t type )
  
  The write_ack_message function writes the proper data acknowledgment message to the client.
  It uses the given file descriptor to identify the client's socket and the type of request
  to determine which message to send. The protocol of the message is specified in Section 7.3.

  void write_err_message ( int sockfd, request_t type )

  The write_err_message function writes the proper error message to the client specified by
  the given file descriptor. The type of request determines the specific content of the message.
  The protocol for error messages is specified in Section 7.4.

  void write_data_packet ( uint32_t size, char *comp_data, char *hash, int sockfd )

  The write_data_packet function puts the data packet into the proper form (Section 7.3) 
  and writes it into the socket identified by the given file descriptor (sockfd).
  The size is the size of the compressed data (comp_data) in bytes, which will be added to
  72 to get the size of the full packet. The hash must be included in the data packet as 
  well and its size is accounted for in the 72 bytes. See the protocol specification
  for more detail.

3.5 Performance Requirements

The server must be able to handle two files, although only one can be 
loaded at a time. In the original implementation, two files were used: 
one "small" one (less than 100,000 records) and one "large" one (over 
200,000 records). The server must also be resilient to heavy load so 
it must be able to handle up to 10,000 sequential requests from several 
clients without failing.

4.0 Data Sources
----------------

Data is provided to the system through files containing Comma
Separated Values (CSV). It is recommended by the original authors of 
this system to choose particular CSV files to use with the application
and alter the code to accommodate the particular categories of 
information. Otherwise, further design will be necessary to make the 
system automatically work with any CSV file. Another important note from 
the authors is that not all categories (fields) of a particular CSV file
are made searchable by this application. This decision was made to reduce 
the complexity of building indexes of the given data and searching said 
indexes. In particular, the original implementation of the system was 
built on CSV fields that were either character or numerical data types, 
were reasonable to be searched by a user relatively familiar with the 
CSV file's structure, and had a larger degree of variability to its 
values.

Note that from this point on in this specification, any mention of 
"data set" or "file" is referring to CSV files located in the
project directory or another location accessible to the system's core
files.

5.0 Data Indexing
-----------------

  5.1 Tree Implementation: AVL
  
  To provide necessary functions to the client, the system must rely
  on a database to store and access given data. The system uses an AVL 
  tree data structure to provide these functions. Trees are comprised of
  nodes implemented as structs with fields to store the height of a node, 
  pointers to child nodes, a record value and number, and a pointer to a 
  different struct for the case of duplicate values. The struct for 
  duplicate values contains fields storing a record value and number,
  and a pointer to to another duplicate struct. When a tree is built
  from a given field, duplicate values are inserted as a node in a linked
  list, beginning at the first instance of the repeated value. Searching
  is performed preorder, returning the node in which the value is found.

    5.1.1 Justification
    
    Authors chose to implement an AVL tree structure over other related 
    structures due to familiarity with standard implementations and the 
    range of capabilities AVL tree structures provide. AVL tree is a self
    balancing binary search tree, where the difference of heights of 
    child subtrees cannot be more than one for any given node. Traditional
    AVL trees do not support duplicate values, so the authors made a 
    design decision to implement linked list structures in nodes where 
    repeated values appeared. For example, in a tree built off the 
    searchable field "Year" the first instance of 1998 would result in a
    new tree node being created and inserted into the AVL tree. All 
    following instances of the year 1998 would result in the creation of
    a duplicate node inserted into the linked list pointed to by the 
    original node. This decision increased the time complexity of
    searching, but was found to be the most efficient method given the 
    AVL tree data structure. The time complexity of our unique AVL tree
    insertion is O(log n) because duplicates are added to the start of 
    the list, and the worst case search time complexity is O(n) because
    of the way duplicate values are handled.     

    5.1.2 Necessary Functions

    There are three standard operations necessary to implement a tree 
    structure: insert, search, and delete. This system does not require
    the arbitrary deletion of a node, only the ability to delete an 
    entire tree. Note that future references to delete describe the 
    deletion of an entire tree rooted at a given node, not a node
    within a given tree. Utility functions are only used by other 
    functions contained in the tree.c file and do not provide function-
    ality to the client. An interface between the back end data indexing
    utility and the server is required to proide full functionality. 
    Information regarding utilities and functions required to fully 
    implement a standard AVL tree is readily available outside of this 
    specification. The functions specified below are unique to this 
    system and are compatible with any standard implementation of an 
    AVL tree structure with minor modifications. 

    int compare(char*, char*) 

    The compare function compares two record values, returning -1 if 
    the first record is less than the second record, 0 if the records
    are equivalent, and 1 if the first record is greater than the second.
    this function is used in both search and insert to determine traver-
    sals. Note that both numeric and string values are generalized as 
    char *, and that numeric values are compared using the C standard
    library function atof().

    node insert(node, char*, int)

    The insert function is used to build a tree from a given dataset. It
    compares the record value of the given node with the values in the 
    tree, creating and inserting a new node in the tree based on the 
    standard definition of an AVL tree. Values less than that of the root
    are inserted in the left subtree, and values greater than that of the
    root are inserted in the right subtree. In the case of repeated values, 
    insert() adds the node to the linked list pointed to by the node 
    storing the first instance of that value. Insert() performs rotations by 
    using helper functions to determine where an imbalance occurred, and 
    which rotation is necessary to become rebalanced.

    node search(node, char*, int)
 
    The search function searches a tree rooted at the given node for the 
    given key. Searches are performed preorder, and if no matching key
    is found, null is returned.

  5.2 Data Storage 

  Since the original file must be accessible among all back-end utilities
  whether they are running in the same process or not, it must be memory-
  mapped into a shared memory region as part of file processing. The
  mmap() system function must be used with no specified address, the size 
  of the file in bytes, read-only permissions, shared privacy settings, 
  and the file descriptor returned from opening the file. Mapping of the 
  file occurs when the load_file function is executed in the treecontrol 
  class (specified in Section 5.4). The memory-mapped file must be 
  un-mapped during the unload_file operation (Section 5.4).
      
  5.3 Data Parsing 
  
  Once the CSV file is opened and memory-mapped using mmap, it must be 
  processed in order to be properly accessible to the tree algorithm.
  This is achieved through the csvindexer class that contains the 
  following methods. Both methods provide error checking for invalid 
  pointer parameters. Data parsing occurs when the build_tree and 
  search_tree functions are executed in the treecontrol class.

  int get_num_records ( char *mapAddr, size_t filesize )

  This method is responsible for determining the number of records in a
  given CSV file. It takes a char * to the first byte of the memory-
  mapped region and the size of the file in bytes (as accessed through
  the fstat() system call). It tracks through the CSV file byte-by-byte 
  and increments the number of records when new line characters ("\n" or 
  "\r") are encountered. The number of records is used as the size of the 
  offsets array that is created and filled by 
  index_mapped_file()(constant OFFSET_SIZE).

  int index_mapped_file ( char *mapAddr, size_t file_size, 
    int offsets[OFFSET_SIZE], int offsets_size)

  This method is responsible for identifying and collecting the byte 
  offsets of every record in the mapped file. It takes a char * to the
  first byte of the memory-mapped region, the size of the file in bytes
  (as accessed through the fstat() system call), an int[] that will store 
  the byte-offsets, and the size of the offsets array (which will be
  accessed through get_num_records()). The offsets array will have 
  indexes that correspond to the "record number" (i.e. offsets[1] 
  contains the byte-offset in the memory-mapped file where the second
  record's first byte is). This makes record look-up by the record number 
  stored in a tree node relatively trivial to implement.

  5.4 Server-Tree Interface [treecontrol]

  There must be an interface between the tree functionality and the
  controller so that the controller can focus on communication and 
  managing the clients. This interface is implemented through wrapper 
  functions in the treecontrol class. 
  All functions in treecontrol must not rely on any global variables in 
  the class itself and thus receive necessary information from 
  parameters. This is due to the multithreaded nature of the server. The 
  controller is responsible for passing the appropriate data to each 
  function and calling the functions in the proper order. Also, all 
  functions that are passed the char *mapAddr parameter are responsible 
  for copying the address if performing any pointer arithmetic. This 
  ensures that the entirety of the mapped region can be freed using the 
  same pointer. 
  The treecontrol class' functions are specified below with explanations 
  of the functionality they add to the tree functions.

  char *load_file ( char *filename, int *file_size )

  This function opens and memory-maps the file with the given file name.
  The file_size parameter is set to be the size of the file in bytes (as
  accessed by the fstat() system call). It returns a char * reference to 
  the first byte of the memory-mapped region, or NULL if an error
  occurred.

  node build_tree ( char *field_name, int file_size, char *mapAddr)

  This function creates a new tree and inserts the value of the given 
  field in every record from the memory-mapped file into it. It takes
  the name of the field (which must be verified to be a "searchable" 
  field), the size of the file in bytes, and the pointer to the beginning
  of the memory-mapped file. It will return a node type (specified in 
  Section 5.1) that is the root of the new tree, or NULL if an error
  occurred. 
  In order to make this function self-reliant, csvindexer's 
  get_num_records and index_mapped_file must be used before beginning to
  build the tree. The offsets array must be dynamically allocated when 
  created and freed at the end of the function. Building the tree 
  involves traversing through the offsets array, accessing the record as
  a string, parsing out the correct value on the line, and inserting the
  value and the record number (index of offsets array) into the tree. 
  Insertion occurs by using the tree's insert() function. Each record 
  must be dynamically allocated and freed at every iteration through the
  offsets array.

  char *search_tree ( node root, char *mapAddr, int file_size, 
      char *value)

  This function searches the tree with the given root for the given
  value and returns a string containing the raw record(s) from the mapped
  file. The returned record is dynamically allocated and the caller is
  responsible for freeing the memory before exiting. 
  In order to make this function self-reliant, csvindexer's 
  get_num_records and index_mapped_file must be used before beginning to 
  build the tree. The offsets array must be dynamically allocated when 
  created and freed at the end of the function. Searching retrieving 
  nodes from the tree, accessing the record in the memory-mapped file via
  the record number, and concatenating them together in order to return 
  the entire set. It uses the node_t struct and the Dups struct (Section
  5.1) to access duplicate records by getting the first node from the 
  tree's search() function and going through the linked list of 
  duplicates stored in the node. If the tree's search() function returns
  a NULL node, the function must return "No records were found with the 
  value of _______.". If the function hits an error, it must return NULL.
  Note that several dynamic memory allocations should be used when 
  accessing additions to the result set and when concatenating those 
  additions to the result set.

  int destroy_tree ( node root )

  This function uses the tree's deleteTree() function with the given root
  to destroy the tree. See the specification for the deleteTree()
  function for further clarification (Section 5.1). It returns 0 if 
  successful, -1 otherwise.

  int unload_file ( char *mapAddr, int file_size )

  This function unloads the current data set by using the munmap() system
  call to unmap the shared memory region. It takes the pointer to the 
  region and the file's size in bytes to pass directly to munmap(). It 
  returns 0 if successful and -1 otherwise.

6.0 Data Compression and Decompression
--------------------

6.1 Compression

The algorithm must first contain a way to parse a CSV file into its 
individual categories. This function also records the number of columns 
in the CSV file for later use. Once the individual categories are 
separated they are passed to a frequency count function. This will record
the frequencies of each category as a struct with the entry and current 
frequency. These structs will be dynamically allocated and accessed like
an array for sorting later. This process is repeated line by line for 
the entire CSV file until there is a frequency of every unique entry in
the file. Empty entries in the CSV file will be recorded as spaces. 
The number of lines in the file should also be kept track of for 
determining the size of the compressed data. 

Once the frequency count is competed the structs will be sorted using
a quicksort algorithm that will sort based on the largest frequency. 
Once the unique entries are sorted the entries are then assigned keys.
These keys are integers that range from 1->Number of unique entries.
The entry that occurs the most will have the key 1. 

After the keys have been assigned a piece of memory the size of the 
number of rows times the number of columns times four is allocated. 
Then the file is traversed line by line again and each line is parsed
into individual categories like before. For every line that is parsed
each entry is compared until the corresponding key is found. After that
the key is written to the memory allocated earlier. This process is 
repeated until all of the entries in the CSV file are converted.

After this the compressed records are stored in a struct with the key
value pairs, the number of rows, the number of columns, and the number
of key value pairs. 

6.2 Decompression

The algorithm described in the Data Compression section is unique in that
it allows line by line decompression of data. Since each four bytes of 
the data represent an entry in the file any part of the data can be 
decompressed. The decompression function should work for one line at a
time. First it seeks to the requested compressed row in the data. Then
it accesses each integer key at a time and compares it to the key-value
list until the value is found. After that the characters are written to 
a piece of allocated memory that will be returned later. 

This process is then repeated for the entire line. The decompression
algorithm also inserts commands back into the appropriate spot in the
line. The line returned should be an exact replica of the line from the
original file. This allows re-usage of the line by line parsing from the
earlier compression algorithm. 

To decompress the entire file, the function for decompressing one line
is called repeatedly for every line in the file. After that each line 
can be written out to a file or sent over an IPC. 

6.3 Implementation Notes

Although data compression is fully implemented in the original
system by A. Malone, other implementations can use exec() system calls
to execute command-line compression programs such as "zip". The 
opportunity presented by command-line programs was unknown to the 
authors of this application at the time that data compression was
implemented originally.

The implementation for compression uses the Linux utility gzip on
the machine as well. The function will first create a temporary file
to write the data into and then compress. The data returned from the
B tree will first be written into the temporary file. After that the
process must fork(). The child should then call exec with gzip and 
the file name. The file's name should be passed as an ID to the function
as well so multiple threads will not read and write to the same file.
Once the data is written and compressed the parent should then read all
the compressed data back in. The function will then fork again and the 
child will call exec with "rm" to remove the temporary file. The 
compressed data is then returned to the server thread to send over to 
the client. As a final note the file name ID must be saved so the 
decompression happens with the same temporary file name.

The implementation for decompression works the same way. The only 
difference is that the program gunzip is used for decompression.
As a reminder the same file name ID must be used in decompression or the
data will come out corrupted. 


7.0 Data Transfer Protocols
---------------------------

Data transfer protocols encompass the exact structure and ordering of
messages sent over a TCP socket between the Client 
and the Server. A client (from the server's perspective)
must be able to send three different types of requests: 
data set loading/unloading, data index building/destroying, and data 
searching. 

These three types of requests are mostly linearly dependent on each
other. For example, a client cannot search a data field that does not 
have an index built yet and an index cannot be built without a 
particular data set loaded in the server. An important exception to this
rule is that upon loading a data set the server also builds an initial
default index on one of the searchable fields to allow the client to 
search immediately.

7.1 Data Set Load/Unload

The first sequence that a new client should initiate with the server
should be the loading of a data set. The system must also provide
the ability for the client to unload the current data set. This enables
the user of the system to move between multiple data sets. The protocol
for the Data Load Request is below.

<Data-Load-Request>: "LOAD " <file-name>
<file-name>: 1*74CHAR ".csv"
                   
Maximum Length: 83 Bytes

Once the client has finished sending all Data Index Build/Destroy and
Data Search requests desired, another data set may be loaded. First
however, the current data set must be unloaded. The protocol for the 
Data Unload Request is below.

<Data-Unload-Request>: "UNLOAD"

Maximum Length: 6 Bytes

7.2 Data Index Build/Destroy

Once a data set has been loaded, the client must be able to request for
an index to be built based upon a searchable field in the data set. 
For example, if the client wants to search for all records in the data 
set with Percentages equal to 50.0 then first it must build the index 
based on the Percentages field. The protocol for an Index Build Request
is below.

<Index-Build-Request>: "BUILD " <field>
<field>: <1*20CHAR>

Maximum Length: 26 Bytes

Since all indexes are persistently stored in an output file [name of
output file], the client also must have the option to destroy indexes 
as it sees fit. If a data set is not useful to the client anymore, it
makes little sense to continue to store indexes associated with it. The
protocol for the Index Destroy Request is below:

<Index-Destroy-Request>: "DESTROY " <field>
<field>: <1*20CHAR> 

Maximum Length: 28 Bytes

7.3 Data Search

Once a data set has been loaded and all indexes have been built for
the searchable fields desired by the client, then the client is allowed
to send a Data Search Request. 

Assuming that the client is connected to the server properly, the first
message sent must be from the client to the server and will be the Data
Search Request. Specification for Data Search Request below.

<Data-Search-Request>: "SEARCH " 1(<field>"="<value>)
<field>: <1*20CHAR>
<value>: <1*20CHAR [ "." 1*3DIGIT]>

Maximum Length: 59 Bytes

The server will accept and acknowledge the successful receipt of the 
request with a Request Received message, defined below.

<Request-Received>: <request-type> " RECEIVED" CRLF
<request-type>: "Data Search Request" | "Data Set Load Request"
                | "Data Set Unload Request"
                | "Data Index Build Request"
                | "Data Index Destroy Request"

Maximum Length:  35 Bytes

When the client successfully received the Request Received message from
the server, it must wait for a message from the server containing the 
Data Result Packet, defined below.

Data Result Packet
|----------------- 64 bytes ------------------|
|-----8 bytes------|--------------------------|
|                 hash value                  |
|    size          | data message             |
|---------------------------------------------|

Maximum Length: 72 + n Bytes (n = size)

The 64 byte hash value is a fixed length number generated by the 
SHA256SUM command-line utility that is necessary to ensure the 
integrity of the compressed data sent over the socket. The client will
first read the 64 bytes of the hash value and store it for later use. 
Next the client will read the 8 byte size n of the compressed data segment
and store that information. The client must store the size as a uint32_t
data type. Finally, the client will read n bytes that will produce the 
compressed data segment returned from the server. 

7.4 Error Message Response

If the server is unable to complete any request that the client sends, 
regardless of whether it is due to user error or an internal failure, 
then the server must send an Error Message Response back to the client
indicating the failure. The protocol for an Error Message is below.

<Error-Message>: <request-name> " FAILED" [<description>] CRLF
<request-name>: "Data Search" | "Data Set Load"
                | "Data Set Unload"
                | "Data Index Build"
                | "Data Index Destroy" 
<description>: *15ALPHA

Maximum Length: 41 Bytes

8.0 Team Contract and Discussion of Responsibilities
----------------------------------------------------

Our team members: Andy Malone, Kearstin Kimm, Elena Trafton, Ben Stoller,
and Courtenay Taylor. Roles and responsibilities are discussed below.

+=======================+=====================+=========================+
| Task                  | Primary team member | Secondary team member   |
+=======================+=====================+=========================+
| State machine         | Elena Trafton       | Courtenay Taylor        |
+-----------------------------------------------------------------------+
| Persistent index      | Kearstin Kimm       | Ben Stoller             |
+-----------------------------------------------------------------------+
| CSV parsing           | Andy Malone         | Courtenay Taylor        |
+-----------------------------------------------------------------------+
| Integrity check       | Ben Stoller         | Kearstin Kimm           |
+-----------------------------------------------------------------------+
| Mega Manager          | Courtenay Taylor    | Andy Malone             |
+-----------------------------------------------------------------------+
| RFC Documentation     | Courtenay Taylor    | Andy Malone             |
+-----------------------------------------------------------------------+
| Memory mapping        | Elena Trafton       | Andy Malone             |
+-----------------------------------------------------------------------+
| Multithreaded Server  | Andy Malone         | Elena Trafton           |
+-----------------------------------------------------------------------+
| Server-Tree Interface | Courtenay Taylor    | Kearstin Kimm           |
+-----------------------------------------------------------------------+
| Framework Tests       | Elena Trafton       | Courtenay Taylor        |
+-----------------------------------------------------------------------+
| AVL Tree              | Kearstin Kimm       | Ben Stoller             |
+-----------------------------------------------------------------------+
| Network Protocols     | Andy Malone         | Courtenay Taylor        |
+-----------------------------------------------------------------------+
| Tree Documentation    | Ben Stoller         | Kearstin Kimm           |
+-----------------------------------------------------------------------+
| Client UI             | Ben Stoller         | Andy Malone             |
+-----------------------------------------------------------------------+


8.1 Team Rules

Whenever something is committed to the mercurial log there will be a
clear description of what was added to the code.

All teams members will bring some form of code or questions to the
meeting.

Code must be documented and formatted before it is committed to
mercurial.

We will use Slack and GroupMe to share portions of code and communicate 
with other team members.

All team members will turn a notification system on for Slack.

During meetings we will peer review code and ask questions.

Weekly goals are to be met by Monday of the following week.

Working meetings will take place on Sunday/Monday and last 2 hours. 
The goal during these meetings is to work on the project in person 
together.

Check-in meetings will continue to take place on Wednesdays from 
7-9PM. The goal for these meetings to to make progress checks on each 
member and the group as a whole.

Members who will miss or be more than 10 minutes late to a meeting must 
notify the others in Slack.

8.2 Clarifications

The "Mega Manager" position is a pseudo-team-leader position that is 
responsible for setting meetings, setting goals for each week (Sprint), 
maintaining lists of tasks to be done, main liaison to Dr. Kirkpatrick, 
and having a general understanding of the system as a whole.

As the project got underway, we decided that it would be better to have
Elena continuously working on testing with the framework that Dr.
Kirkpatrick provided. So the secondary team members represent people 
working closely on a task together rather than code testers. Another note
is that large chunks of code (i.e. server and client) were tested 
incrementally through main functions during individual development as 
well.

9.0 Suggestions for Future Improvement
--------------------------------------

Since this project had certain time and resource constraints (all team 
members are undergraduate students), some aspects of this design may be 
considered incomplete or not well-designed by industry standards. Here 
we list some suggested improvements to the design for future 
implementers.

- Generalize the file-handling algorithms for any CSV-formatted file.
- Use more command-line utilities through exec() system calls.
- Employ more multiprogramming in the back-end utilities to increase 
  efficiency.
- Streamline memory usage and tree building speed in the tree 
  implementation.
- Implement a B-Tree instead of an AVL tree to possibly cut down on 
  rebalancing costs.
- Generalize to work on Linux, MacOS, and Windows systems.
- Create better and more user-friendly user interface, perhaps a GUI.
- Create client-activated server shutdown mechanism and protocol.
- Implement the ability to query a range of values from a tree.
