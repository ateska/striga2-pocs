#include "singlev.h"
#include <stdarg.h>

static FILE * _logging_stream = NULL;

///

int logging_init()
{
	_logging_stream = stderr;
	return 0;
}


void logging_finish()
{
	fflush(_logging_stream);
}

///

void LOG_DEBUG(const char *fmt, ...)
{
	char buffer[511];
	int rc;
	time_t timer;
	struct tm tm_info;

	time(&timer);
    localtime_r(&timer, &tm_info);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S ", &tm_info);

    strcpy(buffer+20, "DEBUG: ");

	va_list args;
	va_start(args, fmt);
	rc = vsnprintf(buffer+27, sizeof(buffer)-(27+1), fmt, args);
	va_end(args);

	buffer[rc+27] = '\n';
	buffer[rc+27+1] = '\0';

	fputs(buffer, _logging_stream ? _logging_stream : stderr);
}


void LOG_WARN(const char *fmt, ...)
{
	char buffer[511];
	int rc;
	time_t timer;
	struct tm tm_info;

	time(&timer);
    localtime_r(&timer, &tm_info);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S ", &tm_info);

    strcpy(buffer+20, " WARN: ");

	va_list args;
	va_start(args, fmt);
	rc = vsnprintf(buffer+27, sizeof(buffer)-(27+1), fmt, args);
	va_end(args);

	buffer[rc+27] = '\n';
	buffer[rc+27+1] = '\0';

	fputs(buffer, _logging_stream ? _logging_stream : stderr);
}


void LOG_ERROR(const char *fmt, ...)
{
	char buffer[511];
	int rc;
	time_t timer;
	struct tm tm_info;

	time(&timer);
    localtime_r(&timer, &tm_info);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S ", &tm_info);

    strcpy(buffer+20, "ERROR: ");

	va_list args;
	va_start(args, fmt);
	rc = vsnprintf(buffer+27, sizeof(buffer)-(27+1), fmt, args);
	va_end(args);

	buffer[rc+27] = '\n';
	buffer[rc+27+1] = '\0';

	fputs(buffer, _logging_stream ? _logging_stream : stderr);
}


void LOG_INFO(const char *fmt, ...)
{
	char buffer[511];
	int rc;
	time_t timer;
	struct tm tm_info;

	time(&timer);
    localtime_r(&timer, &tm_info);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S ", &tm_info);

    strcpy(buffer+20, " INFO: ");

	va_list args;
	va_start(args, fmt);
	rc = vsnprintf(buffer+27, sizeof(buffer)-(27+1), fmt, args);
	va_end(args);

	buffer[rc+27] = '\n';
	buffer[rc+27+1] = '\0';

	fputs(buffer, _logging_stream ? _logging_stream : stderr);
}

void LOG_ERRNO(int errnum, const char *fmt, ...)
{
	char buffer[1023];
	int rc;
	time_t timer;
	struct tm tm_info;

	time(&timer);
    localtime_r(&timer, &tm_info);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S ", &tm_info);

    strcpy(buffer+20, "ERROR: ");

	va_list args;
	va_start(args, fmt);
	rc = vsnprintf(buffer+27, sizeof(buffer)-(27+1), fmt, args);
	va_end(args);

	buffer[rc+27] = ':';
	buffer[rc+27+1] = ' ';

	strerror_r(errnum, &buffer[rc+27+2], sizeof(buffer)-(rc+27+3));
	strcat(buffer,"\n");

	fputs(buffer, _logging_stream ? _logging_stream : stderr);
}
