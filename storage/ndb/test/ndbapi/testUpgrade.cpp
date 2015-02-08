/*
   Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.

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

#include <NDBT.hpp>
#include <NDBT_Test.hpp>
#include <HugoTransactions.hpp>
#include <UtilTransactions.hpp>
#include <NdbRestarter.hpp>
#include <AtrtClient.hpp>
#include <Bitmask.hpp>
#include <NdbBackup.hpp>
#include <ndb_version.h>

static Vector<BaseString> table_list;

struct NodeInfo
{
  int nodeId;
  int processId;
  int nodeGroup;
};

static
int
createEvent(Ndb *pNdb,
            const NdbDictionary::Table &tab,
            bool merge_events = true,
            bool report = true)
{
  char eventName[1024];
  sprintf(eventName,"%s_EVENT",tab.getName());
  
  NdbDictionary::Dictionary *myDict = pNdb->getDictionary();

  if (!myDict) {
    g_err << "Dictionary not found " 
	  << pNdb->getNdbError().code << " "
	  << pNdb->getNdbError().message << endl;
    return NDBT_FAILED;
  }
  
  myDict->dropEvent(eventName);
  
  NdbDictionary::Event myEvent(eventName);
  myEvent.setTable(tab.getName());
  myEvent.addTableEvent(NdbDictionary::Event::TE_ALL); 
  for(int a = 0; a < tab.getNoOfColumns(); a++){
    myEvent.addEventColumn(a);
  }
  myEvent.mergeEvents(merge_events);

  if (report)
    myEvent.setReport(NdbDictionary::Event::ER_SUBSCRIBE);

  int res = myDict->createEvent(myEvent); // Add event to database
  
  if (res == 0)
    myEvent.print();
  else if (myDict->getNdbError().classification ==
	   NdbError::SchemaObjectExists) 
  {
    g_info << "Event creation failed event exists\n";
    res = myDict->dropEvent(eventName);
    if (res) {
      g_err << "Failed to drop event: " 
	    << myDict->getNdbError().code << " : "
	    << myDict->getNdbError().message << endl;
      return NDBT_FAILED;
    }
    // try again
    res = myDict->createEvent(myEvent); // Add event to database
    if (res) {
      g_err << "Failed to create event (1): " 
	    << myDict->getNdbError().code << " : "
	    << myDict->getNdbError().message << endl;
      return NDBT_FAILED;
    }
  }
  else 
  {
    g_err << "Failed to create event (2): " 
	  << myDict->getNdbError().code << " : "
	  << myDict->getNdbError().message << endl;
    return NDBT_FAILED;
  }

  return NDBT_OK;
}

static
int
dropEvent(Ndb *pNdb, const NdbDictionary::Table &tab)
{
  char eventName[1024];
  sprintf(eventName,"%s_EVENT",tab.getName());
  NdbDictionary::Dictionary *myDict = pNdb->getDictionary();
  if (!myDict) {
    g_err << "Dictionary not found " 
	  << pNdb->getNdbError().code << " "
	  << pNdb->getNdbError().message << endl;
    return NDBT_FAILED;
  }
  if (myDict->dropEvent(eventName)) {
    g_err << "Failed to drop event: " 
	  << myDict->getNdbError().code << " : "
	  << myDict->getNdbError().message << endl;
    return NDBT_FAILED;
  }
  return NDBT_OK;
}


static
int
createDropEvent(NDBT_Context* ctx, NDBT_Step* step)
{
  Ndb* pNdb = GETNDB(step);
  NdbDictionary::Dictionary *myDict = pNdb->getDictionary();

  if (ctx->getProperty("NoDDL", Uint32(0)) == 0)
  {
    for (unsigned i = 0; i<table_list.size(); i++)
    {
      int res = NDBT_OK;
      const NdbDictionary::Table* tab = myDict->getTable(table_list[i].c_str());
      if (tab == 0)
      {
        continue;
      }
      if ((res = createEvent(pNdb, *tab) != NDBT_OK))
      {
        return res;
      }
      
      
      
      if ((res = dropEvent(pNdb, *tab)) != NDBT_OK)
      {
        return res;
      }
    }
  }

  return NDBT_OK;
}

/* An enum for expressing how many of the multiple nodes
 * of a given type an action should be applied to
 */
enum NodeSet
{
  All = 0,
  NotAll = 1, /* less than All, or None if there's only 1 */
  None = 2
};

uint getNodeCount(NodeSet set, uint numNodes)
{
  switch(set)
  {
  case All:
    return numNodes;
  case NotAll:
  {
    if (numNodes < 2)
      return 0;
    
    if (numNodes == 2)
      return 1;
    
    uint range = numNodes - 2;
    
    /* At least 1, at most numNodes - 1 */
    return (1 + (rand() % (range + 1)));
  }
  case None:
  {
    return 0;
  }
  default:
    g_err << "Unknown set type : " << set << endl;
    abort();
    return 0;
  }
};


/**
  Test that one node at a time can be upgraded
*/

int runUpgrade_NR1(NDBT_Context* ctx, NDBT_Step* step){
  AtrtClient atrt;

  NodeSet mgmdNodeSet = (NodeSet) ctx->getProperty("MgmdNodeSet", Uint32(0));
  NodeSet ndbdNodeSet = (NodeSet) ctx->getProperty("NdbdNodeSet", Uint32(0));

  SqlResultSet clusters;
  if (!atrt.getClusters(clusters))
    return NDBT_FAILED;

  while (clusters.next())
  {
    uint clusterId= clusters.columnAsInt("id");
    SqlResultSet tmp_result;
    if (!atrt.getConnectString(clusterId, tmp_result))
      return NDBT_FAILED;

    NdbRestarter restarter(tmp_result.column("connectstring"));
    restarter.setReconnect(true); // Restarting mgmd
    g_err << "Cluster '" << clusters.column("name")
          << "@" << tmp_result.column("connectstring") << "'" << endl;

    if (restarter.waitClusterStarted())
      return NDBT_FAILED;

    // Restart ndb_mgmd(s)
    SqlResultSet mgmds;
    if (!atrt.getMgmds(clusterId, mgmds))
      return NDBT_FAILED;
    
    uint mgmdCount = mgmds.numRows();
    uint restartCount = getNodeCount(mgmdNodeSet, mgmdCount);
    
    ndbout << "Restarting "
             << restartCount << " of " << mgmdCount
             << " mgmds" << endl;
      
    while (mgmds.next() && restartCount --)
    {
      ndbout << "Restart mgmd " << mgmds.columnAsInt("node_id") << endl;
      if (!atrt.changeVersion(mgmds.columnAsInt("id"), ""))
        return NDBT_FAILED;
      
      if (restarter.waitConnected())
        return NDBT_FAILED;
      ndbout << "Connected to mgmd"<< endl;
    }
    
    ndbout << "Waiting for started"<< endl;
    if (restarter.waitClusterStarted())
      return NDBT_FAILED;
    ndbout << "Started"<< endl;
    
    // Restart ndbd(s)
    SqlResultSet ndbds;
    if (!atrt.getNdbds(clusterId, ndbds))
      return NDBT_FAILED;

    uint ndbdCount = ndbds.numRows();
    restartCount = getNodeCount(ndbdNodeSet, ndbdCount);
    
    ndbout << "Restarting "
             << restartCount << " of " << ndbdCount
             << " ndbds" << endl;
    
    while(ndbds.next() && restartCount --)
    {
      int nodeId = ndbds.columnAsInt("node_id");
      int processId = ndbds.columnAsInt("id");
      ndbout << "Restart node " << nodeId << endl;
      
      if (!atrt.changeVersion(processId, ""))
        return NDBT_FAILED;
      
      if (restarter.waitNodesNoStart(&nodeId, 1))
        return NDBT_FAILED;
      
      if (restarter.startNodes(&nodeId, 1))
        return NDBT_FAILED;
      
      if (restarter.waitNodesStarted(&nodeId, 1))
        return NDBT_FAILED;
      
      if (createDropEvent(ctx, step))
        return NDBT_FAILED;
    }
  }
  
  ctx->stopTest();
  return NDBT_OK;
}

static
int
runBug48416(NDBT_Context* ctx, NDBT_Step* step)
{
  Ndb* pNdb = GETNDB(step);

  return NDBT_Tables::createTable(pNdb, "I1");
}

static
int
runUpgrade_Half(NDBT_Context* ctx, NDBT_Step* step)
{
  // Assuming 2 replicas

  AtrtClient atrt;

  const bool waitNode = ctx->getProperty("WaitNode", Uint32(0)) != 0;
  const bool event = ctx->getProperty("CreateDropEvent", Uint32(0)) != 0;
  const char * args = "";
  if (ctx->getProperty("KeepFS", Uint32(0)) != 0)
  {
    args = "--initial=0";
  }

  NodeSet mgmdNodeSet = (NodeSet) ctx->getProperty("MgmdNodeSet", Uint32(0));
  NodeSet ndbdNodeSet = (NodeSet) ctx->getProperty("NdbdNodeSet", Uint32(0));

  SqlResultSet clusters;
  if (!atrt.getClusters(clusters))
    return NDBT_FAILED;

  while (clusters.next())
  {
    uint clusterId= clusters.columnAsInt("id");
    SqlResultSet tmp_result;
    if (!atrt.getConnectString(clusterId, tmp_result))
      return NDBT_FAILED;

    NdbRestarter restarter(tmp_result.column("connectstring"));
    restarter.setReconnect(true); // Restarting mgmd
    g_err << "Cluster '" << clusters.column("name")
          << "@" << tmp_result.column("connectstring") << "'" << endl;

    if(restarter.waitClusterStarted())
      return NDBT_FAILED;

    // Restart ndb_mgmd(s)
    SqlResultSet mgmds;
    if (!atrt.getMgmds(clusterId, mgmds))
      return NDBT_FAILED;

    uint mgmdCount = mgmds.numRows();
    uint restartCount = getNodeCount(mgmdNodeSet, mgmdCount);
    
    ndbout << "Restarting "
             << restartCount << " of " << mgmdCount
            << " mgmds" << endl;
      
    while (mgmds.next() && restartCount --)
    {
      ndbout << "Restart mgmd" << mgmds.columnAsInt("node_id") << endl;
      if (!atrt.changeVersion(mgmds.columnAsInt("id"), ""))
        return NDBT_FAILED;

      if(restarter.waitConnected())
        return NDBT_FAILED;
    }

    NdbSleep_SecSleep(5); // TODO, handle arbitration

    // Restart one ndbd in each node group
    SqlResultSet ndbds;
    if (!atrt.getNdbds(clusterId, ndbds))
      return NDBT_FAILED;

    Vector<NodeInfo> nodes;
    while (ndbds.next())
    {
      struct NodeInfo n;
      n.nodeId = ndbds.columnAsInt("node_id");
      n.processId = ndbds.columnAsInt("id");
      n.nodeGroup = restarter.getNodeGroup(n.nodeId);
      nodes.push_back(n);
    }

    uint ndbdCount = ndbds.numRows();
    restartCount = getNodeCount(ndbdNodeSet, ndbdCount);
    
    ndbout << "Restarting "
             << restartCount << " of " << ndbdCount
             << " ndbds" << endl;
    
    int nodesarray[256];
    int cnt= 0;

    Bitmask<4> seen_groups;
    Bitmask<4> restarted_nodes;
    for (Uint32 i = 0; (i<nodes.size() && restartCount); i++)
    {
      int nodeId = nodes[i].nodeId;
      int processId = nodes[i].processId;
      int nodeGroup= nodes[i].nodeGroup;

      if (seen_groups.get(nodeGroup))
      {
        // One node in this node group already down
        continue;
      }
      seen_groups.set(nodeGroup);
      restarted_nodes.set(nodeId);

      ndbout << "Restart node " << nodeId << endl;
      
      if (!atrt.changeVersion(processId, args))
        return NDBT_FAILED;
      
      if (waitNode)
      {
        restarter.waitNodesNoStart(&nodeId, 1);
      }

      nodesarray[cnt++]= nodeId;
      restartCount--;
    }
    
    if (!waitNode)
    {
      if (restarter.waitNodesNoStart(nodesarray, cnt))
        return NDBT_FAILED;
    }

    ndbout << "Starting and wait for started..." << endl;
    if (restarter.startAll())
      return NDBT_FAILED;

    if (restarter.waitClusterStarted())
      return NDBT_FAILED;

    if (event && createDropEvent(ctx, step))
    {
      return NDBT_FAILED;
    }

    // Restart the remaining nodes
    cnt= 0;
    for (Uint32 i = 0; (i<nodes.size() && restartCount); i++)
    {
      int nodeId = nodes[i].nodeId;
      int processId = nodes[i].processId;

      if (restarted_nodes.get(nodeId))
        continue;
      
      ndbout << "Restart node " << nodeId << endl;
      if (!atrt.changeVersion(processId, args))
        return NDBT_FAILED;

      if (waitNode)
      {
        restarter.waitNodesNoStart(&nodeId, 1);
      }

      nodesarray[cnt++]= nodeId;
      restartCount --;
    }

    
    if (!waitNode)
    {
      if (restarter.waitNodesNoStart(nodesarray, cnt))
        return NDBT_FAILED;
    }

    ndbout << "Starting and wait for started..." << endl;
    if (restarter.startAll())
      return NDBT_FAILED;
    
    if (restarter.waitClusterStarted())
      return NDBT_FAILED;

    if (event && createDropEvent(ctx, step))
    {
      return NDBT_FAILED;
    }
  }

  return NDBT_OK;
}



/**
   Test that one node in each nodegroup can be upgraded simultaneously
    - using method1
*/

int runUpgrade_NR2(NDBT_Context* ctx, NDBT_Step* step)
{
  // Assuming 2 replicas

  ctx->setProperty("WaitNode", 1);
  ctx->setProperty("CreateDropEvent", 1);
  int res = runUpgrade_Half(ctx, step);
  ctx->stopTest();
  return res;
}

/**
   Test that one node in each nodegroup can be upgrade simultaneously
    - using method2, ie. don't wait for "nostart" before stopping
      next node
*/

int runUpgrade_NR3(NDBT_Context* ctx, NDBT_Step* step){
  // Assuming 2 replicas

  ctx->setProperty("CreateDropEvent", 1);
  int res = runUpgrade_Half(ctx, step);
  ctx->stopTest();
  return res;
}

/**
   Test that we can upgrade the Ndbds on their own
*/
int runUpgrade_NdbdOnly(NDBT_Context* ctx, NDBT_Step* step)
{
  ctx->setProperty("MgmdNodeSet", (Uint32) NodeSet(None));
  int res = runUpgrade_Half(ctx, step);
  ctx->stopTest();
  return res;
}

/**
   Test that we can upgrade the Ndbds first, then
   the MGMDs
*/
int runUpgrade_NdbdFirst(NDBT_Context* ctx, NDBT_Step* step)
{
  ctx->setProperty("MgmdNodeSet", (Uint32) NodeSet(None));
  int res = runUpgrade_Half(ctx, step);
  if (res == NDBT_OK)
  {
    ctx->setProperty("MgmdNodeSet", (Uint32) NodeSet(All));
    ctx->setProperty("NdbdNodeSet", (Uint32) NodeSet(None));
    res = runUpgrade_Half(ctx, step);
  }
  ctx->stopTest();
  return res;
}

/**
   Upgrade some of the MGMDs
*/
int runUpgrade_NotAllMGMD(NDBT_Context* ctx, NDBT_Step* step)
{
  ctx->setProperty("MgmdNodeSet", (Uint32) NodeSet(NotAll));
  ctx->setProperty("NdbdNodeSet", (Uint32) NodeSet(None));
  int res = runUpgrade_Half(ctx, step);
  ctx->stopTest();
  return res;
}

int runCheckStarted(NDBT_Context* ctx, NDBT_Step* step){

  // Check cluster is started
  NdbRestarter restarter;
  if(restarter.waitClusterStarted() != 0){
    g_err << "All nodes was not started " << endl;
    return NDBT_FAILED;
  }

  // Check atrtclient is started
  AtrtClient atrt;
  if(!atrt.waitConnected()){
    g_err << "atrt server was not started " << endl;
    return NDBT_FAILED;
  }

  // Make sure atrt assigns nodeid != -1
  SqlResultSet procs;
  if (!atrt.doQuery("SELECT * FROM process where type <> \'mysql\'", procs))
    return NDBT_FAILED;

  while (procs.next())
  {
    if (procs.columnAsInt("node_id") == (unsigned)-1){
      ndbout << "Found one process with node_id -1, "
             << "use --fix-nodeid=1 to atrt to fix this" << endl;
      return NDBT_FAILED;
    }
  }

  return NDBT_OK;
}

int 
runCreateAllTables(NDBT_Context* ctx, NDBT_Step* step)
{
  ndbout_c("createAllTables");
  if (NDBT_Tables::createAllTables(GETNDB(step), false, true))
    return NDBT_FAILED;

  for (int i = 0; i<NDBT_Tables::getNumTables(); i++)
    table_list.push_back(BaseString(NDBT_Tables::getTable(i)->getName()));

  return NDBT_OK;
}

int
runCreateOneTable(NDBT_Context* ctx, NDBT_Step* step)
{
  // Table is already created...
  // so we just add it to table_list
  table_list.push_back(BaseString(ctx->getTab()->getName()));

  return NDBT_OK;
}

int runGetTableList(NDBT_Context* ctx, NDBT_Step* step)
{
  table_list.clear();
  ndbout << "Looking for tables ... ";
  for (int i = 0; i<NDBT_Tables::getNumTables(); i++)
  {
    const NdbDictionary::Table* tab = 
      GETNDB(step)->getDictionary()
      ->getTable(NDBT_Tables::getTable(i)
                 ->getName());
    if (tab != NULL)
    {
      ndbout << tab->getName() << " ";
      table_list.push_back(BaseString(tab->getName()));
    }
  }
  ndbout << endl;

  return NDBT_OK;
}

int
runLoadAll(NDBT_Context* ctx, NDBT_Step* step)
{
  Ndb* pNdb = GETNDB(step);
  NdbDictionary::Dictionary * pDict = pNdb->getDictionary();
  int records = ctx->getNumRecords();
  int result = NDBT_OK;
  
  for (unsigned i = 0; i<table_list.size(); i++)
  {
    const NdbDictionary::Table* tab = pDict->getTable(table_list[i].c_str());
    HugoTransactions trans(* tab);
    trans.loadTable(pNdb, records);
    trans.scanUpdateRecords(pNdb, records);
  }
  
  return result;
}

int
runClearAll(NDBT_Context* ctx, NDBT_Step* step)
{
  Ndb* pNdb = GETNDB(step);
  NdbDictionary::Dictionary * pDict = pNdb->getDictionary();
  int records = ctx->getNumRecords();
  int result = NDBT_OK;

  for (unsigned i = 0; i<table_list.size(); i++)
  {
    const NdbDictionary::Table* tab = pDict->getTable(table_list[i].c_str());
    if (tab)
    {
      HugoTransactions trans(* tab);
      trans.clearTable(pNdb, records);
    }
  }
  
  return result;
}


int
runBasic(NDBT_Context* ctx, NDBT_Step* step)
{
  Ndb* pNdb = GETNDB(step);
  NdbDictionary::Dictionary * pDict = pNdb->getDictionary();
  int records = ctx->getNumRecords();
  int result = NDBT_OK;

  int l = 0;
  while (!ctx->isTestStopped())
  {
    for (unsigned i = 0; i<table_list.size(); i++)
    {
      const NdbDictionary::Table* tab = pDict->getTable(table_list[i].c_str());
      HugoTransactions trans(* tab);
      switch(l % 4){
      case 0:
        trans.loadTable(pNdb, records);
        trans.scanUpdateRecords(pNdb, records);
        trans.pkUpdateRecords(pNdb, records);
        trans.pkReadUnlockRecords(pNdb, records);
        break;
      case 1:
        trans.scanUpdateRecords(pNdb, records);
        // TODO make pkInterpretedUpdateRecords work on any table
        // (or check if it does)
        if (strcmp(tab->getName(), "T1") == 0)
          trans.pkInterpretedUpdateRecords(pNdb, records);
        trans.clearTable(pNdb, records/2);
        trans.loadTable(pNdb, records/2);
        break;
      case 2:
        trans.clearTable(pNdb, records/2);
        trans.loadTable(pNdb, records/2);
        trans.clearTable(pNdb, records/2);
        break;
      case 3:
        if (createDropEvent(ctx, step))
        {
          return NDBT_FAILED;
        }
        break;
      }
    }
    l++;
  }
  
  return result;
}

int
rollingRestart(NDBT_Context* ctx, NDBT_Step* step)
{
  // Assuming 2 replicas

  AtrtClient atrt;

  SqlResultSet clusters;
  if (!atrt.getClusters(clusters))
    return NDBT_FAILED;

  while (clusters.next())
  {
    uint clusterId= clusters.columnAsInt("id");
    SqlResultSet tmp_result;
    if (!atrt.getConnectString(clusterId, tmp_result))
      return NDBT_FAILED;

    NdbRestarter restarter(tmp_result.column("connectstring"));
    if (restarter.rollingRestart())
      return NDBT_FAILED;
  }
  
  return NDBT_OK;

}

int runUpgrade_Traffic(NDBT_Context* ctx, NDBT_Step* step){
  // Assuming 2 replicas
  
  ndbout_c("upgrading");
  int res = runUpgrade_Half(ctx, step);
  if (res == NDBT_OK)
  {
    ndbout_c("rolling restarting");
    res = rollingRestart(ctx, step);
  }
  ctx->stopTest();
  return res;
}

int
startPostUpgradeChecks(NDBT_Context* ctx, NDBT_Step* step)
{
  /**
   * This will restart *self* in new version
   */

  BaseString extraArgs;
  if (ctx->getProperty("RestartNoDDL", Uint32(0)))
  {
    /* Ask post-upgrade steps not to perform DDL
     * (e.g. for 6.3->7.0 upgrade)
     */
    extraArgs.append(" --noddl ");
  }

  /**
   * mysql-getopt works so that passing "-n X -n Y" is ok
   *   and is interpreted as "-n Y"
   *
   * so we restart ourselves with testcase-name and "--post-upgrade" appended
   * e.g if testcase is "testUpgrade -n X"
   *     this will restart it as "testUpgrade -n X -n X--post-upgrade"
   */
  BaseString tc;
  tc.assfmt("-n %s--post-upgrade %s", 
            ctx->getCase()->getName(),
            extraArgs.c_str());

  ndbout << "About to restart self with extra arg: " << tc.c_str() << endl;

  AtrtClient atrt;
  int process_id = atrt.getOwnProcessId();
  if (process_id == -1)
  {
    g_err << "Failed to find own process id" << endl;
    return NDBT_FAILED;
  }

  if (!atrt.changeVersion(process_id, tc.c_str()))
    return NDBT_FAILED;

  // Will not be reached...

  return NDBT_OK;
}

int
startPostUpgradeChecksApiFirst(NDBT_Context* ctx, NDBT_Step* step)
{
  /* If Api is upgraded before all NDBDs then it may not 
   * be possible to use DDL from the upgraded API
   * The upgraded Api will decide, but we pass NoDDL
   * in
   */
  ctx->setProperty("RestartNoDDL", 1);
  return startPostUpgradeChecks(ctx, step);
}

int
runPostUpgradeChecks(NDBT_Context* ctx, NDBT_Step* step)
{
  /**
   * Table will be dropped/recreated
   *   automatically by NDBT...
   *   so when we enter here, this is already tested
   */
  NdbBackup backup(GETNDB(step)->getNodeId()+1);

  ndbout << "Starting backup..." << flush;
  if (backup.start() != 0)
  {
    ndbout << "Failed" << endl;
    return NDBT_FAILED;
  }
  ndbout << "done" << endl;


  if ((ctx->getProperty("NoDDL", Uint32(0)) == 0) &&
      (ctx->getProperty("KeepFS", Uint32(0)) != 0))
  {
    /**
     * Bug48227
     * Upgrade with FS 6.3->7.0, followed by table
     * create, followed by Sys restart resulted in 
     * table loss.
     */
    Ndb* pNdb = GETNDB(step);
    NdbDictionary::Dictionary *pDict = pNdb->getDictionary();
    {
      NdbDictionary::Dictionary::List l;
      pDict->listObjects(l);
      for (Uint32 i = 0; i<l.count; i++)
        ndbout_c("found %u : %s", l.elements[i].id, l.elements[i].name);
    }
    
    pDict->dropTable("I3");
    if (NDBT_Tables::createTable(pNdb, "I3"))
    {
      ndbout_c("Failed to create table!");
      ndbout << pDict->getNdbError() << endl;
      return NDBT_FAILED;
    }
    
    {
      NdbDictionary::Dictionary::List l;
      pDict->listObjects(l);
      for (Uint32 i = 0; i<l.count; i++)
        ndbout_c("found %u : %s", l.elements[i].id, l.elements[i].name);
    }
    
    NdbRestarter res;
    if (res.restartAll() != 0)
    {
      ndbout_c("restartAll() failed");
      return NDBT_FAILED;
    }
    
    if (res.waitClusterStarted() != 0)
    {
      ndbout_c("waitClusterStarted() failed");
      return NDBT_FAILED;
    }
    
    if (pDict->getTable("I3") == 0)
    {
      ndbout_c("Table disappered");
      return NDBT_FAILED;
    }
  }

  return NDBT_OK;
}


int
runWait(NDBT_Context* ctx, NDBT_Step* step)
{
  Uint32 waitSeconds = ctx->getProperty("WaitSeconds", Uint32(30));
  while (waitSeconds &&
         !ctx->isTestStopped())
  {    
    NdbSleep_MilliSleep(1000);
    waitSeconds --;
  }
  ctx->stopTest();
  return NDBT_OK;
}

bool versionsSpanBoundary(int verA, int verB, int incBoundaryVer)
{
  int minPeerVer = MIN(verA, verB);
  int maxPeerVer = MAX(verA, verB);

  return ( (minPeerVer <  incBoundaryVer) &&
           (maxPeerVer >= incBoundaryVer) );
}

#define SchemaTransVersion NDB_MAKE_VERSION(6,4,0)

int runPostUpgradeDecideDDL(NDBT_Context* ctx, NDBT_Step* step)
{
  /* We are running post-upgrade, now examine the versions
   * of connected nodes and update the 'NoDDL' variable
   * accordingly
   */
  /* DDL should be ok as long as
   *  1) All data nodes have the same version
   *  2) There is not some version specific exception
   */
  bool useDDL = true;

  Ndb* pNdb = GETNDB(step);
  NdbRestarter restarter;
  int minNdbVer = 0;
  int maxNdbVer = 0;
  int myVer = NDB_VERSION;

  if (restarter.getNodeTypeVersionRange(NDB_MGM_NODE_TYPE_NDB,
                                        minNdbVer,
                                        maxNdbVer) == -1)
  {
    g_err << "getNodeTypeVersionRange call failed" << endl;
    return NDBT_FAILED;
  }

  if (minNdbVer != maxNdbVer)
  {
    useDDL = false;
    ndbout << "Ndbd nodes have mixed versions, DDL not supported" << endl;
  }
  if (versionsSpanBoundary(myVer, minNdbVer, SchemaTransVersion))
  {
    useDDL = false;
    ndbout << "Api and Ndbd versions span schema-trans boundary, DDL not supported" << endl;
  }

  ctx->setProperty("NoDDL", useDDL?0:1);

  if (useDDL)
  {
    ndbout << "Dropping and recreating tables..." << endl;
    
    for (int i=0; i < NDBT_Tables::getNumTables(); i++)
    {  
      /* Drop table (ignoring rc if it doesn't exist etc...) */
      pNdb->getDictionary()->dropTable(NDBT_Tables::getTable(i)->getName());
      int ret= NDBT_Tables::createTable(pNdb, 
                                        NDBT_Tables::getTable(i)->getName(),
                                        false,   // temp
                                        false);  // exists ok
      if(ret)
      {
        NdbError err = pNdb->getDictionary()->getNdbError();

        g_err << "Failed to create table "
              << NDBT_Tables::getTable(i)->getName()
              << " error : " 
              << err
              << endl;

        /* Check for allowed exceptions during upgrade */
        if (err.code == 794)
        {
          /* Schema feature requires data node upgrade */
          if (minNdbVer >= myVer)
          {
            g_err << "Error 794 received, but data nodes are upgraded" << endl;
            // TODO : Dump versions here
            return NDBT_FAILED;
          }
          g_err << "Create table failure due to old version NDBDs, continuing" << endl;
        }
      }
    }
    ndbout << "Done" << endl;
  }

  return NDBT_OK;
}


NDBT_TESTSUITE(testUpgrade);
TESTCASE("Upgrade_NR1",
	 "Test that one node at a time can be upgraded"){
  INITIALIZER(runCheckStarted);
  INITIALIZER(runBug48416);
  STEP(runUpgrade_NR1);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_NR1")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_NR2",
	 "Test that one node in each nodegroup can be upgradde simultaneously"){
  INITIALIZER(runCheckStarted);
  STEP(runUpgrade_NR2);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_NR2")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_NR3",
	 "Test that one node in each nodegroup can be upgradde simultaneously"){
  INITIALIZER(runCheckStarted);
  STEP(runUpgrade_NR3);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_NR3")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_FS",
	 "Test that one node in each nodegroup can be upgrade simultaneously")
{
  TC_PROPERTY("KeepFS", 1);
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  INITIALIZER(runLoadAll);
  STEP(runUpgrade_Traffic);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_FS")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_Traffic",
	 "Test upgrade with traffic, all tables and restart --initial")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  STEP(runUpgrade_Traffic);
  STEP(runBasic);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_Traffic")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_Traffic_FS",
	 "Test upgrade with traffic, all tables and restart using FS")
{
  TC_PROPERTY("KeepFS", 1);
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  STEP(runUpgrade_Traffic);
  STEP(runBasic);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_Traffic_FS")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_Traffic_one",
	 "Test upgrade with traffic, *one* table and restart --initial")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateOneTable);
  STEP(runUpgrade_Traffic);
  STEP(runBasic);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_Traffic_one")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_Traffic_FS_one",
	 "Test upgrade with traffic, all tables and restart using FS")
{
  TC_PROPERTY("KeepFS", 1);
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateOneTable);
  STEP(runUpgrade_Traffic);
  STEP(runBasic);
  VERIFIER(startPostUpgradeChecks);
}
POSTUPGRADE("Upgrade_Traffic_FS_one")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeChecks);
}
TESTCASE("Upgrade_Api_Only",
         "Test that upgrading the Api node only works")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  VERIFIER(startPostUpgradeChecksApiFirst);
}
POSTUPGRADE("Upgrade_Api_Only")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeDecideDDL);
  INITIALIZER(runGetTableList);
  TC_PROPERTY("WaitSeconds", 30);
  STEP(runBasic);
  STEP(runPostUpgradeChecks);
  STEP(runWait);
  FINALIZER(runClearAll);
}
TESTCASE("Upgrade_Api_Before_NR1",
         "Test that upgrading the Api node before the kernel works")
{
  /* Api, then MGMD(s), then NDBDs */
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  VERIFIER(startPostUpgradeChecksApiFirst);
}
POSTUPGRADE("Upgrade_Api_Before_NR1")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeDecideDDL);
  INITIALIZER(runGetTableList);
  STEP(runBasic);
  STEP(runUpgrade_NR1); /* Upgrade kernel nodes using NR1 */
  FINALIZER(runPostUpgradeChecks);
  FINALIZER(runClearAll);
}
TESTCASE("Upgrade_Api_NDBD_MGMD",
         "Test that updating in reverse order works")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  VERIFIER(startPostUpgradeChecksApiFirst);
}
POSTUPGRADE("Upgrade_Api_NDBD_MGMD")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeDecideDDL);
  INITIALIZER(runGetTableList);
  STEP(runBasic);
  STEP(runUpgrade_NdbdFirst);
  FINALIZER(runPostUpgradeChecks);
  FINALIZER(runClearAll);
}
TESTCASE("Upgrade_Mixed_MGMD_API_NDBD",
         "Test that upgrading MGMD/API partially before data nodes works")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runCreateAllTables);
  STEP(runUpgrade_NotAllMGMD); /* Upgrade an MGMD */
  STEP(runBasic);
  VERIFIER(startPostUpgradeChecksApiFirst); /* Upgrade Api */
}
POSTUPGRADE("Upgrade_Mixed_MGMD_API_NDBD")
{
  INITIALIZER(runCheckStarted);
  INITIALIZER(runPostUpgradeDecideDDL);
  INITIALIZER(runGetTableList);
  INITIALIZER(runClearAll); /* Clear rows from old-ver basic run */
  STEP(runBasic);
  STEP(runUpgrade_NdbdFirst); /* Upgrade all Ndbds, then MGMDs finally */
  FINALIZER(runPostUpgradeChecks);
  FINALIZER(runClearAll);
}
  
NDBT_TESTSUITE_END(testUpgrade);

int main(int argc, const char** argv){
  ndb_init();
  NDBT_TESTSUITE_INSTANCE(testUpgrade);
  testUpgrade.setCreateAllTables(true);
  return testUpgrade.execute(argc, argv);
}

template class Vector<NodeInfo>;
