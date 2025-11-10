#pragma once 
#include <cstdlib>
#include <cstddef>
#include <vector>





class ArenaAlloc {
public:
    inline explicit ArenaAlloc(size_t bytes)  
    {
        size_t mb = 1024 * 1024;
        m_size = bytes < mb ? mb : bytes;

        m_buffer.reserve(8);
        m_buffer.push_back(static_cast<std::byte*>(malloc(m_size)));

        m_offset = m_buffer.back(); // m_buffer[0]
    }

    template<typename T>
    inline T* alloc() { // we won't care about alignment rn or an actual rope structure
        
        if (sizeof(T) > m_size) {
            std::cerr << "Too large struct for ArenaAlloc\n";
            std::abort();
        }

        size_t used = static_cast<size_t>(m_offset - m_buffer.back());
        if (used + sizeof(T) > m_size) {
            m_buffer.push_back(static_cast<std::byte*>(malloc(m_size)));
            m_offset = m_buffer.back();
        }
        std::byte* offset = m_offset;
        
        m_offset += sizeof(T);
        return ::new (offset) T{};
    }

    inline ArenaAlloc(const ArenaAlloc& other) = delete;

    inline ArenaAlloc operator=(const ArenaAlloc& other) = delete;

    ~ArenaAlloc() {
        for (auto& ptr : m_buffer) {
            free(ptr);
        }
    }


private:
    size_t m_size;
    std::vector<std::byte*> m_buffer;
    std::byte* m_offset;

};