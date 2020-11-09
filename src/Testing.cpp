#include <iostream>
#include <fstream>
#include <array>
#include <deque>
#include <vector>
#include <chrono>
#include <mutex>


#include "MyDebugger.hpp"
#include "MyVector.hpp"
#include "KVMessage.hpp"
#include "KVStore.hpp"

using namespace std;
using namespace std::chrono;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"

// ---------------------------------------------------------------------------------------------------------------------

int main_kv_message() {
    // Max value of hash if all characters have value 255 and hash is calculated using "(33 * h) + ((char)255) * 255"
    // 111557556025954874611926853334682961517692669563128906605981051103366420166783202320332187257350466191430438956074299228210662031213993588883531412859973447056664768936968951369483701224766727930387416086622583289366352087655294351605590873929473763214784301209237010774075411665643735599839978560104708034582802804166951524569178101560326730771695071712167529076240928206621755367075721376000
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
    char str[] = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";
    uint64_t h = 0, h2 = 0;
    for (int i = 0; i < 256; ++i) {
        h = 33 * h + str[i];
        h2 = 33 * h2 + ((*reinterpret_cast<unsigned char *>(str + i)) ^ random_nums[i]);
    }
    std::cout << h << std::endl;
    std::cout << h2 << std::endl;
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------

int main_file_io() {
    fstream f;
    // f.open("/home/student/Desktop/Fenil/fenil_Laptop/25_fenil_practice/0050_IIT Bombay/CS744 - DECS/CS744-PA4-Key-Value-Store/src/"
    //        "aa.bin", ios::binary | ios::in | ios::out | ios::app);
    // f.close();
    // cout << system("mkdir -p db") << endl;

    fstream File("d.txt", ios::in | ios::out );
    File.seekg(0, ios::end);
    File << "TutorialsPoint";
    // char F[9];
    // File.read(F, 5);
    // F[5] = 0;
    // cout <<F<< endl;
    File.close();
    return 0;
}

int main_vector_test() {
    auto start = high_resolution_clock::now();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    start = high_resolution_clock::now();
    vector<int> arr1;
    arr1.reserve(10'00'00'000);
    for(int64_t i = 0; i < 10'00'00'000; ++i) {
        arr1.push_back(i);
    }
    stop = high_resolution_clock::now();
    for (int64_t i = 1; i < 10'00'00'000; i += 1'0'00'000) {
        cout << arr1[i] << endl;
    }
    // for(auto &i: arr1) cerr << i;
    // cerr.flush();
    duration = duration_cast<microseconds>(stop - start);
    cerr << "Time taken by function: " << duration.count() << " microseconds" << endl;

    start = high_resolution_clock::now();
    MyVector<int> arr2(16);
    // arr2.reserve(10'00)
    for(int64_t i = 0; i < 10'00'00'000; ++i) {
        arr2.push_back(i);
    }
    stop = high_resolution_clock::now();
    for (int64_t i = 1; i < 10'00'00'000; i += 1'0'00'000) {
        cout << arr2.at(i) << endl;
    }
    // for(int64_t i = 0; i < arr2.n; ++i) cerr << arr2.at(i);
    // cerr.flush();
    duration = duration_cast<microseconds>(stop - start);
    cerr << "Time taken by function: " << duration.count() << " microseconds" << endl;

    // for(int64_t i = 0; i < 10'00'00'000; ++i) {
    //     if(arr1[i] != arr2.at(i)) cout << arr1[i] << ", " << arr2.at(i) << endl;
    // }

    return 0;
}

// REFER: https://www.geeksforgeeks.org/measure-execution-time-function-cpp/
int main_mutex_speed(){
    int a1=0, a2=0;
    pthread_mutex_t m1;
    pthread_mutex_init(&m1, nullptr);

    auto start = high_resolution_clock::now();
    for(int32_t i = 0; i < 10'00'000; ++i) {
        pthread_mutex_lock(&m1);
        a1+=1;
        pthread_mutex_unlock(&m1);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;


    start = high_resolution_clock::now();
    mutex m2;
    for(int32_t i = 0; i < 10'00'000; ++i) {
        m2.lock();
        a2+=1;
        m2.unlock();
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: " << duration.count() << " microseconds" << endl;

    if(a1!= a2) {
        cout << "ERROR111";
    }
    pthread_mutex_destroy(&m1);
    return 0;
}

void db_testing() {
    kvPersistentStore.init_kvstore();  // This is present in KVStore.hpp
    for(int32_t i = 0; i < HASH_TABLE_LEN; ++i) {
        if(not kvPersistentStore.file_exists_status.test(i)) continue;
        kvPersistentStore.read_db_file(i);
    }
    // KVMessage kvm;
    // kvm.set_key("asdf");
    // kvm.set_value("1234");
    // kvm.calculate_key_hash();
    // kvPersistentStore.write_to_db(&kvm);
}

int main() {
    // return main_kv_message();
    // return main_file_io();
    main_vector_test();
    // main_mutex_speed();
    // vector<int> a;
    // a.push_back_emplace(1);
    // db_testing();

    return 0;
}

#pragma clang diagnostic pop