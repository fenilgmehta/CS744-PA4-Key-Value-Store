# CS744-PA4-Key-Value-Store

- **Subject** - Design and Engineering of Computing Systems (CS 744)

- **Assignment 4** - A performant and scalable key-value server from scratch using C/C++
    - Multiple clients connect to the key-value server and once connected, repeatedly send requests for either getting the value associated with a key or putting a new key-value pair or deleting an entry associated with a key.
    - Communication between the clients and the server will take place over the network through sockets.
    
- **Team Members**
    - 203050054 - Fenil Mehta
    - 203050110 - Vipin Mahawar
    - 203050064 - Priyesh Jain

### Performance Analysis

- Refer [Performance Analysis Graphs](Performance%20Analysis%20Graphs)


### Compilation and Execution

##### Execute this before running the server: `mkdir -p db ; chmod 777 db ;` - otherwise the server won't be able to write to the database folder/directory

##### Refer [README - Proof of correctness.sh](./README%20-%20Proof%20of%20correctness.sh) to verify that `KVServer.cpp`, `KVClient.cpp`, `KVCache.hpp`, `KVStore.chpp`, `KVClientLibrary.hpp` and `KVMessage.hpp` are working correctly.

- Compiling Server and Client

    ```sh
    # for performance testing
    make all
  
    # for debugging and checking what the client and server are
    # doing using logging (with "std::cout" and "std::cerr")
    make debug
    ```

- Executing Server

    ```sh
    # Can edit the server configuration in "KVServer.conf"
    #  - Key comes before space " ", and Value comes after space " "
    #  - Each Key-Value pair comes on a newline ("\n")
    
    # For performance testing
    ./KVServer
    
    # For viewing how the server is receiving the requests and how they are being served
    ./KVServer_db
    ```

- Executing Client

    ```sh
    # ./KVClient client_request_001.txt SERVER_IP SERVER_PORT
    # ./KVClient_db client_request_001.txt SERVER_IP SERVER_PORT
    
    # For performance testing
    ./KVClient req/client_request_001.txt 127.0.0.1 12345
    
    # For viewing how the client loads the "client_request_[0-9]{3}.txt"
    # and know the results of the requests made
    ./KVClient_db req/client_request_001.txt 127.0.0.1 12345

    # For automatic testing for the correctness of the program
    # "req/client_request_001.txt" contains the requests and
    # "req/client_request_001_sol.txt" contains the possible solution
    # to the former requests file had the server been run for the first
    # time and only a single client been making the requests
    ./KVClient_db req/client_request_001.txt 127.0.0.1 12345 req/client_request_001_sol.txt
    ```
  
- Automatic Testing: these Python 3 scripts can be used to generate testing dataset and be passed to KVClient.cpp for sending the requests to the KVServer.cpp

    ```sh
    request_count=50000
    file_name="client_request_007.txt"
    
    file_name_without_extension="${file_name%.*}"  # REFER: https://stackoverflow.com/questions/965053/extract-filename-and-extension-in-bash
    file_sol="${file_name_without_extension}_sol.txt"
    server_port_number=$( cat KVServer.conf | grep LISTENING_PORT | awk '{print $2}' )
    
    python PythonClientRequestsGenerator.py <<< "${request_count}" > ${file_name} 
    python PythonSolutionsGenerator.py < ${file_name} > "${file_sol}" 
    
    # NOTE: use "make all" and "./KVClient ....." for performance testing
    make debug
    ./KVClient_db ${file_name} 127.0.0.1 ${server_port_number} ${file_sol} 
    ```


### Implementation Details

- Tested with 1 to 10 clients    
- Source Code structure
    - **`KVServer.cpp`** - This is the server program which will accept client connections and serve them
    - **`KVClient.cpp`** - Client program which will make connection with server and then send GET, PUT, DELETE requests to the server process using `KVClientLibrary.hpp` provided API
    - **`KVCache.hpp`** - In Memory Cache which works in collaboration with `KVStore.hpp` to serve `KVServer.cpp`
    - **`KVStore.hpp`** - Interface to hd_GET, hd_SET and hd_DELETE key-value pair to and from the Persistent Storage (i.e. Hard Disk)
        - `hd` in the above interfaces refer to hard disk
    - **`KVClientLibrary.hpp`** - API provided to the Client for making requests to the server. This is a library, which will encode and decode the client request and response messages.
        - For example, at the client side this library will encode the request message to the decided message format, and then send it out to the server process. Similarly, it will decode the response received from the server, and then hand out the decoded response to the KVClient module.
    - **`KVMessage.hpp`** - Client and Server communicate using the message format given in this header file
    - **`MyDebugger.hpp`** - Functions to help with printing of logging/debugging information
    - **`MyMemoryPool.hpp`** - A thread-safe faster alternative to multiple calls of malloc/new operator


### Special Care to be Taken

- It is the role of the KVClient.cpp to correctly set a KVMessage object before sending and request
    - This is necessary because everything at the server side assumes that the message received has been correctly padded with null ('\0') character if the length is < 256  
- When working with multi-threaded program, take care not to allow everyone to modify any file/data structure at the same time and make it inconsistent
- When working with locks, take care not to get into deadlock


### REFERENCES

- [PintOS Thread API](https://github.com/guilload/cs140/blob/master/ps0/pintos_thread.h)
    - [PintOS code](http://people.cs.ksu.edu/~bstinson/courses/cis520/grandepintos.proj1/threads/synch.c)
- [How to make a circular queue thread safe](https://stackoverflow.com/questions/15751410/how-do-i-make-a-circular-queue-thread-safe)
- Other references have been mentioned in the places where they are found to be effective
