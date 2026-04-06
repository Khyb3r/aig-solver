#ifndef ARENA_H
#define ARENA_H
#include <assert.h>
#include <cstdint>
#include <cstring>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#endif

template <typename T>
class Arena {
private:
    unsigned char* buffer;
    size_t buffer_length;
    size_t prev_offset;
    size_t curr_offset;


    static uintptr_t align_forward(uintptr_t pointer, size_t align) {
        assert((align & (align - 1)) == 0);
        uintptr_t mod = pointer & (align - 1);
        if (mod != 0) pointer += align - mod;
        return pointer;
    }

public:
    Arena(void* arena_total_buffer, size_t arena_total_buffer_size) : buffer(static_cast<unsigned char*> (arena_total_buffer)),
                                                                      buffer_length(arena_total_buffer_size), prev_offset(0),
                                                                      curr_offset(0) {}

    T* alloc() {
        T* ptr =  static_cast<T*>(allocate_align(sizeof(T), DEFAULT_ALIGNMENT));
        assert(ptr != nullptr);
        if (!ptr) {
            std::cerr << "Arena out of memory!\n";
            exit(1);
        }
        return new (ptr) T();
    }

    void* allocate_align(size_t size, size_t align) {
        uintptr_t curr = reinterpret_cast<uintptr_t>(this->buffer) + this->curr_offset;
        uintptr_t offset = align_forward(curr, align);
        offset -= reinterpret_cast<uintptr_t>(this->buffer);

        if (offset + size <= this->buffer_length) {
            void* pointer = &this->buffer[offset];
            this->prev_offset = offset;
            this->curr_offset = offset + size;
            memset(pointer, 0, size);
            return pointer;
        }
        return nullptr;
    }

    void free_all() {
        this->curr_offset = 0;
        this->prev_offset = 0;
    }
};

#endif //ARENA_H