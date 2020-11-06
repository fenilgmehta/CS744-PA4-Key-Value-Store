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
    - **`KVClient.c`** - Client program which will make connection with server and then send GET, PUT, DELETE requests to the server process using `KVClientLibrary.hpp` provided API
    - **`KVCache.hpp`** - In Memory Cache which works in collaboration with `KVStore.hpp` to serve`KVServer.c`
    - **`KVStore.hpp`** - Interface to hd_GET, hd_SET and hd_DELETE key-value pair to and from the Storage (Hard Disk - SSD *or* HDD *or* any other type)
        - `hd` in the above interfaces refer to hard disk
    - **`KVClientLibrary.hpp`** - API provided by the server for the Client. This is a library, which will encode and decode your request and response messages.
        - For example, at the client side this library will encode your request message to the decided message format, and then send it out to the server process. Similarly, it will decode the response received from the server, and then hand out the decoded response to the KVClient module. 
    - **`KVMessage.hpp`** - Client and Server communicate using the message format given in this header file
    - **`KVMessageFormat.hpp`**


### Special Care to be Taken
- It is the role of the KVClient.cpp to correctly set a KVMessage object before sending and request
    - This is necessary because everything at the server side assumes that the message received has been correctly padded with null ('\0') character if the length is < 256  
- When working with multi-threaded program, take care not to allow everyone to modify any file/data structure at the same time and make it inconsistent
- When working with locks, take care not to get into deadlock


### REFERENCES
- [PintOS Thread API](https://github.com/guilload/cs140/blob/master/ps0/pintos_thread.h)
    - [PintOS code](http://people.cs.ksu.edu/~bstinson/courses/cis520/grandepintos.proj1/threads/synch.c)
- [How to make a circular queue thread safe](https://stackoverflow.com/questions/15751410/how-do-i-make-a-circular-queue-thread-safe)
