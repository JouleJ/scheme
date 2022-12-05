#pragma once

#include <string>
#include <stdexcept>

struct SyntaxError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct RuntimeError : public std::runtime_error {
    RuntimeError(const std::string& name) : runtime_error(name) {
    }
};

struct NameError : public std::runtime_error {
    NameError(const std::string& name) : runtime_error(name) {
    }
};
