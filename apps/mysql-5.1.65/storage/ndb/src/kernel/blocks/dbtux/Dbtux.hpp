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

#ifndef DBTUX_H
#define DBTUX_H

#include <ndb_limits.h>
#include <SimulatedBlock.hpp>
#include <AttributeDescriptor.hpp>
#include <AttributeHeader.hpp>
#include <ArrayPool.hpp>
#include <DataBuffer.hpp>
#include <DLFifoList.hpp>
#include <md5_hash.hpp>

// big brother
#include <dbtup/Dbtup.hpp>

// signal classes
#include <signaldata/DictTabInfo.hpp>
#include <signaldata/TuxContinueB.hpp>
#include <signaldata/TupFrag.hpp>
#include <signaldata/AlterIndx.hpp>
#include <signaldata/DropTab.hpp>
#include <signaldata/TuxMaint.hpp>
#include <signaldata/AccScan.hpp>
#include <signaldata/TuxBound.hpp>
#include <signaldata/NextScan.hpp>
#include <signaldata/AccLock.hpp>
#include <signaldata/DumpStateOrd.hpp>

// debug
#ifdef VM_TRACE
#include <NdbOut.hpp>
#include <OutputStream.hpp>
#endif

// jams
#undef jam
#undef jamEntry
#ifdef DBTUX_GEN_CPP
#define jam()           jamLine(10000 + __LINE__)
#define jamEntry()      jamEntryLine(10000 + __LINE__)
#endif
#ifdef DBTUX_META_CPP
#define jam()           jamLine(20000 + __LINE__)
#define jamEntry()      jamEntryLine(20000 + __LINE__)
#endif
#ifdef DBTUX_MAINT_CPP
#define jam()           jamLine(30000 + __LINE__)
#define jamEntry()      jamEntryLine(30000 + __LINE__)
#endif
#ifdef DBTUX_NODE_CPP
#define jam()           jamLine(40000 + __LINE__)
#define jamEntry()      jamEntryLine(40000 + __LINE__)
#endif
#ifdef DBTUX_TREE_CPP
#define jam()           jamLine(50000 + __LINE__)
#define jamEntry()      jamEntryLine(50000 + __LINE__)
#endif
#ifdef DBTUX_SCAN_CPP
#define jam()           jamLine(60000 + __LINE__)
#define jamEntry()      jamEntryLine(60000 + __LINE__)
#endif
#ifdef DBTUX_SEARCH_CPP
#define jam()           jamLine(70000 + __LINE__)
#define jamEntry()      jamEntryLine(70000 + __LINE__)
#endif
#ifdef DBTUX_CMP_CPP
#define jam()           jamLine(80000 + __LINE__)
#define jamEntry()      jamEntryLine(80000 + __LINE__)
#endif
#ifdef DBTUX_STAT_CPP
#define jam()           jamLine(90000 + __LINE__)
#define jamEntry()      jamEntryLine(90000 + __LINE__)
#endif
#ifdef DBTUX_DEBUG_CPP
#define jam()           jamLine(100000 + __LINE__)
#define jamEntry()      jamEntryLine(100000 + __LINE__)
#endif
#ifndef jam
#define jam()           jamLine(__LINE__)
#define jamEntry()      jamEntryLine(__LINE__)
#endif

#undef max
#undef min

class Configuration;

class Dbtux : public SimulatedBlock {
public:
  Dbtux(Block_context& ctx);
  virtual ~Dbtux();

  // pointer to TUP instance in this thread
  Dbtup* c_tup;

private:
  // sizes are in words (Uint32)
  STATIC_CONST( MaxIndexFragments = MAX_FRAG_PER_NODE );
  STATIC_CONST( MaxIndexAttributes = MAX_ATTRIBUTES_IN_INDEX );
  STATIC_CONST( MaxAttrDataSize = 2048 );
public:
  STATIC_CONST( DescPageSize = 256 );
private:
  STATIC_CONST( MaxTreeNodeSize = MAX_TTREE_NODE_SIZE );
  STATIC_CONST( MaxPrefSize = MAX_TTREE_PREF_SIZE );
  STATIC_CONST( ScanBoundSegmentSize = 7 );
  STATIC_CONST( MaxAccLockOps = MAX_PARALLEL_OP_PER_SCAN );
  STATIC_CONST( MaxTreeDepth = 32 );    // strict
  BLOCK_DEFINES(Dbtux);

  // forward declarations
  struct DescEnt;

  // Pointer to array of Uint32 represents attribute data and bounds

  typedef Uint32 *Data;
  inline AttributeHeader& ah(Data data) {
    return *reinterpret_cast<AttributeHeader*>(data);
  }

  typedef const Uint32* ConstData;
  inline const AttributeHeader& ah(ConstData data) {
    return *reinterpret_cast<const AttributeHeader*>(data);
  }

  // AttributeHeader size is assumed to be 1 word
  STATIC_CONST( AttributeHeaderSize = 1 );

  /*
   * Logical tuple address, "local key".  Identifies table tuples.
   */
  typedef Uint32 TupAddr;
  STATIC_CONST( NullTupAddr = (Uint32)-1 );

  /*
   * Physical tuple address in TUP.  Provides fast access to table tuple
   * or index node.  Valid within the db node and across timeslices.
   * Not valid between db nodes or across restarts.
   *
   * To avoid wasting an Uint16 the pageid is split in two.
   */
  struct TupLoc {
  private:
    Uint16 m_pageId1;           // page i-value (big-endian)
    Uint16 m_pageId2;
    Uint16 m_pageOffset;        // page offset in words
  public:
    TupLoc();
    TupLoc(Uint32 pageId, Uint16 pageOffset);
    Uint32 getPageId() const;
    void setPageId(Uint32 pageId);
    Uint32 getPageOffset() const;
    void setPageOffset(Uint32 pageOffset);
    bool operator==(const TupLoc& loc) const;
    bool operator!=(const TupLoc& loc) const;
  };

  /*
   * There is no const member NullTupLoc since the compiler may not be
   * able to optimize it to TupLoc() constants.  Instead null values are
   * constructed on the stack with TupLoc().
   */
#define NullTupLoc TupLoc()

  // tree definitions

  /*
   * Tree entry.  Points to a tuple in primary table via physical
   * address of "original" tuple and tuple version.
   *
   * ZTUP_VERSION_BITS must be 15 (or less).
   */
  struct TreeEnt;
  friend struct TreeEnt;
  struct TreeEnt {
    TupLoc m_tupLoc;            // address of original tuple
    unsigned m_tupVersion : 15; // version
    TreeEnt();
    // methods
    bool eqtuple(const TreeEnt ent) const;
    bool eq(const TreeEnt ent) const;
    int cmp(const TreeEnt ent) const;
  };
  STATIC_CONST( TreeEntSize = sizeof(TreeEnt) >> 2 );
  static const TreeEnt NullTreeEnt;

  /*
   * Tree node has 1) fixed part 2) a prefix of index key data for min
   * entry 3) max and min entries 4) rest of entries 5) one extra entry
   * used as work space.
   *
   * struct TreeNode            part 1, size 6 words
   * min prefix                 part 2, size TreeHead::m_prefSize
   * max entry                  part 3
   * min entry                  part 3
   * rest of entries            part 4
   * work entry                 part 5
   *
   * There are 3 links to other nodes: left child, right child, parent.
   * Occupancy (number of entries) is at least 1 except temporarily when
   * a node is about to be removed.
   */
  struct TreeNode;
  friend struct TreeNode;
  struct TreeNode {
    TupLoc m_link[3];           // link to 0-left child 1-right child 2-parent
    unsigned m_side : 2;        // we are 0-left child 1-right child 2-root
    unsigned m_balance : 2;     // balance -1, 0, +1 plus 1 for Solaris CC
    unsigned pad1 : 4;
    Uint8 m_occup;              // current number of entries
    Uint32 m_nodeScan;          // list of scans at this node
    TreeNode();
  };
  STATIC_CONST( NodeHeadSize = sizeof(TreeNode) >> 2 );

  /*
   * Tree node "access size" was for an early version with signal
   * interface to TUP.  It is now used only to compute sizes.
   */
  enum AccSize {
    AccNone = 0,
    AccHead = 1,                // part 1
    AccPref = 2,                // parts 1-3
    AccFull = 3                 // parts 1-5
  };

  /*
   * Tree header.  There is one in each fragment.  Contains tree
   * parameters and address of root node.
   */
  struct TreeHead;
  friend struct TreeHead;
  struct TreeHead {
    Uint8 m_nodeSize;           // words in tree node
    Uint8 m_prefSize;           // words in min prefix
    Uint8 m_minOccup;           // min entries in internal node
    Uint8 m_maxOccup;           // max entries in node
    Uint32 m_entryCount;        // stat: current entries
    TupLoc m_root;              // root node
    TreeHead();
    // methods
    unsigned getSize(AccSize acc) const;
    Data getPref(TreeNode* node) const;
    TreeEnt* getEntList(TreeNode* node) const;
  };

  /*
   * Tree position.  Specifies node, position within node (from 0 to
   * m_occup), and whether the position is at an existing entry or
   * before one (if any).  Position m_occup points past the node and is
   * also represented by position 0 of next node.  Includes direction
   * used by scan.
   */
  struct TreePos;
  friend struct TreePos;
  struct TreePos {
    TupLoc m_loc;               // physical node address
    Uint16 m_pos;               // position 0 to m_occup
    Uint8 m_dir;                // see scanNext
    TreePos();
  };

  // packed metadata

  /*
   * Descriptor page.  The "hot" metadata for an index is stored as
   * a contiguous array of words on some page.
   */
  struct DescPage;
  friend struct DescPage;
  struct DescPage {
    Uint32 m_nextPage;
    Uint32 m_numFree;           // number of free words
    union {
    Uint32 m_data[DescPageSize];
    Uint32 nextPool;
    };
    DescPage();
  };
  typedef Ptr<DescPage> DescPagePtr;
  ArrayPool<DescPage> c_descPagePool;
  Uint32 c_descPageList;

  /*
   * Header for index metadata.  Size must be multiple of word size.
   */
  struct DescHead {
    unsigned m_indexId : 24;
    unsigned pad1 : 8;
  };
  STATIC_CONST( DescHeadSize = sizeof(DescHead) >> 2 );

  /*
   * Attribute metadata.  Size must be multiple of word size.
   *
   * Prefix comparison of char data must use strxfrm and binary
   * comparison.  The charset is currently unused.
   */
  struct DescAttr {
    Uint32 m_attrDesc;          // standard AttributeDescriptor
    Uint16 m_primaryAttrId;
    unsigned m_typeId : 6;
    unsigned m_charset : 10;
  };
  STATIC_CONST( DescAttrSize = sizeof(DescAttr) >> 2 );

  /*
   * Complete metadata for one index. The array of attributes has
   * variable size.
   */
  friend struct DescEnt;
  struct DescEnt {
    DescHead m_descHead;
    DescAttr m_descAttr[1];     // variable size data
  };

  // range scan
 
  /*
   * Scan bounds are stored in linked list of segments.
   */
  typedef DataBuffer<ScanBoundSegmentSize> ScanBound;
  typedef DataBuffer<ScanBoundSegmentSize>::ConstDataBufferIterator ScanBoundIterator;
  typedef DataBuffer<ScanBoundSegmentSize>::DataBufferPool ScanBoundPool;
  ScanBoundPool c_scanBoundPool;

  // ScanLock
  struct ScanLock {
    Uint32 m_accLockOp;
    union {
    Uint32 nextPool;
    Uint32 nextList;
    };
    Uint32 prevList;
  };
  typedef Ptr<ScanLock> ScanLockPtr;
  ArrayPool<ScanLock> c_scanLockPool;
 
  /*
   * Scan operation.
   *
   * Tuples are locked one at a time.  The current lock op is set to
   * RNIL as soon as the lock is obtained and passed to LQH.  We must
   * however remember all locks which LQH has not returned for unlocking
   * since they must be aborted by us when the scan is closed.
   *
   * Scan state describes the entry we are interested in.  There is
   * a separate lock wait flag.  It may be for current entry or it may
   * be for an entry we were moved away from.  In any case nothing
   * happens with current entry before lock wait flag is cleared.
   *
   * An unfinished scan is always linked to some tree node, and has
   * current position and direction (see comments at scanNext).  There
   * is also a copy of latest entry found.
   */
  struct ScanOp;
  friend struct ScanOp;
  struct ScanOp {
    enum {
      Undef = 0,
      First = 1,                // before first entry
      Current = 2,              // at some entry
      Found = 3,                // return current as next scan result
      Blocked = 4,              // found and waiting for ACC lock
      Locked = 5,               // found and locked or no lock needed
      Next = 6,                 // looking for next extry
      Last = 7,                 // after last entry
      Aborting = 8,             // lock wait at scan close
      Invalid = 9               // cannot return REF to LQH currently
    };
    Uint16 m_state;
    Uint16 m_lockwait;
    Uint32 m_userPtr;           // scanptr.i in LQH
    Uint32 m_userRef;
    Uint32 m_tableId;
    Uint32 m_indexId;
    Uint32 m_fragId;
    Uint32 m_fragPtrI;
    Uint32 m_transId1;
    Uint32 m_transId2;
    Uint32 m_savePointId;
    // lock waited for or obtained and not yet passed to LQH
    Uint32 m_accLockOp;
    // locks obtained and passed to LQH but not yet returned by LQH
    DLFifoList<ScanLock>::Head m_accLockOps;
    Uint8 m_readCommitted;      // no locking
    Uint8 m_lockMode;
    Uint8 m_descending;
    ScanBound m_boundMin;
    ScanBound m_boundMax;
    ScanBound* m_bound[2];      // pointers to above 2
    Uint16 m_boundCnt[2];       // number of bounds in each
    TreePos m_scanPos;          // position
    TreeEnt m_scanEnt;          // latest entry found
    Uint32 m_nodeScan;          // next scan at node (single-linked)
    union {
    Uint32 nextPool;
    Uint32 nextList;
    };
    Uint32 prevList;
    ScanOp(ScanBoundPool& scanBoundPool);
  };
  typedef Ptr<ScanOp> ScanOpPtr;
  ArrayPool<ScanOp> c_scanOpPool;

  // indexes and fragments

  /*
   * Ordered index.  Top level data structure.  The primary table (table
   * being indexed) lives in TUP.
   */
  struct Index;
  friend struct Index;
  struct Index {
    enum State {
      NotDefined = 0,
      Defining = 1,
      Online = 2,               // triggers activated and build done
      Dropping = 9
    };
    State m_state;
    DictTabInfo::TableType m_tableType;
    Uint32 m_tableId;
    Uint16 unused;
    Uint16 m_numFrags;
    Uint32 m_fragId[MaxIndexFragments];
    Uint32 m_fragPtrI[MaxIndexFragments];
    Uint32 m_descPage;          // descriptor page
    Uint16 m_descOff;           // offset within the page
    Uint16 m_numAttrs;
    bool m_storeNullKey;
    union {
    Uint32 nextPool;
    };
    Index();
  };
  typedef Ptr<Index> IndexPtr;
  ArrayPool<Index> c_indexPool;

  /*
   * Fragment of an index, as known to DIH/TC.  Represents the two
   * duplicate fragments known to LQH/ACC/TUP.  Includes tree header.
   * There are no maintenance operation records yet.
   */
  struct Frag;
  friend struct Frag;
  struct Frag {
    Uint32 m_tableId;           // copy from index level
    Uint32 m_indexId;
    Uint16 unused;
    Uint16 m_fragId;
    Uint32 m_descPage;          // copy from index level
    Uint16 m_descOff;
    Uint16 m_numAttrs;
    bool m_storeNullKey;
    TreeHead m_tree;
    TupLoc m_freeLoc;           // list of free index nodes
    DLList<ScanOp> m_scanList;  // current scans on this fragment
    Uint32 m_tupIndexFragPtrI;
    Uint32 m_tupTableFragPtrI;
    Uint32 m_accTableFragPtrI;
    union {
    Uint32 nextPool;
    };
    Frag(ArrayPool<ScanOp>& scanOpPool);
  };
  typedef Ptr<Frag> FragPtr;
  ArrayPool<Frag> c_fragPool;

  /*
   * Fragment metadata operation.
   */
  struct FragOp {
    Uint32 m_userPtr;
    Uint32 m_userRef;
    Uint32 m_indexId;
    Uint32 m_fragId;
    Uint32 m_fragPtrI;
    Uint32 m_fragNo;            // fragment number starting at zero
    Uint32 m_numAttrsRecvd;
    union {
    Uint32 nextPool;
    };
    FragOp();
  };
  typedef Ptr<FragOp> FragOpPtr;
  ArrayPool<FragOp> c_fragOpPool;

  // node handles

  /*
   * A node handle is a reference to a tree node in TUP.  It is used to
   * operate on the node.  Node handles are allocated on the stack.
   */
  struct NodeHandle;
  friend struct NodeHandle;
  struct NodeHandle {
    Frag& m_frag;               // fragment using the node
    TupLoc m_loc;               // physical node address
    TreeNode* m_node;           // pointer to node storage
    NodeHandle(Frag& frag);
    NodeHandle(const NodeHandle& node);
    NodeHandle& operator=(const NodeHandle& node);
    // check if unassigned
    bool isNull();
    // getters
    TupLoc getLink(unsigned i);
    unsigned getChilds();       // cannot spell
    unsigned getSide();
    unsigned getOccup();
    int getBalance();
    Uint32 getNodeScan();
    // setters
    void setLink(unsigned i, TupLoc loc);
    void setSide(unsigned i);
    void setOccup(unsigned n);
    void setBalance(int b);
    void setNodeScan(Uint32 scanPtrI);
    // access other parts of the node
    Data getPref();
    TreeEnt getEnt(unsigned pos);
    TreeEnt getMinMax(unsigned i);
    // for ndbrequire and ndbassert
    void progError(int line, int cause, const char* file);
  };

  // methods

  /*
   * DbtuxGen.cpp
   */
  void execCONTINUEB(Signal* signal);
  void execSTTOR(Signal* signal);
  void execREAD_CONFIG_REQ(Signal* signal);
  // utils
  void setKeyAttrs(const Frag& frag);
  void readKeyAttrs(const Frag& frag, TreeEnt ent, unsigned start, Data keyData);
  void readTablePk(const Frag& frag, TreeEnt ent, Data pkData, unsigned& pkSize);
  void copyAttrs(const Frag& frag, ConstData data1, Data data2, unsigned maxlen2 = MaxAttrDataSize);
  void unpackBound(const ScanBound& bound, Data data);

  /*
   * DbtuxMeta.cpp
   */
  void execTUXFRAGREQ(Signal* signal);
  void execTUX_ADD_ATTRREQ(Signal* signal);
  void execALTER_INDX_REQ(Signal* signal);
  void execDROP_TAB_REQ(Signal* signal);
  bool allocDescEnt(IndexPtr indexPtr);
  void freeDescEnt(IndexPtr indexPtr);
  void abortAddFragOp(Signal* signal);
  void dropIndex(Signal* signal, IndexPtr indexPtr, Uint32 senderRef, Uint32 senderData);

  /*
   * DbtuxMaint.cpp
   */
  void execTUX_MAINT_REQ(Signal* signal);
  
  /*
   * DbtuxNode.cpp
   */
  int allocNode(Signal* signal, NodeHandle& node);
  void selectNode(NodeHandle& node, TupLoc loc);
  void insertNode(NodeHandle& node);
  void deleteNode(NodeHandle& node);
  void setNodePref(NodeHandle& node);
  // node operations
  void nodePushUp(NodeHandle& node, unsigned pos, const TreeEnt& ent, Uint32 scanList);
  void nodePushUpScans(NodeHandle& node, unsigned pos);
  void nodePopDown(NodeHandle& node, unsigned pos, TreeEnt& en, Uint32* scanList);
  void nodePopDownScans(NodeHandle& node, unsigned pos);
  void nodePushDown(NodeHandle& node, unsigned pos, TreeEnt& ent, Uint32& scanList);
  void nodePushDownScans(NodeHandle& node, unsigned pos);
  void nodePopUp(NodeHandle& node, unsigned pos, TreeEnt& ent, Uint32 scanList);
  void nodePopUpScans(NodeHandle& node, unsigned pos);
  void nodeSlide(NodeHandle& dstNode, NodeHandle& srcNode, unsigned cnt, unsigned i);
  // scans linked to node
  void addScanList(NodeHandle& node, unsigned pos, Uint32 scanList);
  void removeScanList(NodeHandle& node, unsigned pos, Uint32& scanList);
  void moveScanList(NodeHandle& node, unsigned pos);
  void linkScan(NodeHandle& node, ScanOpPtr scanPtr);
  void unlinkScan(NodeHandle& node, ScanOpPtr scanPtr);
  bool islinkScan(NodeHandle& node, ScanOpPtr scanPtr);

  /*
   * DbtuxTree.cpp
   */
  // add entry
  void treeAdd(Frag& frag, TreePos treePos, TreeEnt ent);
  void treeAddFull(Frag& frag, NodeHandle lubNode, unsigned pos, TreeEnt ent);
  void treeAddNode(Frag& frag, NodeHandle lubNode, unsigned pos, TreeEnt ent, NodeHandle parentNode, unsigned i);
  void treeAddRebalance(Frag& frag, NodeHandle node, unsigned i);
  // remove entry
  void treeRemove(Frag& frag, TreePos treePos);
  void treeRemoveInner(Frag& frag, NodeHandle lubNode, unsigned pos);
  void treeRemoveSemi(Frag& frag, NodeHandle node, unsigned i);
  void treeRemoveLeaf(Frag& frag, NodeHandle node);
  void treeRemoveNode(Frag& frag, NodeHandle node);
  void treeRemoveRebalance(Frag& frag, NodeHandle node, unsigned i);
  // rotate
  void treeRotateSingle(Frag& frag, NodeHandle& node, unsigned i);
  void treeRotateDouble(Frag& frag, NodeHandle& node, unsigned i);

  /*
   * DbtuxScan.cpp
   */
  void execACC_SCANREQ(Signal* signal);
  void execTUX_BOUND_INFO(Signal* signal);
  void execNEXT_SCANREQ(Signal* signal);
  void execACC_CHECK_SCAN(Signal* signal);
  void execACCKEYCONF(Signal* signal);
  void execACCKEYREF(Signal* signal);
  void execACC_ABORTCONF(Signal* signal);
  void scanFirst(ScanOpPtr scanPtr);
  void scanFind(ScanOpPtr scanPtr);
  void scanNext(ScanOpPtr scanPtr, bool fromMaintReq);
  bool scanCheck(ScanOpPtr scanPtr, TreeEnt ent);
  bool scanVisible(ScanOpPtr scanPtr, TreeEnt ent);
  void scanClose(Signal* signal, ScanOpPtr scanPtr);
  void abortAccLockOps(Signal* signal, ScanOpPtr scanPtr);
  void addAccLockOp(ScanOpPtr scanPtr, Uint32 accLockOp);
  void removeAccLockOp(ScanOpPtr scanPtr, Uint32 accLockOp);
  void releaseScanOp(ScanOpPtr& scanPtr);

  /*
   * DbtuxSearch.cpp
   */
  bool searchToAdd(Frag& frag, ConstData searchKey, TreeEnt searchEnt, TreePos& treePos);
  bool searchToRemove(Frag& frag, ConstData searchKey, TreeEnt searchEnt, TreePos& treePos);
  void searchToScan(Frag& frag, ConstData boundInfo, unsigned boundCount, bool descending, TreePos& treePos);
  void searchToScanAscending(Frag& frag, ConstData boundInfo, unsigned boundCount, TreePos& treePos);
  void searchToScanDescending(Frag& frag, ConstData boundInfo, unsigned boundCount, TreePos& treePos);

  /*
   * DbtuxCmp.cpp
   */
  int cmpSearchKey(const Frag& frag, unsigned& start, ConstData searchKey, ConstData entryData, unsigned maxlen = MaxAttrDataSize);
  int cmpScanBound(const Frag& frag, unsigned dir, ConstData boundInfo, unsigned boundCount, ConstData entryData, unsigned maxlen = MaxAttrDataSize);

  /*
   * DbtuxStat.cpp
   */
  void execREAD_PSEUDO_REQ(Signal* signal);
  void statRecordsInRange(ScanOpPtr scanPtr, Uint32* out);
  Uint32 getEntriesBeforeOrAfter(Frag& frag, TreePos pos, unsigned idir);
  unsigned getPathToNode(NodeHandle node, Uint16* path);

  /*
   * DbtuxDebug.cpp
   */
  void execDUMP_STATE_ORD(Signal* signal);
#ifdef VM_TRACE
  struct PrintPar {
    char m_path[100];           // LR prefix
    unsigned m_side;            // expected side
    TupLoc m_parent;            // expected parent address
    int m_depth;                // returned depth
    unsigned m_occup;           // returned occupancy
    TreeEnt m_minmax[2];        // returned subtree min and max
    bool m_ok;                  // returned status
    PrintPar();
  };
  void printTree(Signal* signal, Frag& frag, NdbOut& out);
  void printNode(Frag& frag, NdbOut& out, TupLoc loc, PrintPar& par);
  friend class NdbOut& operator<<(NdbOut&, const TupLoc&);
  friend class NdbOut& operator<<(NdbOut&, const TreeEnt&);
  friend class NdbOut& operator<<(NdbOut&, const TreeNode&);
  friend class NdbOut& operator<<(NdbOut&, const TreeHead&);
  friend class NdbOut& operator<<(NdbOut&, const TreePos&);
  friend class NdbOut& operator<<(NdbOut&, const DescAttr&);
  friend class NdbOut& operator<<(NdbOut&, const ScanOp&);
  friend class NdbOut& operator<<(NdbOut&, const Index&);
  friend class NdbOut& operator<<(NdbOut&, const Frag&);
  friend class NdbOut& operator<<(NdbOut&, const FragOp&);
  friend class NdbOut& operator<<(NdbOut&, const NodeHandle&);
  FILE* debugFile;
  NdbOut debugOut;
  unsigned debugFlags;
  enum {
    DebugMeta = 1,              // log create and drop index
    DebugMaint = 2,             // log maintenance ops
    DebugTree = 4,              // log and check tree after each op
    DebugScan = 8,              // log scans
    DebugLock = 16              // log ACC locks
  };
  STATIC_CONST( DataFillByte = 0xa2 );
  STATIC_CONST( NodeFillByte = 0xa4 );
#endif

  // start up info
  Uint32 c_internalStartPhase;
  Uint32 c_typeOfStart;

  /*
   * Global data set at operation start.  Unpacked from index metadata.
   * Not passed as parameter to methods.  Invalid across timeslices.
   *
   * TODO inline all into index metadata
   */

  // index key attr ids with sizes in AttributeHeader format
  Data c_keyAttrs;

  // pointers to index key comparison functions
  NdbSqlUtil::Cmp** c_sqlCmp;

  /*
   * Other buffers used during the operation.
   */

  // buffer for search key data with headers
  Data c_searchKey;

  // buffer for current entry key data with headers
  Data c_entryKey;

  // buffer for scan bounds and keyinfo (primary key)
  Data c_dataBuffer;

  // inlined utils
  DescEnt& getDescEnt(Uint32 descPage, Uint32 descOff);
  Uint32 getTupAddr(const Frag& frag, TreeEnt ent);
  static unsigned min(unsigned x, unsigned y);
  static unsigned max(unsigned x, unsigned y);
};

// Dbtux::TupLoc

inline
Dbtux::TupLoc::TupLoc() :
  m_pageId1(RNIL >> 16),
  m_pageId2(RNIL & 0xFFFF),
  m_pageOffset(0)
{
}

inline
Dbtux::TupLoc::TupLoc(Uint32 pageId, Uint16 pageOffset) :
  m_pageId1(pageId >> 16),
  m_pageId2(pageId & 0xFFFF),
  m_pageOffset(pageOffset)
{
}

inline Uint32
Dbtux::TupLoc::getPageId() const
{
  return (m_pageId1 << 16) | m_pageId2;
}

inline void
Dbtux::TupLoc::setPageId(Uint32 pageId)
{
  m_pageId1 = (pageId >> 16);
  m_pageId2 = (pageId & 0xFFFF);
}

inline Uint32
Dbtux::TupLoc::getPageOffset() const
{
  return (Uint32)m_pageOffset;
}

inline void
Dbtux::TupLoc::setPageOffset(Uint32 pageOffset)
{
  m_pageOffset = (Uint16)pageOffset;
}

inline bool
Dbtux::TupLoc::operator==(const TupLoc& loc) const
{
  return
    m_pageId1 == loc.m_pageId1 &&
    m_pageId2 == loc.m_pageId2 &&
    m_pageOffset == loc.m_pageOffset;
}

inline bool
Dbtux::TupLoc::operator!=(const TupLoc& loc) const
{
  return ! (*this == loc);
}

// Dbtux::TreeEnt

inline
Dbtux::TreeEnt::TreeEnt() :
  m_tupLoc(),
  m_tupVersion(0)
{
}

inline bool
Dbtux::TreeEnt::eqtuple(const TreeEnt ent) const
{
  return
    m_tupLoc == ent.m_tupLoc;
}

inline bool
Dbtux::TreeEnt::eq(const TreeEnt ent) const
{
  return
    m_tupLoc == ent.m_tupLoc &&
    m_tupVersion == ent.m_tupVersion;
}

inline int
Dbtux::TreeEnt::cmp(const TreeEnt ent) const
{
  if (m_tupLoc.getPageId() < ent.m_tupLoc.getPageId())
    return -1;
  if (m_tupLoc.getPageId() > ent.m_tupLoc.getPageId())
    return +1;
  if (m_tupLoc.getPageOffset() < ent.m_tupLoc.getPageOffset())
    return -1;
  if (m_tupLoc.getPageOffset() > ent.m_tupLoc.getPageOffset())
    return +1;
  /*
   * Guess if one tuple version has wrapped around.  This is well
   * defined ordering on existing versions since versions are assigned
   * consecutively and different versions exists only on uncommitted
   * tuple.  Assuming max 2**14 uncommitted ops on same tuple.
   */
  const unsigned version_wrap_limit = (1 << (ZTUP_VERSION_BITS - 1));
  if (m_tupVersion < ent.m_tupVersion) {
    if (ent.m_tupVersion - m_tupVersion < version_wrap_limit)
      return -1;
    else
      return +1;
  }
  if (m_tupVersion > ent.m_tupVersion) {
    if (m_tupVersion - ent.m_tupVersion < version_wrap_limit)
      return +1;
    else
      return -1;
  }
  return 0;
}

// Dbtux::TreeNode

inline
Dbtux::TreeNode::TreeNode() :
  m_side(2),
  m_balance(0 + 1),
  pad1(0),
  m_occup(0),
  m_nodeScan(RNIL)
{
  m_link[0] = NullTupLoc;
  m_link[1] = NullTupLoc;
  m_link[2] = NullTupLoc;
}

// Dbtux::TreeHead

inline
Dbtux::TreeHead::TreeHead() :
  m_nodeSize(0),
  m_prefSize(0),
  m_minOccup(0),
  m_maxOccup(0),
  m_entryCount(0),
  m_root()
{
}

inline unsigned
Dbtux::TreeHead::getSize(AccSize acc) const
{
  switch (acc) {
  case AccNone:
    return 0;
  case AccHead:
    return NodeHeadSize;
  case AccPref:
    return NodeHeadSize + m_prefSize + 2 * TreeEntSize;
  case AccFull:
    return m_nodeSize;
  }
  return 0;
}

inline Dbtux::Data
Dbtux::TreeHead::getPref(TreeNode* node) const
{
  Uint32* ptr = (Uint32*)node + NodeHeadSize;
  return ptr;
}

inline Dbtux::TreeEnt*
Dbtux::TreeHead::getEntList(TreeNode* node) const
{
  Uint32* ptr = (Uint32*)node + NodeHeadSize + m_prefSize;
  return (TreeEnt*)ptr;
}

// Dbtux::TreePos

inline
Dbtux::TreePos::TreePos() :
  m_loc(),
  m_pos(ZNIL),
  m_dir(255)
{
}

// Dbtux::DescPage

inline
Dbtux::DescPage::DescPage() :
  m_nextPage(RNIL),
  m_numFree(ZNIL)
{
  for (unsigned i = 0; i < DescPageSize; i++) {
#ifdef VM_TRACE
    m_data[i] = 0x13571357;
#else
    m_data[i] = 0;
#endif
  }
}

// Dbtux::ScanOp

inline
Dbtux::ScanOp::ScanOp(ScanBoundPool& scanBoundPool) :
  m_state(Undef),
  m_lockwait(false),
  m_userPtr(RNIL),
  m_userRef(RNIL),
  m_tableId(RNIL),
  m_indexId(RNIL),
  m_fragPtrI(RNIL),
  m_transId1(0),
  m_transId2(0),
  m_savePointId(0),
  m_accLockOp(RNIL),
  m_accLockOps(),
  m_readCommitted(0),
  m_lockMode(0),
  m_descending(0),
  m_boundMin(scanBoundPool),
  m_boundMax(scanBoundPool),
  m_scanPos(),
  m_scanEnt(),
  m_nodeScan(RNIL)
{
  m_bound[0] = &m_boundMin;
  m_bound[1] = &m_boundMax;
  m_boundCnt[0] = 0;
  m_boundCnt[1] = 0;
}

// Dbtux::Index

inline
Dbtux::Index::Index() :
  m_state(NotDefined),
  m_tableType(DictTabInfo::UndefTableType),
  m_tableId(RNIL),
  m_numFrags(0),
  m_descPage(RNIL),
  m_descOff(0),
  m_numAttrs(0),
  m_storeNullKey(false)
{
  for (unsigned i = 0; i < MaxIndexFragments; i++) {
    m_fragId[i] = ZNIL;
    m_fragPtrI[i] = RNIL;
  };
}

// Dbtux::Frag

inline
Dbtux::Frag::Frag(ArrayPool<ScanOp>& scanOpPool) :
  m_tableId(RNIL),
  m_indexId(RNIL),
  m_fragId(ZNIL),
  m_descPage(RNIL),
  m_descOff(0),
  m_numAttrs(ZNIL),
  m_storeNullKey(false),
  m_tree(),
  m_freeLoc(),
  m_scanList(scanOpPool),
  m_tupIndexFragPtrI(RNIL)
{
  m_tupTableFragPtrI = RNIL;
  m_accTableFragPtrI = RNIL;
}

// Dbtux::FragOp

inline
Dbtux::FragOp::FragOp() :
  m_userPtr(RNIL),
  m_userRef(RNIL),
  m_indexId(RNIL),
  m_fragId(ZNIL),
  m_fragPtrI(RNIL),
  m_fragNo(ZNIL),
  m_numAttrsRecvd(ZNIL)
{
}

// Dbtux::NodeHandle

inline
Dbtux::NodeHandle::NodeHandle(Frag& frag) :
  m_frag(frag),
  m_loc(),
  m_node(0)
{
}

inline
Dbtux::NodeHandle::NodeHandle(const NodeHandle& node) :
  m_frag(node.m_frag),
  m_loc(node.m_loc),
  m_node(node.m_node)
{
}

inline Dbtux::NodeHandle&
Dbtux::NodeHandle::operator=(const NodeHandle& node)
{
  ndbassert(&m_frag == &node.m_frag);
  m_loc = node.m_loc;
  m_node = node.m_node;
  return *this;
}

inline bool
Dbtux::NodeHandle::isNull()
{
  return m_node == 0;
}

inline Dbtux::TupLoc
Dbtux::NodeHandle::getLink(unsigned i)
{
  ndbrequire(i <= 2);
  return m_node->m_link[i];
}

inline unsigned
Dbtux::NodeHandle::getChilds()
{
  return (m_node->m_link[0] != NullTupLoc) + (m_node->m_link[1] != NullTupLoc);
}

inline unsigned
Dbtux::NodeHandle::getSide()
{
  return m_node->m_side;
}

inline unsigned
Dbtux::NodeHandle::getOccup()
{
  return m_node->m_occup;
}

inline int
Dbtux::NodeHandle::getBalance()
{
  return (int)m_node->m_balance - 1;
}

inline Uint32
Dbtux::NodeHandle::getNodeScan()
{
  return m_node->m_nodeScan;
}

inline void
Dbtux::NodeHandle::setLink(unsigned i, TupLoc loc)
{
  ndbrequire(i <= 2);
  m_node->m_link[i] = loc;
}

inline void
Dbtux::NodeHandle::setSide(unsigned i)
{
  ndbrequire(i <= 2);
  m_node->m_side = i;
}

inline void
Dbtux::NodeHandle::setOccup(unsigned n)
{
  TreeHead& tree = m_frag.m_tree;
  ndbrequire(n <= tree.m_maxOccup);
  m_node->m_occup = n;
}

inline void
Dbtux::NodeHandle::setBalance(int b)
{
  ndbrequire(abs(b) <= 1);
  m_node->m_balance = (unsigned)(b + 1);
}

inline void
Dbtux::NodeHandle::setNodeScan(Uint32 scanPtrI)
{
  m_node->m_nodeScan = scanPtrI;
}

inline Dbtux::Data
Dbtux::NodeHandle::getPref()
{
  TreeHead& tree = m_frag.m_tree;
  return tree.getPref(m_node);
}

inline Dbtux::TreeEnt
Dbtux::NodeHandle::getEnt(unsigned pos)
{
  TreeHead& tree = m_frag.m_tree;
  TreeEnt* entList = tree.getEntList(m_node);
  const unsigned occup = m_node->m_occup;
  ndbrequire(pos < occup);
  return entList[(1 + pos) % occup];
}

inline Dbtux::TreeEnt
Dbtux::NodeHandle::getMinMax(unsigned i)
{
  const unsigned occup = m_node->m_occup;
  ndbrequire(i <= 1 && occup != 0);
  return getEnt(i == 0 ? 0 : occup - 1);
}

// parameters for methods

#ifdef VM_TRACE
inline
Dbtux::PrintPar::PrintPar() :
  // caller fills in
  m_path(),
  m_side(255),
  m_parent(),
  // default return values
  m_depth(0),
  m_occup(0),
  m_ok(true)
{
}
#endif

// utils

inline Dbtux::DescEnt&
Dbtux::getDescEnt(Uint32 descPage, Uint32 descOff)
{
  DescPagePtr pagePtr;
  pagePtr.i = descPage;
  c_descPagePool.getPtr(pagePtr);
  ndbrequire(descOff < DescPageSize);
  DescEnt* descEnt = (DescEnt*)&pagePtr.p->m_data[descOff];
  return *descEnt;
}

inline Uint32
Dbtux::getTupAddr(const Frag& frag, TreeEnt ent)
{
  const Uint32 tableFragPtrI = frag.m_tupTableFragPtrI;
  const TupLoc tupLoc = ent.m_tupLoc;
  Uint32 tupAddr = NullTupAddr;
  c_tup->tuxGetTupAddr(tableFragPtrI, tupLoc.getPageId(), tupLoc.getPageOffset(), tupAddr);
  jamEntry();
  return tupAddr;
}

inline unsigned
Dbtux::min(unsigned x, unsigned y)
{
  return x < y ? x : y;
}

inline unsigned
Dbtux::max(unsigned x, unsigned y)
{
  return x > y ? x : y;
}

#endif
