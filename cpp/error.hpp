#ifndef BRMH_ERROR_HPP
#define BRMH_ERROR_HPP

#include <exception>

namespace brmh {

class BrmhError : public std::exception {
public:
    virtual const char* what() const noexcept override;
};

} // namespace brmh

#endif // BRMH_ERROR_HPP
