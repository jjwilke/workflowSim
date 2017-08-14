#ifndef MYIPCTUNNEL_H
#define MYIPCTUNNEL_H

#include <fcntl.h>
#include <cstdio>
#include <vector>
#include <string>
#include <errno.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <macros.h>

#include <myCircularBuffer.h>

class TraceTunnel {

    struct ProtectedSharedData {
        volatile uint32_t expectedChildren;
        size_t shmSegSize;
        size_t numBuffers;
        size_t offsets[0];  // Actual size:  numBuffers + 2
    };

protected:
    cir_buf_t *sharedData;

private:
    //

    cir_buf_pair_t reserveSpace(size_t workSpace = 0)
    {
        size_t space = CIR_BUF_SIZE + workSpace;
        bool outOfBounds = ((nextAllocPtr + space) - (uint8*)shmPtr) > shmSize;
        if (outOfBounds)
        {
            return cir_buf_make_pair_t(0, NULL);
        }
    }

    size_t static calculateShmemSize(size_t numBuffers, size_t bufferSize)
    {
        long page_size = sysconf(_SC_PAGESIZE);

        /* Count how many pages are needed, at minimum */
        size_t psd = 1 + ((sizeof(ProtectedSharedData) + (1+numBuffers)*sizeof(size_t)) / page_size);
        size_t buffer = 1 + CIR_BUF_SIZE + (WORKSPACE_SIZE / page_size);
        size_t shdata = 1 + ((sizeof(ShareDataType) + sizeof(ProtectedSharedData)) / page_size);

        /* Alloc 2 extra pages, just in case */
        return (2 + isd + shdata + numBuffers*buffer) * page_size;
    }


};



}
}
}

#endif /* MYIPCTUNNEL_H */
