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

#define DBTUX_GEN_CPP
#include "Dbtux.hpp"

Dbtux::Dbtux(Block_context& ctx) :
  SimulatedBlock(DBTUX, ctx),
  c_tup(0),
  c_descPageList(RNIL),
#ifdef VM_TRACE
  debugFile(0),
  debugOut(*new NullOutputStream()),
  debugFlags(0),
#endif
  c_internalStartPhase(0),
  c_typeOfStart(NodeState::ST_ILLEGAL_TYPE),
  c_dataBuffer(0)
{
  BLOCK_CONSTRUCTOR(Dbtux);
  // verify size assumptions (also when release-compiled)
  ndbrequire(
      (sizeof(TreeEnt) & 0x3) == 0 &&
      (sizeof(TreeNode) & 0x3) == 0 &&
      (sizeof(DescHead) & 0x3) == 0 &&
      (sizeof(DescAttr) & 0x3) == 0
  );
  /*
   * DbtuxGen.cpp
   */
  addRecSignal(GSN_CONTINUEB, &Dbtux::execCONTINUEB);
  addRecSignal(GSN_STTOR, &Dbtux::execSTTOR);
  addRecSignal(GSN_READ_CONFIG_REQ, &Dbtux::execREAD_CONFIG_REQ, true);
  /*
   * DbtuxMeta.cpp
   */
  addRecSignal(GSN_TUXFRAGREQ, &Dbtux::execTUXFRAGREQ);
  addRecSignal(GSN_TUX_ADD_ATTRREQ, &Dbtux::execTUX_ADD_ATTRREQ);
  addRecSignal(GSN_ALTER_INDX_REQ, &Dbtux::execALTER_INDX_REQ);
  addRecSignal(GSN_DROP_TAB_REQ, &Dbtux::execDROP_TAB_REQ);
  /*
   * DbtuxMaint.cpp
   */
  addRecSignal(GSN_TUX_MAINT_REQ, &Dbtux::execTUX_MAINT_REQ);
  /*
   * DbtuxScan.cpp
   */
  addRecSignal(GSN_ACC_SCANREQ, &Dbtux::execACC_SCANREQ);
  addRecSignal(GSN_TUX_BOUND_INFO, &Dbtux::execTUX_BOUND_INFO);
  addRecSignal(GSN_NEXT_SCANREQ, &Dbtux::execNEXT_SCANREQ);
  addRecSignal(GSN_ACC_CHECK_SCAN, &Dbtux::execACC_CHECK_SCAN);
  addRecSignal(GSN_ACCKEYCONF, &Dbtux::execACCKEYCONF);
  addRecSignal(GSN_ACCKEYREF, &Dbtux::execACCKEYREF);
  addRecSignal(GSN_ACC_ABORTCONF, &Dbtux::execACC_ABORTCONF);
  /*
   * DbtuxStat.cpp
   */
  addRecSignal(GSN_READ_PSEUDO_REQ, &Dbtux::execREAD_PSEUDO_REQ);
  /*
   * DbtuxDebug.cpp
   */
  addRecSignal(GSN_DUMP_STATE_ORD, &Dbtux::execDUMP_STATE_ORD);
}

Dbtux::~Dbtux()
{
}

void
Dbtux::execCONTINUEB(Signal* signal)
{
  jamEntry();
  const Uint32* data = signal->getDataPtr();
  switch (data[0]) {
  case TuxContinueB::DropIndex: // currently unused
    {
      IndexPtr indexPtr;
      c_indexPool.getPtr(indexPtr, data[1]);
      dropIndex(signal, indexPtr, data[2], data[3]);
    }
    break;
  default:
    ndbrequire(false);
    break;
  }
}

/*
 * STTOR is sent to one block at a time.  In NDBCNTR it triggers
 * NDB_STTOR to the "old" blocks.  STTOR carries start phase (SP) and
 * NDB_STTOR carries internal start phase (ISP).
 *
 *      SP      ISP     activities
 *      1       none
 *      2       1       
 *      3       2       recover metadata, activate indexes
 *      4       3       recover data
 *      5       4-6     
 *      6       skip    
 *      7       skip    
 *      8       7       build non-logged indexes on SR
 *
 * DBTUX catches type of start (IS, SR, NR, INR) at SP 3 and updates
 * internal start phase at SP 7.  These are used to prevent index
 * maintenance operations caused by redo log at SR.
 */
void
Dbtux::execSTTOR(Signal* signal)
{
  jamEntry();
  Uint32 startPhase = signal->theData[1];
  switch (startPhase) {
  case 1:
    jam();
    CLEAR_ERROR_INSERT_VALUE;
    c_tup = (Dbtup*)globalData.getBlock(DBTUP);
    ndbrequire(c_tup != 0);
    break;
  case 3:
    jam();
    c_typeOfStart = signal->theData[7];
    break;
  case 7:
    c_internalStartPhase = 6;
  default:
    jam();
    break;
  }
  signal->theData[0] = 0;       // garbage
  signal->theData[1] = 0;       // garbage
  signal->theData[2] = 0;       // garbage
  signal->theData[3] = 1;
  signal->theData[4] = 3;       // for c_typeOfStart
  signal->theData[5] = 7;       // for c_internalStartPhase
  signal->theData[6] = 255;
  sendSignal(NDBCNTR_REF, GSN_STTORRY, signal, 7, JBB);
}

void
Dbtux::execREAD_CONFIG_REQ(Signal* signal)
{
  jamEntry();
 
  const ReadConfigReq * req = (ReadConfigReq*)signal->getDataPtr();
  Uint32 ref = req->senderRef;
  Uint32 senderData = req->senderData;
  ndbrequire(req->noOfParameters == 0);

  Uint32 nIndex;
  Uint32 nFragment;
  Uint32 nAttribute;
  Uint32 nScanOp; 
  Uint32 nScanBatch;

  const ndb_mgm_configuration_iterator * p = 
    m_ctx.m_config.getOwnConfigIterator();
  ndbrequire(p != 0);

  ndbrequire(!ndb_mgm_get_int_parameter(p, CFG_TUX_INDEX, &nIndex));
  ndbrequire(!ndb_mgm_get_int_parameter(p, CFG_TUX_FRAGMENT, &nFragment));
  ndbrequire(!ndb_mgm_get_int_parameter(p, CFG_TUX_ATTRIBUTE, &nAttribute));
  ndbrequire(!ndb_mgm_get_int_parameter(p, CFG_TUX_SCAN_OP, &nScanOp));
  ndbrequire(!ndb_mgm_get_int_parameter(p, CFG_DB_BATCH_SIZE, &nScanBatch));

  const Uint32 nDescPage = (nIndex * DescHeadSize + nAttribute * DescAttrSize + DescPageSize - 1) / DescPageSize;
  const Uint32 nScanBoundWords = nScanOp * ScanBoundSegmentSize * 4;
  const Uint32 nScanLock = nScanOp * nScanBatch;
  
  c_indexPool.setSize(nIndex);
  c_fragPool.setSize(nFragment);
  c_descPagePool.setSize(nDescPage);
  c_fragOpPool.setSize(MaxIndexFragments);
  c_scanOpPool.setSize(nScanOp);
  c_scanBoundPool.setSize(nScanBoundWords);
  c_scanLockPool.setSize(nScanLock);
  /*
   * Index id is physical array index.  We seize and initialize all
   * index records now.
   */
  IndexPtr indexPtr;
  while (1) {
    jam();
    refresh_watch_dog();
    c_indexPool.seize(indexPtr);
    if (indexPtr.i == RNIL) {
      jam();
      break;
    }
    new (indexPtr.p) Index();
  }
  // allocate buffers
  c_keyAttrs = (Uint32*)allocRecord("c_keyAttrs", sizeof(Uint32), MaxIndexAttributes);
  c_sqlCmp = (NdbSqlUtil::Cmp**)allocRecord("c_sqlCmp", sizeof(NdbSqlUtil::Cmp*), MaxIndexAttributes);
  c_searchKey = (Uint32*)allocRecord("c_searchKey", sizeof(Uint32), MaxAttrDataSize);
  c_entryKey = (Uint32*)allocRecord("c_entryKey", sizeof(Uint32), MaxAttrDataSize);
  c_dataBuffer = (Uint32*)allocRecord("c_dataBuffer", sizeof(Uint64), (MaxAttrDataSize + 1) >> 1);
  // ack
  ReadConfigConf * conf = (ReadConfigConf*)signal->getDataPtrSend();
  conf->senderRef = reference();
  conf->senderData = senderData;
  sendSignal(ref, GSN_READ_CONFIG_CONF, signal, 
	     ReadConfigConf::SignalLength, JBB);
}

// utils

void
Dbtux::setKeyAttrs(const Frag& frag)
{
  Data keyAttrs = c_keyAttrs; // global
  NdbSqlUtil::Cmp** sqlCmp = c_sqlCmp; // global
  const unsigned numAttrs = frag.m_numAttrs;
  const DescEnt& descEnt = getDescEnt(frag.m_descPage, frag.m_descOff);
  for (unsigned i = 0; i < numAttrs; i++) {
    jam();
    const DescAttr& descAttr = descEnt.m_descAttr[i];
    Uint32 size = AttributeDescriptor::getSizeInWords(descAttr.m_attrDesc);
    // set attr id and fixed size
    ah(keyAttrs) = AttributeHeader(descAttr.m_primaryAttrId, size);
    keyAttrs += 1;
    // set comparison method pointer
    const NdbSqlUtil::Type& sqlType = NdbSqlUtil::getTypeBinary(descAttr.m_typeId);
    ndbrequire(sqlType.m_cmp != 0);
    *(sqlCmp++) = sqlType.m_cmp;
  }
}

void
Dbtux::readKeyAttrs(const Frag& frag, TreeEnt ent, unsigned start, Data keyData)
{
  ConstData keyAttrs = c_keyAttrs; // global
  const Uint32 tableFragPtrI = frag.m_tupTableFragPtrI;
  const TupLoc tupLoc = ent.m_tupLoc;
  const Uint32 tupVersion = ent.m_tupVersion;
  ndbrequire(start < frag.m_numAttrs);
  const Uint32 numAttrs = frag.m_numAttrs - start;
  // skip to start position in keyAttrs only
  keyAttrs += start;
  int ret = c_tup->tuxReadAttrs(tableFragPtrI, tupLoc.getPageId(), tupLoc.getPageOffset(), tupVersion, keyAttrs, numAttrs, keyData);
  jamEntry();
  // TODO handle error
  ndbrequire(ret > 0);
#ifdef VM_TRACE
  if (debugFlags & (DebugMaint | DebugScan)) {
    debugOut << "readKeyAttrs:" << endl;
    ConstData data = keyData;
    Uint32 totalSize = 0;
    for (Uint32 i = start; i < frag.m_numAttrs; i++) {
      Uint32 attrId = ah(data).getAttributeId();
      Uint32 dataSize = ah(data).getDataSize();
      debugOut << i << " attrId=" << attrId << " size=" << dataSize;
      data += 1;
      for (Uint32 j = 0; j < dataSize; j++) {
        debugOut << " " << hex << data[0];
        data += 1;
      }
      debugOut << endl;
      totalSize += 1 + dataSize;
    }
    ndbassert((int)totalSize == ret);
  }
#endif
}

void
Dbtux::readTablePk(const Frag& frag, TreeEnt ent, Data pkData, unsigned& pkSize)
{
  const Uint32 tableFragPtrI = frag.m_tupTableFragPtrI;
  const TupLoc tupLoc = ent.m_tupLoc;
  int ret = c_tup->tuxReadPk(tableFragPtrI, tupLoc.getPageId(), tupLoc.getPageOffset(), pkData, true);
  jamEntry();
  // TODO handle error
  ndbrequire(ret > 0);
  pkSize = ret;
}

/*
 * Copy attribute data with headers.  Input is all index key data.
 * Copies whatever fits.
 */
void
Dbtux::copyAttrs(const Frag& frag, ConstData data1, Data data2, unsigned maxlen2)
{
  unsigned n = frag.m_numAttrs;
  unsigned len2 = maxlen2;
  while (n != 0) {
    jam();
    const unsigned dataSize = ah(data1).getDataSize();
    // copy header
    if (len2 == 0)
      return;
    data2[0] = data1[0];
    data1 += 1;
    data2 += 1;
    len2 -= 1;
    // copy data
    for (unsigned i = 0; i < dataSize; i++) {
      if (len2 == 0)
        return;
      data2[i] = data1[i];
      len2 -= 1;
    }
    data1 += dataSize;
    data2 += dataSize;
    n -= 1;
  }
#ifdef VM_TRACE
  memset(data2, DataFillByte, len2 << 2);
#endif
}

void
Dbtux::unpackBound(const ScanBound& bound, Data dest)
{
  ScanBoundIterator iter;
  bound.first(iter);
  const unsigned n = bound.getSize();
  unsigned j;
  for (j = 0; j < n; j++) {
    dest[j] = *iter.data;
    bound.next(iter);
  }
}

BLOCK_FUNCTIONS(Dbtux)
