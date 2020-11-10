#include <iostream>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <vector>
#include <sys/stat.h>

#include "MyDebugger.hpp"
#include "KVMessage.hpp"
#include "KVClientLibrary.hpp"

using namespace std;
using namespace std::chrono;

inline void press_enter_to_continue(const char *msg) {
#ifdef DEBUGGING_ON
    // REFER: https://stackoverflow.com/questions/903221/press-enter-to-continue
    cout << '\n' << msg;
    cout.flush();
    cin.ignore();
#endif
}

/* Used for load testing */
void start_sending_requests(std::vector<KVMessage> &dataset, const char *server_ip = "127.0.0.1",
                            const char *server_port = "12345") {
#ifdef DEBUGGING_ON
    log_info("-----+-----+-----", true);
    log_info(string() + "IP Address = " + server_ip);
    log_info(string() + "Port Number = " + server_port);
    log_info("-----+-----+-----");
    log_info(dataset.size());
    for (auto &i : dataset) {
        log_info(string("") + to_string((int) i.status_code) + " " + i.key + " " +
                 ((i.status_code == KVMessage::EnumGET) ? i.value : " "));
    }
    log_info("-----+-----+-----");
#endif

    ClientServerConnection connection(server_ip, server_port);

    press_enter_to_continue("Press ENTER to continue with sending the requests");

    int32_t request_number = 1;
    for (auto &i : dataset) {
        i.fix_key_nulling();

        log_info(string("Working on Request Number = ") + to_string(request_number), true);
        log_info(string("    ") + "Request code = " + to_string(i.status_code)
                 + " [" + i.status_code_to_string() + "]");
        log_info(string("    ") + "Key = " + i.key);
        if (i.is_request_code_PUT()) log_info(string("    ") + "Message = " + i.value);

        ++request_number;

        // No other case is possible because they are handled while reading the dataset file
        switch (i.status_code) {
            case KVMessage::EnumGET:
                connection.GET(i);
                log_info(string() + "Result of GET = \"" + connection.resultValue + "\"");
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

/* Used for load testing */
void auto_testing(std::vector<KVMessage> &dataset, const char *server_ip,
                  const char *server_port, const std::vector<std::string> &dataset_results) {
#ifdef DEBUGGING_ON
    log_info("-----+-----+-----+-----", true);
    log_info(string() + "IP Address = " + server_ip);
    log_info(string() + "Port Number = " + server_port);
    log_info("-----+-----+-----+-----");
    log_info(string() + "Dataset Size = " + to_string(dataset.size()));
    // for (auto &i : dataset) {
    //     log_info(string("") + to_string((int) i.status_code) + " " + i.key + " " +
    //              ((i.status_code == KVMessage::EnumGET) ? i.value : " "));
    // }
    log_info("-----+-----+-----+-----");
#endif

    ClientServerConnection connection(server_ip, server_port);

    press_enter_to_continue("Press ENTER to begin with Auto Testing");

    int32_t request_number = 1;
    log_info("AUTO Testing begins now...");
    int32_t errorCount = 0;
    for (size_t i = 0; i < dataset.size(); ++i) {
        KVMessage &kvMessage = dataset.at(i);
        kvMessage.fix_key_nulling();

        if (kvMessage.is_request_code_PUT()) log_info(string("    ") + "Message = " + kvMessage.value);
        ++request_number;

        // No other case is possible because they are handled while reading the dataset file
        switch (kvMessage.status_code) {
            case KVMessage::EnumGET:
                connection.GET(kvMessage);
                if ((
                            connection.resultStatusCode == KVMessage::StatusCodeValueSUCCESS
                            && dataset_results.at(i) == std::string(connection.resultValue)
                    ) || (
                            connection.resultStatusCode == KVMessage::StatusCodeValueERROR &&
                            dataset_results.at(i) == "-ERROR-"
                    )) {
                    log_success("Request Number = " + std::to_string(i + 1) + " / " + std::to_string(dataset.size()));
                } else {
                    ++errorCount;
                    log_error(string("Request Number = ") + to_string(request_number), true);
                    log_info(string("    ") + "Request code = " + to_string(kvMessage.status_code) + " [" +
                             kvMessage.status_code_to_string() + "]");
                    log_info(string("    ") + "Key = " + kvMessage.key);
                }

                // if (not equal(kvMessage.value, kvMessage.value + 256, connection.resultValue)) {
                //     log_error("*** Server response was not consistent with what was expected if this is "
                //               "the only client updating the server, or if all clients are making request "
                //               "for different keys ***");
                // }
                break;
            case KVMessage::EnumPUT:
                connection.PUT(kvMessage);
                if ((
                            connection.resultStatusCode == KVMessage::StatusCodeValueSUCCESS
                    ) || (
                            connection.resultStatusCode == KVMessage::StatusCodeValueERROR
                            && dataset_results.at(i) == "-ERROR-")
                        ) {
                    log_success("Request Number = " + std::to_string(i + 1) + " / " + std::to_string(dataset.size()));
                } else {
                    ++errorCount;
                    log_error(string("Request Number = ") + to_string(request_number), true);
                    log_info(string("    ") + "Request code = " + to_string(kvMessage.status_code) + " [" +
                             kvMessage.status_code_to_string() + "]");
                    log_info(string("    ") + "Key = " + kvMessage.key);
                    log_info(string("    ") + "Value = " + kvMessage.value);
                }
                break;
            case KVMessage::EnumDEL:
                connection.DELETE(kvMessage);
                if ((
                            connection.resultStatusCode == KVMessage::StatusCodeValueSUCCESS
                    ) || (
                            connection.resultStatusCode == KVMessage::StatusCodeValueERROR
                            && dataset_results.at(i) == "-ERROR-")
                        ) {
                    log_success("Request Number = " + std::to_string(i + 1) + " / " + std::to_string(dataset.size()));
                } else {
                    ++errorCount;
                    log_error(string("Request Number = ") + to_string(request_number), true);
                    log_info(string("    ") + "Request code = " + to_string(kvMessage.status_code) + " [" +
                             kvMessage.status_code_to_string() + "]");
                    log_info(string("    ") + "Key = " + kvMessage.key);
                }
                break;
        }
    }

    if (errorCount == 0) {
        log_success("AUTO Testing completed " + std::string() + "SUCCESSFULLY   :)", true, true);
    } else {
        log_error("AUTO Testing completed " + std::string() + ("with errors = " + std::to_string(errorCount)),
                  true, true);
    }
}


// REFER: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
inline bool file_exists(const char *name) {
    struct stat buffer{};
    return (stat(name, &buffer) == 0);
}

std::vector<KVMessage> load_client_request_dataset(char *filename) {
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

    std::vector<KVMessage> dataset(requestCount);  // the constructor reserves the space required

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

std::vector<std::string> load_client_request_results(char *filename) {
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

    std::vector<std::string> dataset_results(requestCount);  // the constructor reserves the space required

    std::string temp;
    for (uint32_t i = 0; i < requestCount; ++i) {
        // Request Codes: 1=GET, 2=PUT, 3=DELETE
        fileReader >> dataset_results.at(i);
    }

    fileReader.close();
    return dataset_results;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cout << "Usage:"
             << "\n./" << argv[0] << " REQUESTS_FILE 127.0.0.1 12345"
             << "\n./" << argv[0] << " REQUESTS_FILE 127.0.0.1 12345 REQUESTS_FILE_SOLUTION"
             << "\n\nNOTE: replace the IP address and the PORT number with that of the \"KVServer\"\n";
        return 0;
    }

    // REFER: https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/
    if (argc >= 2) {
        log_info("Loading client requests dataset...", true);
        std::vector<KVMessage> dataset = load_client_request_dataset(argv[1]);
        log_info("Loading successfully complete\n");

        if (argc == 5) {
            // The expected OUTPUT's file is also passed as a parameter
            std::vector<std::string> dataset_results = load_client_request_results(argv[4]);

            auto start = high_resolution_clock::now();
            auto_testing(dataset, argv[2], argv[3], dataset_results);
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(stop - start);
            cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
            return 0;
        }

        // TODO: add technique to measure execution time
        // REFER: https://www.geeksforgeeks.org/measure-execution-time-function-cpp/
        auto start = high_resolution_clock::now();
        start_sending_requests(dataset, (argc >= 3) ? argv[2] : "127.0.0.1", (argc >= 4) ? argv[3] : "12345");
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by function: " << duration.count() << " microseconds" << endl;
    }

    return 0;
}