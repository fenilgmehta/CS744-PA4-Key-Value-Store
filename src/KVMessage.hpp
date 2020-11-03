#ifndef PA_4_KEY_VALUE_STORE_KVMESSAGE_HPP
#define PA_4_KEY_VALUE_STORE_KVMESSAGE_HPP

#include <cstdint>

#define KV_STR_LEN 256

struct KVMessageHash {
    uint64_t hash1, hash2;

    KVMessageHash() : hash1{}, hash2{} {}
};

struct KVMessage {
    uint8_t status_code;
    char key[256], value[256];

    KVMessage() : status_code{}, key{}, value{} {}

    struct KVMessageHash get_key_hash() const {
        /* 
         * For hash1:
         *     My custom implementation which is mix of hash2 algorithm and XOR
         *     
         * For hash2:
         *     REFER: https://stackoverflow.com/questions/11546791/what-is-the-best-hash-function-for-rabin-karp-algorithm
         *
         * "random_nums" is generated using the below Python Code:
         * >>> import random
         * >>> arr = list(range(256))
         * >>> random.shuffle(arr)
         * >>> print(arr)
         *
         * */
        const static uint64_t random_nums[KV_STR_LEN] = {14, 182, 49, 60, 211, 165, 125, 232, 71, 166, 133, 237, 13, 78, 76, 25, 215, 221, 108, 140, 254, 53, 119, 79, 239, 113,
                                                         188, 126, 157, 144, 121, 34, 50, 197, 66, 82, 106, 247, 47, 158, 44, 201, 136, 85, 175, 220, 167, 130, 147, 90, 74, 253,
                                                         186, 185, 84, 97, 217, 226, 7, 218, 141, 69, 139, 6, 142, 143, 98, 240, 195, 42, 173, 63, 159, 5, 255, 8, 161, 123, 200,
                                                         245, 146, 48, 251, 4, 110, 250, 212, 30, 223, 190, 231, 21, 229, 72, 205, 95, 162, 118, 174, 227, 180, 57, 209, 70, 94, 11,
                                                         135, 155, 28, 154, 179, 152, 35, 127, 129, 100, 132, 204, 107, 243, 145, 138, 101, 248, 23, 228, 83, 91, 164, 156, 210, 40,
                                                         134, 81, 187, 89, 171, 170, 103, 58, 31, 208, 214, 196, 55, 169, 149, 15, 32, 236, 216, 99, 54, 0, 202, 234, 61, 2, 199,
                                                         59, 230, 20, 150, 39, 43, 177, 12, 86, 178, 10, 16, 75, 225, 112, 176, 219, 189, 26, 41, 233, 122, 117, 109, 64, 224, 27,
                                                         193, 116, 87, 192, 33, 191, 244, 62, 115, 172, 203, 252, 111, 222, 238, 183, 38, 213, 105, 92, 194, 3, 102, 235, 80, 56,
                                                         168, 51, 65, 68, 17, 93, 77, 67, 184, 246, 128, 207, 131, 9, 137, 181, 22, 163, 242, 37, 114, 24, 52, 206, 45, 104, 1, 148,
                                                         19, 160, 36, 18, 198, 29, 249, 46, 96, 124, 241, 120, 151, 88, 73, 153};

        // REFER: https://stackoverflow.com/questions/12773257/does-auto-type-assignments-of-a-pointer-in-c11-require
        auto p = reinterpret_cast<const unsigned char *>(key);
        struct KVMessageHash hash;
        int i;

        for (i = 0; i < KV_STR_LEN; i++) {
            hash.hash1 = 33 * hash.hash1 + (random_nums[i] * p[i]);
            hash.hash2 = 33 * hash.hash2 + p[i];
        }

        return hash;
    }

    inline bool is_req_GET() const { return status_code == 1; }

    inline bool is_req_PUT() const { return status_code == 2; }

    inline bool is_req_DEL() const { return status_code == 3; }

    inline bool is_req_SUCCESS() const { return status_code == 200; }

    inline bool is_req_ERROR() const { return status_code == 240; }
};

#endif // PA_4_KEY_VALUE_STORE_KVMESSAGE_HPP
