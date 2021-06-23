/* Copyright (C) 2004-2006 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "messages.h"

#include <my_global.h>
#include <mysql_com.h>

#include "mysqld_error.h"
#include "mysql_manager_error.h"


static const char *mysqld_error_message(unsigned sql_errno)
{
  switch (sql_errno) {
  case ER_HANDSHAKE_ERROR:
    return "Bad handshake";
  case ER_OUT_OF_RESOURCES:
    return "Out of memory;  Check if mysqld or some other process"
           " uses all available memory. If not you may have to use"
           " 'ulimit' to allow mysqld to use more memory or you can"
           " add more swap space";
  case ER_ACCESS_DENIED_ERROR:
    return "Access denied. Bad username/password pair";
  case ER_NOT_SUPPORTED_AUTH_MODE:
    return "Client does not support authentication protocol requested by"
           " server; consider upgrading MySQL client";
  case ER_UNKNOWN_COM_ERROR:
    return "Unknown command";
  case ER_SYNTAX_ERROR:
    return "You have an error in your command syntax. Check the manual that"
           " corresponds to your MySQL Instance Manager version for the right"
           " syntax to use";
  case ER_BAD_INSTANCE_NAME:
    return "Unknown instance name";
  case ER_INSTANCE_IS_NOT_STARTED:
    return "Cannot stop instance. Perhaps the instance is not started, or was"
           " started manually, so IM cannot find the pidfile.";
  case ER_INSTANCE_ALREADY_STARTED:
    return "The instance is already started";
  case ER_CANNOT_START_INSTANCE:
    return "Cannot start instance. Possible reasons are wrong instance options"
           " or resources shortage";
  case ER_OFFSET_ERROR:
    return "Cannot read negative number of bytes";
  case ER_STOP_INSTANCE:
    return "Cannot stop instance";
  case ER_READ_FILE:
    return "Cannot read requested part of the logfile";
  case ER_NO_SUCH_LOG:
    return "The instance has no such log enabled";
  case ER_OPEN_LOGFILE:
    return "Cannot open log file";
  case ER_GUESS_LOGFILE:
    return "Cannot guess the log filename. Try specifying full log name"
           " in the instance options";
  case ER_ACCESS_OPTION_FILE:
    return "Cannot open the option file to edit. Check permissions";
  case ER_DROP_ACTIVE_INSTANCE:
    return "Cannot drop an active instance. You should stop it first";
  case ER_CREATE_EXISTING_INSTANCE:
    return "Instance already exists";
  case ER_INSTANCE_MISCONFIGURED:
    return "Instance is misconfigured. Cannot start it";
  case ER_MALFORMED_INSTANCE_NAME:
    return "Malformed instance name.";
  case ER_INSTANCE_IS_ACTIVE:
    return "The instance is active. Stop the instance first";
  case ER_THERE_IS_ACTIVE_INSTACE:
    return "At least one instance is active. Stop all instances first";
  case ER_INCOMPATIBLE_OPTION:
    return "Instance Manager-specific options are prohibited from being used "
           "in the configuration of mysqld-compatible instances";
  case ER_CONF_FILE_DOES_NOT_EXIST:
    return "Configuration file does not exist";
  default:
    DBUG_ASSERT(0);
    return 0;
  }
}


const char *message(unsigned sql_errno)
{
  return mysqld_error_message(sql_errno);
}


const char *errno_to_sqlstate(unsigned sql_errno)
{
  return mysql_errno_to_sqlstate(sql_errno);
}
