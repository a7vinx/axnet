#include <string>
#include <cassert>

#include "util/buffer.hh"

void buffer_test() {
    axn::Buffer buf(256);
    // Initial state.
    assert(buf.ReadableSize() == 0);
    assert(buf.WritableSize() == 256);
    assert(buf.Retrieve(100).size() == 0);

    // Read and write.
    char s[256] = "test string";
    buf.Append(s, 5);
    assert(buf.ReadableSize() == 5);
    assert(buf.WritableSize() == 251);
    assert(buf.Retrieve(3) == "tes");
    assert(buf.ReadableSize() == 2);
    assert(buf.WritableSize() == 251);
    buf.Append("something");
    assert(buf.ReadableSize() == 11);
    assert(buf.WritableSize() == 242);
    assert(*(buf.ReadableBegin() + 3) == 'o');
    assert(*(buf.WritableBegin() - 1) == 'g');

    // Space management.
    buf.ReserveWritable(10);
    assert(buf.WritableSize() == 242);
    assert(buf.RetrieveAll() == "t something");
    buf.ReserveWritable(256);
    assert(buf.ReadableSize() == 0);
    assert(buf.WritableSize() == 256);
    buf.ShrinkToFit(128);
    assert(buf.ReadableSize() == 0);
    assert(buf.WritableSize() == 128);
    buf.ReserveWritable(512);
    assert(buf.ReadableSize() == 0);
    assert(buf.WritableSize() == 512);
}

int main() {
    buffer_test();
}
