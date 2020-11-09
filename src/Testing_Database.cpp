#include <string>

using namespace std;

#include "MyDebugger.hpp"
#include "KVStore.hpp"

void read_all_db() {
    for (uint_fast32_t i = 0; i < HASH_TABLE_LEN; ++i) {
        if (not kvPersistentStore.file_exists_status.test(i)) continue;
        kvPersistentStore.read_db_file(i);
    }
}

int main(int argc, char *argv[]) {
    // Compiled using:
    //     g++ -O3 Testing_Database.cpp KVStoreFileNames_FINAL.obj -o TestingDatabase
    kvPersistentStore.init_kvstore();  // This is present in KVStore.hpp

    log_info(argc);
    log_info(argv[1], false, true);

    if (argc == 3 && argv[1][0] == '.') {
        KVMessage message;
        message.set_key(argv[2]);
        message.calculate_key_hash();
        log_info("Hash1 for \"" + string(argv[2]) + "\"= " + to_string(message.hash1));
        log_info("Hash2 for \"" + string(argv[2]) + "\"= " + to_string(message.hash2));
        log_info("File Name  = Hash1 % " + to_string(HASH_TABLE_LEN) + " = " +
                 to_string(message.hash1 % HASH_TABLE_LEN));
        log_info("File Index = Hash1 %  " + to_string(1000) + " = " + to_string(message.hash1 % 1000));
    } else if (argc == 1) {
        read_all_db();
    } else {
        for (int i = 1; i < argc; ++i) {
            log_info("Reading Database = " + string(argv[i]), true, true);
            kvPersistentStore.read_db_file(atoi(argv[i]));
        }
    }

    return 0;
}