#ifndef PIPE_CONNECTION_UTILS_HPP
#define PIPE_CONNECTION_UTILS_HPP

#include "pipe_exception.hpp"
#include "contact_list.hpp"
#include "simple_presence.hpp"

#include <TelepathyQt/DBusError>

inline void setContactListDbusError(const PipeException<ContactListError> &e, Tp::DBusError *dbusError) {
    switch(e.error()) {
        case ContactListError::NETWORK_ERROR:
            dbusError->set(TP_QT_ERROR_NETWORK_ERROR, e.what());
            break;
        case ContactListError::NOT_IMPLEMENTED:
            dbusError->set(TP_QT_ERROR_NOT_IMPLEMENTED, e.what());
            break;
        case ContactListError::UNDEFINED:
        case ContactListError::NOT_LOADED:
        case ContactListError::NOT_AVAILABLE:
            dbusError->set(TP_QT_ERROR_NOT_AVAILABLE, e.what());
            break;
        case ContactListError::SERVICE_BUSY:
            dbusError->set(TP_QT_ERROR_SERVICE_BUSY, e.what());
            break;
        case ContactListError::NOT_YET:
            dbusError->set(TP_QT_ERROR_NOT_YET, e.what());
            break;
        case ContactListError::INVALID_HANDLE:
            dbusError->set(TP_QT_ERROR_INVALID_HANDLE, e.what());
            break;
    }
}

inline void setSimplePresenceDbusError(const SimplePresenceException &e, Tp::DBusError *dbusError) {
    switch(e.error()) {
        case SimplePresenceError::UNDEFINED:
        case SimplePresenceError::CONTACT_LIST_REQUIRED:
        case SimplePresenceError::NOT_AVAILABLE:
            dbusError->set(TP_QT_ERROR_NOT_AVAILABLE, e.what());
            break;
        case SimplePresenceError::NETWORK_ERROR:
            dbusError->set(TP_QT_ERROR_NETWORK_ERROR, e.what());
            break;
        case SimplePresenceError::INVALID_ARGUMENT:
            dbusError->set(TP_QT_ERROR_INVALID_ARGUMENT, e.what());
            break;
    }
}

#endif
