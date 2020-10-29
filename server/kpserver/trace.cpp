#include "trace.h"

#include <stdio.h>
#include <cstdarg>

#if !defined(NOTRACE)
	void trace_handler(const char *file, const char *func, int line, const char *msg, ...)
	{
		static char bf[4096] = { 0 };

		va_list args;
		va_start(args, msg);
		_vsnprintf_s(bf, 4095, msg, args);
		va_end(args);

		bf[4095] = 0;

        printf("%s\n", bf);
	}
#endif

#if !defined(NOASSERT)
	bool assert_handler(const char *file, const char *func, int line, const char *exp)
	{
		static char bf[1024];

		bf[1023] = 0;
		_snprintf_s(bf, 1023, "expr: %s\nfile: %s\nfunc:, %s\nline: %d", exp, file, func, line);
		
        printf("assert fail: %s\n", bf);

		return 1;
	}
#endif
