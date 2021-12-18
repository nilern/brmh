#include "bumparena.hpp"

namespace brmh {

BumpArena::BumpArena() : bumpers_(), current_() {}

void* BumpArena::alloc(std::size_t size) {
    void* allocation = current_.alloc(size);
    if (allocation) {
        return allocation;
    } else {
        bumpers_.push_back(std::move(current_));
        current_ = Bumper();
        return current_.alloc(size);
    }
}

BumpArena::Bumper::Bumper() {
    start = new char[SIZE];
    free = start + SIZE;
}

BumpArena::Bumper::Bumper(Bumper&& bumper) : start(bumper.start), free(bumper.free) {
    bumper.start = nullptr;
}

BumpArena::Bumper& BumpArena::Bumper::operator=(Bumper&& bumper) {
    start = bumper.start;
    free = bumper.free;
    bumper.start = nullptr;
    return *this;
}

BumpArena::Bumper::~Bumper() {
    delete[] start;
}

void* BumpArena::Bumper::alloc(std::size_t size) {
    std::size_t address = reinterpret_cast<std::size_t>(free);
    // FIXME: `usubl` not right for arbitrary arch:
    if (__builtin_usubl_overflow(address, size, &address)) { // Bump
        return nullptr; // Overflow due to huge `size`
    }
    address &= ~(alignof(max_align_t) - 1); // Round down for alignment

    char* ptr = reinterpret_cast<char*>(address);
    if (ptr < start) {
        return nullptr; // Did not fit
    } else {
        return free = ptr;
    }
}

} // namespace brmh
