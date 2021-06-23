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

#ifndef BACKUP_H
#define BACKUP_H

#include <ndb_limits.h>
#include <SimulatedBlock.hpp>

#include "FsBuffer.hpp"
#include "BackupFormat.hpp"

#include <NodeBitmask.hpp>
#include <SimpleProperties.hpp>

#include <SLList.hpp>
#include <DLFifoList.hpp>
#include <DLCFifoList.hpp>
#include <SignalCounter.hpp>
#include <blocks/mutexes.hpp>

#include <NdbTCP.h>
#include <NdbTick.h>
#include <Array.hpp>

/**
 * Backup - This block manages database backup and restore
 */
class Backup : public SimulatedBlock
{
public:
  Backup(Block_context& ctx);
  virtual ~Backup();
  BLOCK_DEFINES(Backup);
  
protected:

  void execSTTOR(Signal* signal);
  void execREAD_CONFIG_REQ(Signal* signal);
  void execDUMP_STATE_ORD(Signal* signal);
  void execREAD_NODESCONF(Signal* signal);
  void execNODE_FAILREP(Signal* signal);
  void execINCL_NODEREQ(Signal* signal);
  void execCONTINUEB(Signal* signal);
  
  /**
   * Testing
   */
  void execBACKUP_REF(Signal* signal);
  void execBACKUP_CONF(Signal* signal);
  void execBACKUP_ABORT_REP(Signal* signal);
  void execBACKUP_COMPLETE_REP(Signal* signal);
  
  /**
   * Signals sent from master
   */
  void execDEFINE_BACKUP_REQ(Signal* signal);
  void execBACKUP_DATA(Signal* signal);
  void execSTART_BACKUP_REQ(Signal* signal);
  void execBACKUP_FRAGMENT_REQ(Signal* signal);
  void execBACKUP_FRAGMENT_COMPLETE_REP(Signal* signal);
  void execSTOP_BACKUP_REQ(Signal* signal);
  void execBACKUP_STATUS_REQ(Signal* signal);
  void execABORT_BACKUP_ORD(Signal* signal);
 
  /**
   * The actual scan
   */
  void execSCAN_HBREP(Signal* signal);
  void execTRANSID_AI(Signal* signal);
  void execSCAN_FRAGREF(Signal* signal);
  void execSCAN_FRAGCONF(Signal* signal);

  /**
   * Trigger logging
   */
  void execBACKUP_TRIG_REQ(Signal* signal);
  void execTRIG_ATTRINFO(Signal* signal);
  void execFIRE_TRIG_ORD(Signal* signal);
  
  /**
   * DICT signals
   */
  void execLIST_TABLES_CONF(Signal* signal);
  void execGET_TABINFOREF(Signal* signal);
  void execGET_TABINFO_CONF(Signal* signal);
  void execCREATE_TRIG_REF(Signal* signal);
  void execCREATE_TRIG_CONF(Signal* signal);
  void execDROP_TRIG_REF(Signal* signal);
  void execDROP_TRIG_CONF(Signal* signal);

  /**
   * DIH signals
   */
  void execDI_FCOUNTCONF(Signal* signal);
  void execDIGETPRIMCONF(Signal* signal);

  /**
   * FS signals
   */
  void execFSOPENREF(Signal* signal);
  void execFSOPENCONF(Signal* signal);

  void execFSCLOSEREF(Signal* signal);
  void execFSCLOSECONF(Signal* signal);
  
  void execFSAPPENDREF(Signal* signal);
  void execFSAPPENDCONF(Signal* signal);
  
  void execFSREMOVEREF(Signal* signal);
  void execFSREMOVECONF(Signal* signal);

  /**
   * Master functinallity
   */
  void execBACKUP_REQ(Signal* signal);
  void execABORT_BACKUP_REQ(Signal* signal);
  
  void execDEFINE_BACKUP_REF(Signal* signal);
  void execDEFINE_BACKUP_CONF(Signal* signal);

  void execSTART_BACKUP_REF(Signal* signal);
  void execSTART_BACKUP_CONF(Signal* signal);

  void execBACKUP_FRAGMENT_REF(Signal* signal);
  void execBACKUP_FRAGMENT_CONF(Signal* signal);

  void execSTOP_BACKUP_REF(Signal* signal);
  void execSTOP_BACKUP_CONF(Signal* signal);
  
  void execBACKUP_STATUS_CONF(Signal* signal);

  void execUTIL_SEQUENCE_REF(Signal* signal);
  void execUTIL_SEQUENCE_CONF(Signal* signal);

  void execWAIT_GCP_REF(Signal* signal);
  void execWAIT_GCP_CONF(Signal* signal);
  
  void execLCP_PREPARE_REQ(Signal* signal);
  void execLCP_FRAGMENT_REQ(Signal*);
  void execEND_LCPREQ(Signal* signal);
private:
  void defineBackupMutex_locked(Signal* signal, Uint32 ptrI,Uint32 retVal);
  void dictCommitTableMutex_locked(Signal* signal, Uint32 ptrI,Uint32 retVal);

public:
  struct Node {
    Uint32 nodeId;
    Uint32 alive;
    Uint32 nextList;
    union { Uint32 prevList; Uint32 nextPool; };
  };
  typedef Ptr<Node> NodePtr;

#define BACKUP_WORDS_PER_PAGE 8191
  struct Page32 {
    Uint32 data[BACKUP_WORDS_PER_PAGE];
    Uint32 nextPool;
  };
  typedef Ptr<Page32> Page32Ptr;

  struct Attribute {
    enum Flags {
      COL_NULLABLE = 0x1,
      COL_FIXED    = 0x2,
      COL_DISK     = 0x4
    };
    struct Data {
      Uint16 m_flags;
      Uint16 attrId;
      Uint32 sz32;       // No of 32 bit words
      Uint32 offset;     // Relative DataFixedAttributes/DataFixedKeys
      Uint32 offsetNull; // In NullBitmask
    } data;
    union {
      Uint32 nextPool;
      Uint32 nextList;
    };
    Uint32 prevList;
  };
  typedef Ptr<Attribute> AttributePtr;
  
  struct Fragment {
    Uint64 noOfRecords;
    Uint32 tableId;
    Uint16 node;
    Uint16 fragmentId;
    Uint8 scanned;  // 0 = not scanned x = scanned by node x
    Uint8 scanning; // 0 = not scanning x = scanning on node x
    Uint8 lcp_no;
    Uint32 nextPool;
  };
  typedef Ptr<Fragment> FragmentPtr;

  struct Table {
    Table(ArrayPool<Attribute> &, ArrayPool<Fragment> &);
    
    Uint64 noOfRecords;

    Uint32 tableId;
    Uint32 schemaVersion;
    Uint32 tableType;
    Uint32 noOfNull;
    Uint32 noOfAttributes;
    Uint32 noOfVariable;
    Uint32 sz_FixedAttributes;
    Uint32 triggerIds[3];
    bool   triggerAllocated[3];
    
    DLFifoList<Attribute> attributes;
    Array<Fragment> fragments;

    Uint32 nextList;
    union { Uint32 nextPool; Uint32 prevList; };
  };
  typedef Ptr<Table> TablePtr;

  struct OperationRecord {
  public:
    OperationRecord(Backup & b) : backup(b) {}

    /**
     * Once per table
     */
    void init(const TablePtr & ptr);
    
    /**
     * Once per fragment
     */
    bool newFragment(Uint32 tableId, Uint32 fragNo);
    bool fragComplete(Uint32 tableId, Uint32 fragNo, bool fill_record);
    
    /**
     * Once per scan frag (next) req/conf
     */
    bool newScan();
    bool scanConf(Uint32 noOfOps, Uint32 opLen);
    bool closeScan();
    
    /**
     * Per record
     */
    void newRecord(Uint32 * base);
    bool finished();
    
    /**
     * Per attribute
     */
    void     nullVariable();
    void     nullAttribute(Uint32 nullOffset);
    Uint32 * newNullable(Uint32 attrId, Uint32 sz);
    Uint32 * newAttrib(Uint32 offset, Uint32 sz);
    Uint32 * newVariable(Uint32 id, Uint32 sz);
    
  private:
    Uint32* base; 
    Uint32* dst_Length;
    Uint32* dst_Bitmask;
    Uint32* dst_FixedAttribs;
    BackupFormat::DataFile::VariableData* dst_VariableData;
    
    Uint32 noOfAttributes; // No of Attributes
    Uint32 attrLeft;       // No of attributes left

    Uint32 opNoDone;
    Uint32 opNoConf;
    Uint32 opLen;

  public:
    Uint32* dst;
    Uint32 attrSzTotal; // No of AI words received
    Uint32 tablePtr;    // Ptr.i to current table

    FsBuffer dataBuffer;
    Uint64 noOfRecords;
    Uint64 noOfBytes;
    Uint32 maxRecordSize;
    
  private:
    Uint32* scanStart;
    Uint32* scanStop;

    /**
     * sizes of part
     */
    Uint32 sz_Bitmask;
    Uint32 sz_FixedAttribs;

  public:
    union { Uint32 nextPool; Uint32 nextList; };
    Uint32 prevList;
  private:

    Backup & backup;
    BlockNumber number() const { return backup.number(); }
    void progError(int line, int cause, const char * extra) { 
      backup.progError(line, cause, extra); 
    }
  };
  friend struct OperationRecord;

  struct TriggerRecord {
    TriggerRecord() { event = ~0;}
    OperationRecord * operation;
    BackupFormat::LogFile::LogEntry * logEntry;
    Uint32 maxRecordSize;
    Uint32 tableId;
    Uint32 tab_ptr_i;
    Uint32 event;
    Uint32 backupPtr;
    Uint32 errorCode;
    union { Uint32 nextPool; Uint32 nextList; };
  };
  typedef Ptr<TriggerRecord> TriggerPtr;
  
  /**
   * BackupFile - At least 3 per backup
   */
  struct BackupFile {
    BackupFile(Backup & backup, ArrayPool<Page32> & pp) 
      : operation(backup),  pages(pp) {}
    
    Uint32 backupPtr; // Pointer to backup record
    Uint32 tableId;
    Uint32 fragmentNo;
    Uint32 filePointer;
    Uint32 errorCode;
    BackupFormat::FileType fileType;
    OperationRecord operation;
    
    Array<Page32> pages;
    Uint32 nextList;
    union { Uint32 prevList; Uint32 nextPool; };
    
    enum {
      BF_OPEN         = 0x1
      ,BF_OPENING     = 0x2
      ,BF_CLOSING     = 0x4
      ,BF_FILE_THREAD = 0x8
      ,BF_SCAN_THREAD = 0x10
      ,BF_LCP_META    = 0x20
    };
    Uint32 m_flags;
    Uint32 m_pos;
  }; 
  typedef Ptr<BackupFile> BackupFilePtr;
 

  /**
   * State for BackupRecord
   */
  enum State {
    INITIAL  = 0,
    DEFINING = 1, // Defining backup content and parameters
    DEFINED  = 2,  // DEFINE_BACKUP_CONF sent in slave, received all in master
    STARTED  = 3,  // Creating triggers
    SCANNING = 4, // Scanning fragments
    STOPPING = 5, // Closing files
    CLEANING = 6, // Cleaning resources
    ABORTING = 7  // Aborting backup
  };

  static const Uint32 validSlaveTransitionsCount;
  static const Uint32 validMasterTransitionsCount;
  static const State validSlaveTransitions[];
  static const State validMasterTransitions[];
  
  class CompoundState {
  public:
    CompoundState(Backup & b, 
		  const State valid[],
		  Uint32 count, Uint32 _id) 
      : backup(b)
      , validTransitions(valid),
	noOfValidTransitions(count), id(_id)
    { 
      state = INITIAL;
      abortState = state;
    }
    
    void setState(State s);
    State getState() const { return state;}
    State getAbortState() const { return abortState;}
    
    void forceState(State s);
    
    BlockNumber number() const { return backup.number(); }
    void progError(int line, int cause, const char * extra) { 
      backup.progError(line, cause, extra); 
    }
  private:
    Backup & backup;
    State state;     
    State abortState;     /**
			     When state == ABORTING, this contains the state 
			     when the abort started
			  */
    const State * validTransitions;
    const Uint32 noOfValidTransitions;
    const Uint32 id;
  };
  friend class CompoundState;
  
  /**
   * Backup record
   *
   * One record per backup
   */
  struct BackupRecord {
    BackupRecord(Backup& b, 
		 ArrayPool<Table> & tp, 
		 ArrayPool<BackupFile> & bp,
		 ArrayPool<TriggerRecord> & trp) 
      : slaveState(b, validSlaveTransitions, validSlaveTransitionsCount,1)
      , tables(tp), triggers(trp), files(bp)
      , ctlFilePtr(RNIL), logFilePtr(RNIL), dataFilePtr(RNIL)
      , masterData(b), backup(b)

      {
      }
    
    Uint32 m_gsn;
    CompoundState slaveState; 
    
    Uint32 clientRef;
    Uint32 clientData;
    Uint32 flags;
    Uint32 signalNo;
    Uint32 backupId;
    Uint32 backupKey[2];
    Uint32 masterRef;
    Uint32 errorCode;
    NdbNodeBitmask nodes;
    
    Uint64 noOfBytes;
    Uint64 noOfRecords;
    Uint64 noOfLogBytes;
    Uint64 noOfLogRecords;
    
    Uint32 startGCP;
    Uint32 currGCP;
    Uint32 stopGCP;
    DLCFifoList<Table> tables;
    SLList<TriggerRecord> triggers;
    
    SLList<BackupFile> files; 
    Uint32 ctlFilePtr;  // Ptr.i to ctl-file
    Uint32 logFilePtr;  // Ptr.i to log-file
    Uint32 dataFilePtr; // Ptr.i to first data-file
    
    Uint32 backupDataLen;  // Used for (un)packing backup request
    SimpleProperties props;// Used for (un)packing backup request

    struct SlaveData {
      SignalCounter trigSendCounter;
      Uint32 gsn;
      struct {
	Uint32 tableId;
      } createTrig;
      struct {
	Uint32 tableId;
      } dropTrig;
    } slaveData;

    struct MasterData {
      MasterData(Backup & b) 
	{
	}
      MutexHandle2<BACKUP_DEFINE_MUTEX> m_defineBackupMutex;
      MutexHandle2<DICT_COMMIT_TABLE_MUTEX> m_dictCommitTableMutex;

      Uint32 gsn;
      SignalCounter sendCounter;
      Uint32 errorCode;
      union {
	struct {
	  Uint32 startBackup;
	} waitGCP;
	struct {
	  Uint32 signalNo;
	  Uint32 noOfSignals;
	  Uint32 tablePtr;
	} startBackup;
	struct {
	  Uint32 dummy;
	} stopBackup;
      };
    } masterData;
    
    Uint32 nextList;
    union { Uint32 prevList; Uint32 nextPool; };

    void setErrorCode(Uint32 errCode){
      if(errorCode == 0)
	errorCode = errCode;
    }

    bool checkError() const {
      return errorCode != 0;
    }

    bool is_lcp() const {
      return backupDataLen == ~(Uint32)0;
    }

    Backup & backup;
    BlockNumber number() const { return backup.number(); }
    void progError(int line, int cause, const char * extra) { 
      backup.progError(line, cause, extra); 
    }
  };
  friend struct BackupRecord;
  typedef Ptr<BackupRecord> BackupRecordPtr;

  struct Config {
    Uint32 m_dataBufferSize;
    Uint32 m_logBufferSize;
    Uint32 m_minWriteSize;
    Uint32 m_maxWriteSize;
    Uint32 m_lcp_buffer_size;
    
    Uint32 m_disk_write_speed_sr;
    Uint32 m_disk_write_speed;
    Uint32 m_disk_synch_size;
    Uint32 m_diskless;
    Uint32 m_o_direct;
  };
  
  /**
   * Variables
   */
  Uint32 * c_startOfPages;
  NodeId c_masterNodeId;
  SLList<Node> c_nodes;
  NdbNodeBitmask c_aliveNodes;
  DLList<BackupRecord> c_backups;
  Config c_defaults;

  /*
    Variables that control checkpoint to disk speed
  */
  Uint32 m_curr_disk_write_speed;
  Uint32 m_words_written_this_period;
  Uint32 m_overflow_disk_write;
  Uint32 m_reset_delay_used;
  NDB_TICKS m_reset_disk_speed_time;
  static const int  DISK_SPEED_CHECK_DELAY = 100;
  
  STATIC_CONST(NO_OF_PAGES_META_FILE = 
	       (2*MAX_WORDS_META_FILE + BACKUP_WORDS_PER_PAGE - 1) / 
	       BACKUP_WORDS_PER_PAGE);

  /**
   * Pools
   */
  ArrayPool<Table> c_tablePool;
  ArrayPool<Attribute> c_attributePool;  
  ArrayPool<BackupRecord> c_backupPool;
  ArrayPool<BackupFile> c_backupFilePool;
  ArrayPool<Page32> c_pagePool;
  ArrayPool<Fragment> c_fragmentPool;
  ArrayPool<Node> c_nodePool;
  ArrayPool<TriggerRecord> c_triggerPool;

  void checkFile(Signal*, BackupFilePtr);
  void checkScan(Signal*, BackupFilePtr);
  void fragmentCompleted(Signal*, BackupFilePtr);
  
  void backupAllData(Signal* signal, BackupRecordPtr);
  
  void getFragmentInfo(Signal*, BackupRecordPtr, TablePtr, Uint32 fragNo);
  void getFragmentInfoDone(Signal*, BackupRecordPtr);
  
  void openFiles(Signal* signal, BackupRecordPtr ptr);
  void openFilesReply(Signal*, BackupRecordPtr ptr, BackupFilePtr);
  void closeFiles(Signal*, BackupRecordPtr ptr);
  void closeFile(Signal*, BackupRecordPtr, BackupFilePtr);
  void closeFilesDone(Signal*, BackupRecordPtr ptr);  
  
  void sendDefineBackupReq(Signal *signal, BackupRecordPtr ptr);

  void defineBackupReply(Signal* signal, BackupRecordPtr ptr, Uint32 nodeId);
  void createTrigReply(Signal* signal, BackupRecordPtr ptr);
  void alterTrigReply(Signal* signal, BackupRecordPtr ptr);
  void startBackupReply(Signal* signal, BackupRecordPtr ptr, Uint32);
  void stopBackupReply(Signal* signal, BackupRecordPtr ptr, Uint32 nodeId);
  
  void defineBackupRef(Signal*, BackupRecordPtr, Uint32 errCode = 0);
  void backupFragmentRef(Signal * signal, BackupFilePtr filePtr);

  void nextFragment(Signal*, BackupRecordPtr);
  
  void sendCreateTrig(Signal*, BackupRecordPtr ptr, TablePtr tabPtr);
  void createAttributeMask(TablePtr tab, Bitmask<MAXNROFATTRIBUTESINWORDS>&);
  void sendStartBackup(Signal*, BackupRecordPtr, TablePtr);
  void sendAlterTrig(Signal*, BackupRecordPtr ptr);

  void sendDropTrig(Signal*, BackupRecordPtr ptr);
  void sendDropTrig(Signal* signal, BackupRecordPtr ptr, TablePtr tabPtr);
  void dropTrigReply(Signal*, BackupRecordPtr ptr);
  
  void sendSignalAllWait(BackupRecordPtr ptr, Uint32 gsn, Signal *signal, 
			 Uint32 signalLength,
			 bool executeDirect = false);
  bool haveAllSignals(BackupRecordPtr ptr, Uint32 gsn, Uint32 nodeId);

  void sendStopBackup(Signal*, BackupRecordPtr ptr);
  void sendAbortBackupOrd(Signal* signal, BackupRecordPtr ptr, Uint32 errCode);
  void sendAbortBackupOrdSlave(Signal* signal, BackupRecordPtr ptr, 
			       Uint32 errCode);
  void masterAbort(Signal*, BackupRecordPtr ptr);
  void masterSendAbortBackup(Signal*, BackupRecordPtr ptr);
  void slaveAbort(Signal*, BackupRecordPtr ptr);
  
  void abortFile(Signal* signal, BackupRecordPtr ptr, BackupFilePtr filePtr);
  void abortFileHook(Signal* signal, BackupFilePtr filePtr, bool scanDone);
  
  bool verifyNodesAlive(BackupRecordPtr, const NdbNodeBitmask& aNodeBitMask);
  bool checkAbort(BackupRecordPtr ptr);
  void checkNodeFail(Signal* signal,
		     BackupRecordPtr ptr,
		     NodeId newCoord,
		     Uint32 theFailedNodes[NodeBitmask::Size]);
  void masterTakeOver(Signal* signal, BackupRecordPtr ptr);


  NodeId getMasterNodeId() const { return c_masterNodeId; }
  bool findTable(const BackupRecordPtr &, TablePtr &, Uint32 tableId) const;
  bool parseTableDescription(Signal*, BackupRecordPtr ptr, TablePtr, const Uint32*, Uint32);
  
  bool insertFileHeader(BackupFormat::FileType, BackupRecord*, BackupFile*);
  void sendBackupRef(Signal* signal, BackupRecordPtr ptr, Uint32 errorCode);
  void sendBackupRef(BlockReference ref, Uint32 flags, Signal *signal,
		     Uint32 senderData, Uint32 errorCode);
  void dumpUsedResources();
  void cleanup(Signal*, BackupRecordPtr ptr);
  void abort_scan(Signal*, BackupRecordPtr ptr);
  void removeBackup(Signal*, BackupRecordPtr ptr);

  void sendSTTORRY(Signal*);
  void createSequence(Signal* signal);
  void createSequenceReply(Signal*, class UtilSequenceConf *);

  void lcp_open_file(Signal* signal, BackupRecordPtr ptr);
  void lcp_open_file_done(Signal*, BackupRecordPtr);
  void lcp_close_file_conf(Signal* signal, BackupRecordPtr);

  bool ready_to_write(bool ready, Uint32 sz, bool eof, BackupFile *fileP);
};

inline
void
Backup::OperationRecord::newRecord(Uint32 * p){
  base = p;
  dst_Length       = p; p += 1;
  dst_Bitmask      = p; p += sz_Bitmask;
  dst_FixedAttribs = p; p += sz_FixedAttribs;
  dst_VariableData = (BackupFormat::DataFile::VariableData*)p;
  BitmaskImpl::clear(sz_Bitmask, dst_Bitmask);
  attrLeft = noOfAttributes;
  attrSzTotal = 0;
}

inline
Uint32 *
Backup::OperationRecord::newAttrib(Uint32 offset, Uint32 sz){
  attrLeft--;
  dst = dst_FixedAttribs + offset;
  return dst;
}

inline
void
Backup::OperationRecord::nullAttribute(Uint32 offsetNull){
  attrLeft --;
  BitmaskImpl::set(sz_Bitmask, dst_Bitmask, offsetNull);
}

inline
void
Backup::OperationRecord::nullVariable()
{
  attrLeft --;
}

inline
Uint32 *
Backup::OperationRecord::newNullable(Uint32 id, Uint32 sz){
  Uint32 sz32 = (sz + 3) >> 2;

  attrLeft--;
  
  dst = &dst_VariableData->Data[0];
  dst_VariableData->Sz = htonl(sz);
  dst_VariableData->Id = htonl(id);
  
  dst_VariableData = (BackupFormat::DataFile::VariableData *)(dst + sz32);
  
  // Clear all bits on newRecord -> dont need to clear this
  // BitmaskImpl::clear(sz_Bitmask, dst_Bitmask, offsetNull);
  return dst;
}

inline
Uint32 *
Backup::OperationRecord::newVariable(Uint32 id, Uint32 sz){
  Uint32 sz32 = (sz + 3) >> 2;

  attrLeft--;
  
  dst = &dst_VariableData->Data[0];
  dst_VariableData->Sz = htonl(sz);
  dst_VariableData->Id = htonl(id);
  
  dst_VariableData = (BackupFormat::DataFile::VariableData *)(dst + sz32);
  return dst;
}

inline
bool
Backup::OperationRecord::finished(){
  if(attrLeft != 0){
    return false;
  }
  
  opLen += attrSzTotal;
  opNoDone++;
  
  scanStop = dst = (Uint32 *)dst_VariableData;
  
  const Uint32 len = (dst - base - 1);
  * dst_Length = htonl(len);
  
  noOfRecords++;
  
  return true;
}

#endif
