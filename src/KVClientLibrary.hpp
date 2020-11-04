#ifndef PA_4_KEY_VALUE_STORE_KVCLIENTLIBRARY_HPP
#define PA_4_KEY_VALUE_STORE_KVCLIENTLIBRARY_HPP

#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cctype>

#include "MyDebugger.hpp"
#include "KVMessage.hpp"

// REFER: https://www.bogotobogo.com/cplusplus/sockets_server_client.php
// REFER: https://stackoverflow.com/questions/1593946/what-is-af-inet-and-why-do-i-need-it

/*
 *  Code to test the below thing
 *

    const char REQUEST_FAILURE_MSG[] = "Key not found";
    char arr[100] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};
    std::copy(std::begin(REQUEST_FAILURE_MSG), std::end(REQUEST_FAILURE_MSG), arr);
    std::cout << "'" << arr << "'" << "\n";
    std::cout << (int) arr[12] << ", " << (int) arr[13] << ", " << (int) arr[14] << ", " << (int) arr[15] << "\n";

    const char REQUEST_FAILURE_MSG_2[] = "Key not found\0";
    char arr_2[100] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};
    std::copy(std::begin(REQUEST_FAILURE_MSG_2), std::end(REQUEST_FAILURE_MSG_2), arr_2);
    std::cout << "'" << arr_2 << "'" << "\n";
    std::cout << (int) arr_2[12] << ", " << (int) arr_2[13] << ", " << (int) arr_2[14] << ", " << (int) arr_2[15] << "\n";

 * */
const static char REQUEST_FAILURE_MSG[] = "Key not found";  // Internally this has length 14 and last character (i.e. index = 13) is '\0'

struct ClientServerConnection {
    int socketFD;  // used to communicated with the Server over the socket
    uint8_t resultStatusCode;
    char resultValue[256];

    ClientServerConnection(const char *serverIP, const char *serverPort) : resultStatusCode{}, resultValue{} {
        // REFERRED: B.E. Computer Network's file transfer program

        // REFER: https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream
        socketFD = socket(AF_INET, SOCK_STREAM, 0);

        if (socketFD < 0) {
            log_error("Unable to open the socket");
            exit(4);
        }

        struct hostent *server;
        server = gethostbyname(serverIP);
        if (server == nullptr) {
            log_error("No such host");
            exit(5);
        }

        struct sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(atoi(serverPort));
        // REFER: https://stackoverflow.com/questions/19207745/htons-function-in-socket-programing
        // REFER: https://www.geeksforgeeks.org/converting-strings-numbers-cc/

        // connect()
        if (connect(socketFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            log_error("Connection with the server failed");
            exit(6);
        }
    }

    ~ClientServerConnection() {
        close(socketFD);
    }

#ifdef DEBUGGING_ON
#define ASSERT_SUCCESS(rw_operation) \
    if ((rw_operation) < 0) {        \
        log_error(std::string("") + "Error reading/writing to the server: \"" + #rw_operation + "\""); \
        log_error("EXITING (status=7)"); \
        exit(7);                         \
    }
#else
#define ASSERT_SUCCESS(rw_operation) rw_operation;
#endif

    void GET(const struct KVMessage &message) {
        // 1 represents GET request
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>("1"), 1))
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>(message.key), 256))

        ASSERT_SUCCESS(read(socketFD, reinterpret_cast<void *>(resultStatusCode), 1))
        ASSERT_SUCCESS(read(socketFD, reinterpret_cast<void *>(resultValue), 256))
        print_result_returned("GET");
    }

    void PUT(const struct KVMessage &message) {
        // 2 represents PUT request
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>("2"), 1))
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>(message.key), 256))
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>(message.value), 256))

        ASSERT_SUCCESS(read(socketFD, reinterpret_cast<void *>(resultStatusCode), 1))
        if (resultStatusCode == 240) {
            ASSERT_SUCCESS(read(socketFD, reinterpret_cast<void *>(resultValue), 256))
            std::copy(std::begin(REQUEST_FAILURE_MSG), std::end(REQUEST_FAILURE_MSG), resultValue);
        }
        print_result_returned("PUT");
    }

    void DELETE(const struct KVMessage &message) {
        // 3 represents DELETE request
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>("3"), 1))
        ASSERT_SUCCESS(write(socketFD, reinterpret_cast<const void *>(message.key), 256))

        ASSERT_SUCCESS(read(socketFD, reinterpret_cast<void *>(resultStatusCode), 1))
        if (resultStatusCode == 240) {
            ASSERT_SUCCESS(read(socketFD, reinterpret_cast<void *>(resultValue), 256))
            std::copy(std::begin(REQUEST_FAILURE_MSG), std::end(REQUEST_FAILURE_MSG), resultValue);
        }
        print_result_returned("DELETE");
    }

    void print_result_returned(const char *operationName) {
        if (resultStatusCode == 200) {
            log_info(std::string(operationName) + ": was successful");
        } else if (resultStatusCode == 240) {
            log_warning(std::string(operationName) + ": resultStatusCode = 240");
            log_warning(std::string("    ") + "--> Returned Messaged says: \"" + resultValue + "\"");
        } else {
            log_error("INVALID resultStatusCode = " + std::to_string((int) resultStatusCode), true, true);
        }
    }

#undef ASSERT_SUCCESS

};

#endif // PA_4_KEY_VALUE_STORE_KVCLIENTLIBRARY_HPP
