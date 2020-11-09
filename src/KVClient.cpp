#include <iostream>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <sys/stat.h>

#include "MyDebugger.hpp"
#include "MyVector.hpp"
#include "KVMessage.hpp"
#include "KVClientLibrary.hpp"

using namespace std;
using namespace std::chrono;


/* Used for load testing */
void start_sending_requests(MyVector<KVMessage> &dataset, const char *server_ip = "127.0.0.1",
                            const char *server_port = "12345") {
#ifdef DEBUGGING_ON
    log_info("-----+-----+-----", true);
    log_info(string() + "IP Address = " + server_ip);
    log_info(string() + "Port Number = " + server_port);
    log_info("-----+-----+-----");
    log_info(dataset.size());
    for (int64_t i = 0; i < dataset.n; ++i) {
        log_info(string("") + to_string((int) dataset.at(i).status_code) + " " + dataset.at(i).key + " " +
                 ((dataset.at(i).status_code == KVMessage::EnumGET) ? dataset.at(i).value : " "));
    }
    log_info("-----+-----+-----");
#endif

    ClientServerConnection connection(server_ip, server_port);
    int32_t request_number = 1;
    for (size_t j = 0; j < dataset.n; ++j) {
        KVMessage &i = dataset.at(j);
        i.fix_key_nulling();

        log_info(string("Working on Request Number = ") + to_string(request_number), true);
        log_info(
                string("    ") + "Request code = " + to_string(i.status_code) + " [" + i.status_code_to_string() + "]");
        log_info(string("    ") + "Key = " + i.key);
        if (i.is_request_code_PUT()) log_info(string("    ") + "Message = " + i.value);
        ++request_number;

        // No other case is possible because they are handled while reading the dataset file
        switch (i.status_code) {
            case KVMessage::EnumGET:
                connection.GET(i);
                log_info(string() + "Result of GET = \"" + connection.resultValue + "\"");
                // if (not equal(i.value, i.value + 256, connection.resultValue)) {
                //     log_error("*** Server response was not consistent with what was expected if this is "
                //               "the only client updating the server, or if all clients are making request "
                //               "for different keys ***");
                // }
                break;
            case KVMessage::EnumPUT:
                connection.PUT(i);
                break;
            case KVMessage::EnumDEL:
                connection.DELETE(i);
                break;
        }
    }
}


// REFER: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
inline bool file_exists(const char *name) {
    struct stat buffer{};
    return (stat(name, &buffer) == 0);
}

MyVector<KVMessage> load_client_request_dataset(char *filename) {
    if (not file_exists(filename)) {
        log_error(string("") + "FILE not found: \"" + filename + "\"");
        exit(1);
    }
    ifstream fileReader;
    fileReader.open(filename, ios::in);

    // REFER: https://stackoverflow.com/questions/3640852/difference-between-file-is-open-and-file-fail
    if (fileReader.fail() || (not fileReader.is_open())) {
        log_error("File opening failed");
        exit(2);
    }

    // REFER: https://www.tutorialspoint.com/cplusplus/cpp_files_streams.htm
    uint32_t requestCount = 0;
    fileReader >> requestCount;
    log_info(string("Request Count = ") + to_string(requestCount));

    MyVector<KVMessage> dataset(requestCount);  // the constructor reserves the space required
    dataset.n = requestCount;  // change the size as we will directly set elements in the below loop

    uint32_t request_type;
    struct KVMessage temp;
    for (uint32_t i = 0; i < requestCount; ++i) {
        // Request Codes: 1=GET, 2=PUT, 3=DELETE
        fileReader >> request_type;
        if (not KVMessage::is_request_code_valid(request_type)) {
            log_error("Invalid value of Request Code = " + to_string(request_type));
            fileReader.close();
            exit(3);
        }

        temp.status_code = request_type;
        fileReader >> temp.key;
        if (KVMessage::is_request_code_PUT(request_type))
            fileReader >> temp.value;  // "Value" is only required for PUT requests
        dataset.at(i) = temp;
    }

    fileReader.close();
    return dataset;
}

int main(int argc, char *argv[]) {
    // REFER: https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/
    if (argc >= 2) {
        log_info("Loading client requests dataset...", true);
        MyVector<KVMessage> dataset = load_client_request_dataset(argv[1]);
        log_info("Loading successfully complete\n");

#ifdef DEBUGGING_ON
        // TODO: comment this when using Bash/Python script to launch multiple clients
        // REFER: https://stackoverflow.com/questions/903221/press-enter-to-continue
        cout << "Press Enter to Continue";
        cin.ignore();
#endif

        // TODO: add technique to measure execution time
        // REFER: https://www.geeksforgeeks.org/measure-execution-time-function-cpp/
        auto start = high_resolution_clock::now();
        start_sending_requests(dataset, (argc >= 3) ? argv[2] : "127.0.0.1", (argc >= 4) ? argv[3] : "12345");
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by function: " << duration.count() << " microseconds" << endl;

        return 0;
    }

    // TODO
    return 0;
}