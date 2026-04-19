#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

using namespace std;
using namespace std::chrono;

const int N = 100000; // Number of log entries
const int WORKLOAD_SIZE = 1000;

// Trivial simulation of a workload.
void do_work() {
    volatile int x = 0;
    for (int i = 0; i < WORKLOAD_SIZE; i++) {
        x += rand();
    }
}

// Just perform the workload, with no output logs at all.
void test_silent() {
    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        do_work();
    }
    auto end = high_resolution_clock::now();
    cout << "test_silent completed in " 
         << duration_cast<milliseconds>(end - start).count()
         << "ms\n";
}

// ofstream abstracts away buffered writes for you. This is a buffered test.
void test_buffered_fstream() {
    ofstream out("log_fstream.txt");

    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        do_work();
        out << "Log line " << i << "\n";
    }
    out.close(); // Flush once at the end

    auto end = high_resolution_clock::now();
    cout << "test_buffered completed in " 
         << duration_cast<milliseconds>(end - start).count()
         << "ms\n";
}

// Synchronous writes. Safe, but slow. Manual fsync between lines.
void test_fsync() {
    // Open file write-only, create if not exists, truncate to length 0
    int fd = open("log_sync.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[128];

    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        do_work();
        int len = snprintf(buf, sizeof(buf), "Log line %d\n", i);
        write(fd, buf, len);
        // fsync(fd); // <----
    }
    close(fd);
    auto end = high_resolution_clock::now();
    cout << "test_fsync completed in " 
         << duration_cast<milliseconds>(end - start).count()
         << "ms\n";
}

void test_printf() {
    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        do_work();
        printf("Log line %d\n", i);
    }
    auto end = high_resolution_clock::now();
    cout << "printf time: "
         << duration_cast<milliseconds>(end - start).count()
         << " ms\n";
}

int main(int argc, char* argv[]) {
    if ( argc > 1 && argv[1]) {
        test_printf();
        return 0;
    }
    
    test_buffered_fstream();
    test_silent();
    test_fsync();

    return 0;
}