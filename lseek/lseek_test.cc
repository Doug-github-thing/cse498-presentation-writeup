#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace std::chrono;

const char* FILENAME = "input_file.txt";
const int N = 100000; // Number of lines to read
const int line_len = 11;  // length of each line (bytes)

void test_lseek() {
    // Open file read-only
    int fd = open(FILENAME, O_RDONLY, 0644);
    char buf[line_len];
    vector<string> vec;
    vec.resize(N);
    int byte_counter = 0;

    auto start = high_resolution_clock::now();
    
    // Read the file backwards using lseek between each read to simulate random access
    lseek(fd, -1 * line_len, SEEK_END);
    for (int i=0; i < N; i++) {
        auto bytes_read = read(fd, buf, line_len);
        vec.emplace_back(buf);
        lseek(fd, -2 * line_len, SEEK_CUR);
    }
    close(fd);
    auto end = high_resolution_clock::now();
    
    cout << "test_lseek completed in " 
         << duration_cast<milliseconds>(end - start).count()
         << "ms\n";
}

void test_sequential() {
    // Open file read-only
    int fd = open(FILENAME, O_RDONLY, 0644);
    char buf[line_len];
    vector<string> vec;
    vec.resize(N);

    auto start = high_resolution_clock::now();
    // Read the whole file by performing reads sequentially, with no seeking.
    for (int i=0; i < N; i++) {
        auto bytes_read = read(fd, buf, line_len);
        vec.emplace_back(buf);
    }
    close(fd);
    auto end = high_resolution_clock::now();
    
    cout << "test_sequential completed in " 
         << duration_cast<milliseconds>(end - start).count()
         << "ms\n";
}

int main(int argc, char* argv[]) {

    test_lseek();
    test_sequential();

    return 0;
}
