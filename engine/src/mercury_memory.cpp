#include "mercury_memory.h"
#include "mercury_utils.h"
#include "ll/os.h"
#include <iostream>
#include <cstring>

using namespace mercury;
using namespace mercury::memory;

ReservedAllocator::ReservedAllocator(const ReservedAllocator::InitDesc &desc)
{
    numBuckets = (u8)desc.bucketsInfo.size();
    buckets = (Bucket *)malloc(sizeof(Bucket) * numBuckets);
    bucketSizes = (u16 *)malloc(sizeof(u16) * numBuckets);
    for (size_t i = 0; i < numBuckets; ++i)
    {
        new (&buckets[i]) Bucket(desc.bucketsInfo[i].elementSize, desc.bucketsInfo[i].maximumReservedSize, desc.bucketsInfo[i].initialCommittedSize);
        bucketSizes[i] = desc.bucketsInfo[i].elementSize;
    }
}

ReservedAllocator::~ReservedAllocator()
{
    for (int i = 0; i < numBuckets; ++i)
    {
        buckets[i].~Bucket();
    }
    free(buckets);
    free(bucketSizes);
}

void *ReservedAllocator::Allocate(size_t size)
{
    totalAllocationsCount++;
    totalAllocatedMemUser += size;

    int bucketIndex = -1;

    for (size_t i = 0; i < numBuckets; ++i)
    {
        if (bucketSizes[i] >= size)
        {
            bucketIndex = i;
            break;
        }
    }

    if (bucketIndex != -1)
    {
        totalAllocatedMemSystem += buckets[bucketIndex].elementSize;
        return buckets[bucketIndex].Allocate(size);
    }

    totalAllocatedMemSystem += size;
    totalMallocAllocations++;
    return malloc(size);
}

void *ReservedAllocator::ReAllocate(void *ptr, u16 size)
{
    int bucketIndexNew = -1;
    int bucketIndexOld = -1;

    for (size_t i = 0; i < numBuckets; ++i)
    {
        if (buckets[i].IsPtrInBucketRange(ptr))
        {
            bucketIndexOld = i;
        }

        if (bucketSizes[i] >= size)
        {
            bucketIndexNew = i;
            break;
        }
    }

    if (bucketIndexNew <= bucketIndexOld) // do nothing
        return ptr;
 
    if (bucketIndexNew == -1 && bucketIndexOld == -1) // reallocate to system memory
    {
        return realloc(ptr, size);
    }

    void *newPtr = Allocate(size);
    memcpy(newPtr, ptr, size);
    Deallocate(ptr);

    return newPtr;
}

void ReservedAllocator::Deallocate(void *ptr)
{
    if (ptr == nullptr)
        return;

    for (size_t i = 0; i < numBuckets; ++i)
    {
        if (buckets[i].IsPtrInBucketRange(ptr))
        {
#ifdef MERCURY_USE_MEMORY_STAT
            buckets[i].DeallocateAndReturnUserSize(ptr);
#else
            buckets[i].Deallocate(ptr);
#endif

            return;
        }
    }

    free(ptr);
}

ReservedAllocator::Bucket::Bucket(u16 elementSize, u32 maximumReservedSize, u32 initialCommitedSize)
{
    auto *os = ll::os::gOS;
    this->elementSize = elementSize; 
    this->maximumReservedSize = maximumReservedSize;

    beginRegion = os->ReserveMemory(maximumReservedSize);
    os->CommitMemory(beginRegion, initialCommitedSize);

    commitedElements = (u32)(initialCommitedSize / elementSize);
    maximumReservedElements = (u32)(maximumReservedSize / elementSize);
 
#ifdef MERCURY_USE_MEMORY_STAT
    totalCommittedMem = initialCommitedSize;
    allocationUserSizes.reserve(std::min(4096u, initialCommitedSize / elementSize));
#endif
}

ReservedAllocator::Bucket::~Bucket()
{
    auto *os = ll::os::gOS;
    os->DecommitMemory(beginRegion, commitedElements * elementSize);
    os->ReleaseMemory(beginRegion, maximumReservedSize);
}

void *ReservedAllocator::Bucket::Allocate(u16 size)
{
#ifdef MERCURY_USE_MEMORY_STAT
    allocationsCount++;
    allocatedMemUser += size;
    allocatedMemSystem += elementSize;

    totalAllocationsCount++;
    totalAllocatedMemUser += size;
    totalAllocatedMemSystem += elementSize;
#endif

    if (freeList.empty())
    {
        if (lastAllocatedID >= maximumReservedElements)
            return nullptr;

        if (lastAllocatedID >= commitedElements)
        {
            // reserve next page
            auto *os = ll::os::gOS;

            int numCommitedPages = mercury::utils::math::alignUp(256 * elementSize, os->GetPageSize());

            os->CommitMemory((u8 *)beginRegion + (commitedElements * elementSize), numCommitedPages * os->GetPageSize());
            commitedElements += (u32)(numCommitedPages / elementSize);

            
#ifdef MERCURY_USE_MEMORY_STAT
            totalSystemPagesCommitted += numCommitedPages;
            totalCommittedMem += os->GetPageSize() * numCommitedPages;
#endif
        }

        void *result = (u8 *)beginRegion + (lastAllocatedID * elementSize);

#ifdef MERCURY_USE_MEMORY_STAT
        allocationUserSizes.push_back(size);
#endif

        lastAllocatedID++;

        return result;
    }
    else
    {
        u32 currentId = freeList.top();
        freeList.pop();

        void *result = (u8 *)beginRegion + (currentId * elementSize);

#ifdef MERCURY_USE_MEMORY_STAT
        allocationUserSizes[currentId] = size;
#endif

        return result;
    }
    return nullptr;
}

void ReservedAllocator::Bucket::Deallocate(void *ptr)
{
    if (ptr == nullptr)
        return;

    u32 offset = (u32)((u8 *)ptr - (u8 *)beginRegion);
    MERCURY_ASSERT(offset % elementSize == 0);

    u32 id = offset / elementSize;
    MERCURY_ASSERT(id < maximumReservedElements);

    freeList.push(id);

#ifdef MERCURY_USE_MEMORY_STAT
    allocationUserSizes[id] = 0;
#endif
}

#ifdef MERCURY_USE_MEMORY_STAT
u16 ReservedAllocator::Bucket::DeallocateAndReturnUserSize(void *ptr)
{
    if (ptr == nullptr)
        return 0;

    u32 offset = (u32)((u8 *)ptr - (u8 *)beginRegion);
    MERCURY_ASSERT(offset % elementSize == 0);

    u32 id = offset / elementSize;
    MERCURY_ASSERT(id < maximumReservedElements);

    u16 result = allocationUserSizes[id];

#ifdef MERCURY_USE_MEMORY_STAT
    allocationsCount--;
    allocatedMemUser -= result;
    allocatedMemSystem -= elementSize;
#endif

    allocationUserSizes[id] = 0;
    return result;
}
#endif

bool ReservedAllocator::Bucket::IsPtrInBucketRange(void *ptr)
{
    if (ptr == nullptr)
        return false;

    u32 offset = (u32)((u8 *)ptr - (u8 *)beginRegion);
    return offset >= 0 && offset < maximumReservedSize;
}

void ReservedAllocator::DumpStatsPerBucketTotal()
{
    if(!this)
        return;
        
#ifdef MERCURY_USE_MEMORY_STAT
    std::cout << "Total Allocations: " << totalAllocationsCount << std::endl;
    std::cout << "Total Mallocs: " << totalMallocAllocations << std::endl;
    std::cout << "Total Allocated Memory (User): " << mercury::utils::string::format_size(totalAllocatedMemUser) << std::endl;
    std::cout << "Total Allocated Memory (System): " << mercury::utils::string::format_size(totalAllocatedMemSystem) << std::endl;

    for (size_t i = 0; i < numBuckets; ++i)
    {
        std::cout << "Bucket " << buckets[i].elementSize << std::endl;
        buckets[i].DumpStatsTotal();
    }
#else
    std::cout << "Memory statistics are not available. Define MERCURY_USE_MEMORY_STAT" << std::endl;
#endif
}

#ifdef MERCURY_USE_MEMORY_STAT

void ReservedAllocator::Bucket::DumpStatsTotal()
{
    std::cout << "  Total Reserved Virtual Memory: " << mercury::utils::string::format_size(maximumReservedSize) << std::endl;
    std::cout << "  Total Committed Memory: " << mercury::utils::string::format_size(totalCommittedMem) << std::endl;
    std::cout << "  Total Committed Pages in Runtime: " << totalSystemPagesCommitted << std::endl;
    std::cout << std::endl;
    std::cout << "  Allocated Elements: " << allocationsCount << std::endl;
    std::cout << "  Allocated Memory (User): " << mercury::utils::string::format_size(allocatedMemUser) << std::endl;
    std::cout << "  Allocated Memory (System): " << mercury::utils::string::format_size(allocatedMemSystem) << std::endl;
    std::cout << std::endl;
    std::cout << "  Total allocations count: " << totalAllocationsCount << std::endl;
    std::cout << "  Total Allocated Memory (User): " << mercury::utils::string::format_size(totalAllocatedMemUser) << std::endl;
    std::cout << "  Total Allocated Memory (System): " << mercury::utils::string::format_size(totalAllocatedMemSystem) << std::endl;
}

#endif