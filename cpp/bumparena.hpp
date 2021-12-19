#ifndef BRMH_BUMPARENA_HPP
#define BRMH_BUMPARENA_HPP

#include <cstddef>
#include <vector>

namespace brmh {

struct BumpArena {
    BumpArena();

    template<class T>
    void* alloc() {
        void* allocation = current_.alloc(sizeof(T));
        if (allocation) {
            return allocation;
        } else {
            bumpers_.push_back(std::move(current_));
            current_ = Bumper();
            return current_.alloc(sizeof(T));
        }
    }

    template<class T>
    void* alloc_array(std::size_t count) {
        const std::size_t size = sizeof(T) * count;
        void* allocation = current_.alloc(size);
        if (allocation) {
            return allocation;
        } else {
            bumpers_.push_back(std::move(current_));
            current_ = Bumper();
            return current_.alloc(size);
        }
    }

private:
    struct Bumper {
        static const std::size_t SIZE = 1 << 20; // 1 MiB

        Bumper();
        ~Bumper();

        Bumper(Bumper&& bumper);
        Bumper& operator=(Bumper&& bumper);

        Bumper(const Bumper& bumper) = delete;
        Bumper& operator=(const Bumper& bumper) = delete;

        void* alloc(std::size_t size);

        char* start;
        char* free;
    };

    std::vector<Bumper> bumpers_;
    Bumper current_;
};

} // namespace brmh

#endif // BRMH_BUMPARENA_HPP
