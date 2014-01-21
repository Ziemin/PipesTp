#ifndef PIPE_EXCEPTION_HPP
#define PIPE_EXCEPTION_HPP

#include <exception>
#include <string>

template <typename Error>
class PipeException : std::exception {

    public:
        PipeException(std::string message)
            : message(std::move(message)) { }
        PipeException(std::string message, Error error) 
            : message(std::move(message)), _error(error) { }

    public:
        Error error() const {
            return _error;
        }

        virtual const char* what() const throw() {
            return message.c_str();
        }

    private:
        const std::string message;
        Error _error;
};

#endif
