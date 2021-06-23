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

#include "FastScheduler.hpp"
#include "RefConvert.hpp"

#include "Emulator.hpp"
#include "VMSignal.hpp"

#include <SignalLoggerManager.hpp>
#include <BlockNumbers.h>
#include <GlobalSignalNumbers.h>
#include <signaldata/EventReport.hpp>
#include "LongSignal.hpp"
#include <NdbTick.h>

#define MIN_NUMBER_OF_SIG_PER_DO_JOB 64
#define MAX_NUMBER_OF_SIG_PER_DO_JOB 2048
#define EXTRA_SIGNALS_PER_DO_JOB 32

FastScheduler::FastScheduler()
{
   // These constants work for sun only, but they should be initated from
   // Emulator.C as soon as VMTime has been initiated.
   theJobBuffers[0].newBuffer(JBASIZE);
   theJobBuffers[1].newBuffer(JBBSIZE);
   theJobBuffers[2].newBuffer(JBCSIZE);
   theJobBuffers[3].newBuffer(JBDSIZE);
   clear();
}

FastScheduler::~FastScheduler()
{
}

void 
FastScheduler::clear()
{
  int i;
  // Make sure the restart signals are not sent too early
  // the prio is set back in 'main' using the 'ready' method.
  globalData.highestAvailablePrio = LEVEL_IDLE;
  globalData.sendPackedActivated = 0;
  globalData.activateSendPacked = 0;
  for (i = 0; i < JB_LEVELS; i++){
    theJobBuffers[i].clear();
  }
  globalData.JobCounter = 0;
  globalData.JobLap = 0;
  globalData.loopMax = 32;
  globalData.VMSignals[0].header.theSignalId = 0;
  
  theDoJobTotalCounter = 0;
  theDoJobCallCounter = 0;
}

void
FastScheduler::activateSendPacked()
{
  globalData.sendPackedActivated = 1;
  globalData.activateSendPacked = 0;
  globalData.loopMax = 2048;
}//FastScheduler::activateSendPacked()

//------------------------------------------------------------------------
// sendPacked is executed at the end of the loop.
// To ensure that we don't send any messages before executing all local
// packed signals we do another turn in the loop (unless we have already
// executed too many signals in the loop).
//------------------------------------------------------------------------
void 
FastScheduler::doJob()
{
  Uint32 loopCount = 0;
  Uint32 TminLoops = getBOccupancy() + EXTRA_SIGNALS_PER_DO_JOB;
  Uint32 TloopMax = (Uint32)globalData.loopMax;
  if (TminLoops < TloopMax) {
    TloopMax = TminLoops;
  }//if
  if (TloopMax < MIN_NUMBER_OF_SIG_PER_DO_JOB) {
    TloopMax = MIN_NUMBER_OF_SIG_PER_DO_JOB;
  }//if
  register Signal* signal = getVMSignals();
  register Uint32 tHighPrio= globalData.highestAvailablePrio;
  do{
    while ((tHighPrio < LEVEL_IDLE) && (loopCount < TloopMax)) {
      // signal->garbage_register(); 
      // To ensure we find bugs quickly
      register Uint32 gsnbnr = theJobBuffers[tHighPrio].retrieve(signal);
      register BlockNumber reg_bnr = gsnbnr & 0xFFF;
      register GlobalSignalNumber reg_gsn = gsnbnr >> 16;
      globalData.incrementWatchDogCounter(1);
      if (reg_bnr > 0) {
        Uint32 tJobCounter = globalData.JobCounter;
        Uint32 tJobLap = globalData.JobLap;
        SimulatedBlock* b = globalData.getBlock(reg_bnr);
        theJobPriority[tJobCounter] = (Uint8)tHighPrio;
        globalData.JobCounter = (tJobCounter + 1) & 4095;
        globalData.JobLap = tJobLap + 1;
	
#ifdef VM_TRACE_TIME
	Uint32 us1, us2;
	Uint64 ms1, ms2;
	NdbTick_CurrentMicrosecond(&ms1, &us1);
	b->m_currentGsn = reg_gsn;
#endif
	
	getSections(signal->header.m_noOfSections, signal->m_sectionPtr);
#ifdef VM_TRACE
        {
          if (globalData.testOn) {
	    signal->header.theVerId_signalNumber = reg_gsn;
	    signal->header.theReceiversBlockNumber = reg_bnr;
	    
            globalSignalLoggers.executeSignal(signal->header,
					      tHighPrio,
					      &signal->theData[0], 
					      globalData.ownId,
                                              signal->m_sectionPtr,
                                              signal->header.m_noOfSections);
          }//if
        }
#endif
        b->executeFunction(reg_gsn, signal);
	releaseSections(signal->header.m_noOfSections, signal->m_sectionPtr);
	signal->header.m_noOfSections = 0;
#ifdef VM_TRACE_TIME
	NdbTick_CurrentMicrosecond(&ms2, &us2);
	Uint64 diff = ms2;
	diff -= ms1;
	diff *= 1000000;
	diff += us2;
	diff -= us1;
	b->addTime(reg_gsn, diff);
#endif
        tHighPrio = globalData.highestAvailablePrio;
      } else {
        tHighPrio++;
        globalData.highestAvailablePrio = tHighPrio;
      }//if
      loopCount++;
    }//while
    sendPacked();
    tHighPrio = globalData.highestAvailablePrio;
    if(getBOccupancy() > MAX_OCCUPANCY)
    {
      if(loopCount != TloopMax)
	abort();
      assert( loopCount == TloopMax );
      TloopMax += 512;
    }
  } while ((getBOccupancy() > MAX_OCCUPANCY) ||
           ((loopCount < TloopMax) &&
            (tHighPrio < LEVEL_IDLE)));

  theDoJobCallCounter ++;
  theDoJobTotalCounter += loopCount;
  if (theDoJobCallCounter == 8192) {
    reportDoJobStatistics(theDoJobTotalCounter >> 13);
    theDoJobCallCounter = 0;
    theDoJobTotalCounter = 0;
  }//if

}//FastScheduler::doJob()

void FastScheduler::sendPacked()
{
  if (globalData.sendPackedActivated == 1) {
    SimulatedBlock* b_lqh = globalData.getBlock(DBLQH);
    SimulatedBlock* b_tc = globalData.getBlock(DBTC);
    SimulatedBlock* b_tup = globalData.getBlock(DBTUP);
    Signal* signal = getVMSignals();
    b_lqh->executeFunction(GSN_SEND_PACKED, signal);
    b_tc->executeFunction(GSN_SEND_PACKED, signal);
    b_tup->executeFunction(GSN_SEND_PACKED, signal);
    return;
  } else if (globalData.activateSendPacked == 0) {
    return;
  } else {
    activateSendPacked();
  }//if
  return;
}//FastScheduler::sendPacked()

Uint32
APZJobBuffer::retrieve(Signal* signal)
{              
  Uint32 tOccupancy = theOccupancy;
  Uint32 myRPtr = rPtr;
  BufferEntry& buf = buffer[myRPtr];
  Uint32 gsnbnr;
  Uint32 cond =  (++myRPtr == bufSize) - 1;
  Uint32 tRecBlockNo = buf.header.theReceiversBlockNumber;
  
  if (tOccupancy != 0) {
    if (tRecBlockNo != 0) {
      // Transform protocol to signal. 
      rPtr = myRPtr & cond;
      theOccupancy = tOccupancy - 1;
      gsnbnr = buf.header.theVerId_signalNumber << 16 | tRecBlockNo;
      
      Uint32 tSignalId = globalData.theSignalId;
      Uint32 tLength = buf.header.theLength;
      Uint32 tFirstData = buf.theDataRegister[0];
      signal->header = buf.header;
      
      // Recall our signal Id for restart purposes
      buf.header.theSignalId = tSignalId;  
      globalData.theSignalId = tSignalId + 1;
      
      Uint32* tDataRegPtr = &buf.theDataRegister[0];
      Uint32* tSigDataPtr = signal->getDataPtrSend();
      *tSigDataPtr = tFirstData;
      tDataRegPtr++;
      tSigDataPtr++;
      Uint32  tLengthCopied = 1;
      while (tLengthCopied < tLength) {
        Uint32 tData0 = tDataRegPtr[0];
        Uint32 tData1 = tDataRegPtr[1];
        Uint32 tData2 = tDataRegPtr[2];
        Uint32 tData3 = tDataRegPtr[3];
	
        tDataRegPtr += 4;
        tLengthCopied += 4;
	
        tSigDataPtr[0] = tData0;
        tSigDataPtr[1] = tData1;
        tSigDataPtr[2] = tData2;
        tSigDataPtr[3] = tData3;
        tSigDataPtr += 4;
      }//while

      /**
       * Copy sections references (copy all without if-statements)
       */
      tDataRegPtr = &buf.theDataRegister[tLength];
      SegmentedSectionPtr * tSecPtr = &signal->m_sectionPtr[0];
      Uint32 tData0 = tDataRegPtr[0];
      Uint32 tData1 = tDataRegPtr[1];
      Uint32 tData2 = tDataRegPtr[2];
      
      tSecPtr[0].i = tData0;
      tSecPtr[1].i = tData1;
      tSecPtr[2].i = tData2;
      
      //---------------------------------------------------------
      // Prefetch of buffer[rPtr] is done here. We prefetch for
      // read both the first cache line and the next 64 byte
      // entry
      //---------------------------------------------------------
      PREFETCH((void*)&buffer[rPtr]);
      PREFETCH((void*)(((char*)&buffer[rPtr]) + 64));
      return gsnbnr;
    } else {
      bnr_error();
      return 0; // Will never come here, simply to keep GCC happy.
    }//if
  } else {
    //------------------------------------------------------------
    // The Job Buffer was empty, signal this by return zero.
    //------------------------------------------------------------
    return 0;
  }//if
}//APZJobBuffer::retrieve()

void 
APZJobBuffer::signal2buffer(Signal* signal,
			    BlockNumber bnr, GlobalSignalNumber gsn,
			    BufferEntry& buf)
{
  Uint32 tSignalId = globalData.theSignalId;
  Uint32 tFirstData = signal->theData[0];
  Uint32 tLength = signal->header.theLength;
  Uint32 tSigId  = buf.header.theSignalId;
  
  buf.header = signal->header;
  buf.header.theVerId_signalNumber = gsn;
  buf.header.theReceiversBlockNumber = bnr;
  buf.header.theSendersSignalId = tSignalId - 1;
  buf.header.theSignalId = tSigId;
  buf.theDataRegister[0] = tFirstData;
  
  Uint32 tLengthCopied = 1;
  Uint32* tSigDataPtr = &signal->theData[1];
  Uint32* tDataRegPtr = &buf.theDataRegister[1];
  while (tLengthCopied < tLength) {
    Uint32 tData0 = tSigDataPtr[0];
    Uint32 tData1 = tSigDataPtr[1];
    Uint32 tData2 = tSigDataPtr[2];
    Uint32 tData3 = tSigDataPtr[3];
    
    tLengthCopied += 4;
    tSigDataPtr += 4;

    tDataRegPtr[0] = tData0;
    tDataRegPtr[1] = tData1;
    tDataRegPtr[2] = tData2;
    tDataRegPtr[3] = tData3;
    tDataRegPtr += 4;
  }//while

  /**
   * Copy sections references (copy all without if-statements)
   */
  tDataRegPtr = &buf.theDataRegister[tLength];
  SegmentedSectionPtr * tSecPtr = &signal->m_sectionPtr[0];
  Uint32 tData0 = tSecPtr[0].i;
  Uint32 tData1 = tSecPtr[1].i;
  Uint32 tData2 = tSecPtr[2].i;
  tDataRegPtr[0] = tData0;
  tDataRegPtr[1] = tData1;
  tDataRegPtr[2] = tData2;
}//APZJobBuffer::signal2buffer()

void
APZJobBuffer::insert(const SignalHeader * const sh,
		     const Uint32 * const theData, const Uint32 secPtrI[3]){
  Uint32 tOccupancy = theOccupancy + 1;
  Uint32 myWPtr = wPtr;
  register BufferEntry& buf = buffer[myWPtr];
  
  if (tOccupancy < bufSize) {
    Uint32 cond =  (++myWPtr == bufSize) - 1;
    wPtr = myWPtr & cond;
    theOccupancy = tOccupancy;
    
    buf.header = * sh;
    const Uint32 len = buf.header.theLength;
    memcpy(buf.theDataRegister, theData, 4 * len);
    memcpy(&buf.theDataRegister[len], &secPtrI[0], 4 * 3);
    //---------------------------------------------------------
    // Prefetch of buffer[wPtr] is done here. We prefetch for
    // write both the first cache line and the next 64 byte
    // entry
    //---------------------------------------------------------
    WRITEHINT((void*)&buffer[wPtr]);
    WRITEHINT((void*)(((char*)&buffer[wPtr]) + 64));
    
  } else {
    jbuf_error();
  }//if
}
APZJobBuffer::APZJobBuffer()
  : bufSize(0), buffer(NULL), memRef(NULL)
{
  clear();
}

APZJobBuffer::~APZJobBuffer()
{
  delete [] buffer;
}

void
APZJobBuffer::newBuffer(int size)
{
  buffer = new BufferEntry[size + 1]; // +1 to support "overrrun"
  if(buffer){
#ifndef NDB_PURIFY
    ::memset(buffer, 0, (size * sizeof(BufferEntry)));
#endif
    bufSize = size;
  } else
    bufSize = 0;
}

void
APZJobBuffer::clear()
{
  rPtr = 0;
  wPtr = 0;
  theOccupancy = 0;
}

/**
 * Function prototype for print_restart
 *
 *   Defined later in this file
 */
void print_restart(FILE * output, Signal* signal, Uint32 aLevel);

void FastScheduler::dumpSignalMemory(FILE * output)
{
  SignalT<25> signalT;
  Signal &signal= *(Signal*)&signalT;
  Uint32 ReadPtr[5];
  Uint32 tJob;
  Uint32 tLastJob;

  fprintf(output, "\n");
 
  if (globalData.JobLap > 4095) {
    if (globalData.JobCounter != 0)
      tJob = globalData.JobCounter - 1;
    else
      tJob = 4095;
    tLastJob = globalData.JobCounter;
  } else {
    if (globalData.JobCounter == 0)
      return; // No signals sent
    else {
      tJob = globalData.JobCounter - 1;
      tLastJob = 4095;
    }
  }
  ReadPtr[0] = theJobBuffers[0].getReadPtr();
  ReadPtr[1] = theJobBuffers[1].getReadPtr();
  ReadPtr[2] = theJobBuffers[2].getReadPtr();
  ReadPtr[3] = theJobBuffers[3].getReadPtr();
  
  do {
    unsigned char tLevel = theJobPriority[tJob];
    globalData.incrementWatchDogCounter(4);
    if (ReadPtr[tLevel] == 0)
      ReadPtr[tLevel] = theJobBuffers[tLevel].getBufSize() - 1;
    else
      ReadPtr[tLevel]--;
    
    theJobBuffers[tLevel].retrieveDump(&signal, ReadPtr[tLevel]);
    print_restart(output, &signal, tLevel);
    
    if (tJob == 0)
      tJob = 4095;
    else
      tJob--;
    
  } while (tJob != tLastJob);
  fflush(output);
}

void
FastScheduler::prio_level_error()
{
  ERROR_SET(ecError, NDBD_EXIT_WRONG_PRIO_LEVEL, 
	    "Wrong Priority Level", "FastScheduler.C");
}

void 
jbuf_error()
{
  ERROR_SET(ecError, NDBD_EXIT_BLOCK_JBUFCONGESTION, 
	    "Job Buffer Full", "APZJobBuffer.C");
}

void 
bnr_error()
{
  ERROR_SET(ecError, NDBD_EXIT_BLOCK_BNR_ZERO, 
	    "Block Number Zero", "FastScheduler.C");
}

void
print_restart(FILE * output, Signal* signal, Uint32 aLevel)
{
  fprintf(output, "--------------- Signal ----------------\n");
  SignalLoggerManager::printSignalHeader(output, 
					 signal->header,
					 aLevel,
					 globalData.ownId, 
					 true);
  SignalLoggerManager::printSignalData  (output, 
					 signal->header,
					 &signal->theData[0]);
}

/**
 * This method used to be a Cmvmi member function
 * but is now a "ordinary" function"
 *
 * See TransporterCallback.cpp for explanation
 */
void 
FastScheduler::reportDoJobStatistics(Uint32 tMeanLoopCount) {
  SignalT<2> signalT;
  Signal &signal= *(Signal*)&signalT;

  memset(&signal.header, 0, sizeof(signal.header));
  signal.header.theLength = 2;
  signal.header.theSendersSignalId = 0;
  signal.header.theSendersBlockRef = numberToRef(0, 0);  

  signal.theData[0] = NDB_LE_JobStatistic;
  signal.theData[1] = tMeanLoopCount;
  
  execute(&signal, JBA, CMVMI, GSN_EVENT_REP);
}

