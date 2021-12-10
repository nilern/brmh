#ifndef BRMH_FILENAME_HPP
#define BRMH_FILENAME_HPP


namespace brmh {

struct Filename {
    Filename() = delete;
    ~Filename();

    static Filename create(const char* chars);

private:
    explicit Filename(const char* chars);

    char const* const chars_;
};

} // namespace brmh

#endif // BRMH_FILENAME_HPP
