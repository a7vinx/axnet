#ifndef _AXN_BUFFER_HH_
#define _AXN_BUFFER_HH_

#include <string>
#include <vector>
#include <cstdlib>

namespace axn {

class Buffer {
public:
    Buffer(std::size_t size = 1024)
        : buf_(size), read_index_{0}, write_index_{0} {}

    // High-level reading interface.
    std::string Retrieve(std::size_t n);
    std::string RetrieveAll() { return Retrieve(ReadableSize()); }

    // High-level writing interface.
    void Append(const char* p, std::size_t n);
    void Append(const std::string& str) { Append(&str[0], str.size()); }

    // Low-level reading interface.
    const char* ReadableBegin() const { return BufBegin() + read_index_; }
    std::size_t ReadableSize() const { return write_index_ - read_index_; }
    void Read(std::size_t n) { read_index_ += n; }

    // Low-level writing interface.
    char* WritableBegin() { return BufBegin() + write_index_; }
    std::size_t WritableSize() const { return buf_.size() - write_index_; }
    void ReserveWritable(std::size_t n);
    void Written(std::size_t n) { write_index_ += n; }

    // Shrink.
    void ShrinkToFit(std::size_t reserve);

private:
    const char* BufBegin() const { return &*buf_.begin(); }
    char* BufBegin() { return &*buf_.begin(); }
    void MakeSpace(std::size_t n);

    std::vector<char> buf_;
    std::size_t read_index_;
    std::size_t write_index_;
};

}
#endif
