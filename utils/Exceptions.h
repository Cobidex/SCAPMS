#pragma once

#include <stdexcept>
#include <string>

namespace scapms::utils {

// Bad name, age, contact, date, time or search input.
class ValidationException : public std::runtime_error {
public:
    explicit ValidationException(const std::string& msg) : std::runtime_error(msg) {}
};

// An ID that is already taken.
class DuplicateIdException : public std::runtime_error {
public:
    explicit DuplicateIdException(const std::string& msg) : std::runtime_error(msg) {}
};

// A file that cannot be opened or written, or a malformed line.
class FileException : public std::runtime_error {
public:
    explicit FileException(const std::string& msg) : std::runtime_error(msg) {}
};

// A patient or appointment ID that does not exist.
class NotFoundException : public std::runtime_error {
public:
    explicit NotFoundException(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace scapms::utils
