#ifndef PA_4_KEY_VALUE_STORE_KVSTORE_HPP
#define PA_4_KEY_VALUE_STORE_KVSTORE_HPP

#include <fstream>
#include <array>
#include <bitset>
#include <sys/stat.h>
#include <unistd.h>
#include "KVMessage.hpp"
#include "KVStoreFileNames.hpp"

// Number of linked-lists that point to circular lists of "CacheNode"
// The number of files in the Persistent Storage is equal to the below value
const uint_fast32_t HASH_TABLE_LEN = 16384;
const uint_fast64_t FILE_TABLE_LEN = 1000;
const uint_fast64_t MAX_UINT64 = std::numeric_limits<uint64_t>::max();
const char EMPTY_STRING[256] = {};

// Single Entry in file:
//     uint64_t leftIdx (64 bits), uint64_t rightIdx (64 bits),
//     uint64_t hash1 (64 bits), uint64_t hash2 (64 bits),
//     char Key[256], char Value[256]
struct KVStore {
    std::array<std::shared_mutex, HASH_TABLE_LEN> file_locks;
    std::array<std::fstream, HASH_TABLE_LEN> file_fd;
    std::bitset<HASH_TABLE_LEN> file_exists_status;

    /* NOTE: it is important to call this before using other function of this struct */
    void init_kvstore() {
        // REFER: https://www.tutorialspoint.com/system-function-in-c-cplusplus
        if (system("mkdir -p db") != 0) {
            // mkdir failed
            log_error("Make Directory \"mkdir -p db\" failed :(");
            log_error("Exiting (status=60)");
            exit(60);
        }

        // This is VERY IMPORTANT
        chdir("db");

        for (int i = 0; i <= FILE_TABLE_LEN; ++i) {
            file_exists_status.set(
                    i, does_file_exists(kvStoreFileNames[i])
            );
        }
    }

    /* ASSUMED: ptr has following values filled: {hash1, hash2, key}
     * */
    bool read_from_db(struct KVMessage *ptr) {
        // REFER: https://en.cppreference.com/w/cpp/thread/shared_lock/shared_lock
        // Read lock is automatically acquired when the constructor is called
        // And, it is released as soon as the destructor is called
        uint64_t file_idx = (ptr->hash1) % HASH_TABLE_LEN;
        std::shared_lock read_lock(file_locks[file_idx]);

        // return false if file does not exists
        if (not file_exists_status.test(file_idx)) {
            return false;
        }

        file_fd[file_idx].open(kvStoreFileNames[file_idx], std::ios::in | std::ios::binary);
        if ((not file_fd[file_idx].is_open()) || file_fd[file_idx].fail()) {
            log_error(std::string("") + "Unable to open Database file: \"" + kvStoreFileNames[file_idx] + "\"");
            return false;
        }

        static uint64_t leftIdx, rightIdx, hash1_file, hash2_file;
        static char key_file[256];

        const uint64_t inside_file_idx = (ptr->hash1) % FILE_TABLE_LEN;
        file_fd[file_idx].seekg(get_seek_val(inside_file_idx));
        file_fd[file_idx].read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

        if (is_file_entry_empty(leftIdx, rightIdx)) {
            // There is no entry for this "inside_file_idx" val
            file_fd[file_idx].close();
            return false;
        }

        // First entry matches the "Key"
        if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
            file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
            if (std::equal(key_file, key_file + 256, ptr->key)) {
                // match found
                file_fd[file_idx].read(reinterpret_cast<char *>(ptr->value), 256);
                file_fd[file_idx].close();
                return true;
            }
        }

        uint64_t current_file_idx = rightIdx;
        while (current_file_idx != inside_file_idx) {
            file_fd[file_idx].seekg(get_seek_val(current_file_idx));
            file_fd[file_idx].read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
            file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
            file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
            file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

            if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
                file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
                if (std::equal(key_file, key_file + 256, ptr->key)) {
                    // match found
                    file_fd[file_idx].read(reinterpret_cast<char *>(ptr->value), 256);
                    file_fd[file_idx].close();
                    return true;
                }
            }

            current_file_idx = rightIdx;
        }

        file_fd[file_idx].close();
        return false;
    }

    /* ASSUMED: ptr has following values filled: {hash1, hash2, key, value}
     * */
    void write_to_db(struct KVMessage *ptr) {
        uint64_t file_idx = (ptr->hash1) % HASH_TABLE_LEN;

        // REFER: https://stackoverflow.com/questions/39185420/is-there-a-shared-lock-guard-and-if-not-what-would-it-look-like
        std::unique_lock write_lock(file_locks[file_idx]);

        if (not file_exists_status.test(file_idx)) {
            // File does NOT exists
            // Create the file
            file_exists_status.set(file_idx);

            // std::ios::app causes the file to be created
            file_fd[file_idx].open(kvStoreFileNames[file_idx],
                                   std::ios::in | std::ios::out | std::ios::binary | std::ios::app);

            // IMPORTANT: insert "FILE_TABLE_LEN" number of blank entries
            for (int i = 0; i < FILE_TABLE_LEN; ++i) {
                file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
                file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
            }
        } else {
            file_fd[file_idx].open(kvStoreFileNames[file_idx], std::ios::in | std::ios::out | std::ios::binary);
        }

        static uint64_t leftIdx, rightIdx, hash1_file, hash2_file;
        static char key_file[256];

        const uint64_t inside_file_idx = (ptr->hash1) % FILE_TABLE_LEN;
        file_fd[file_idx].seekg(get_seek_val(inside_file_idx));
        file_fd[file_idx].read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

        if (is_file_entry_empty(leftIdx, rightIdx)) {
            file_fd[file_idx].seekg(get_seek_val(inside_file_idx));
            leftIdx = rightIdx = inside_file_idx;
            file_fd[file_idx].write(reinterpret_cast<const char *>(&leftIdx), sizeof(uint64_t));
            file_fd[file_idx].write(reinterpret_cast<const char *>(&rightIdx), sizeof(uint64_t));
            file_fd[file_idx].write(reinterpret_cast<const char *>(&(ptr->hash1)), sizeof(uint64_t));
            file_fd[file_idx].write(reinterpret_cast<const char *>(&(ptr->hash2)), sizeof(uint64_t));
            file_fd[file_idx].write(reinterpret_cast<const char *>(ptr->key), 256);
            file_fd[file_idx].write(reinterpret_cast<const char *>(ptr->value), 256);

            file_fd[file_idx].close();
            return;
        }

        // TODO: SEARCH through the file and replace the the entry if found, else create a new entry at the end of the fileentry if found, else create a new entry at the end of the file
        if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
            file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
            if (std::equal(key_file, key_file + 256, ptr->key)) {
                // match found
                file_fd[file_idx].write(reinterpret_cast<const char *>(ptr->value), 256);
                file_fd[file_idx].close();
                return;
            }
        }

        uint64_t current_file_idx = rightIdx;
        while (current_file_idx != inside_file_idx) {
            file_fd[file_idx].seekg(get_seek_val(current_file_idx));
            file_fd[file_idx].read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
            file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
            file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
            file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

            if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
                file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
                if (std::equal(key_file, key_file + 256, ptr->key)) {
                    // match found
                    file_fd[file_idx].write(reinterpret_cast<const char *>(ptr->value), 256);
                    file_fd[file_idx].close();
                    return;
                }
            }

            // This is useful when "ptr->key" is not present in the file.
            // After loop termination, "leftIdx" will point to the last
            // entry of the double linked list stored of the file.
            leftIdx = current_file_idx;

            current_file_idx = rightIdx;
        }

        // No entry exists for the given key "ptr->key"
        // So, we add a new entry at the end of the file

        file_fd[file_idx].seekp(0, std::ios::end);  // moves the write pointer to the end of the file

        // REFER: https://www.tutorialspoint.com/tellp-in-file-handling-with-cplusplus
        uint64_t new_entry_position = static_cast<uint64_t>(file_fd[file_idx].tellp()) / 8;

        file_fd[file_idx].write(reinterpret_cast<const char *>(&leftIdx), sizeof(uint64_t));
        file_fd[file_idx].write(reinterpret_cast<const char *>(&inside_file_idx), sizeof(uint64_t));
        file_fd[file_idx].write(reinterpret_cast<const char *>(&(ptr->hash1)), sizeof(uint64_t));
        file_fd[file_idx].write(reinterpret_cast<const char *>(&(ptr->hash2)), sizeof(uint64_t));
        file_fd[file_idx].write(reinterpret_cast<const char *>(ptr->key), 256);
        file_fd[file_idx].write(reinterpret_cast<const char *>(ptr->value), 256);

        file_fd[file_idx].seekp(get_seek_val(leftIdx) + sizeof(uint64_t));  // set pointer to rightIdx
        file_fd[file_idx].write(reinterpret_cast<const char *>(&new_entry_position), sizeof(uint64_t));

        file_fd[file_idx].seekp(get_seek_val(inside_file_idx));  // set pointer to leftIdx
        file_fd[file_idx].write(reinterpret_cast<const char *>(&new_entry_position), sizeof(uint64_t));

        file_fd[file_idx].close();
    }

private:
    static inline uint64_t get_seek_val(uint64_t idx) {
        // Division by 8 is necessary as file read/write pointer moves by bytes not bits
        return idx * (4 * sizeof(uint64_t) + 256 + 256);
    }

    // REFER: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    static inline bool does_file_exists(const char *name) {
        struct stat buffer{};
        return (stat(name, &buffer) == 0);
    }

    static inline bool is_file_entry_empty(const uint64_t leftIdx, const uint64_t rightIdx) {
        return (leftIdx == rightIdx && leftIdx == MAX_UINT64);
        return true;
    }
} kvPersistentStore;

#endif // PA_4_KEY_VALUE_STORE_KVSTORE_HPP
