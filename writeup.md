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

#### TODO: `lseek`

more Random vs sequential

#### TODO: Impact of Logging

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

### TODO: Impact of Logging Code Example

Demo the performance impact of using no logs, print logs, user-space buffered write logs, OS-space buffered write logs, and completely unbuffered write logs.
