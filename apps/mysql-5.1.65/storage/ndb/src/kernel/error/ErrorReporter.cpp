/* Copyright (C) 2003 MySQL AB

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


#include <ndb_global.h>

#include <ndbd_exit_codes.h>
#include "ErrorReporter.hpp"

#include <FastScheduler.hpp>
#include <DebuggerNames.hpp>
#include <NdbHost.h>
#include <NdbConfig.h>
#include <Configuration.hpp>
#include "EventLogger.hpp"

#include <NdbAutoPtr.hpp>

#define MESSAGE_LENGTH 500

static int WriteMessage(int thrdMessageID,
			const char* thrdProblemData, 
			const char* thrdObjRef,
			Uint32 thrdTheEmulatedJamIndex,
			Uint8 thrdTheEmulatedJam[]);

static void dumpJam(FILE* jamStream, 
		    Uint32 thrdTheEmulatedJamIndex, 
		    Uint8 thrdTheEmulatedJam[]);

extern EventLogger g_eventLogger;
const char*
ErrorReporter::formatTimeStampString(){
  TimeModule DateTime;          /* To create "theDateTimeString" */
  
  static char theDateTimeString[39]; 
  /* Used to store the generated timestamp */
  /* ex: "Wednesday 18 September 2000 - 18:54:37" */

  DateTime.setTimeStamp();
  
  BaseString::snprintf(theDateTimeString, 39, "%s %d %s %d - %s:%s:%s", 
	   DateTime.getDayName(), DateTime.getDayOfMonth(),
	   DateTime.getMonthName(), DateTime.getYear(), DateTime.getHour(),
	   DateTime.getMinute(), DateTime.getSecond());
  
  return (const char *)&theDateTimeString;
}

int
ErrorReporter::get_trace_no(){
  
  FILE *stream;
  unsigned int traceFileNo;
  
  char *file_name= NdbConfig_NextTraceFileName(globalData.ownId);
  NdbAutoPtr<char> tmp_aptr(file_name);

  /* 
   * Read last number from tracefile
   */  
  stream = fopen(file_name, "r+");
  if (stream == NULL){
    traceFileNo = 1;
  } else {
    char buf[255];
    fgets(buf, 255, stream);
    const int scan = sscanf(buf, "%u", &traceFileNo);
    if(scan != 1){
      traceFileNo = 1;
    }
    fclose(stream);
    traceFileNo++;
  }

  /**
   * Wrap tracefile no 
   */
  Uint32 tmp = globalEmulatorData.theConfiguration->maxNoOfErrorLogs();
  if (traceFileNo > tmp ) {
    traceFileNo = 1;
  }

  /**
   *  Save new number to the file
   */
  stream = fopen(file_name, "w");
  if(stream != NULL){
    fprintf(stream, "%u", traceFileNo);
    fclose(stream);
  }

  return traceFileNo;
}


void
ErrorReporter::formatMessage(int faultID,
			     const char* problemData, 
			     const char* objRef,
			     const char* theNameOfTheTraceFile,
			     char* messptr){
  int processId;
  ndbd_exit_classification cl;
  ndbd_exit_status st;
  const char *exit_msg = ndbd_exit_message(faultID, &cl);
  const char *exit_cl_msg = ndbd_exit_classification_message(cl, &st);
  const char *exit_st_msg = ndbd_exit_status_message(st);

  processId = NdbHost_GetProcessId();
  
  BaseString::snprintf(messptr, MESSAGE_LENGTH,
	   "Time: %s\n"
           "Status: %s\n"
	   "Message: %s (%s)\n"
	   "Error: %d\n"
           "Error data: %s\n"
	   "Error object: %s\n"
           "Program: %s\n"
	   "Pid: %d\n"
           "Trace: %s\n"
           "Version: %s\n"
           "***EOM***\n", 
	   formatTimeStampString() , 
           exit_st_msg,
	   exit_msg, exit_cl_msg,
	   faultID, 
	   (problemData == NULL) ? "" : problemData, 
	   objRef, 
	   my_progname, 
	   processId, 
	   theNameOfTheTraceFile ? theNameOfTheTraceFile : "<no tracefile>",
		       NDB_VERSION_STRING);

  // Add trailing blanks to get a fixed lenght of the message
  while (strlen(messptr) <= MESSAGE_LENGTH-3){
    strcat(messptr, " ");
  }
  
  strcat(messptr, "\n");
  
  return;
}

NdbShutdownType ErrorReporter::s_errorHandlerShutdownType = NST_ErrorHandler;

void
ErrorReporter::setErrorHandlerShutdownType(NdbShutdownType nst)
{
  s_errorHandlerShutdownType = nst;
}

void childReportError(int error);

void
ErrorReporter::handleAssert(const char* message, const char* file, int line, int ec)
{
  char refMessage[100];

#ifdef NO_EMULATED_JAM
  BaseString::snprintf(refMessage, 100, "file: %s lineNo: %d",
	   file, line);
#else
  const Uint32 blockNumber = theEmulatedJamBlockNumber;
  const char *blockName = getBlockName(blockNumber);

  BaseString::snprintf(refMessage, 100, "%s line: %d (block: %s)",
	   file, line, blockName);
#endif
  WriteMessage(ec, message, refMessage,
	       theEmulatedJamIndex, theEmulatedJam);

  childReportError(ec);

  NdbShutdown(s_errorHandlerShutdownType);
  exit(1);                                      // Deadcode
}

void
ErrorReporter::handleError(int messageID,
			   const char* problemData, 
			   const char* objRef,
			   NdbShutdownType nst)
{
  WriteMessage(messageID, problemData,
	       objRef, theEmulatedJamIndex, theEmulatedJam);

  g_eventLogger.info(problemData);
  g_eventLogger.info(objRef);

  childReportError(messageID);

  if(messageID == NDBD_EXIT_ERROR_INSERT){
    NdbShutdown(NST_ErrorInsert);
  } else {
    if (nst == NST_ErrorHandler)
      nst = s_errorHandlerShutdownType;
    NdbShutdown(nst);
  }
}

int 
WriteMessage(int thrdMessageID,
	     const char* thrdProblemData, const char* thrdObjRef,
	     Uint32 thrdTheEmulatedJamIndex,
	     Uint8 thrdTheEmulatedJam[]){
  FILE *stream;
  unsigned offset;
  unsigned long maxOffset;  // Maximum size of file.
  char theMessage[MESSAGE_LENGTH];

  /**
   * Format trace file name
   */
  char *theTraceFileName= 0;
  if (globalData.ownId > 0)
    theTraceFileName= NdbConfig_TraceFileName(globalData.ownId,
					      ErrorReporter::get_trace_no());
  NdbAutoPtr<char> tmp_aptr1(theTraceFileName);
  
  // The first 69 bytes is info about the current offset
  Uint32 noMsg = globalEmulatorData.theConfiguration->maxNoOfErrorLogs();

  maxOffset = (69 + (noMsg * MESSAGE_LENGTH));
  
  char *theErrorFileName= (char *)NdbConfig_ErrorFileName(globalData.ownId);
  NdbAutoPtr<char> tmp_aptr2(theErrorFileName);

  stream = fopen(theErrorFileName, "r+");
  if (stream == NULL) { /* If the file could not be opened. */
    
    // Create a new file, and skip the first 69 bytes, 
    // which are info about the current offset
    stream = fopen(theErrorFileName, "w");
    if(stream == NULL)
    {
      fprintf(stderr,"Unable to open error log file: %s\n", theErrorFileName);
      return -1;
    }
    fprintf(stream, "%s%u%s", "Current byte-offset of file-pointer is: ", 69,
	    "                        \n\n\n");   
    
    // ...and write the error-message...
    ErrorReporter::formatMessage(thrdMessageID,
				 thrdProblemData, thrdObjRef,
				 theTraceFileName, theMessage);
    fprintf(stream, "%s", theMessage);
    fflush(stream);
    
    /* ...and finally, at the beginning of the file, 
       store the position where to
       start writing the next message. */
    offset = ftell(stream);
    // If we have not reached the maximum number of messages...
    if (offset <= (maxOffset - MESSAGE_LENGTH)){
      fseek(stream, 40, SEEK_SET);
      // ...set the current offset...
      fprintf(stream,"%d", offset);
    } else {
      fseek(stream, 40, SEEK_SET);
      // ...otherwise, start over from the beginning.
      fprintf(stream, "%u%s", 69, "             ");
    }
  } else {
    // Go to the latest position in the file...
    fseek(stream, 40, SEEK_SET);
    fscanf(stream, "%u", &offset);
    fseek(stream, offset, SEEK_SET);
    
    // ...and write the error-message there...
    ErrorReporter::formatMessage(thrdMessageID,
				 thrdProblemData, thrdObjRef,
				 theTraceFileName, theMessage);
    fprintf(stream, "%s", theMessage);
    fflush(stream);
    
    /* ...and finally, at the beginning of the file, 
       store the position where to
       start writing the next message. */
    offset = ftell(stream);
    
    // If we have not reached the maximum number of messages...
    if (offset <= (maxOffset - MESSAGE_LENGTH)){
      fseek(stream, 40, SEEK_SET);
      // ...set the current offset...
      fprintf(stream,"%d", offset);
    } else {
      fseek(stream, 40, SEEK_SET);
      // ...otherwise, start over from the beginning.
      fprintf(stream, "%u%s", 69, "             ");
    }
  }
  fflush(stream);
  fclose(stream);
  
  if (theTraceFileName) {
    // Open the tracefile...
    FILE *jamStream = fopen(theTraceFileName, "w");
  
    //  ...and "dump the jam" there.
    //  ErrorReporter::dumpJam(jamStream);
    if(thrdTheEmulatedJam != 0){
      dumpJam(jamStream, thrdTheEmulatedJamIndex, thrdTheEmulatedJam);
    }
  
    /* Dont print the jobBuffers until a way to copy them, 
       like the other variables,
       is implemented. Otherwise when NDB keeps running, 
       with this function running
       in the background, the jobBuffers will change during runtime. And when
       they're printed here, they will not be correct anymore.
    */
    globalScheduler.dumpSignalMemory(jamStream);
  
    fclose(jamStream);
  }

  return 0;
}

void 
dumpJam(FILE *jamStream, 
	Uint32 thrdTheEmulatedJamIndex, 
	Uint8 thrdTheEmulatedJam[]) {
#ifndef NO_EMULATED_JAM   
  // print header
  const int maxaddr = 8;
  fprintf(jamStream, "JAM CONTENTS up->down left->right ?=not block entry\n");
  fprintf(jamStream, "%-7s ", "BLOCK");
  for (int i = 0; i < maxaddr; i++)
    fprintf(jamStream, "%-6s ", "ADDR");
  fprintf(jamStream, "\n");

  // treat as array of Uint32
  const Uint32 *base = (Uint32 *)thrdTheEmulatedJam;
  const int first = thrdTheEmulatedJamIndex / sizeof(Uint32);	// oldest
  int cnt, idx;

  // look for first block entry
  for (cnt = 0, idx = first; cnt < EMULATED_JAM_SIZE; cnt++, idx++) {
    if (idx >= EMULATED_JAM_SIZE)
      idx = 0;
    const Uint32 aJamEntry = base[idx];
    if (aJamEntry > (1 << 20))
      break;
  }

  // 1. if first entry is a block entry, it is printed in the main loop
  // 2. else if any block entry exists, the jam starts in an unknown block
  // 3. else if no block entry exists, the block is theEmulatedJamBlockNumber
  // a "?" indicates first addr is not a block entry
  if (cnt == 0)
    ;
  else if (cnt < EMULATED_JAM_SIZE)
    fprintf(jamStream, "%-7s?", "");
  else {
    const Uint32 aBlockNumber = theEmulatedJamBlockNumber;
    const char *aBlockName = getBlockName(aBlockNumber);
    if (aBlockName != 0)
      fprintf(jamStream, "%-7s?", aBlockName);
    else
      fprintf(jamStream, "0x%-5X?", aBlockNumber);
  }

  // loop over all entries
  int cntaddr = 0;
  for (cnt = 0, idx = first; cnt < EMULATED_JAM_SIZE; cnt++, idx++) {
    globalData.incrementWatchDogCounter(4);	// watchdog not to kill us ?
    if (idx >= EMULATED_JAM_SIZE)
      idx = 0;
    const Uint32 aJamEntry = base[idx];
    if (aJamEntry > (1 << 20)) {
      const Uint32 aBlockNumber = aJamEntry >> 20;
      const char *aBlockName = getBlockName(aBlockNumber);
      if (cnt > 0)
	  fprintf(jamStream, "\n");
      if (aBlockName != 0)
	fprintf(jamStream, "%-7s ", aBlockName);
      else
	fprintf(jamStream, "0x%-5X ", aBlockNumber);
      cntaddr = 0;
    }
    if (cntaddr == maxaddr) {
      fprintf(jamStream, "\n%-7s ", "");
      cntaddr = 0;
    }
    fprintf(jamStream, "%06u ", aJamEntry & 0xFFFFF);
    cntaddr++;
  }
  fprintf(jamStream, "\n");
  fflush(jamStream);
#endif // ifndef NO_EMULATED_JAM
}
