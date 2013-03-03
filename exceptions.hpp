#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <cerrno>
#include <cstring>
#include <exception>
#include <string>
#include <SDL.h>
#include <GLES2/gl2.h>

class SDLException : public std::exception
{
public:
    SDLException(const std::string& fname)
        : fname(fname), reason(SDL_GetError()) {}

    ~SDLException() throw() {}

    const char* what() const throw()
    {
        return (fname + ": " + reason).c_str();
    }

private:
    std::string fname;
    std::string reason;
};

class GLException : public std::exception
{
public:
    GLException(const std::string& reason = "GL error")
        : reason(reason) {}

    ~GLException() throw() {}

    const char* what() const throw()
    {
        return reason.c_str();
    }

private:
    std::string reason;
};

class EGLException : public std::exception
{
public:
    EGLException(const std::string& reason)
        : reason(reason) {}

    ~EGLException() throw() {}

    const char* what() const throw()
    {
        return reason.c_str();
    }

private:
    std::string reason;
};

class SysException : public std::exception
{
public:
    SysException(const std::string& fname)
        : fname(fname), reason(strerror(errno)) {}

    ~SysException() throw() {}

    const char* what() const throw()
    {
        return (fname + ": " + reason).c_str();
    }

private:
    std::string fname;
    std::string reason;
};

#endif  // EXCEPTIONS_HPP
