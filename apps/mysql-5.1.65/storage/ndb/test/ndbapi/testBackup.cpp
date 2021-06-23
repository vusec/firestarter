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

#include <NDBT.hpp>
#include <NDBT_Test.hpp>
#include <HugoTransactions.hpp>
#include <UtilTransactions.hpp>
#include <NdbBackup.hpp>


#define CHECK(b) if (!(b)) { \
  g_err << "ERR: "<< step->getName() \
         << " failed on line " << __LINE__ << endl; \
  result = NDBT_FAILED; \
  continue; } 


int runLoadTable(NDBT_Context* ctx, NDBT_Step* step){

  int records = ctx->getNumRecords();
  HugoTransactions hugoTrans(*ctx->getTab());
  if (hugoTrans.loadTable(GETNDB(step), records) != 0){
    return NDBT_FAILED;
  }
  return NDBT_OK;
}

bool testMaster = true;
bool testSlave = false;

int setMaster(NDBT_Context* ctx, NDBT_Step* step){
  testMaster = true;
  testSlave = false;
  return NDBT_OK;
}
int setMasterAsSlave(NDBT_Context* ctx, NDBT_Step* step){
  testMaster = true;
  testSlave = true;
  return NDBT_OK;
}
int setSlave(NDBT_Context* ctx, NDBT_Step* step){
  testMaster = false;
  testSlave = true;
  return NDBT_OK;
}

int runAbort(NDBT_Context* ctx, NDBT_Step* step){
  NdbBackup backup(GETNDB(step)->getNodeId()+1);

  NdbRestarter restarter;

  if (restarter.getNumDbNodes() < 2){
    ctx->stopTest();
    return NDBT_OK;
  }

  if(restarter.waitClusterStarted(60) != 0){
    g_err << "Cluster failed to start" << endl;
    return NDBT_FAILED;
  }

  if (testMaster) {
    if (testSlave) {
      if (backup.NFMasterAsSlave(restarter) != NDBT_OK){
	return NDBT_FAILED;
      }
    } else {
      if (backup.NFMaster(restarter) != NDBT_OK){
	return NDBT_FAILED;
      }
    }
  } else {
    if (backup.NFSlave(restarter) != NDBT_OK){
      return NDBT_FAILED;
    }
  }
  
  return NDBT_OK;
}

int runFail(NDBT_Context* ctx, NDBT_Step* step){
  NdbBackup backup(GETNDB(step)->getNodeId()+1);

  NdbRestarter restarter;

  if (restarter.getNumDbNodes() < 2){
    ctx->stopTest();
    return NDBT_OK;
  }

  if(restarter.waitClusterStarted(60) != 0){
    g_err << "Cluster failed to start" << endl;
    return NDBT_FAILED;
  }

  if (testMaster) {
    if (testSlave) {
      if (backup.FailMasterAsSlave(restarter) != NDBT_OK){
	return NDBT_FAILED;
      }
    } else {
      if (backup.FailMaster(restarter) != NDBT_OK){
	return NDBT_FAILED;
      }
    }
  } else {
    if (backup.FailSlave(restarter) != NDBT_OK){
      return NDBT_FAILED;
    }
  }

  return NDBT_OK;
}

int runBackupOne(NDBT_Context* ctx, NDBT_Step* step){
  NdbBackup backup(GETNDB(step)->getNodeId()+1);
  unsigned backupId = 0;

  if (backup.start(backupId) == -1){
    return NDBT_FAILED;
  }
  ndbout << "Started backup " << backupId << endl;
  ctx->setProperty("BackupId", backupId);

  return NDBT_OK;
}

int
runBackupLoop(NDBT_Context* ctx, NDBT_Step* step){
  NdbBackup backup(GETNDB(step)->getNodeId()+1);
  unsigned backupId = 0;
  
  int loops = ctx->getNumLoops();
  while(!ctx->isTestStopped() && loops--)
  {
    if (backup.start(backupId) == -1)
    {
      sleep(1);
      loops++;
    }
    else
    {
      sleep(3);
    }
  }

  ctx->stopTest();
  return NDBT_OK;
}

int
runDDL(NDBT_Context* ctx, NDBT_Step* step){
  Ndb* pNdb= GETNDB(step);
  NdbDictionary::Dictionary* pDict = pNdb->getDictionary();
  
  const int tables = NDBT_Tables::getNumTables();
  while(!ctx->isTestStopped())
  {
    const int tab_no = rand() % (tables);
    NdbDictionary::Table tab = *NDBT_Tables::getTable(tab_no);
    BaseString name= tab.getName();
    name.appfmt("-%d", step->getStepNo());
    tab.setName(name.c_str());
    if(pDict->createTable(tab) == 0)
    {
      HugoTransactions hugoTrans(* pDict->getTable(name.c_str()));
      if (hugoTrans.loadTable(pNdb, 10000) != 0){
	return NDBT_FAILED;
      }
      
      while(pDict->dropTable(tab.getName()) != 0 &&
	    pDict->getNdbError().code != 4009)
	g_err << pDict->getNdbError() << endl;
      
      sleep(1);

    }
  }
  return NDBT_OK;
}


int runDropTablesRestart(NDBT_Context* ctx, NDBT_Step* step){
  NdbRestarter restarter;

  Ndb* pNdb = GETNDB(step);

  const NdbDictionary::Table *tab = ctx->getTab();
  pNdb->getDictionary()->dropTable(tab->getName());

  if (restarter.restartAll(false) != 0)
    return NDBT_FAILED;

  if (restarter.waitClusterStarted() != 0)
    return NDBT_FAILED;
  
  return NDBT_OK;
}

int runRestoreOne(NDBT_Context* ctx, NDBT_Step* step){
  NdbBackup backup(GETNDB(step)->getNodeId()+1);
  unsigned backupId = ctx->getProperty("BackupId"); 

  ndbout << "Restoring backup " << backupId << endl;

  if (backup.restore(backupId) == -1){
    return NDBT_FAILED;
  }

  return NDBT_OK;
}

int runVerifyOne(NDBT_Context* ctx, NDBT_Step* step){
  int records = ctx->getNumRecords();
  Ndb* pNdb = GETNDB(step);
  int result = NDBT_OK;
  int count = 0;

  const NdbDictionary::Table* tab = 
    GETNDB(step)->getDictionary()->getTable(ctx->getTab()->getName());
  if(tab == 0)
    return NDBT_FAILED;
  
  UtilTransactions utilTrans(* tab);
  HugoTransactions hugoTrans(* tab);

  do{

    // Check that there are as many records as we expected
    CHECK(utilTrans.selectCount(pNdb, 64, &count) == 0);

    g_err << "count = " << count;
    g_err << " records = " << records;
    g_err << endl;

    CHECK(count == records);

    // Read and verify every record
    CHECK(hugoTrans.pkReadRecords(pNdb, records) == 0);
    
  } while (false);

  return result;
}

int runClearTable(NDBT_Context* ctx, NDBT_Step* step){
  int records = ctx->getNumRecords();
  
  UtilTransactions utilTrans(*ctx->getTab());
  if (utilTrans.clearTable2(GETNDB(step),  records) != 0){
    return NDBT_FAILED;
  }
  return NDBT_OK;
}

int runDropTable(NDBT_Context* ctx, NDBT_Step* step){
  GETNDB(step)->getDictionary()->dropTable(ctx->getTab()->getName());
  return NDBT_OK;
}

#include "bank/Bank.hpp"

int runCreateBank(NDBT_Context* ctx, NDBT_Step* step){
  Bank bank(ctx->m_cluster_connection);
  int overWriteExisting = true;
  if (bank.createAndLoadBank(overWriteExisting, 10) != NDBT_OK)
    return NDBT_FAILED;
  return NDBT_OK;
}

int runBankTimer(NDBT_Context* ctx, NDBT_Step* step){
  Bank bank(ctx->m_cluster_connection);
  int wait = 30; // Max seconds between each "day"
  int yield = 1; // Loops before bank returns 

  while (ctx->isTestStopped() == false) {
    bank.performIncreaseTime(wait, yield);
  }
  return NDBT_OK;
}

int runBankTransactions(NDBT_Context* ctx, NDBT_Step* step){
  Bank bank(ctx->m_cluster_connection);
  int wait = 10; // Max ms between each transaction
  int yield = 100; // Loops before bank returns 

  while (ctx->isTestStopped() == false) {
    bank.performTransactions(wait, yield);
  }
  return NDBT_OK;
}

int runBankGL(NDBT_Context* ctx, NDBT_Step* step){
  Bank bank(ctx->m_cluster_connection);
  int yield = 20; // Loops before bank returns 
  int result = NDBT_OK;

  while (ctx->isTestStopped() == false) {
    if (bank.performMakeGLs(yield) != NDBT_OK){
      ndbout << "bank.performMakeGLs FAILED" << endl;
      result = NDBT_FAILED;
    }
  }
  return NDBT_OK;
}

int runBankSum(NDBT_Context* ctx, NDBT_Step* step){
  Bank bank(ctx->m_cluster_connection);
  int wait = 2000; // Max ms between each sum of accounts
  int yield = 1; // Loops before bank returns 
  int result = NDBT_OK;

  while (ctx->isTestStopped() == false) {
    if (bank.performSumAccounts(wait, yield) != NDBT_OK){
      ndbout << "bank.performSumAccounts FAILED" << endl;
      result = NDBT_FAILED;
    }
  }
  return result ;
}

int runDropBank(NDBT_Context* ctx, NDBT_Step* step){
  Bank bank(ctx->m_cluster_connection);
  if (bank.dropBank() != NDBT_OK)
    return NDBT_FAILED;
  return NDBT_OK;
}

int runBackupBank(NDBT_Context* ctx, NDBT_Step* step){
  int loops = ctx->getNumLoops();
  int l = 0;
  int maxSleep = 30; // Max seconds between each backup
  Ndb* pNdb = GETNDB(step);
  NdbBackup backup(GETNDB(step)->getNodeId()+1);
  unsigned minBackupId = ~0;
  unsigned maxBackupId = 0;
  unsigned backupId = 0;
  int result = NDBT_OK;

  while (l < loops && result != NDBT_FAILED){

    if (pNdb->waitUntilReady() != 0){
      result = NDBT_FAILED;
      continue;
    }

    // Sleep for a while
    NdbSleep_SecSleep(maxSleep);
    
    // Perform backup
    if (backup.start(backupId) != 0){
      ndbout << "backup.start failed" << endl;
      result = NDBT_FAILED;
      continue;
    }
    ndbout << "Started backup " << backupId << endl;

    // Remember min and max backupid
    if (backupId < minBackupId)
      minBackupId = backupId;

    if (backupId > maxBackupId)
      maxBackupId = backupId;
    
    ndbout << " maxBackupId = " << maxBackupId 
	   << ", minBackupId = " << minBackupId << endl;
    ctx->setProperty("MinBackupId", minBackupId);    
    ctx->setProperty("MaxBackupId", maxBackupId);    
    
    l++;
  }

  ctx->stopTest();

  return result;
}

int runRestoreBankAndVerify(NDBT_Context* ctx, NDBT_Step* step){
  NdbRestarter restarter;
  NdbBackup backup(GETNDB(step)->getNodeId()+1);
  unsigned minBackupId = ctx->getProperty("MinBackupId");
  unsigned maxBackupId = ctx->getProperty("MaxBackupId");
  unsigned backupId = minBackupId;
  int result = NDBT_OK;
  int errSumAccounts = 0;
  int errValidateGL = 0;

  ndbout << " maxBackupId = " << maxBackupId << endl;
  ndbout << " minBackupId = " << minBackupId << endl;
  
  while (backupId <= maxBackupId){

    // TEMPORARY FIX
    // To erase all tables from cache(s)
    // To be removed, maybe replaced by ndb.invalidate();
    runDropTable(ctx,step);
    {
      Bank bank(ctx->m_cluster_connection);
      
      if (bank.dropBank() != NDBT_OK){
	result = NDBT_FAILED;
	break;
      }
    }
    // END TEMPORARY FIX

    ndbout << "Performing restart" << endl;
    if (restarter.restartAll(false) != 0)
      return NDBT_FAILED;

    if (restarter.waitClusterStarted() != 0)
      return NDBT_FAILED;

    ndbout << "Restoring backup " << backupId << endl;
    if (backup.restore(backupId) == -1){
      return NDBT_FAILED;
    }
    ndbout << "Backup " << backupId << " restored" << endl;

    // Let bank verify
    Bank bank(ctx->m_cluster_connection);

    int wait = 0;
    int yield = 1;
    if (bank.performSumAccounts(wait, yield) != 0){
      ndbout << "bank.performSumAccounts FAILED" << endl;
      ndbout << "  backupId = " << backupId << endl << endl;
      result = NDBT_FAILED;
      errSumAccounts++;
    }

    if (bank.performValidateAllGLs() != 0){
      ndbout << "bank.performValidateAllGLs FAILED" << endl;
      ndbout << "  backupId = " << backupId << endl << endl;
      result = NDBT_FAILED;
      errValidateGL++;
    }

    backupId++;
  }
  
  if (result != NDBT_OK){
    ndbout << "Verification of backup failed" << endl
	   << "  errValidateGL="<<errValidateGL<<endl
	   << "  errSumAccounts="<<errSumAccounts<<endl << endl;
  }
  
  return result;
}

NDBT_TESTSUITE(testBackup);
TESTCASE("BackupOne", 
	 "Test that backup and restore works on one table \n"
	 "1. Load table\n"
	 "2. Backup\n"
	 "3. Drop tables and restart \n"
	 "4. Restore\n"
	 "5. Verify count and content of table\n"){
  INITIALIZER(runLoadTable);
  INITIALIZER(runBackupOne);
  INITIALIZER(runDropTablesRestart);
  INITIALIZER(runRestoreOne);
  VERIFIER(runVerifyOne);
  FINALIZER(runClearTable);
}
TESTCASE("BackupDDL", 
	 "Test that backup and restore works on with DDL ongoing\n"
	 "1. Backups and DDL (create,drop,table.index)"){
  INITIALIZER(runLoadTable);
  STEP(runBackupLoop);
  STEP(runDDL);
  STEP(runDDL);
  FINALIZER(runClearTable);
}
TESTCASE("BackupBank", 
	 "Test that backup and restore works during transaction load\n"
	 " by backing up the bank"
	 "1.  Create bank\n"
	 "2a. Start bank and let it run\n"
	 "2b. Perform loop number of backups of the bank\n"
	 "    when backups are finished tell bank to close\n"
	 "3.  Restart ndb -i and reload each backup\n"
	 "    let bank verify that the backup is consistent\n"
	 "4.  Drop bank\n"){
  INITIALIZER(runCreateBank);
  STEP(runBankTimer);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankTransactions);
  STEP(runBankGL);
  // TODO  STEP(runBankSum);
  STEP(runBackupBank);
  VERIFIER(runRestoreBankAndVerify);
  //  FINALIZER(runDropBank);
}
TESTCASE("NFMaster", 
	 "Test that backup behaves during node failiure\n"){
  INITIALIZER(setMaster);
  STEP(runAbort);

}
TESTCASE("NFMasterAsSlave", 
	 "Test that backup behaves during node failiure\n"){
  INITIALIZER(setMasterAsSlave);
  STEP(runAbort);

}
TESTCASE("NFSlave", 
	 "Test that backup behaves during node failiure\n"){
  INITIALIZER(setSlave);
  STEP(runAbort);

}
TESTCASE("FailMaster", 
	 "Test that backup behaves during node failiure\n"){
  INITIALIZER(setMaster);
  STEP(runFail);

}
TESTCASE("FailMasterAsSlave", 
	 "Test that backup behaves during node failiure\n"){
  INITIALIZER(setMasterAsSlave);
  STEP(runFail);

}
TESTCASE("FailSlave", 
	 "Test that backup behaves during node failiure\n"){
  INITIALIZER(setSlave);
  STEP(runFail);

}
NDBT_TESTSUITE_END(testBackup);

int main(int argc, const char** argv){
  ndb_init();
  return testBackup.execute(argc, argv);
}


