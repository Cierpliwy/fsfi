#include "test.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

TEST_FUNC()
{
    int fd, fd2, fd3, tmp;
    struct stat fs, fs2;
    char *addr;
    CHECKPOINT();

    // Create test1 file
    ASSERT(creat("test1", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH),
           == -1, fd, "test1 file descriptor");

    // Duplicate descriptors
    ASSERT(dup(fd), == -1, fd2, "duplicate test1 file descriptor");

    // Get file statistics and compare them
    ASSERT(fstat(fd, &fs), == -1, tmp, "get test1 file statistics"); 
    ASSERT(fstat(fd2, &fs2), == -1, tmp, "get test1 dup file statistics"); 
    ASSERT(memcmp(&fs, &fs2, sizeof(struct stat)), != 0, tmp,
           "compare if two statistic structures are the same"); 
   
    // Create symlink before any file changes.
    ASSERT(symlink("test1", "test1symlink"), == -1, tmp,
           "creating symlink to test1");

    // Allocate 256 B to a file
    ASSERT(fallocate(fd, 0, 0, 256), == -1, tmp, "allocating 256B to test1");

    // Jump to 512B
    ASSERT(lseek(fd2, 512, SEEK_SET), != 512, tmp, "jump to 512B");

    // Write 256 numbers
    int i;
    for (i = 0; i < 256; ++i) {
        unsigned char c = i;
        char msg[30];
        sprintf(msg, "write %d B to test1", i);
        ASSERT(write(fd, &c, 1), != 1, tmp, msg); 
    }

    // Synchronize files
    CALLFUNC(sync(), "synchronize files to disk");

    // Open test1 once again
    ASSERT(open("test1", O_RDWR), == -1, fd3, "open test1 once again");

    // Mmap first 256B
    ASSERT_CAST(mmap(NULL, 256, PROT_WRITE | PROT_READ, MAP_SHARED, fd3, 0),
                == (void*)(-1), addr, "map test1 to memory", char*);
    char *addrorig = addr;

    for (i = 0; i < 256; ++i) {
        *addr = 0xff;
        ++addr;
    }

    // Synchronize memory with disk
    ASSERT(msync(addrorig, 256, MS_SYNC | MS_INVALIDATE), == -1, tmp,
           "synchronize mapped memory");

    // Unmap memory
    ASSERT(munmap(addrorig, 256), == -1, tmp, "unmap memory");

    // Copy final file
    ASSERT(link("test1", "test1cpy"), == -1, tmp, "create hard link");

    // Change permissions to read only
    ASSERT(fchmod(fd, 0444), == -1, tmp, "change permissions to read only");

    // Close files
    ASSERT(close(fd), == -1, tmp, "close descriptor nr 1");
    ASSERT(close(fd2), == -1, tmp, "close descriptor nr 2");
    ASSERT(close(fd3), == -1, tmp, "close descriptor nr 3");
}

CHECK_FUNC()
{
    int fd, tmp;
    struct stat fs;
    unsigned char buffer[768];
    CHECKPOINT();

    // Open file for check
    ASSERT(open("test1", O_RDONLY), == -1, fd, "open test1");

    ASSERT(fstat(fd, &fs), == -1, tmp, "bad fstat");
    ASSERT(fs.st_mode & 0777, != 0444, tmp, "bad mode");
    ASSERT(fs.st_size, != 768, tmp, "bad size");

    ASSERT(read(fd, buffer, 768), != 768, tmp, "bad read"); 

    int i;
    for (i = 0; i < 256; ++i)
        ASSERT(buffer[i], != 0xff, tmp, "bad value");

    for (i = 256; i < 512; ++i)
        ASSERT(buffer[i], != 0, tmp, "bad value");

    for (i = 512; i < 768; ++i)
        ASSERT(buffer[i], != i-512, tmp, "bad value");

    ASSERT(close(fd), == -1, tmp, "close descriptor");
}
