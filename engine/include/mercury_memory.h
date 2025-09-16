#pragma once

#include "mercury_api.h"
#include <vector>
#include <stack>

namespace mercury {

namespace memory {
    struct ReservedAllocator
    {
        struct InitDesc
        {
            struct MemoryBucketInfo
            {
                u16 elementSize = 16;
                u32 maximumReservedSize = 0;
                u32 initialCommittedSize = 0;
            };

            std::vector<MemoryBucketInfo> bucketsInfo;
        };

        /// @brief Memory bucket for desired element size. Element size must be less than 64KB.
        /// maximumReservedSize and initialCommittedSize must be a multiple of the page size.
        /// maximumReservedSize and initialCommittedSize must evenly divide by the elementSize.
        struct Bucket
        {
            Bucket(u16 elementSize, u32 maximumReservedSize, u32 initialCommitedSize);
            ~Bucket();

            void* beginRegion = nullptr;
            u32 maximumReservedSize = 0;

            u32 commitedElements = 0;            
            u32 lastAllocatedID = 0;
            u32 maximumReservedElements = 0;
            u16 elementSize = 0;
            std::stack<u32> freeList;

            void* Allocate(u16 size);
            void Deallocate(void* ptr);
     
            bool IsPtrInBucketRange(void* ptr);


            #ifdef MERCURY_USE_MEMORY_STAT

            u64 totalCommittedMem = 0;

            //current allocations
            u64 allocationsCount = 0;
            u64 allocatedMemSystem = 0;
            u64 allocatedMemUser = 0;

            u64 totalAllocationsCount = 0;
            u64 totalAllocatedMemSystem = 0;
            u64 totalAllocatedMemUser = 0;

            u64 totalSystemPagesCommitted = 0;
            std::vector<u16> allocationUserSizes;
            u16 DeallocateAndReturnUserSize(void* ptr);

            void ResetFrame();
            void DumpStatsTotal();
            void DumpStateFrame();
            #endif
        };

        Bucket *buckets;

        //dedicated sizes for improve cache locality while finding correct bucket
        u16 *bucketSizes; 
        u8 numBuckets;

        ReservedAllocator(const InitDesc& desc);
        ~ReservedAllocator();

        void* Allocate(size_t size);
        void Deallocate(void* ptr);
        void* ReAllocate(void* ptr, u16 size);
       
        #ifdef MERCURY_USE_MEMORY_STAT

        //current allocations
        u64 allocationsCount = 0;
        u64 allocatedMemSystem = 0;
        u64 allocatedMemUser = 0;

        u64 totalAllocationsCount = 0;
        u64 totalAllocatedMemSystem = 0;
        u64 totalAllocatedMemUser = 0;

        u64 totalReallocsCount = 0;
        u64 totalReallocsSaveBySameBucket = 0;
        u64 totalSystemAllocationsCount = 0;

        u64 totalMallocAllocations = 0;
        #endif

        void ResetFrame();
        void DumpStatsTotal();
        void DumpStateFrame();

        void DumpStatsPerBucketTotal();
        void DumpStatePerBucketFrame();
    };

    extern ReservedAllocator* gGraphicsMemoryAllocator;
}
}