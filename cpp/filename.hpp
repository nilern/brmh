#ifndef BRMH_FILENAME_HPP
#define BRMH_FILENAME_HPP


namespace brmh {

struct Filename {
    explicit Filename(const char* chars);
    ~Filename();

    const char* c_str() const;

private:
    const char* chars_;
};

} // namespace brmh

#endif // BRMH_FILENAME_HPP
