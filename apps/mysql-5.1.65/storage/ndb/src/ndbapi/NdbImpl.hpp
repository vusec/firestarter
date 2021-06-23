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

#ifndef NDB_IMPL_HPP
#define NDB_IMPL_HPP

#include <ndb_global.h>
#include <Ndb.hpp>
#include <NdbOut.hpp>
#include <NdbError.hpp>
#include <NdbCondition.h>
#include <NdbReceiver.hpp>
#include <NdbOperation.hpp>
#include <kernel/ndb_limits.h>

#include <NdbTick.h>

#include "ndb_cluster_connection_impl.hpp"
#include "NdbDictionaryImpl.hpp"
#include "ObjectMap.hpp"

template <class T>
struct Ndb_free_list_t 
{
  Ndb_free_list_t();
  ~Ndb_free_list_t();
  
  int fill(Ndb*, Uint32 cnt);
  T* seize(Ndb*);
  void release(T*);
  void clear();
  Uint32 get_sizeof() const { return sizeof(T); }
  T * m_free_list;
  Uint32 m_alloc_cnt, m_free_cnt;
};

/**
 * Private parts of the Ndb object (corresponding to Ndb.hpp in public API)
 */
class NdbImpl {
public:
  NdbImpl(Ndb_cluster_connection *, Ndb&);
  ~NdbImpl();

  int send_event_report(Uint32 *data, Uint32 length);

  Ndb &m_ndb;

  Ndb_cluster_connection_impl &m_ndb_cluster_connection;
  TransporterFacade *m_transporter_facade;

  NdbDictionaryImpl m_dictionary;

  // Ensure good distribution of connects
  Uint32 theCurrentConnectIndex;
  Ndb_cluster_connection_node_iter m_node_iter;

  NdbObjectIdMap theNdbObjectIdMap;

  Uint32 theNoOfDBnodes; // The number of DB nodes  
  Uint8 theDBnodes[MAX_NDB_NODES]; // The node number of the DB nodes

 // 1 indicates to release all connections to node 
  Uint32 the_release_ind[MAX_NDB_NODES];

  NdbWaiter             theWaiter;

  NdbEventOperationImpl *m_ev_op;

  int m_optimized_node_selection;

  BaseString m_dbname; // Database name
  BaseString m_schemaname; // Schema name

  BaseString m_prefix; // Buffer for preformatted internal name <db>/<schema>/

  int update_prefix()
  {
    if (!m_prefix.assfmt("%s%c%s%c", m_dbname.c_str(), table_name_separator,
                         m_schemaname.c_str(), table_name_separator))
    {
      return -1;
    }
    return 0;
  }

/*
  We need this friend accessor function to work around a HP compiler problem,
  where template class friends are not working.
*/
  static inline void setNdbError(Ndb &ndb,int code){
    ndb.theError.code = code;
    return;
  }

  BaseString m_systemPrefix; // Buffer for preformatted for <sys>/<def>/

  /**
   * NOTE free lists must be _after_ theNdbObjectIdMap take
   *   assure that destructors are run in correct order
   * NOTE these has to be in this specific order to make destructor run in
   *      correct order
   */
  Ndb_free_list_t<NdbRecAttr> theRecAttrIdleList;  
  Ndb_free_list_t<NdbApiSignal> theSignalIdleList;
  Ndb_free_list_t<NdbLabel> theLabelList;
  Ndb_free_list_t<NdbBranch> theBranchList;
  Ndb_free_list_t<NdbSubroutine> theSubroutineList;
  Ndb_free_list_t<NdbCall> theCallList;
  Ndb_free_list_t<NdbBlob> theNdbBlobIdleList;
  Ndb_free_list_t<NdbReceiver> theScanList;
  Ndb_free_list_t<NdbIndexScanOperation> theScanOpIdleList;
  Ndb_free_list_t<NdbOperation>  theOpIdleList;  
  Ndb_free_list_t<NdbIndexOperation> theIndexOpIdleList;
  Ndb_free_list_t<NdbTransaction> theConIdleList; 
};

#ifdef VM_TRACE
#define TRACE_DEBUG(x) ndbout << x << endl;
#else
#define TRACE_DEBUG(x)
#endif

#define CHECK_STATUS_MACRO \
   {if (checkInitState() == -1) { theError.code = 4100; DBUG_RETURN(-1);}}
#define CHECK_STATUS_MACRO_VOID \
   {if (checkInitState() == -1) { theError.code = 4100; DBUG_VOID_RETURN;}}
#define CHECK_STATUS_MACRO_ZERO \
   {if (checkInitState() == -1) { theError.code = 4100; DBUG_RETURN(0);}}
#define CHECK_STATUS_MACRO_NULL \
   {if (checkInitState() == -1) { theError.code = 4100; DBUG_RETURN(NULL);}}

inline
void *
Ndb::int2void(Uint32 val){
  return theImpl->theNdbObjectIdMap.getObject(val);
}

inline
NdbReceiver *
Ndb::void2rec(void* val){
  return (NdbReceiver*)val;
}

inline
NdbTransaction *
Ndb::void2con(void* val){
  return (NdbTransaction*)val;
}

inline
NdbOperation*
Ndb::void2rec_op(void* val){
  return (NdbOperation*)(void2rec(val)->getOwner());
}

inline
NdbIndexOperation*
Ndb::void2rec_iop(void* val){
  return (NdbIndexOperation*)(void2rec(val)->getOwner());
}

inline 
NdbTransaction * 
NdbReceiver::getTransaction(){ 
  return ((NdbOperation*)m_owner)->theNdbCon;
}


inline
int
Ndb::checkInitState()
{
  theError.code = 0;

  if (theInitState != Initialised)
    return -1;
  return 0;
}

Uint32 convertEndian(Uint32 Data);

enum LockMode { 
  Read, 
  Update,
  Insert,
  Delete 
};

template<class T>
inline
Ndb_free_list_t<T>::Ndb_free_list_t()
{
  m_free_list= 0; 
  m_alloc_cnt= m_free_cnt= 0; 
}

template<class T>
inline
Ndb_free_list_t<T>::~Ndb_free_list_t()
{
  clear();
}
    
template<class T>
inline
int
Ndb_free_list_t<T>::fill(Ndb* ndb, Uint32 cnt)
{
  if (m_free_list == 0)
  {
    m_free_cnt++;
    m_alloc_cnt++;
    m_free_list = new T(ndb);
    if (m_free_list == 0)
    {
      NdbImpl::setNdbError(*ndb, 4000);
      assert(false);
      return -1;
    }
  }
  while(m_alloc_cnt < cnt)
  {
    T* obj= new T(ndb);
    if(obj == 0)
    {
      NdbImpl::setNdbError(*ndb, 4000);
      assert(false);
      return -1;
    }
    obj->next(m_free_list);
    m_free_cnt++;
    m_alloc_cnt++;
    m_free_list = obj;
  }
  return 0;
}

template<class T>
inline
T*
Ndb_free_list_t<T>::seize(Ndb* ndb)
{
  T* tmp = m_free_list;
  if (tmp)
  {
    m_free_list = (T*)tmp->next();
    tmp->next(NULL);
    m_free_cnt--;
    return tmp;
  }
  
  if((tmp = new T(ndb)))
  {
    m_alloc_cnt++;
  }
  else
  {
    NdbImpl::setNdbError(*ndb, 4000);
    assert(false);
  }
  return tmp;
}

template<class T>
inline
void
Ndb_free_list_t<T>::release(T* obj)
{
  obj->next(m_free_list);
  m_free_list = obj;
  m_free_cnt++;
}


template<class T>
inline
void
Ndb_free_list_t<T>::clear()
{
  T* obj = m_free_list;
  while(obj)
  {
    T* curr = obj;
    obj = (T*)obj->next();
    delete curr;
    m_alloc_cnt--;
  }
}

#endif
