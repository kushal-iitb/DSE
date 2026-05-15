#include "spsc_queue.hpp"

namespace DSE::spsc{
    bool DSE::spsc::SpscQueue::open(const char* shm_name){
        std::strncpy(this->shm_name , shm_name , sizeof(this->shm_name)-1);
        this->shm_name[sizeof(this->shm_name)-1] = '\0';
        
        int flags = O_CREAT | O_RDWR ;
        int prot = PROT_READ | PROT_WRITE; 

        fd = ::shm_open(shm_name , flags , 0644);
        if(fd == -1){
            DSE_LOG_ERROR(" failed to create shm file ");
            return false;
        }
        if(::ftruncate(fd , SHM_TOTAL_BYTES) == -1){
            DSE_LOG_ERROR(" erron on ftruncate function ");
            close();
            return false;
        }

        base = ::mmap(nullptr , SHM_TOTAL_BYTES , prot , MAP_SHARED , fd , 0);
        if(base == MAP_FAILED){
            DSE_LOG_ERROR( " mmap function failed ");
            base = nullptr;
            close();
            return false;
        }

        mapped_bytes = SHM_TOTAL_BYTES;

        ctrl = static_cast<ControlBlock*>(base);
        slots = reinterpret_cast<Slot*>(static_cast<char*>(base)+sizeof(ControlBlock));

        if(ctrl->magic != SHM_MAGIC){
            ctrl->capacity = QUEUE_CAPACITY;
            ctrl->slot_size = SLOT_SIZE;
            ctrl->version = 1;
            ctrl->write_pos.store(0 , std::memory_order_relaxed);
            ctrl->read_pos.store(0 , std::memory_order_relaxed);
            ctrl->magic = SHM_MAGIC;
        }
        return true;
    }

    void DSE::spsc::SpscQueue::close(){
        if(base){
            ::munmap(base , mapped_bytes);
            base = nullptr;
            mapped_bytes = 0;
        }
        if(fd != -1){
            ::close(fd);
            fd = -1;
        }    
        ctrl = nullptr;
        slots = nullptr;
    }

    bool DSE::spsc::SpscQueue::try_push(const void* msg , size_t bytes) noexcept {
        if (bytes == 0 || bytes > SLOT_SIZE)
        return false;

        const uint64_t w = ctrl->write_pos.load(std::memory_order_relaxed);
        const uint64_t r = ctrl->read_pos.load(std::memory_order_acquire);

        if(w-r>=QUEUE_CAPACITY)
        return false;

        Slot& slot = slots[w & QUEUE_MASK];
        std::memcpy(slot.data , msg , bytes);

        if(bytes<SLOT_SIZE)
        std::memset(slot.data+bytes , 0 , SLOT_SIZE - bytes);

        ctrl->write_pos.store(w+1 , std::memory_order_release);

        return true;


    }


}