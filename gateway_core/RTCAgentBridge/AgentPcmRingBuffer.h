#ifndef __AGENT_PCM_RING_BUFFER_H__
#define __AGENT_PCM_RING_BUFFER_H__

#include <mutex>
#include <vector>

class AgentPcmRingBuffer {
public:
    explicit AgentPcmRingBuffer(size_t capacity);
    ~AgentPcmRingBuffer();
    
public:
    size_t Capacity() const;
    size_t Size() const;
    size_t FreeSpace() const;

    bool Write(const int16_t* data, size_t n);
    bool ReadExact(int16_t* out, size_t n);
    size_t Read(int16_t* out, size_t n);
    void Clear();

private:
    void _copy_out_l(int16_t* out, size_t n);
    
private:
    mutable std::mutex _mutex;
    std::vector<int16_t> _buf;
    size_t _head = 0;
    size_t _size = 0;

private:
	AgentPcmRingBuffer(const AgentPcmRingBuffer&) = delete;
	AgentPcmRingBuffer& operator=(const AgentPcmRingBuffer&) = delete;    
};

#endif // __AGENT_PCM_RING_BUFFER_H__