#ifndef PA_4_KEY_VALUE_STORE_KVMESSAGE_HPP
#define PA_4_KEY_VALUE_STORE_KVMESSAGE_HPP

#include <cstdint>
#include <string>

#define KV_STR_LEN 256

struct KVMessage {
    // Everything depends on this enum about what value to use for each "status_code"
    enum StatusCodeEnum {
        EnumGET = 1, EnumPUT = 2, EnumDEL = 3, EnumSUCCESS = 200, EnumERROR = 240
    };
    constexpr static const char ERROR_MESSAGE[256] = "Entry not found";
    static const uint8_t StatusCodeValueGET = EnumGET;
    static const uint8_t StatusCodeValuePUT = EnumPUT;
    static const uint8_t StatusCodeValueDEL = EnumDEL;
    static const uint8_t StatusCodeValueSUCCESS = EnumSUCCESS;
    static const uint8_t StatusCodeValueERROR = EnumERROR;

    uint8_t status_code;
    char key[256], value[256];
    uint64_t hash1, hash2;

    KVMessage() : status_code{}, key{}, value{}, hash1{0}, hash2{0} {}

    /* "ptr" is a null terminated pointer to char array
     *
     * The main role of this method is to ensure that all characters
     * after the '\0' character are null ('\0') in the variable "key"
     * */
    void set_key(const char *ptr) {
        int_fast16_t i = 0;
        for (; i < 256 && (*ptr); ++i, ++ptr) {
            key[i] = *ptr;
        }
        for (; i < 256; ++i) {
            key[i] = '\0';
        }
    }

    /* "ptr" is a null terminated pointer to char array
     *
     * The main role of this method is to ensure that all characters
     * after the '\0' character are null ('\0') in the variable "value"
     * */
    void set_value(const char *ptr) {
        int_fast16_t i = 0;
        for (; i < 256 && (*ptr); ++i, ++ptr) {
            value[i] = *ptr;
        }
        for (; i < 256; ++i) {
            value[i] = '\0';
        }
    }

    void set_key_fast(const char *ptr) {
        for (int i = 0; i < 256; ++i)
            key[i] = ptr[i];
    }

    void set_value_fast(const char *ptr) {
        for (int i = 0; i < 256; ++i)
            value[i] = ptr[i];
    }

    void fix_key_nulling() {
        bool zeroEncountered = false;
        for(char & i : key) {
            if(i == '\0') zeroEncountered = true;
            if(zeroEncountered) i = '\0';
        }
    }

    void calculate_key_hash() {
        // if value of any hash is non-zero, then return, because it means that it has been calculated in the past
        // if (hash1 != 0 || hash2 != 0) return;
        // NOTE: above line is commented because we are re-calculating hash for the same
        //       KVMessage object with the updated value of the "Key"

        /* 
         * For hash1:
         *     REFER: https://stackoverflow.com/questions/11546791/what-is-the-best-hash-function-for-rabin-karp-algorithm
         *
         * For hash2:
         *     My custom implementation which is mix of hash1 algorithm and XOR
         *
         * "random_nums" is generated using the below Python Code:
         * >>> import random
         * >>> arr = list(range(256))
         * >>> random.shuffle(arr)
         * >>> print(arr)
         *
         * */
        const static uint64_t random_nums[KV_STR_LEN] = {14, 182, 49, 60, 211, 165, 125, 232, 71, 166, 133, 237, 13, 78,
                                                         76, 25, 215, 221, 108, 140, 254, 53, 119, 79, 239, 113, 188,
                                                         126, 157, 144, 121, 34, 50, 197, 66, 82, 106, 247, 47, 158, 44,
                                                         201, 136, 85, 175, 220, 167, 130, 147, 90, 74, 253, 186, 185,
                                                         84, 97, 217, 226, 7, 218, 141, 69, 139, 6, 142, 143, 98, 240,
                                                         195, 42, 173, 63, 159, 5, 255, 8, 161, 123, 200, 245, 146, 48,
                                                         251, 4, 110, 250, 212, 30, 223, 190, 231, 21, 229, 72, 205, 95,
                                                         162, 118, 174, 227, 180, 57, 209, 70, 94, 11, 135, 155, 28,
                                                         154, 179, 152, 35, 127, 129, 100, 132, 204, 107, 243, 145, 138,
                                                         101, 248, 23, 228, 83, 91, 164, 156, 210, 40, 134, 81, 187, 89,
                                                         171, 170, 103, 58, 31, 208, 214, 196, 55, 169, 149, 15, 32,
                                                         236, 216, 99, 54, 0, 202, 234, 61, 2, 199, 59, 230, 20, 150,
                                                         39, 43, 177, 12, 86, 178, 10, 16, 75, 225, 112, 176, 219, 189,
                                                         26, 41, 233, 122, 117, 109, 64, 224, 27, 193, 116, 87, 192, 33,
                                                         191, 244, 62, 115, 172, 203, 252, 111, 222, 238, 183, 38, 213,
                                                         105, 92, 194, 3, 102, 235, 80, 56, 168, 51, 65, 68, 17, 93, 77,
                                                         67, 184, 246, 128, 207, 131, 9, 137, 181, 22, 163, 242, 37,
                                                         114, 24, 52, 206, 45, 104, 1, 148, 19, 160, 36, 18, 198, 29,
                                                         249, 46, 96, 124, 241, 120, 151, 88, 73, 153};

        // REFER: https://stackoverflow.com/questions/12773257/does-auto-type-assignments-of-a-pointer-in-c11-require
        auto p = reinterpret_cast<const unsigned char *>(key);
        int_fast16_t i;

        // Very IMPORTANT to initialize the hash values to 0 to ensure
        // that past calls to calculate_key_hash(...) does not affect
        // the results
        hash1 = hash2 = 0;
        for (i = 0; i < KV_STR_LEN; i++) {
            hash1 = 33 * hash1 + p[i];
            hash2 = 33 * hash2 + (random_nums[i] * p[i]);
        }
    }

    inline std::string status_code_to_string() const {
        if(status_code == EnumGET) return "GET";
        if(status_code == EnumPUT) return "PUT";
        if(status_code == EnumDEL) return "DELETE";
        if(status_code == EnumSUCCESS) return "SUCCESS";
        if(status_code == EnumERROR) return "ERROR";
        return "Invalid status code";
    }

    [[nodiscard]] inline bool is_request_code_GET() const { return is_request_code_GET(status_code); }

    [[nodiscard]] inline bool is_request_code_PUT() const { return is_request_code_PUT(status_code); }

    [[nodiscard]] inline bool is_request_code_DEL() const { return is_request_code_DEL(status_code); }

    [[nodiscard]] inline bool is_request_code_valid() const { return is_request_code_valid(status_code); }

    [[nodiscard]] inline bool is_request_result_SUCCESS() const { return is_request_result_SUCCESS(status_code); }

    [[nodiscard]] inline bool is_request_result_ERROR() const { return is_request_result_ERROR(status_code); }

    [[nodiscard]] inline static bool is_request_code_GET(const int statusCode) {
        return statusCode == StatusCodeValueGET;
    }

    [[nodiscard]] inline static bool is_request_code_PUT(const int statusCode) {
        return statusCode == StatusCodeValuePUT;
    }

    [[nodiscard]] inline static bool is_request_code_DEL(const int statusCode) {
        return statusCode == StatusCodeValueDEL;
    }

    [[nodiscard]] inline static bool is_request_code_valid(const int statusCode) {
        return (1 <= statusCode && statusCode <= 3);
    }

    [[nodiscard]] inline static bool is_request_result_SUCCESS(const int statusCode) {
        return statusCode == StatusCodeValueSUCCESS;
    }

    [[nodiscard]] inline static bool is_request_result_ERROR(const int statusCode) {
        return statusCode == StatusCodeValueERROR;
    }

    inline void set_request_code_SUCCESS() { status_code = StatusCodeValueSUCCESS; }

    inline void set_request_code_ERROR() { status_code = StatusCodeValueERROR; }
};

// REFER: https://stackoverflow.com/questions/3025997/defining-static-const-integer-members-in-class-definition
// IMPORTANT: this is necessary for static variables, and since the variables are const, they are assigned value
//            in the class itself.
const uint8_t KVMessage::StatusCodeValueGET;
const uint8_t KVMessage::StatusCodeValuePUT;
const uint8_t KVMessage::StatusCodeValueDEL;
const uint8_t KVMessage::StatusCodeValueSUCCESS;
const uint8_t KVMessage::StatusCodeValueERROR;
constexpr char KVMessage::ERROR_MESSAGE[256];


#endif // PA_4_KEY_VALUE_STORE_KVMESSAGE_HPP
