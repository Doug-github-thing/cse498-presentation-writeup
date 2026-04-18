# Considerations for the Storage System

## Introduction

What does storage system optimization mean from the perspective of a performance engineer? 

We will cover the following topics in this resource:

1. Application Specific Trade-offs
1. Examining Interactions With Storage
1. Using Storage to Improve Performance
1. LSM Trees
1. Storage System Fault Tolerance
1. Case Studies

## Application Specific Trade-offs 

A key aspect of performance engineering is understanding the trade-offs that go into each engineering decision made about a system. Understanding the requirements of the application is essential to effective optimization. Interacting with the storage system is no exception.

- Runtime Performance vs Space Utilization
- Runtime Performance vs Data Integrity
- Runtime Performance vs Observability (Logging)
- Space Utilization vs Storage Reliability
- Space Utilization vs Time To Market

As we explore how to better interact with the storage system when we look at optimizing our code, these trade-offs will continue to come up in be explored in detail.

### Space Utilization

#### Using Space Efficiently

There are sometimes cases where the needs of an application will require a programmer to regard efficient utlization of the storage system as a parameter of particular importance. This is true whenever storage space is limited such as porting videogames for consoles, but a more extreme example is in embedded systems. Due to low level design decisions, a programmer for embedded systems may find themselves writing code to run on devices where storage space can be on the order of kilobytes. If significant amounts of data are to be written to devices of such limited size, care must be taken to ensure that the needs of the application can be made to fit within the confines of the hardware.

##### TODO: Maybe talk about compression in general a little bit. Not in detail, just that compression exists and should be used if this is something important to the application

#### Using Space Inefficiently

In many cases however, storage space is not the most important parameter to be optimized for, and there are in fact many cases where the parameters more important to an application rely on techniques that intentianally use storage space inefficiently.

These will be expanded upon in the Using Storage to Improve Performance section.

## Examining Interactions with the Storage System

- fsync
- Fragmentation
- Thrashing
- Improper access patterns
- Read/write amplification
- Logging

### `fsync` and Buffering

`fsync` is a syscall that ensures that the file descriptor being operated on has completed its write to the disk. Upon completion, the program can be confident that the data will be recoverable in the event of a crash or power failure.*It is expensive.* However, it is the only way for an operating system to be sure that the requested data has made it to the storage device.

Significant speed improvements can be achieved by buffering writes to minimize the number of `fsync` calls in a program. However the biggest trade-off to consider in this case is that of data integrity. If using heavy write buffering, the larger the buffer size prior to an `fsync`, the more data can be lost in the event of a crash. The Runtime Performance vs Data Integrity trade-off requires us to be conscious of when we can afford to lose a little data here and there, and when we can't. A text editor can afford to miss a few recent uncommitted lines. We expect a videogame to likely not be able to recover all of the most up to date data following a device power failure. These are applications where the user experience is much more important than absolute data integrity. In contrast, computer systems responsible for critical infrastructure, banking, government records, systems compliant with FDA Title 21 CFR Part 11 are application areas where data integrity is held at a much higher regard than user experience. Design decisions must be made with those considerations in mind.

TODO Expand on Spear point: In addition to the data integrity risks of large buffers, using buffers may also hinder performance by negatively affecting cache locality / page table... You almost always want to base buffer on page size.

### TODO: Fragmentation

### TODO: Thrashing

### TODO: Read/Write Amplification

### TODO: Access Patterns

Random vs sequential. Note, general advice is very much like when using RAM, but for different reasons!

#### `lseek`

`lseek` is a syscall that allows for seeking to a specific byte offset in a given file descriptor. This is how random access in a file is performed. Being a system call, this brings some overhead with it each time it is needed. Sequential reads from a file are able to eliminate `lseek` overhead if a file needs to be read multiple times by a program. 

See the referenced code in the Impact of Rnadom Access Pattern Code Example at the end of this document.

The code example demonstrates costs of random accessing lines in a file using `lseek`. It does so by reading a file with 100000 lines forwards fully sequentially, and by reading the same file backwards, using `lseek` at every line, seeking backwards each time. On my machine, the sequential read pattern without any seeking completes in 53ms, while the random access read pattern using `lseek` at every line completes in 99ms. This is a roughly 2x speedup for reading the same amount of data. In practice, this means that sometimes it may be more performant to read a large chunk of sequential data, potentially reading more data than is actually needed by the program in order to keep the data accesses sequential.

#### Impact of Logging

A significant aspect of maintaining software systems is to be able to have visibility into the operation of the production code. This allows the maintainers to be able to see into the system as it runs, or in many cases after it crashes. As with any good tool, its use is not free. This is the Runtime Performance vs Observability tradeoff. By requiring additional actions from your system, there will be a cost to performing these actions. In many cases, due to ease of use and interacting with a program, the developer may want to print logs directly to stdout or stderr. However as we will see in the following code example, this can incur its own overhead compared to further usage of the storage system. The code example demonstrates what we can expect to pay in performance for different types of file writing paradigms to gain some insight into the cost of specific file system interactions. 

See the referenced code in the Impact of Logging and Buffering Code Example at the end of this document.

Running this benchmark, the expected behavior is that the `silent` will be the fastest process due to no additional overhead being taken on, trading off all observability for performance. Next fastest is `buffered_ofstream`, which buffers writes in user space. This takes syscall overhead away, and results in performance surprisingly close to the `silent`, due to the fact that ofstream is already implemented to optimize for performance. The next test is `buffered_os`, which drops the user abstraction layer of ofstream, leaving behind write syscalls that get buffered by the OS. Due to still using write buffering, this implementation is still significantly faster than the `fsync` version due to minimizing the total number of times that the system reaches out to the storage media itself. Finally, the `fsync` test demonstrates just how costly the `fsync` operation actually is. 

Comparison of these last two tests demonstrate not only the Performance vs Observability tradeoff, but also the Performance vs Data Integrity. These are the exact same function, except one manually fsyncs between each write and the other does not. The result is that the really slow one actually gets the benefit of having more stable crash performance, since in between each iteration, the OS gets the guarantee from the storage medium that the data that the user has attemtped to write has actually make it to the hardware. Conversely, the significantly faster buffered versions all have inferior crash stability because even after the benchmark thinks that the write operation was performed, the data never makes it to the storage medium until the final `fsync` occurs at the end. That means data could still be lost between writing to the buffer and writing to disk, resulting in data integrity bugs in the program.

## Using Storage to Improve Performance

Parity bits and error correction codes are used by storage systems at the hardware level to identify or even correct for data storage issues over time. This does not however allow for protection against filesystem corruption. Checksums should be used to detect corruption above the device layer.

Memoization is the practice of storing the results to expensive calls to functions. This is often done in main memory and may not always reach the storage system, but in the context of larger data systems, this could spill over. 

In databases, this manifests as denormalization. Duplicate copies of data can be included near other pieces of data which will need to often be accessed together. This optimization increases storage utilization, harms write performance of the database due to data being stored in multiple places, but can significantly improve read performance for the most common cases.

##### TODO: Liam add denormalization image and explanation

## TODO: LSM Trees Liam add this I guess. DIdn't know where better to put this, it's kind of its own thing


## Storage System Fault Tolerance

Hardware already does some things for you that you don't need to worry about. But it helps to be aware of what they can and what they can't do for you.
TODO: Doug expand on this.

## Case Studies

### TODO: Data Logger

Examine the extreme case where storage space usage is the primary concern.

### TODO: PC games

Examine the extreme case where storage space usage is NOT a concern. But also it kind of can be sometimes if porting to console.

### Impact of Random Access Pattern Example

See `lseek` for relevant analysis.

```python
# Use this script to generate the input file for the C++ random access benchmark demonstration
with open("input_file.txt", 'w') as file:
  for i in range(100000):
    file.write(f"line {i:5}\n")
```

```C++
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
```

### Impact of Logging and Buffering Code Example

See Impact of Logging section for relevant analysis.

```C++
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

// Just perform the workload with no output logs at all.
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
// Note, ofstream buffers in user space so the OS is not responsible for
// handling the write buffering. Fewer syscalls compared to test_buffered_os.
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

// By not calling fsync directly, the OS is free to buffer file writes.
void test_buffered_os() {
    // Open file write-only, create if not exists, truncate to length 0
    int fd = open("log_sync.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[128];

    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        do_work();
        int len = snprintf(buf, sizeof(buf), "Log line %d\n", i);
        write(fd, buf, len);
        // fsync(fd); // Do not call fsync directly, leave to OS to buffer.
    }
    close(fd);
    auto end = high_resolution_clock::now();
    cout << "test_fsync completed in " 
         << duration_cast<milliseconds>(end - start).count()
         << "ms\n";
}

// Calls fsync every line. Slow but safe.
void test_fsync() {
    // Open file write-only, create if not exists, truncate to length 0
    int fd = open("log_sync.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[128];

    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        do_work();
        int len = snprintf(buf, sizeof(buf), "Log line %d\n", i);
        write(fd, buf, len);
        fsync(fd); 
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
    test_printf(); 
    test_buffered_fstream();
    test_buffered_os();
    test_silent();
    test_fsync();

    return 0;
}
```
