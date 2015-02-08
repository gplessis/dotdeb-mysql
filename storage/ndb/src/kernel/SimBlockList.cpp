/*
   Copyright (c) 2003, 2010, Oracle and/or its affiliates. All rights reserved.

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

#include "SimBlockList.hpp"
#include <Emulator.hpp>
#include <SimulatedBlock.hpp>
#include <Cmvmi.hpp>
#include <Ndbfs.hpp>
#include <Dbacc.hpp>
#include <Dbdict.hpp>
#include <Dbdih.hpp>
#include <Dblqh.hpp>
#include <Dbspj.hpp>
#include <Dbtc.hpp>
#include <Dbtup.hpp>
#include <Ndbcntr.hpp>
#include <Qmgr.hpp>
#include <Trix.hpp>
#include <Backup.hpp>
#include <DbUtil.hpp>
#include <Suma.hpp>
#include <Dbtux.hpp>
#include <tsman.hpp>
#include <lgman.hpp>
#include <pgman.hpp>
#include <restore.hpp>
#include <Dbinfo.hpp>
#include <NdbEnv.h>
#include <LocalProxy.hpp>
#include <DblqhProxy.hpp>
#include <DbspjProxy.hpp>
#include <DbaccProxy.hpp>
#include <DbtupProxy.hpp>
#include <DbtuxProxy.hpp>
#include <BackupProxy.hpp>
#include <RestoreProxy.hpp>
#include <PgmanProxy.hpp>
#include <mt.hpp>

#ifndef VM_TRACE
#define NEW_BLOCK(B) new B
#else
enum SIMBLOCKLIST_DUMMY { A_VALUE = 0 };

void * operator new (size_t sz, SIMBLOCKLIST_DUMMY dummy){
  char * tmp = (char *)malloc(sz);
  if (!tmp)
    abort();

#ifndef NDB_PURIFY
#ifdef VM_TRACE
  const int initValue = 0xf3;
#else
  const int initValue = 0x0;
#endif
  
  const int p = (sz / 4096);
  const int r = (sz % 4096);
  
  for(int i = 0; i<p; i++)
    memset(tmp+(i*4096), initValue, 4096);
  
  if(r > 0)
    memset(tmp+p*4096, initValue, r);

#endif
  
  return tmp;
}
#define NEW_BLOCK(B) new(A_VALUE) B
#endif

void
SimBlockList::load(EmulatorData& data){
  noOfBlocks = NO_OF_BLOCKS;
  theList = new SimulatedBlock * [noOfBlocks];
  if (!theList)
  {
    ERROR_SET(fatal, NDBD_EXIT_MEMALLOC,
              "Failed to create the block list", "");
  }

  Block_context ctx(*data.theConfiguration, *data.m_mem_manager);
  
  SimulatedBlock * fs = 0;
  {
    Uint32 dl;
    const ndb_mgm_configuration_iterator * p = 
      ctx.m_config.getOwnConfigIterator();
    if(p && !ndb_mgm_get_int_parameter(p, CFG_DB_DISCLESS, &dl) && dl){
      fs = NEW_BLOCK(VoidFs)(ctx);
    } else { 
      fs = NEW_BLOCK(Ndbfs)(ctx);
    }
  }

  const bool mtLqh = globalData.isNdbMtLqh;

  if (!mtLqh)
    theList[0] = NEW_BLOCK(Pgman)(ctx);
  else
    theList[0] = NEW_BLOCK(PgmanProxy)(ctx);
  theList[1]  = NEW_BLOCK(Lgman)(ctx);
  theList[2]  = NEW_BLOCK(Tsman)(ctx);
  if (!mtLqh)
    theList[3]  = NEW_BLOCK(Dbacc)(ctx);
  else
    theList[3]  = NEW_BLOCK(DbaccProxy)(ctx);
  theList[4]  = NEW_BLOCK(Cmvmi)(ctx);
  theList[5]  = fs;
  theList[6]  = NEW_BLOCK(Dbdict)(ctx);
  theList[7]  = NEW_BLOCK(Dbdih)(ctx);
  if (!mtLqh)
    theList[8]  = NEW_BLOCK(Dblqh)(ctx);
  else
    theList[8]  = NEW_BLOCK(DblqhProxy)(ctx);
  theList[9]  = NEW_BLOCK(Dbtc)(ctx);
  if (!mtLqh)
    theList[10] = NEW_BLOCK(Dbtup)(ctx);
  else
    theList[10] = NEW_BLOCK(DbtupProxy)(ctx);
  theList[11] = NEW_BLOCK(Ndbcntr)(ctx);
  theList[12] = NEW_BLOCK(Qmgr)(ctx);
  theList[13] = NEW_BLOCK(Trix)(ctx);
  if (!mtLqh)
    theList[14] = NEW_BLOCK(Backup)(ctx);
  else
    theList[14] = NEW_BLOCK(BackupProxy)(ctx);
  theList[15] = NEW_BLOCK(DbUtil)(ctx);
  theList[16] = NEW_BLOCK(Suma)(ctx);
  if (!mtLqh)
    theList[17] = NEW_BLOCK(Dbtux)(ctx);
  else
    theList[17] = NEW_BLOCK(DbtuxProxy)(ctx);
  if (!mtLqh)
    theList[18] = NEW_BLOCK(Restore)(ctx);
  else
    theList[18] = NEW_BLOCK(RestoreProxy)(ctx);
  theList[19] = NEW_BLOCK(Dbinfo)(ctx);
  theList[20]  = NEW_BLOCK(Dbspj)(ctx);
  assert(NO_OF_BLOCKS == 21);

  if (globalData.isNdbMt) {
    add_main_thr_map();
    if (globalData.isNdbMtLqh) {
      for (int i = 0; i < noOfBlocks; i++)
        theList[i]->loadWorkers();
    }
    finalize_thr_map();
  }

  // Check that all blocks could be created
  for (int i = 0; i < noOfBlocks; i++)
  {
    if (!theList[i])
    {
      ERROR_SET(fatal, NDBD_EXIT_MEMALLOC,
                "Failed to create block", "");
    }
  }
}

void
SimBlockList::unload(){
  if(theList != 0){
    for(int i = 0; i<noOfBlocks; i++){
      if(theList[i] != 0){
#ifdef VM_TRACE
	theList[i]->~SimulatedBlock();
	free(theList[i]);
#else
        delete(theList[i]);
#endif
	theList[i] = 0;
      }
    }
    delete [] theList;
    theList    = 0;
    noOfBlocks = 0;
  }
}
