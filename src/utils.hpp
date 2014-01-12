#ifndef PIPE_UTILS_HPP
#define PIPE_UTILS_HPP

#include <memory>
#include <QtDebug>

template <typename Function>
struct ScopeExit {

    ScopeExit(Function f) : f(f) { }
    ~ScopeExit() { f(); }

    private:

        Function f;
};

template <typename Function>
ScopeExit<Function> makeScopeExit(Function f) {
    return ScopeExit<Function>(f);
}

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SCOPE_EXIT(code) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = makeScopeExit([&](){code;})


template <typename T> 
struct MoveOnCopy {

    mutable T value;
    
    MoveOnCopy(T &&value) : value(std::move(value)) { }
    MoveOnCopy(const MoveOnCopy &other) : value(std::move(other.value)) { }
    MoveOnCopy(MoveOnCopy &&other) : value(std::move(other.value)) { }
    MoveOnCopy& operator=(const MoveOnCopy& other) {
        value = std::move(other.value);
        return *this;
    }
    MoveOnCopy& operator=(MoveOnCopy &&other) {
        value = std::move(other.value);
        return *this;
    }
};

inline QDebug pDebug() {
    return qDebug() << "PIPE DEBUG: ";
}

inline QDebug pWarning() {
    return qWarning() << "PIPE WARNING: ";
}

inline QDebug pCritical() {
    return qCritical() << "PIPE CRITICAL: ";
}

#endif
