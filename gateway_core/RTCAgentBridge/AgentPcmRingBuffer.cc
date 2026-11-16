#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "AgentPcmRingBuffer"
#include <utils/Log.h>

#include <algorithm>
#include <cstring>

#include "AgentPcmRingBuffer.h"

AgentPcmRingBuffer::AgentPcmRingBuffer(size_t capacity)
 : _buf(capacity) {

}

AgentPcmRingBuffer::~AgentPcmRingBuffer() {
    Clear();
}

size_t AgentPcmRingBuffer::Capacity() const {
    return _buf.size();
}

size_t AgentPcmRingBuffer::Size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _size;
}

size_t AgentPcmRingBuffer::FreeSpace() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _buf.size() - _size;    
}

bool AgentPcmRingBuffer::Write(const int16_t* data, size_t n) {
    if(n == 0) {
        return true;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    const size_t cap  = _buf.size();
    if(data == nullptr || n > cap - _size) {
        ALOGE("not enough space");
        return false;
    }

    const size_t tail = (_head + _size) % cap; 
    const size_t cp_size = std::min(n, cap - tail);
    std::memcpy(&_buf[tail], data, cp_size * sizeof(int16_t));
    if(n > cp_size) {
        std::memcpy(&_buf[0], data + cp_size, (n - cp_size) * sizeof(int16_t));
    }
    _size += n;
    return true;
}

bool AgentPcmRingBuffer::ReadExact(int16_t* out, size_t n) {
    if(n == 0) {
        return true;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if(out == nullptr || n > _size) {
        return false;
    }

    _copy_out_l(out, n);

    return true;
}

size_t AgentPcmRingBuffer::Read(int16_t* out, size_t n) {
    std::lock_guard<std::mutex> lock(_mutex);
    const size_t take = std::min(n, _size);
    if(take > 0 && out != nullptr) {
        _copy_out_l(out, take);
    }

    return take;
}

void AgentPcmRingBuffer::Clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    ALOGV(">>>>>>>> Clear size %d", _size);
    _head = 0;
    _size = 0;
}

void AgentPcmRingBuffer::_copy_out_l(int16_t* out, size_t n) {
    const size_t cap = _buf.size();
    const size_t cp_size = std::min(n, cap - _head);
    std::memcpy(out, &_buf[_head], cp_size * sizeof(int16_t));
    if(n > cp_size) {
        std::memcpy(out + cp_size, &_buf[0], (n - cp_size) * sizeof(int16_t));
    }

    _head = (_head + n) % cap;
    _size -= n;
}