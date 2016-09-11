/* 
 * File:   native_lib.h
 * Author: Ivan Deras (ideras@gmail.com)
 *
 * Created on September 7, 2016, 1:12 PM
 */

#ifndef ELIB_H
#define ELIB_H
#include <string>

using namespace std;

#ifdef _WIN32
#include <windows.h>
typedef HINSTANCE HLIB;
typedef void* HFUNC;
#elif __linux__
#include <dlfcn.h>
typedef void* HLIB;
typedef void* HFUNC;
#else
#   error "Unknown compiler"
#endif

HLIB openLibrary(const char *lib_name);
HFUNC getFunctionAddr(HLIB lhandle, const char *func_name);
void closeLibrary(HLIB lhandle);
string getLibFullName(string lib_name);

#endif /* ELIB_H */

