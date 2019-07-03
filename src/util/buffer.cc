#include <algorithm>

#include "buffer.hh"

namespace axn {

void Buffer::Append(const char* p, std::size_t n) {
    if (WritableSize() < n)
        MakeSpace(n);
    std::copy(p, p + n, WritableBegin());
    write_index_ += n;
}

std::string Buffer::Retrieve(std::size_t n) {
    if (n > ReadableSize())
        return RetrieveAll();
    const char* retrieve_posp = ReadableBegin();
    read_index_ += n;
    return {retrieve_posp, n};
}

void Buffer::ReserveWritable(std::size_t n) {
    if (WritableSize() < n)
        MakeSpace(n);
}

void Buffer::ShrinkToFit(std::size_t reserve) {
    MakeSpace(0);
    buf_.resize(write_index_ + reserve);
}

void Buffer::MakeSpace(std::size_t n) {
    if (read_index_ + WritableSize() >= n) {
        std::copy(ReadableBegin(), (const char*)WritableBegin(), BufBegin());
        write_index_ -= read_index_;
        read_index_ = 0;
    } else {
        buf_.resize(write_index_ + n);
    }
}

}
