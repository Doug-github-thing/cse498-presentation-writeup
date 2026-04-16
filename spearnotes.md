What does SSO mean?
- Thinking about disk/ssd works under the hood and how to modify program
  functions to optimize for it
- Different data structures based on hardware
- Hard disks and defragmenting them
- Go to disk as little as possible
- Dealing with physical faults
- Preventing physical faults (e.g., wear leveling)

### Outline
- Storage usage
 -- integrity performance tradeoff
 -- Compression

- Cost of storage
 -- Storage system metrics
 -- Fragmentation, syncing, thrashing
 -- Lseek
 -- Read/write amplification

- Controlling interaction with storage
 -- Buffering
 -- WAL
 -- LSM

- Using storage to improve performance

### Application-Specific Tradeoffs

When can you afford to risk losing writes in favor of performance
- If recomputing is faster than retrieval, especially in databases
- Video game checkpoints (not gamebreaking)
- If you can recover fast enough, or know persistence will eventually work,
  you can delay writing for a while
- MSWord autosaving.  Situation-dependent.  Google docs autosaves on every
  character.
- Pharmaceutica lab data: no compromises!  Can't lose anything or FDA shuts
  you down.

Data logger Case Study
- Embedded controller with environmental monitoring device
- Very small 8kb storage system.  If you need to store data point every 5
  minutes, just 2 floats.  Must be able to recover all data since last time
  data was read.
- If you don't need the whole float, then you can be a lot shorter with the
  amount of data to write.
- 32-bit timestamp and two float32s: 3000 data points == 62KB, too big fr 8KB
  storage
- Only 392 data points in memory: 1.3 days of coverage.
- Only 22 bits needed for timestamp, make it relative to start time.  Also,
  data values can be much smaller for temp and humidity.  Now 3000 data
  points is 24KB, can fit 3.5 days of coverage in 8KB (1000 data points)
- But this has padding because 10 bits of padding, 10 bits of humidity.
- Note, too, that FP isn't supported, so switching to Ints is a lot faster :)
- If you are precise about 5-minute intervals, then don't save timestamp.
  Also don't have padding.
  - Now 7506 bytes for 3000 data points, so it fits.  10 days at a time!
- You could add parity bits to know which words of memory are valid
- You might risk losing the previous write if you crash in the middle of the
  current write.

Storage Drive Metrics
- Sequential vs Random access
- Read vs write
- IOps and throughput
- Reads generally faster than writes
  - SSD writes even slower, because erase block before writing
- Need to optimize workloads around the behavior of the disk
- On the benchmark, 1 vs 8 ops queued before waiting for a response.
  - Sequential faster than rand
  - Reads faster than writes
  - Bigger queues lead to higher throughput
  - Bigger blocks of access lead to higher throughput
- Throughput is IOps x BlockSize
- Then we looked at IOps.  Random IOps were very high, because of the small
  block size and the big queue.
  (You were discussing disks here.  It might help to have a picture of a disk
  in the write-up to help illustrate the cost of seeking).
- SQL Database: usually don't read the whole database at once.  If you read
  all sequential data, it would be gigabytes of data.  So instead, you can do
  a random read, 158K times per second, because of the high IOps rate.
- So sequential for bulk stuff, random for small stuff
- On SSD, random is not as bad as HD
- Queue determines number of parallel operations.  Depth determines how many
  ops per queue before waiting for a response.  NVMe drives have 64K queues,
  64K ops per queue, so highly parallelizable.

How can storage be used *inefficiently* to increase performance
- Amount of storage is big, you can be sloppy...
- Precompute something and store it.  Memoization.  This includes aggregation
  in databases.  You could compute the aggregation as you do the insertions.
- Sanity checks like parity bits.
- Write sequentially, clean it up later (WAL)
- Duplicate / redundant data.  Instead of many scattered files, if you know
  what the queries will be, clone the data.  For example, if you have foreign
  keys in your database, Denormalize accordingly.  (Liam's description was
  good, but a picture would have helped a lot!)
- Store raw uncompressed data.  (I wasn't convinced by this example.  Is the
  compute overhead really significant?)

Case Study: Video Game Storage
- Over 100GB of data for a 3D environment
- Lots of redundant / duplicate textures and assets.  E.g., store the texture
  in every level's asset package.
- Usually an optimization for HDD behavior, since you can read one file, no
  seeks.
- Reduces dev time, since you don't need to rebuild packages when updating
  assets.
- Gamers actually buy HDDs, not SDDs, for this reason!
- I added that texture packing gives other benefits in terms of metadata
  savings, better compression.

Case Study: HellDivers 2
- Switched from buge 154GB downloader to a "slim" 23GB installation
- Turned out that the game wasn't HDD-bound, so deduplicating didn't
  introduce a performance penalty.  Since maps are procedurally generated,
  and that takes a long time, the HDD worst-case overhead is masked by level
  generation.  Essentially pipelining was all they needed!
- They probably saved a ton of dev time.  The fix was done via
  subcontracting.
- They only noticed this because of porting to console, having the slim
  install.

### What hurts storage performance

- fsync
- Fragmentation
- Thrashing
- Improper access patterns
- Read/write amplification

I encouraged some discussion of fsync.  The main idea is about "what if it
crashes".  Liam followed up by talking about data integrity, and also about
how it only stalls the current thread (and other I/O in the queues).  That
was good to add :)

Fragmentation: Liam's example described how you might have external
fragmentation.  Preallocating is a strategy to employ.  Another is to avoid
in-place updates.  This is a big deal on HDD.  On SSD, where the cost of
random access is lower, defragmenting can be bad.  Every read and write of an
SSD block degrades the perforance of the block, so if you frequently defrag
the data, you're wearing out the device / worsening the lifecycle of your
drive.

Thrashing: OS overflows to the page file or swap space when RAM fills up.
This is especially bad in heavy I/O contexts.  I brought up an example with
Android lifecycles and how to avoid thrashing on context switches with SSD
devices.

Access Patterns:  If you structure workload as random for no good reason, or
as sequential when random is better, that hurts performance.

Amplification: Misaligning read/write sizes causes unnecessary overheads.
- SSDs read/write in blocks.  Writes in 4KB pages, erase in 256KB blocks.
  Writes don't overwrite, they erase the whole block first.
- SSD has garbage collection to do the copy/erase cycle.
- **I think you were a bit off in this explanation**.  I think it's that if
  you write an amount that's not 4KB, that's where you have the worst
  amplification....
- SSDs do have mitigations, of course...
- There's also a cache on the SSD.  QLC is the cheapest way to implement SSD.
  Four levels can be measured, so writing takes a long time.  But the fast
  cache is SLC, only one level of measurement.  So a fast SLC cache is
  helpful.

LSeek vs Full Files:
- Lseek is for random access.  Can break spatial locality.  Sequential access
  / working on full files improves performance.

Note that SSD GC can create predictability issues.
- It's important to keep your disk under a certain capacity, so there is room
  for the firmware to do the fancy stuff.  For example, Windows keeps some
  unallocated space for these optimizations.  80% is a good target.

Write Amplification Mitigations
- I brought up VARCHAR and preallocating
- (Read amplification is usually things like the data spanning a few blocks,
  or things like that).
- Solutions include buffering, keeping data contiguous, aligning writes to
  SSD page/block sizes.

Note that a lot of this is just like RAM, but the cost of getting it wrong is
MUCH larger.


# Day 2 #######################################################

Interacting with Storage
- Need to tune interaction with storage to match characteristics
  - W/O fsync, writes are lazy, staying in RAM, but reads are not.
  - So writes worse than reads in general, but not with buffering
- Structuring write-heavy workloads can be faster depending on freq/size of
  writes
  - Precompute/store results on insert
  - Leverage the SLC cache on an SSD
  - Write redundant copies of data together for better read access
    - Nice ERD and discussion of denormalization
    - I encouraged a discussion about "what is the common case".  it's *not*
      cascading deletes, or about changing user names.
    - But this does have write amplification
  - I encouraged a discussion about TSO and how it relates to the write
    buffering stuff.

Write-Ahead Logging
- Write new data into a small append-only location on disk
- Append-only, sequential writes to a log
  - Converts random writes into sequential I/O
  - Better throughput, latency is more stable, crash safety
- My Additions: much easier to manage variable sizes, and know the difference
  between WAL and journalling.  WAL is journalling that happens early and
  pipelines the FSYNC with the work that happens after writing to the
  journal.  In contrast, sometimes you can journal but only at the end.  In
  that case you avoid seeking, but you don't pipeline memory work with the
  fsync.

Buffering
- You might want to buffer writes for larger sequential writes
- Why?  Controlling buffer size can be important
  - Control fault tolerance
  - Big buffers might be bad for cache locality, page tables, etc.
- You almost always want to base buffer size on page size
  - Avoid write amplification
- I mentioned the caution about when your buffer flushes to another buffer
  - An example is the move to the "unified buffer cache" in Linux 2.4->2.6 (I
    think).
- I also mentioned how "going around the OS" can introduce errors.  You could
  break the fault tolerance expectations of the program, or you could even
  break low-level stuff like violating ABA safety.

Impact of Logging
- Often want to log results and information
- Printing results can be very slow
  - More logs improve error tracing and debugging, but hurt performance
  - Developers need to find a balance between transparency and performance
- Example: Printfs vs buffered writes and fsync'd writes
- Storing logs on disk can reduce cost of logging (cheaper than writing to
  terminal)
- Buffering reduces fsync overhead at cost of crash tolerance
  - C++ ofstream buffers writes by default
- The exapmle code was nice :)
- Considering "write-without-fsync" and "fprintf to stderr instead of printf
  to stdout" could be useful too.
- It would be be good to clarify: if writes that reach the SSD's queue are
  sure to happen, is that guaranteed by the fsync?
- Nice demonstration of DVFS: when the laptop was plugged in, things sped up

LSM Trees
- write data is accumulated in memory, flushed to disk as sorted L0 files
- When L0 is full, compaction happens
  - Select set of L0 files, identify all keys present
  - Merge data into L1 files with overlapping key ranges, since L1 files
    globally ordered
  - During merge, update things, remove obsolete data, and do other cleanup
- This creates a tree of files, where each level has files covering a larger
  key range
- Initiall changes accumulate in a memtable in RAM (and update WAL).
  - Flushing to L0 happens when memtable is full
- L0 file is a single flush
- You can think of l0 as 10MB, L1 as 100MB, L2 as 1000 MB, and so forth
- Files at a given level are larger than files at a preceding level.
- End is one file of the entire key range
- With this, all writes go to small, low-level files.  One-time writes.

Why good?
- Writes don't search, upate, and sync.  Only happen to make new L0 file or
  do a compaction
  - Writes sequential on disk
  - Avoid large file fsyncs, only happen when lower levels overflow
- Efficient read access
  - Searches all L0 files, move up a level and search 1 file per level
  - Fast for recently updated data, always sequential
  - Worst case is all L0 plus one file per subsequent level
  - Fast for ranges, since files ordered in ranges
- I noted there's an analogy to memory caches where the higher levels have
  stale data
- Nice discussion of comparison to B+-Tree.  Maybe a picture would help
- I also added discussion about range() vs scan().  You followed up on that
  very nicely, mentioning that each L0 file is sorted, but the L0 files can
  have overlapping ranges.
- Note: log-structured because of constant scaling factor, not because it's
  10x bigger at each level.
- Most expensive step is compaction step, because you have to sort all your
  L0 data, and merge it into a larger file.  But you don't do that while
  writing, you do that in idle time on workloads.  Also, it does the GC on
  the merge steps, which is nice.
- LSM trees invented in 1996.  Took off with SSDs in mid 2000s, because
  well-suited for bursty cache writes to SLC, and for page-sized writes to
  the SSD.
- Really good question about concurrent threads.  Key point is that app
  threads only write to WAL, but might read from levels.  Compaction is
  orthogonal, different threads.  And memtable handles a lot of it.
- Note that RocksDB extends LevelDB with a concurrent memtable (among other
  things).  FB is almost all running RocksDB now.

HW Fault Recovery
- ECC
- Wear leveling
- Bad block management, remap failing sectors
- Firware-level gc in SSDs
- I wanted to add that these apply in HDD, not just SSD

HW can help protect integrity of bits on device, protect life of device, and
degrade gracefully.  HW can't protect against file system corruption, total
drive failure, etc.

Redundancy can help with that (RAID is across disks; checksums to detect
corruption above device layer).

Also geographic replication

I pointed out that RAID 2-4 are not practical.  In an interview, don't think
you should explain them.  They're just to help make the conceptual leap from
1 to 5.