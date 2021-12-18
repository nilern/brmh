#ifndef BRMH_BUMPARENA_HPP
#define BRMH_BUMPARENA_HPP

#include <cstddef>
#include <vector>

namespace brmh {

struct BumpArena {
    BumpArena();

    void* alloc(std::size_t size);

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
