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

#include <signaldata/DumpStateOrd.hpp>
#include <NdbBackup.hpp>
#include <NdbOut.hpp>
#include <NDBT_Output.hpp>
#include <NdbConfig.h>
#include <ndb_version.h>
#include <NDBT.hpp>
#include <NdbSleep.h>
#include <random.h>
#include <NdbTick.h>

#define CHECK(b, m) { int _xx = b; if (!(_xx)) { \
  ndbout << "ERR: "<< m \
           << "   " << "File: " << __FILE__ \
           << " (Line: " << __LINE__ << ")" << "- " << _xx << endl; \
  return NDBT_FAILED; } }

#include <ConfigRetriever.hpp>
#include <mgmapi.h>
#include <mgmapi_config_parameters.h>
#include <mgmapi_configuration.hpp>

int 
NdbBackup::start(unsigned int & _backup_id){

  
  if (!isConnected())
    return -1;

  ndb_mgm_reply reply;
  reply.return_code = 0;

  if (ndb_mgm_start_backup(handle,
			   2, // wait until completed
			   &_backup_id,
			   &reply) == -1) {
    g_err << "Error: " << ndb_mgm_get_latest_error(handle) << endl;
    g_err << "Error msg: " << ndb_mgm_get_latest_error_msg(handle) << endl;
    g_err << "Error desc: " << ndb_mgm_get_latest_error_desc(handle) << endl;
    return -1;
  }

  if(reply.return_code != 0){
    g_err  << "PLEASE CHECK CODE NdbBackup.cpp line=" << __LINE__ << endl;
    g_err << "Error: " << ndb_mgm_get_latest_error(handle) << endl;
    g_err << "Error msg: " << ndb_mgm_get_latest_error_msg(handle) << endl;
    g_err << "Error desc: " << ndb_mgm_get_latest_error_desc(handle) << endl;
    return reply.return_code;
  }
  return 0;
}


const char * 
NdbBackup::getBackupDataDirForNode(int _node_id){

  /**
   * Fetch configuration from management server
   */
  ndb_mgm_configuration *p;
  if (connect())
    return NULL;

  if ((p = ndb_mgm_get_configuration(handle, 0)) == 0)
  {
    const char * s= ndb_mgm_get_latest_error_msg(handle);
    if(s == 0)
      s = "No error given!";
      
    ndbout << "Could not fetch configuration" << endl;
    ndbout << s << endl;
    return NULL;
  }
  
  /**
   * Setup cluster configuration data
   */
  ndb_mgm_configuration_iterator iter(* p, CFG_SECTION_NODE);
  if (iter.find(CFG_NODE_ID, _node_id)){
    ndbout << "Invalid configuration fetched, DB missing" << endl;
    return NULL;
  }

  unsigned int type = NODE_TYPE_DB + 1;
  if(iter.get(CFG_TYPE_OF_SECTION, &type) || type != NODE_TYPE_DB){
    ndbout <<"type = " << type << endl;
    ndbout <<"Invalid configuration fetched, I'm wrong type of node" << endl;
    return NULL;
  }  
  
  const char * path;
  if (iter.get(CFG_DB_BACKUP_DATADIR, &path)){
    ndbout << "BackupDataDir not found" << endl;
    return NULL;
  }

  return path;

}

int  
NdbBackup::execRestore(bool _restore_data,
		       bool _restore_meta,
		       int _node_id,
		       unsigned _backup_id){
  const int buf_len = 1000;
  char buf[buf_len];

  ndbout << "getBackupDataDir "<< _node_id <<endl;

  const char* path = getBackupDataDirForNode(_node_id);
  if (path == NULL)
    return -1;  

  ndbout << "getHostName "<< _node_id <<endl;
  const char *host;
  if (!getHostName(_node_id, &host)){
    return -1;
  }

  /* 
   * Copy  backup files to local dir
   */ 

  BaseString::snprintf(buf, buf_len,
	   "scp %s:%s/BACKUP/BACKUP-%d/BACKUP-%d*.%d.* .",
	   host, path,
	   _backup_id,
	   _backup_id,
	   _node_id);

  ndbout << "buf: "<< buf <<endl;
  int res = system(buf);  
  
  ndbout << "scp res: " << res << endl;
  
  BaseString::snprintf(buf, 255, "%sndb_restore -c \"%s:%d\" -n %d -b %d %s %s .", 
#if 1
	   "",
#else
	   "valgrind --leak-check=yes -v "
#endif
	   ndb_mgm_get_connected_host(handle),
	   ndb_mgm_get_connected_port(handle),
	   _node_id, 
	   _backup_id,
	   _restore_data?"-r":"",
	   _restore_meta?"-m":"");

  ndbout << "buf: "<< buf <<endl;
  res = system(buf);

  ndbout << "ndb_restore res: " << res << endl;

  return res;
  
}

int 
NdbBackup::restore(unsigned _backup_id){
  
  if (!isConnected())
    return -1;

  if (getStatus() != 0)
    return -1;

  int res; 

  // restore metadata first and data for first node
  res = execRestore(true, true, ndbNodes[0].node_id, _backup_id);

  // Restore data once for each node
  for(size_t i = 1; i < ndbNodes.size(); i++){
    res = execRestore(true, false, ndbNodes[i].node_id, _backup_id);
  }
  
  return 0;
}

// Master failure
int
NFDuringBackupM_codes[] = {
  10003,
  10004,
  10007,
  10008,
  10009,
  10010,
  10012,
  10013
};

// Slave failure
int
NFDuringBackupS_codes[] = {
  10014,
  10015,
  10016,
  10017,
  10018,
  10020
};

// Master takeover etc...
int
NFDuringBackupSL_codes[] = {
  10001,
  10002,
  10021
};

int 
NdbBackup::NFMaster(NdbRestarter& _restarter){
  const int sz = sizeof(NFDuringBackupM_codes)/sizeof(NFDuringBackupM_codes[0]);
  return NF(_restarter, NFDuringBackupM_codes, sz, true);
}

int 
NdbBackup::NFMasterAsSlave(NdbRestarter& _restarter){
  const int sz = sizeof(NFDuringBackupS_codes)/sizeof(NFDuringBackupS_codes[0]);
  return NF(_restarter, NFDuringBackupS_codes, sz, true);
}

int 
NdbBackup::NFSlave(NdbRestarter& _restarter){
  const int sz = sizeof(NFDuringBackupS_codes)/sizeof(NFDuringBackupS_codes[0]);
  return NF(_restarter, NFDuringBackupS_codes, sz, false);
}

int 
NdbBackup::NF(NdbRestarter& _restarter, int *NFDuringBackup_codes, const int sz, bool onMaster){
  int nNodes = _restarter.getNumDbNodes();
  {
    if(nNodes == 1)
      return NDBT_OK;
    
    int nodeId = _restarter.getMasterNodeId();

    CHECK(_restarter.restartOneDbNode(nodeId, false, true, true) == 0,
	  "Could not restart node "<< nodeId);
    
    CHECK(_restarter.waitNodesNoStart(&nodeId, 1) == 0,
	  "waitNodesNoStart failed");
    
    CHECK(_restarter.startNodes(&nodeId, 1) == 0,
	  "failed to start node");
  }
  
  CHECK(_restarter.waitClusterStarted() == 0,
	"waitClusterStarted failed");
  
  myRandom48Init(NdbTick_CurrentMillisecond());

  for(int i = 0; i<sz; i++){

    int error = NFDuringBackup_codes[i];
    unsigned int backupId;

    const int masterNodeId = _restarter.getMasterNodeId();
    CHECK(masterNodeId > 0, "getMasterNodeId failed");
    int nodeId;

    nodeId = masterNodeId;
    if (!onMaster) {
      int randomId;
      while (nodeId == masterNodeId) {
	randomId = myRandom48(nNodes);
	nodeId = _restarter.getDbNodeId(randomId);
      }
    }

    g_err << "NdbBackup::NF node = " << nodeId 
	   << " error code = " << error << " masterNodeId = "
	   << masterNodeId << endl;


    int val[] = { DumpStateOrd::CmvmiSetRestartOnErrorInsert, 1 };
    CHECK(_restarter.dumpStateOneNode(nodeId, val, 2) == 0,
	  "failed to set RestartOnErrorInsert");
    CHECK(_restarter.insertErrorInNode(nodeId, error) == 0,
	  "failed to set error insert");
   
    g_info << "error inserted"  << endl;
    NdbSleep_SecSleep(1);

    g_info << "starting backup"  << endl;
    int r = start(backupId);
    g_info << "r = " << r
	   << " (which should fail) started with id = "  << backupId << endl;
    if (r == 0) {
      g_err << "Backup should have failed on error_insertion " << error << endl
	    << "Master = " << masterNodeId << "Node = " << nodeId << endl;
      return NDBT_FAILED;
    }

    CHECK(_restarter.waitNodesNoStart(&nodeId, 1) == 0,
	  "waitNodesNoStart failed");

    g_info << "number of nodes running " << _restarter.getNumDbNodes() << endl;

    if (_restarter.getNumDbNodes() != nNodes) {
      g_err << "Failure: cluster not up" << endl;
      return NDBT_FAILED;
    }

    g_info << "starting new backup"  << endl;
    CHECK(start(backupId) == 0,
	  "failed to start backup");
    g_info << "(which should succeed) started with id = "  << backupId << endl;

    g_info << "starting node"  << endl;
    CHECK(_restarter.startNodes(&nodeId, 1) == 0,
	  "failed to start node");

    CHECK(_restarter.waitClusterStarted() == 0,
	  "waitClusterStarted failed");
    g_info << "node started"  << endl;

    int val2[] = { 24, 2424 };
    CHECK(_restarter.dumpStateAllNodes(val2, 2) == 0,
	  "failed to check backup resources RestartOnErrorInsert");
    
    CHECK(_restarter.insertErrorInNode(nodeId, 10099) == 0,
	  "failed to set error insert");

    NdbSleep_SecSleep(1);
  }

  return NDBT_OK;
}

int
FailS_codes[] = {
  10025,
  10027,
  10033,
  10035,
  10036
};

int
FailM_codes[] = {
  10023,
  10024,
  10025,
  10026,
  10027,
  10028,
  10031,
  10033,
  10035
};

int 
NdbBackup::FailMaster(NdbRestarter& _restarter){
  const int sz = sizeof(FailM_codes)/sizeof(FailM_codes[0]);
  return Fail(_restarter, FailM_codes, sz, true);
}

int 
NdbBackup::FailMasterAsSlave(NdbRestarter& _restarter){
  const int sz = sizeof(FailS_codes)/sizeof(FailS_codes[0]);
  return Fail(_restarter, FailS_codes, sz, true);
}

int 
NdbBackup::FailSlave(NdbRestarter& _restarter){
  const int sz = sizeof(FailS_codes)/sizeof(FailS_codes[0]);
  return Fail(_restarter, FailS_codes, sz, false);
}

int 
NdbBackup::Fail(NdbRestarter& _restarter, int *Fail_codes, const int sz, bool onMaster){

  CHECK(_restarter.waitClusterStarted() == 0,
	"waitClusterStarted failed");

  int nNodes = _restarter.getNumDbNodes();

  myRandom48Init(NdbTick_CurrentMillisecond());

  for(int i = 0; i<sz; i++){
    int error = Fail_codes[i];
    unsigned int backupId;

    const int masterNodeId = _restarter.getMasterNodeId();
    CHECK(masterNodeId > 0, "getMasterNodeId failed");
    int nodeId;

    nodeId = masterNodeId;
    if (!onMaster) {
      int randomId;
      while (nodeId == masterNodeId) {
	randomId = myRandom48(nNodes);
	nodeId = _restarter.getDbNodeId(randomId);
      }
    }

    g_err << "NdbBackup::Fail node = " << nodeId 
	   << " error code = " << error << " masterNodeId = "
	   << masterNodeId << endl;

    CHECK(_restarter.insertErrorInNode(nodeId, error) == 0,
	  "failed to set error insert");
   
    g_info << "error inserted"  << endl;
    g_info << "waiting some before starting backup"  << endl;

    g_info << "starting backup"  << endl;
    int r = start(backupId);
    g_info << "r = " << r
	   << " (which should fail) started with id = "  << backupId << endl;
    if (r == 0) {
      g_err << "Backup should have failed on error_insertion " << error << endl
	    << "Master = " << masterNodeId << "Node = " << nodeId << endl;
      return NDBT_FAILED;
    }
    
    CHECK(_restarter.waitClusterStarted() == 0,
	  "waitClusterStarted failed");

    CHECK(_restarter.insertErrorInNode(nodeId, 10099) == 0,
	  "failed to set error insert");

    NdbSleep_SecSleep(5);
    
    int val2[] = { 24, 2424 };
    CHECK(_restarter.dumpStateAllNodes(val2, 2) == 0,
	  "failed to check backup resources RestartOnErrorInsert");
    
  }

  return NDBT_OK;
}

