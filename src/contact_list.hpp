#ifndef PIPE_CONTACT_LIST_HPP
#define PIPE_CONTACT_LIST_HPP

#include <QObject>

class PipeContactList;

class LoadContactListException : std::exception {
    friend class PipeContactList;
    
    protected:
        LoadContactListException(std::string message);

    public:
        virtual const char* what() const throw();
};

class PipeContactList {

    public:
        bool isLoaded();
        void loadContactList();

    private:
        bool loaded = false;

};

#endif
