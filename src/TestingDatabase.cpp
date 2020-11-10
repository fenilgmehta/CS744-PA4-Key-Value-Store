#include <string>

using namespace std;

#include "MyDebugger.hpp"
#include "KVStore.hpp"
#include "KVClientLibrary.hpp"

void read_all_db() {
    for (uint_fast32_t i = 0; i < HASH_TABLE_LEN; ++i) {
        if (not kvPersistentStore.file_exists_status.test(i)) continue;
        kvPersistentStore.read_db_file(i);
    }
}

/*

g++ TestingDatabase.cpp KVStoreFileNames_FINAL.obj -o TestingDatabase

./TestingDatabase 123
./TestingDatabase . w ABCD 1234 123
./TestingDatabase . w LMNO 2345 123
./TestingDatabase . w PQR 9876 123
./TestingDatabase 123
./TestingDatabase . w LMNO ZZ 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d PQRZZ 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d ABCD 123
./TestingDatabase 123
./TestingDatabase . d LMNO 123
./TestingDatabase 123
./TestingDatabase . d PQR 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d ABCD 123
./TestingDatabase 123
./TestingDatabase . d PQR 123
./TestingDatabase 123
./TestingDatabase . d LMNO 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d LMNO 123
./TestingDatabase 123
./TestingDatabase . d ABCD 123
./TestingDatabase 123
./TestingDatabase . d PQR 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d LMNO 123
./TestingDatabase 123
./TestingDatabase . d PQR 123
./TestingDatabase 123
./TestingDatabase . d ABCD 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d PQR 123
./TestingDatabase 123
./TestingDatabase . d ABCD 123
./TestingDatabase 123
./TestingDatabase . d LMNO 123
./TestingDatabase 123

./TestingDatabase 123
./TestingDatabase . d PQR 123
./TestingDatabase 123
./TestingDatabase . d LMNO 123
./TestingDatabase 123
./TestingDatabase . d ABCD 123
./TestingDatabase 123


 * */
int main(int argc, char *argv[]) {
    if (argc == 1) {
        cout << "Usage:"
             << "\n./a.out . KEY"
             << "\n./a.out . w KEY VALUE [HASH]"
             << "\n./a.out . d KEY [HASH]"
             << "\n./a.out . r KEY [HASH]"
             << "\n./a.out . send IP PORT w KEY VALUE"
             << "\n./a.out . send IP PORT d KEY"
             << "\n./a.out . send IP PORT r KEY"
             << "\n./a.out ."
             << "\n./a.out DB_FILE_NAME [DB_FILE_NAME [...]]\n";
        return 0;
    }
    // Compiled using:
    //     g++ -O3 TestingDatabase.cpp KVStoreFileNames_FINAL.obj -o TestingDatabase
    kvPersistentStore.init_kvstore();  // This is present in KVStore.hpp

    cout << "\n-----------------\n";
    log_info("argc = " + to_string(argc), true);
    log_info("argv[1] = " + string(argv[1]), false, true);

    if (argc == 3 && argv[1][0] == '.') {
        // Calculate Hash
        // INPUT:
        //     ./a.out . KEY
        KVMessage message;
        message.set_key(argv[2]);
        message.calculate_key_hash();
        log_info("Hash1 for \"" + string(argv[2]) + "\"= " + to_string(message.hash1));
        log_info("Hash2 for \"" + string(argv[2]) + "\"= " + to_string(message.hash2));
        log_info("File Name  = Hash1 % " + to_string(HASH_TABLE_LEN) + " = " +
                 to_string(message.hash1 % HASH_TABLE_LEN));
        log_info("File Index = Hash1 %  " + to_string(1000) + " = " + to_string(message.hash1 % FILE_TABLE_LEN));
    } else if (argc >= 4 && argv[1][0] == '.') {
        KVMessage message;
        message.set_key(argv[3]);

        if (argv[2][0] == 'w') {
            // WRITE to file
            // INPUT:
            //     ./a.out . w KEY VALUE [HASH]
            message.set_value(argv[4]);
            if (argc >= 6) {
                message.hash1 = message.hash2 = atoll(argv[5]);
            }
            kvPersistentStore.write_to_db(&message);
        } else if (argv[2][0] == 'd') {
            // DELETE from file
            // INPUT:
            //     ./a.out . d KEY [HASH]
            if (argc >= 5) {
                message.hash1 = message.hash2 = atoll(argv[4]);
            }
            const bool resultStatus = kvPersistentStore.delete_from_db(&message);
            if (resultStatus) log_success("DELETE completed for key = " + string(message.key));
            else log_error("DELETE failed for key = " + string(message.key));
        } else if (argv[2][0] == 'r'){
            // READ from file
            // INPUT:
            //     ./a.out . r KEY [HASH]

            if (argc >= 5) {
                message.hash1 = message.hash2 = atoll(argv[4]);
            }
            const bool resultStatus = kvPersistentStore.read_from_db(&message);
            if (resultStatus) {
                log_success("READ completed for key = " + string(message.key));
                log_info("GIVEN key = " + string(message.key));
                log_info("FETCHED Value = " + string(message.value));
            } else {
                log_error("READ failed for key = " + string(message.key));
            }
        } else {
            // ./a.out . send IP PORT w KEY VALUE
            // ./a.out . send IP PORT d KEY
            // ./a.out . send IP PORT r KEY
            // SEND request to client
            ClientServerConnection c(argv[3], argv[4]);
            KVMessage m;
            m.set_key(argv[6]);
            switch(argv[5][0]) {
                case 'w':
                    m.set_value(argv[7]);
                    c.PUT(m);
                    break;
                case 'd':
                    c.DELETE(m);
                    break;
                case 'r':
                    c.GET(m);
                    if(c.resultStatusCode == KVMessage::StatusCodeValueSUCCESS) {
                        log_success("GET() value = " + string(c.resultValue));
                    } else {
                        log_error("GET() failed");
                    }
                    break;
                default:
                    log_error("Invalid option: \"" + string(argv[5]) + "\"");
            }
        }
    } else if (argc == 2 && argv[1][0] == '.' && argv[1][1] == '\0') {
        // INPUT:
        //     ./a.out .
        read_all_db();
    } else {
        // INPUT:
        //     ./a.out FILE_NAME [FILE_NAME [...]]
        for (int i = 1; i < argc; ++i) {
            log_info("Reading Database = " + string(argv[i]), true, true);
            kvPersistentStore.read_db_file(atoi(argv[i]));
        }
    }

    return 0;
}