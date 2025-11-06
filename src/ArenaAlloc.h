#pragma once 
#include <cstdlib>
#include <cstddef>





class ArenaAlloc {
public:
    inline explicit ArenaAlloc(size_t bytes) 
        : m_size(bytes) 
    {
        m_buffer = static_cast<std::byte*>(malloc(m_size));
        m_offset = m_buffer;
    }

    template<typename T>
    inline T* alloc() { // we won't care about alignment rn
        std::byte* offset = m_offset;
        
        if (offset + sizeof(T) > m_buffer + m_size) {
            std::cerr << "ArenaAlloc overflow\n"; 
            std::abort();
        }
        m_offset += sizeof(T);
        
        return ::new (offset) T{};
    }

    inline ArenaAlloc(const ArenaAlloc& other) = delete;

    inline ArenaAlloc operator=(const ArenaAlloc& other) = delete;

    ~ArenaAlloc() {
        free(m_buffer);
    }


private:
    size_t m_size;
    std::byte* m_buffer;
    std::byte* m_offset;

};