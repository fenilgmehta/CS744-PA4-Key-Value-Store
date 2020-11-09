#ifndef PA_4_KEY_VALUE_STORE_KVSTORE_HPP
#define PA_4_KEY_VALUE_STORE_KVSTORE_HPP

#include <shared_mutex>
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

    KVStore() = default;

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

        for (int i = 0; i < HASH_TABLE_LEN; ++i) {
            file_exists_status.set(
                    i, does_file_exists(kvStoreFileNames[i])
            );
        }
    }

    /* ASSUMED: ptr has following values filled: {hash1, hash2, key}
     *
     * Returns: true if GET was successful (i.e. Key was either present in the Persistent Storage)
     *          The "Value" corresponding to "ptr->key" will be stored in "ptr->value"
     *        : false if "Key" is not present
     * */
    bool read_from_db(struct KVMessage *ptr) {
        // REFER: https://en.cppreference.com/w/cpp/thread/shared_lock/shared_lock
        // Read lock is automatically acquired when the constructor is called
        // And, it is released as soon as the destructor is called
        uint64_t file_idx = (ptr->hash1) % HASH_TABLE_LEN;
        std::shared_lock read_lock(file_locks[file_idx]);

        log_info("read_from_db(...)", true);
        log_info(std::string() + "    hash1 = " + std::to_string(ptr->hash1));
        log_info(std::string() + "    hahs2 = " + std::to_string(ptr->hash2));
        log_info(std::string() + "    key   = " + ptr->key);
        log_info(std::string() + "    value = " + ptr->value);
        log_info(std::string() + "    FILE  = " + std::to_string(file_idx));

        // return false if file does not exists
        if (not file_exists_status.test(file_idx)) {
            log_info("    File does not exists");
            return false;
        }

        file_fd[file_idx].open(kvStoreFileNames[file_idx], std::ios::in | std::ios::binary);
        if ((not file_fd[file_idx].is_open()) || file_fd[file_idx].fail()) {
            log_error(std::string("") + "Unable to open Database File: \"" + kvStoreFileNames[file_idx] + "\"");
            return false;
        }

        log_info("    File successfully opened");

        uint64_t leftIdx, rightIdx, hash1_file, hash2_file;
        char key_file[256];

        const uint64_t inside_file_idx = (ptr->hash1) % FILE_TABLE_LEN;
        file_fd[file_idx].seekg(get_seek_val(inside_file_idx));
        file_fd[file_idx].read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

        if (is_file_entry_empty(leftIdx, rightIdx)) {
            // There is no entry for this "inside_file_idx" val
            log_info("    Entry List is empty");
            file_fd[file_idx].close();
            return false;
        }

        // First entry matches the "Key"
        if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
            file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
            if (std::equal(key_file, key_file + 256, ptr->key)) {
                // match found
                log_info("    First entry matched");
                file_fd[file_idx].read(reinterpret_cast<char *>(ptr->value), 256);
                file_fd[file_idx].close();
                return true;
            }
        }

        log_info("    inside_file_idx = " + std::to_string(inside_file_idx));
        log_info("    leftIdx = " + std::to_string(leftIdx));
        log_info("    rightIdx = " + std::to_string(rightIdx));
        log_info("    More then one entry found, and first entry did NOT match");
        uint64_t current_file_idx = rightIdx;
        while (current_file_idx != inside_file_idx) {
            log_info("        Working on idx = " + std::to_string(current_file_idx));
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
        std::fstream fs;

        log_info("write_to_db(...)", true);
        log_info(std::string() + "    hash1 = " + std::to_string(ptr->hash1));
        log_info(std::string() + "    hahs2 = " + std::to_string(ptr->hash2));
        log_info(std::string() + "    key   = " + ptr->key);
        log_info(std::string() + "    value = " + ptr->value);
        log_info(std::string() + "    FILE  = " + std::to_string(file_idx));

        // REFER: https://stackoverflow.com/questions/39185420/is-there-a-shared-lock-guard-and-if-not-what-would-it-look-like
        std::unique_lock write_lock(file_locks[file_idx]);

        if (not file_exists_status.test(file_idx)) {
            // File does NOT exists
            // Create the file
            file_exists_status.set(file_idx);

            // REFER: https://www.geeksforgeeks.org/c-program-to-create-a-file/
            // std::ios::app causes the file to be created BUT will insert all content
            // to the end of the file even after performing seekp(...)
            fs.open(kvStoreFileNames[file_idx],
                                   std::ios::out | std::ios::binary);

            if(!fs) {
                log_error(std::string() + "    Failed to create file: \"" + kvStoreFileNames[file_idx] + "\"");
            } else {
                log_info("    File successfully CREATED: " + std::string(kvStoreFileNames[file_idx]));
            }

            // IMPORTANT: insert "FILE_TABLE_LEN" number of blank entries
            for (int i = 0; i < FILE_TABLE_LEN; ++i) {
                fs.write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                fs.write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                fs.write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                fs.write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                fs.write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
                fs.write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
            }
            fs.close();
        }

        fs.open(kvStoreFileNames[file_idx], std::ios::in | std::ios::out | std::ios::binary);

        if(fs.fail() || fs.eof()) {
            log_error("fs.fail() OR fs.eof() for file = " + std::string(kvStoreFileNames[file_idx]));
            return;
        }
        log_info(std::string() + "    File successfully OPENED: " + kvStoreFileNames[file_idx]);

        uint64_t leftIdx=0, rightIdx=0, hash1_file=0, hash2_file=0;
        char key_file[256];

        const uint64_t inside_file_idx = (ptr->hash1) % FILE_TABLE_LEN;
        fs.seekg(get_seek_val(inside_file_idx));
        fs.read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
        fs.read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
        fs.read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
        fs.read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

        if (is_file_entry_empty(leftIdx, rightIdx)) {
            log_info(std::string("    write_to_db [") + std::to_string(file_idx) + "] : is_file_entry_empty");
            log_info(std::string() + "        inside_file_idx = " + std::to_string(inside_file_idx));
            fs.seekp(get_seek_val(inside_file_idx));
            leftIdx = rightIdx = inside_file_idx;
            fs.write(reinterpret_cast<const char *>(&leftIdx), sizeof(uint64_t));
            fs.write(reinterpret_cast<const char *>(&rightIdx), sizeof(uint64_t));
            fs.write(reinterpret_cast<const char *>(&(ptr->hash1)), sizeof(uint64_t));
            fs.write(reinterpret_cast<const char *>(&(ptr->hash2)), sizeof(uint64_t));
            fs.write(reinterpret_cast<const char *>(ptr->key), 256);
            fs.write(reinterpret_cast<const char *>(ptr->value), 256);

            fs.close();
            return;
        }

        // SEARCH through the file and replace the the entry if found,
        // else create a new entry at the end of the file
        if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
            fs.read(reinterpret_cast<char *>(key_file), 256);
            if (std::equal(key_file, key_file + 256, ptr->key)) {
                // match found
                log_info("    First entry matched");
                fs.write(reinterpret_cast<const char *>(ptr->value), 256);
                fs.close();
                return;
            }
        }

        log_info("    inside_file_idx = " + std::to_string(inside_file_idx));
        log_info("    leftIdx = " + std::to_string(leftIdx));
        log_info("    rightIdx = " + std::to_string(rightIdx));
        log_info("    More then one entry found");
        uint64_t current_file_idx = rightIdx;
        while (current_file_idx != inside_file_idx) {
            log_info("        Working on idx = " + std::to_string(current_file_idx));
            fs.seekg(get_seek_val(current_file_idx));
            fs.read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

            if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
                fs.read(reinterpret_cast<char *>(key_file), 256);
                if (std::equal(key_file, key_file + 256, ptr->key)) {
                    // match found
                    fs.write(reinterpret_cast<const char *>(ptr->value), 256);
                    fs.close();
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

        log_info("    No match found. Creating new entry at the EOF");
        fs.seekp(0, std::ios::end);  // moves the write pointer to the end of the file

        // REFER: https://www.tutorialspoint.com/tellp-in-file-handling-with-cplusplus
        uint64_t new_entry_position = static_cast<uint64_t>(fs.tellp()) / 8;

        fs.write(reinterpret_cast<const char *>(&leftIdx), sizeof(uint64_t));
        fs.write(reinterpret_cast<const char *>(&inside_file_idx), sizeof(uint64_t));
        fs.write(reinterpret_cast<const char *>(&(ptr->hash1)), sizeof(uint64_t));
        fs.write(reinterpret_cast<const char *>(&(ptr->hash2)), sizeof(uint64_t));
        fs.write(reinterpret_cast<const char *>(ptr->key), 256);
        fs.write(reinterpret_cast<const char *>(ptr->value), 256);

        fs.seekp(get_seek_val(leftIdx) + sizeof(uint64_t));  // set pointer to rightIdx
        fs.write(reinterpret_cast<const char *>(&new_entry_position), sizeof(uint64_t));

        fs.seekp(get_seek_val(inside_file_idx));  // set pointer to leftIdx
        fs.write(reinterpret_cast<const char *>(&new_entry_position), sizeof(uint64_t));

        fs.close();
    }

    /* Returns: true if entry found in Persistent Storage and successfully deleted
     *        : false if file does not exists or entry not found in Persistent Storage
     * */
    bool delete_from_db(struct KVMessage *ptr) {
        uint64_t file_idx = (ptr->hash1) % HASH_TABLE_LEN;

        // REFER: https://stackoverflow.com/questions/39185420/is-there-a-shared-lock-guard-and-if-not-what-would-it-look-like
        std::unique_lock write_lock(file_locks[file_idx]);

        if (not file_exists_status.test(file_idx)) {
            // File does NOT exists
            return false;
        }

        file_fd[file_idx].open(kvStoreFileNames[file_idx], std::ios::in | std::ios::out | std::ios::binary);
        if ((not file_fd[file_idx].is_open()) || file_fd[file_idx].fail()) {
            log_error(std::string("") + "Unable to open Database File: \"" + kvStoreFileNames[file_idx] + "\"");
            return false;
        }

        uint64_t leftIdx, rightIdx, hash1_file, hash2_file, leftIdx_of_key_to_delete, idx_of_key_to_delete;
        char key_file[256], value_file[256];

        const uint64_t inside_file_idx = (ptr->hash1) % FILE_TABLE_LEN;
        file_fd[file_idx].seekg(get_seek_val(inside_file_idx));
        file_fd[file_idx].read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
        file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));

        if (is_file_entry_empty(leftIdx, rightIdx)) {
            file_fd[file_idx].close();
            return false;
        }

        // First entry matches the "Key"
        if (hash1_file == ptr->hash1 && hash2_file == ptr->hash2) {
            file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
            if (std::equal(key_file, key_file + 256, ptr->key)) {
                // match found
                file_fd[file_idx].seekg(get_seek_val(inside_file_idx));

                if (leftIdx == rightIdx) {
                    // NOTE: this should be true if there is only one entry for this "inside_file_idx"
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    // NOTE: No need of clearing the key-value content as we know that the
                    // Doubly Linked List is emptfy from the value of leftIdx and rightIdx
                    // file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
                    // file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
                } else if (rightIdx == MAX_UINT64) {
                    log_error("delete_from_db(...) rightIdx==MAXUINT64 case should have been handled earlier, "
                              "leftIdx = " + std::to_string(leftIdx) + ", rightIdx = " + std::to_string(rightIdx));
                    return false;
                } else {
                    // NOTE: More than ONE entry found
                    //       This will work even if there are only two entries
                    idx_of_key_to_delete = inside_file_idx;
                    leftIdx_of_key_to_delete = leftIdx;

                    // read RHS of node to delete
                    file_fd[file_idx].seekg(get_seek_val(rightIdx) + sizeof(uint64_t));
                    file_fd[file_idx].read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
                    file_fd[file_idx].read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
                    file_fd[file_idx].read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));
                    file_fd[file_idx].read(reinterpret_cast<char *>(key_file), 256);
                    file_fd[file_idx].read(reinterpret_cast<char *>(value_file), 256);
                    // TODO: verity the below seek statement
                    // delete the RHS of node to delete
                    file_fd[file_idx].seekg(-SIZE_OF_ONE_ENTRY, std::ios::cur);
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
                    file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);

                    // update leftIdx of RHS of RHS of NodeToDelete
                    file_fd[file_idx].seekg(get_seek_val(rightIdx));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&idx_of_key_to_delete), sizeof(uint64_t));

                    file_fd[file_idx].seekg(get_seek_val(idx_of_key_to_delete) + sizeof(uint64_t));
                    // leftIdx remain unchanged for "idx_of_key_to_delete"
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&rightIdx), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&(hash1_file)), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&(hash2_file)), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(key_file), 256);
                    file_fd[file_idx].write(reinterpret_cast<const char *>(value_file), 256);
                }

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

                    // Works for both:
                    // a. Last node of the Doubly Linked List is to be deleted
                    // b. Node between head and tail of Doubly Linked List is to be deleted

                    // Delete the node
                    file_fd[file_idx].seekg(get_seek_val(current_file_idx));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&MAX_UINT64), sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);
                    file_fd[file_idx].write(reinterpret_cast<const char *>(EMPTY_STRING), 256);

                    // Update rightIdx of LHS of node to delete
                    file_fd[file_idx].seekg(get_seek_val(leftIdx) + sizeof(uint64_t));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&rightIdx), sizeof(uint64_t));

                    // Update leftIdx of head node
                    file_fd[file_idx].seekg(get_seek_val(rightIdx));
                    file_fd[file_idx].write(reinterpret_cast<const char *>(&leftIdx), sizeof(uint64_t));

                    file_fd[file_idx].close();
                    return true;
                }
            }

            current_file_idx = rightIdx;
        }

        file_fd[file_idx].close();

        // Entry not found
        return false;
    }

    void read_db_file(const int num) {
        if (not file_exists_status.test(num)) {
            log_error("read_db_file(" + std::to_string(num) + ") file does not exists");
            return;
        }

        log_success("READING: " + std::to_string(num), true);

        std::fstream fs;
        fs.open(kvStoreFileNames[num], std::ios::in | std::ios::binary);
        if ((not fs.is_open()) || fs.fail()) {
            log_error(std::string("") + "Unable to open Database File: \"" + kvStoreFileNames[num] + "\"");
            return;
        }

        uint64_t leftIdx, rightIdx, hash1_file, hash2_file, leftIdx_of_key_to_delete, idx_of_key_to_delete;
        char key_file[256], value_file[256];

        int32_t i = -1;
        // fs.seekg(get_seek_val(1000));
        while (fs.is_open() && (not fs.eof())) {
            fs.read(reinterpret_cast<char *>(&leftIdx), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(&rightIdx), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(&(hash1_file)), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(&(hash2_file)), sizeof(uint64_t));
            fs.read(reinterpret_cast<char *>(key_file), 256);
            fs.read(reinterpret_cast<char *>(value_file), 256);
            ++i;

            if (leftIdx == rightIdx && leftIdx == MAX_UINT64) {
                continue;
            }
            log_info("tellg() = " + std::to_string(fs.tellg()), true);
            log_info(std::string() + std::to_string(i)+","
                     + std::to_string(leftIdx) + "," + std::to_string(rightIdx)
                     + "," + std::to_string(hash1_file) + "," + std::to_string(hash2_file)
                     + "," + key_file + "," + value_file);
        }
        log_info(std::string() + "File entries count = " + std::to_string(i), true);
        fs.close();
    }

private:
    static const int_fast32_t SIZE_OF_ONE_ENTRY = (4 * sizeof(uint64_t) + 256 + 256);

    static inline uint64_t get_seek_val(uint64_t idx) {
        // Division by 8 is necessary as file read/write pointer moves by bytes not bits
        return idx * SIZE_OF_ONE_ENTRY;
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
};

KVStore kvPersistentStore = {};

const int_fast32_t KVStore::SIZE_OF_ONE_ENTRY;

#endif // PA_4_KEY_VALUE_STORE_KVSTORE_HPP
