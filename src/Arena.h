#ifndef ARENA_H
#define ARENA_H
#include <assert.h>
#include <cstdint>
#include <cstring>

#ifndef DEFAULT_ALIGNMENT
// Align memory on 16 byte boundaries (64-bit systems) or 8 byte boundaries (32-bit systems)
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#endif

template <typename T>
class Arena {
private:
    // Pointer to block of memory the arena manages
    unsigned char* buffer;
    // Size of the block
    size_t buffer_length;
    // Last allocation start pointer within arena
    size_t prev_offset;
    // Current pointer within the arena
    size_t curr_offset;

    // Aligns the passed in pointer to the next valid boundary
    static uintptr_t align_forward(uintptr_t pointer, size_t align) {
        // Check align is a power of 2 (bit flip everything as a mask and then AND with original to get 0)
        assert((align & (align - 1)) == 0);
        // Checks how many bits remaining to align
        uintptr_t mod = pointer & (align - 1);
        // If there is a remainder, round up to the next valid boundary
        if (mod != 0) pointer += align - mod;
        return pointer;
    }

public:
    Arena(void* arena_total_buffer, size_t arena_total_buffer_size) : buffer(static_cast<unsigned char*> (arena_total_buffer)),
                                                                      buffer_length(arena_total_buffer_size), prev_offset(0),
                                                                      curr_offset(0) {}

    // Used on each allocation to linearly allocate within the arena
    T* alloc() {
        // Pointer to allocated object
        T* ptr =  static_cast<T*>(allocate_align(sizeof(T), DEFAULT_ALIGNMENT));
        assert(ptr != nullptr);
        if (!ptr) {
            std::cerr << "Arena out of memory!\n";
            exit(1);
        }
        // Construct the object at that address
        return new (ptr) T();
    }

    void* allocate_align(size_t size, size_t align) {
        // Pointer to current area within the arena
        uintptr_t curr = reinterpret_cast<uintptr_t>(this->buffer) + this->curr_offset;
        // Round up to the correct address if necessary
        uintptr_t offset = align_forward(curr, align);
        // How far the offset is within the buffer if it is non 0
        offset -= reinterpret_cast<uintptr_t>(this->buffer);

        // Ensure arena won't be full from this new allocation
        if (offset + size <= this->buffer_length) {
            // Next allocation starts here
            void* pointer = &this->buffer[offset];
            // Update offsets
            this->prev_offset = offset;
            this->curr_offset = offset + size;
            // Zero memory for the object being allocated
            memset(pointer, 0, size);
            return pointer;
        }
        // Arena becomes full (alloc() will fail)
        return nullptr;
    }

    // Resets offsets
    void free_all() {
        this->curr_offset = 0;
        this->prev_offset = 0;
    }
};

#endif //ARENA_H