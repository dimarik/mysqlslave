#ifndef MYSQL_EXCEPTIONS_H_
#define MYSQL_EXCEPTIONS_H_

#include <stdexcept>
#include <stdio.h>
#include <stdarg.h>
#include <mysql.h>

#include "logevent.h"

#define MYSQL_ERRMSG_MAX_LEN 1024


namespace mysql 
{
	
class CException : public std::exception
{
public:
	CException(const char* fmt, ...)
	{
		va_list ap;
		va_start(ap,fmt);
		vsnprintf(_errmsg, MYSQL_ERRMSG_MAX_LEN, fmt, ap);
	}

	virtual ~CException() throw()
	{
	}
   
	virtual const char* what() const throw()
	{
		return _errmsg;
	}

protected:
	char _errmsg[MYSQL_ERRMSG_MAX_LEN];
};


class CLogEventException : public std::exception
{
public:
	CLogEventException(CLogEvent* event, const char* msg)
	{
		if (event)
		{
			snprintf(_errmsg, MYSQL_ERRMSG_MAX_LEN, "%s: %s", event->get_type_code_str(), msg);
		}
		else
		{
			strncpy(_errmsg, msg, MYSQL_ERRMSG_MAX_LEN);
		}
	}
	
	CLogEventException(int event_number, const char* msg) 
	{
		snprintf(_errmsg, MYSQL_ERRMSG_MAX_LEN, "event number %d: %s", event_number, msg);
	}
	
	virtual ~CLogEventException() throw()
	{
	}

	virtual const char* what() const throw()
	{
		return _errmsg;
	}

protected:
	char _errmsg[MYSQL_ERRMSG_MAX_LEN];
};

}


#endif

