#ifndef CASE_HANDLER_HPP
#define CASE_HANDLER_HPP

#include <functional>

struct CaseHandlerException : std::exception {

    CaseHandlerException() = default;

    CaseHandlerException(const char* message) : message(message)
    {

    }

    virtual const char* what() const noexcept {
        return message;
    }

    private:
        const char* message;

};

template<typename Res = void>
class CaseHandler {

    public:

        CaseHandler() : result(false) 
        { }

        template<typename Function, typename ...Args>
            CaseHandler(Function f, Args&&... args) : result(true)
            {
                fun = std::bind(f, std::forward<Args>(args)...);
            }

        CaseHandler(const CaseHandler &other) = delete;
        CaseHandler& operator=(const CaseHandler &other) = delete;

        CaseHandler(CaseHandler &&other) : result(other.result)
        {

            other.result = false;
            if(result) fun = std::move(other.fun);
        }

        CaseHandler& operator=(CaseHandler &&other) {

            result = other.result;
            other.result = false;
            if(result) fun = std::move(other.fun);

            return *this;
        }

        operator bool() const {
            return result;
        }

        Res operator()() const {

            if(!result) throw CaseHandlerException("Cannot handle result, because handler is false");
            return fun();
        }

    private:

        bool result;
        std::function<Res()> fun;

};

#endif
