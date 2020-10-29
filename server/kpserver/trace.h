#pragma once

#include <stdio.h>


#if !defined(NOASSERT)
bool assert_handler(const char*, const char*, int, const char*);
#define ASSERT(exp) { if(!(exp) && assert_handler(__FILE__, __FUNCTION__, __LINE__, #exp)); }
#else
#define ASSERT(exp) { }
#endif

#if !defined(NOTRACE)
void trace_handler(const char*, const char*, int, const char*, ...);
#ifdef WIN32
#define TRACE(msg, ...) { trace_handler(__FILE__, __FUNCTION__, __LINE__, msg, __VA_ARGS__); }
#else //GCC
#define TRACE(msg, ...) { trace_handler(__FILE__, __FUNCTION__, __LINE__, msg, ##__VA_ARGS__); }
#endif
#else
#define TRACE(msg, ...) { }
#endif

#if !defined(NOVERIFY)
#define VERIFY(exp, msg, ...) { if (!(exp)) TRACE(msg, __VA_ARGS__); }
#else
#define VERIFY(exp, msg, ...) { }
#endif

