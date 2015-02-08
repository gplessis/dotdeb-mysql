/*
   Copyright (c) 2003, 2010, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <ndb_global.h>

#include "Logger.hpp"

#include <LogHandler.hpp>
#include <ConsoleLogHandler.hpp>
#include <FileLogHandler.hpp>
#include "LogHandlerList.hpp"

#ifdef _WIN32
#include "EventLogHandler.hpp"
#else
#include <SysLogHandler.hpp>
#endif

const char* Logger::LoggerLevelNames[] = { "ON      ", 
					   "DEBUG   ",
					   "INFO    ",
					   "WARNING ",
					   "ERROR   ",
					   "CRITICAL",
					   "ALERT   ",
					   "ALL     "
					 };
Logger::Logger() : 
  m_pCategory("Logger"),
  m_pConsoleHandler(NULL),
  m_pFileHandler(NULL),
  m_pSyslogHandler(NULL)
{
  m_pHandlerList = new LogHandlerList();
  m_mutex= NdbMutex_Create();
  m_handler_mutex= NdbMutex_Create();
  disable(LL_ALL);
  enable(LL_ON);
  enable(LL_INFO);
}

Logger::~Logger()
{
  removeAllHandlers();
  delete m_pHandlerList;
  NdbMutex_Destroy(m_handler_mutex);
  NdbMutex_Destroy(m_mutex);
}

void 
Logger::setCategory(const char* pCategory)
{
  Guard g(m_mutex);
  m_pCategory = pCategory;
}

bool
Logger::createConsoleHandler(NdbOut &out)
{
  Guard g(m_handler_mutex);

  if (m_pConsoleHandler)
    return true; // Ok, already exist

  LogHandler* log_handler = new ConsoleLogHandler(out);
  if (!log_handler)
    return false;

  if (!addHandler(log_handler))
  {
    delete log_handler;
    return false;
  }

  m_pConsoleHandler = log_handler;
  return true;
}

void 
Logger::removeConsoleHandler()
{
  Guard g(m_handler_mutex);
  if (removeHandler(m_pConsoleHandler))
  {
    m_pConsoleHandler = NULL;
  }
}

bool
Logger::createEventLogHandler(const char* source_name)
{
#ifdef _WIN32
  Guard g(m_handler_mutex);

  LogHandler* log_handler = new EventLogHandler(source_name);
  if (!log_handler)
    return false;

  if (!addHandler(log_handler))
  {
    delete log_handler;
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool
Logger::createFileHandler(char*filename)
{
  Guard g(m_handler_mutex);

  if (m_pFileHandler)
    return true; // Ok, already exist

  LogHandler* log_handler = filename ? new FileLogHandler(filename)
                                     : new FileLogHandler();
  if (!log_handler)
    return false;

  if (!addHandler(log_handler))
  {
    delete log_handler;
    return false;
  }

  m_pFileHandler = log_handler;
  return true;
}

void 
Logger::removeFileHandler()
{
  Guard g(m_handler_mutex);
  if (removeHandler(m_pFileHandler))
  {
    m_pFileHandler = NULL;
  }
}

bool
Logger::createSyslogHandler()
{
#ifdef _WIN32
  return false;
#else
  Guard g(m_handler_mutex);

  if (m_pSyslogHandler)
    return true; // Ok, already exist

  LogHandler* log_handler = new SysLogHandler();
  if (!log_handler)
    return false;

  if (!addHandler(log_handler))
  {
    delete log_handler;
    return false;
  }

  m_pSyslogHandler = log_handler;
  return true;
#endif
}

void 
Logger::removeSyslogHandler()
{
  Guard g(m_handler_mutex);
  if (removeHandler(m_pSyslogHandler))
  {
    m_pSyslogHandler = NULL;
  }
}

bool
Logger::addHandler(LogHandler* pHandler)
{
  Guard g(m_mutex);
  assert(pHandler != NULL);

  if (!pHandler->is_open() &&
      !pHandler->open())
  {
    // Failed to open
    return false;
  }

  if (!m_pHandlerList->add(pHandler))
    return false;

  return true;
}

bool
Logger::addHandler(const BaseString &logstring, int *err, int len, char* errStr) {
  size_t i;
  Vector<BaseString> logdest;
  DBUG_ENTER("Logger::addHandler");

  logstring.split(logdest, ";");

  for(i = 0; i < logdest.size(); i++) {
    DBUG_PRINT("info",("adding: %s",logdest[i].c_str()));

    Vector<BaseString> v_type_args;
    logdest[i].split(v_type_args, ":", 2);

    BaseString type(v_type_args[0]);
    BaseString params;
    if(v_type_args.size() >= 2)
      params = v_type_args[1];

    LogHandler *handler = NULL;

#ifndef _WIN32
    if(type == "SYSLOG")
    {
      handler = new SysLogHandler();
    } else 
#endif
    if(type == "FILE")
      handler = new FileLogHandler();
    else if(type == "CONSOLE")
      handler = new ConsoleLogHandler();
    
    if(handler == NULL)
    {
      BaseString::snprintf(errStr,len,"Could not create log destination: %s",
                           logdest[i].c_str());
      DBUG_RETURN(false);
    }

    if(!handler->parseParams(params))
    {
      *err= handler->getErrorCode();
      if(handler->getErrorStr())
        strncpy(errStr, handler->getErrorStr(), len);
      delete handler;
      DBUG_RETURN(false);
    }

    if (!addHandler(handler))
    {
      BaseString::snprintf(errStr,len,"Could not add log destination: %s",
                           logdest[i].c_str());
      delete handler;
      DBUG_RETURN(false);
    }
  }

  DBUG_RETURN(true);
}

bool
Logger::removeHandler(LogHandler* pHandler)
{
  Guard g(m_mutex);
  int rc = false;
  if (pHandler != NULL)
  {
    if (pHandler == m_pConsoleHandler)
      m_pConsoleHandler= NULL;
    if (pHandler == m_pFileHandler)
      m_pFileHandler= NULL;
    if (pHandler == m_pSyslogHandler)
      m_pSyslogHandler= NULL;

    rc = m_pHandlerList->remove(pHandler);
  }

  return rc;
}

void
Logger::removeAllHandlers()
{
  Guard g(m_mutex);
  m_pHandlerList->removeAll();

  m_pConsoleHandler= NULL;
  m_pFileHandler= NULL;
  m_pSyslogHandler= NULL;
}

bool
Logger::isEnable(LoggerLevel logLevel) const
{
  Guard g(m_mutex);
  if (logLevel == LL_ALL)
  {
    for (unsigned i = 1; i < MAX_LOG_LEVELS; i++)
      if (!m_logLevels[i])
	return false;
    return true;
  }
  return m_logLevels[logLevel];
}

void
Logger::enable(LoggerLevel logLevel)
{
  Guard g(m_mutex);
  if (logLevel == LL_ALL)
  {
    for (unsigned i = 0; i < MAX_LOG_LEVELS; i++)
    {
      m_logLevels[i] = true;
    }
  }
  else 
  {
    m_logLevels[logLevel] = true;
  }
}

void 
Logger::enable(LoggerLevel fromLogLevel, LoggerLevel toLogLevel)
{
  Guard g(m_mutex);
  if (fromLogLevel > toLogLevel)
  {
    LoggerLevel tmp = toLogLevel;
    toLogLevel = fromLogLevel;
    fromLogLevel = tmp;
  }

  for (int i = fromLogLevel; i <= toLogLevel; i++)
  {
    m_logLevels[i] = true;
  } 
}

void
Logger::disable(LoggerLevel logLevel)
{
  Guard g(m_mutex);
  if (logLevel == LL_ALL)
  {
    for (unsigned i = 0; i < MAX_LOG_LEVELS; i++)
    {
      m_logLevels[i] = false;
    }
  }
  else
  {
    m_logLevels[logLevel] = false;
  }
}

void 
Logger::alert(const char* pMsg, ...) const
{
  va_list ap;
  va_start(ap, pMsg);
  log(LL_ALERT, pMsg, ap);
  va_end(ap);
}

void 
Logger::critical(const char* pMsg, ...) const
{
  va_list ap;
  va_start(ap, pMsg);
  log(LL_CRITICAL, pMsg, ap);  
  va_end(ap);
}
void 
Logger::error(const char* pMsg, ...) const
{
  va_list ap;
  va_start(ap, pMsg);
  log(LL_ERROR, pMsg, ap);  
  va_end(ap);
}
void 
Logger::warning(const char* pMsg, ...) const
{
  va_list ap;
  va_start(ap, pMsg);
  log(LL_WARNING, pMsg, ap);
  va_end(ap);
}

void 
Logger::info(const char* pMsg, ...) const
{
  va_list ap;
  va_start(ap, pMsg);
  log(LL_INFO, pMsg, ap);
  va_end(ap);
}

void 
Logger::debug(const char* pMsg, ...) const
{
  va_list ap;
  va_start(ap, pMsg);
  log(LL_DEBUG, pMsg, ap);
  va_end(ap);
}

void 
Logger::log(LoggerLevel logLevel, const char* pMsg, va_list ap) const
{
  Guard g(m_mutex);
  if (m_logLevels[LL_ON] && m_logLevels[logLevel])
  {
    char buf[MAX_LOG_MESSAGE_SIZE];
    BaseString::vsnprintf(buf, sizeof(buf), pMsg, ap);
    LogHandler* pHandler = NULL;
    while ( (pHandler = m_pHandlerList->next()) != NULL)
    {
      pHandler->append(m_pCategory, logLevel, buf);
    }
  }
}

void Logger::setRepeatFrequency(unsigned val)
{
  LogHandler* pHandler;
  while ((pHandler = m_pHandlerList->next()) != NULL)
  {
    pHandler->setRepeatFrequency(val);
  }
}
