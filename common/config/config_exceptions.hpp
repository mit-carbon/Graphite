//Exceptions that the library might throw in case of
//error conditions as noted.
#ifndef CONFIG_EXCEPTIONS_HPP
#define CONFIG_EXCEPTIONS_HPP
// Config Class
// Author: Charles Gruenwald III
#include <iostream>
#include <exception>

using namespace std;
namespace config
{

//This error gets thrown then the parse is unsuccessful
class parserError: public exception
{
    public:
    parserError(const std::string & leftover)
        :
            m_leftover("parser Error: " + leftover)
    {
    }

    virtual ~parserError() throw() {}

    virtual const char* what() const throw()
    {
        return m_leftover.c_str();
    }

    private:
        std::string m_leftover;

};

//This error gets thrown when a key is not found
class KeyNotFound: public exception
{
    virtual const char* what() const throw()
    {
        return "Key not found.";
    }
};

//This error gets thrown when a key is not found
class FileNotFound: public exception
{
public:
    virtual const char* what() const throw()
    {
        return "File not found.";
    }
};


//This error gets thrown then the parse is unsuccessful
class SaveError: public exception
{
    public:
    SaveError(const std::string & error_str)
        :
            m_error("Save Error: " + error_str)
    {
    }

    virtual ~SaveError() throw() {}

    virtual const char* what() const throw()
    {
        return m_error.c_str();
    }

    private:
        std::string m_error;

};

}
#endif
