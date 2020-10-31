# CS744-PA4-Key-Value-Store

- **Subject** - Design and Engineering of Computing Systems (CS 744)
- **Assignment 4** - TO build a performant and scalable Key Value Store from scratch using the *C language*
    - Multiple clients connect to the key-value server and once connected, repeatedly send requests for either getting the value associated with a key or putting a new key-value pair.
    - Communication between the clients and the server will take place over the network through sockets.
- **Team Members**
    - 203050054 - Fenil Mehta
    - 203050110 - Vipin Mahawar
    - 203050xxx - Priyesh Jain


### Implementation Details
- Source Code structure
    - **`KVServer.c`** - This is the server program which will accept client connections and serve them
    - **`KVClient.c`** - Client program which will make connection with server and then send GET, PUT, DELETE requests to the server process using ``
    - **`KVCache.h`** - In Memory Cache which works in collaboration with `KVStore.h` to serve`KVServer.c`
    - **`KVStore.h`** - Interface to hd_GET, hd_SET and hd_DELETE key-value pair to and from the Storage (Hard Disk - SSD *or* HDD *or* any other type)
        - `hd` in the above interfaces refer to hard disk
    - **`KVClientLibrary.h`** - API provided by the server for the Client. This is a library, which will encode and decode your request and response messages.
        - For example, at the client side this library will encode your request message to the decided message format, and then send it out to the server process. Similarly, it will decode the response received from the server, and then hand out the decoded response to the KVClient module. 
    - **`KVMessage.h`** - Client and Server communicate using the message format given in this header file
    - **`KVMessageFormat.h`** - 
