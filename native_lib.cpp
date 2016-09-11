#include "native_lib.h"

#ifdef _WIN32

HLIB openLibrary(const char *lib_name)
{
    return LoadLibrary(lib_name);
}

HFUNC getFunctionAddr(HLIB lhandle, const char *func_name)
{
    return GetProcAddress(lhandle, func_name);
}

void closeLibrary(HLIB lhandle)
{
    FreeLibrary(lhandle);
}

string getLibFullName(string lib_name)
{
	return lib_name + ".dll";
}

#elif __linux__

HLIB openLibrary(const char *lib_name)
{
    return dlopen(lib_name, RTLD_LAZY);               
}

HFUNC getFunctionAddr(HLIB lhandle, const char *func_name)
{
    return dlsym(lhandle, func_name);
}

void closeLibrary(HLIB lhandle)
{
    dlclose(lhandle);
}

string getLibFullName(string lib_name)
{
	if (lib_name.compare("libc") == 0)
		lib_name += ".so.6";
	else
		lib_name += ".so";

	return lib_name;
}

#endif
