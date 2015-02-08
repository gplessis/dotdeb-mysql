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



#define DBACC_C
#include "Dbacc.hpp"

#define DEBUG(x) { ndbout << "ACC::" << x << endl; }

void Dbacc::initData() 
{
  cdirarraysize = ZDIRARRAY;
  coprecsize = ZOPRECSIZE;
  cpagesize = ZPAGESIZE;
  ctablesize = ZTABLESIZE;
  cfragmentsize = ZFRAGMENTSIZE;
  cdirrangesize = ZDIRRANGESIZE;
  coverflowrecsize = ZOVERFLOWRECSIZE;
  cscanRecSize = ZSCAN_REC_SIZE;

  
  dirRange = 0;
  directoryarray = 0;
  fragmentrec = 0;
  operationrec = 0;
  overflowRecord = 0;
  page8 = 0;
  scanRec = 0;
  tabrec = 0;

  cnoOfAllocatedPagesMax = cnoOfAllocatedPages = cpagesize = cpageCount = 0;
  // Records with constant sizes

  RSS_OP_COUNTER_INIT(cnoOfFreeFragrec);

}//Dbacc::initData()

void Dbacc::initRecords() 
{
  {
    AllocChunk chunks[16];
    const Uint32 pages = (cpagesize + 3) / 4;
    const Uint32 chunkcnt = allocChunks(chunks, 16, RT_DBTUP_PAGE, pages,
                                        CFG_DB_INDEX_MEM);

    /**
     * Set base ptr
     */
    Ptr<GlobalPage> pagePtr;
    m_shared_page_pool.getPtr(pagePtr, chunks[0].ptrI);
    page8 = (Page8*)pagePtr.p;

    /**
     * 1) Build free-list per chunk
     * 2) Add chunks to cfirstfreepage-list
     */
    cfirstfreepage = RNIL;
    cpagesize = 0;
    cpageCount = 0;
    for (Int32 i = chunkcnt - 1; i >= 0; i--)
    {
      Ptr<GlobalPage> pagePtr;
      m_shared_page_pool.getPtr(pagePtr, chunks[i].ptrI);
      const Uint32 cnt = 4 * chunks[i].cnt; // 4 8k per 32k
      Page8* base = (Page8*)pagePtr.p;
      ndbrequire(base >= page8);
      const Uint32 ptrI = Uint32(base - page8);
      for (Uint32 j = 0; j < cnt; j++)
      {
        refresh_watch_dog();
        base[j].word32[0] = ptrI + j + 1;
      }

      base[cnt-1].word32[0] = cfirstfreepage;
      cfirstfreepage = ptrI;

      cpageCount += cnt;
      if (ptrI + cnt > cpagesize)
        cpagesize = ptrI + cnt;
    }
  }

  operationrec = (Operationrec*)allocRecord("Operationrec",
					    sizeof(Operationrec),
					    coprecsize);

  dirRange = (DirRange*)allocRecord("DirRange",
				    sizeof(DirRange), 
				    cdirrangesize);

  directoryarray = (Directoryarray*)allocRecord("Directoryarray",
						sizeof(Directoryarray), 
						cdirarraysize);

  fragmentrec = (Fragmentrec*)allocRecord("Fragmentrec",
					  sizeof(Fragmentrec), 
					  cfragmentsize);

  overflowRecord = (OverflowRecord*)allocRecord("OverflowRecord",
						sizeof(OverflowRecord),
						coverflowrecsize);

  scanRec = (ScanRec*)allocRecord("ScanRec",
				  sizeof(ScanRec), 
				  cscanRecSize);

  tabrec = (Tabrec*)allocRecord("Tabrec",
				sizeof(Tabrec),
				ctablesize);
}//Dbacc::initRecords()

Dbacc::Dbacc(Block_context& ctx, Uint32 instanceNumber):
  SimulatedBlock(DBACC, ctx, instanceNumber),
  c_tup(0)
{
  BLOCK_CONSTRUCTOR(Dbacc);

  // Transit signals
  addRecSignal(GSN_DUMP_STATE_ORD, &Dbacc::execDUMP_STATE_ORD);
  addRecSignal(GSN_DEBUG_SIG, &Dbacc::execDEBUG_SIG);
  addRecSignal(GSN_CONTINUEB, &Dbacc::execCONTINUEB);
  addRecSignal(GSN_ACC_CHECK_SCAN, &Dbacc::execACC_CHECK_SCAN);
  addRecSignal(GSN_EXPANDCHECK2, &Dbacc::execEXPANDCHECK2);
  addRecSignal(GSN_SHRINKCHECK2, &Dbacc::execSHRINKCHECK2);
  addRecSignal(GSN_READ_PSEUDO_REQ, &Dbacc::execREAD_PSEUDO_REQ);

  // Received signals
  addRecSignal(GSN_STTOR, &Dbacc::execSTTOR);
  addRecSignal(GSN_ACCKEYREQ, &Dbacc::execACCKEYREQ);
  addRecSignal(GSN_ACCSEIZEREQ, &Dbacc::execACCSEIZEREQ);
  addRecSignal(GSN_ACCFRAGREQ, &Dbacc::execACCFRAGREQ);
  addRecSignal(GSN_NEXT_SCANREQ, &Dbacc::execNEXT_SCANREQ);
  addRecSignal(GSN_ACC_ABORTREQ, &Dbacc::execACC_ABORTREQ);
  addRecSignal(GSN_ACC_SCANREQ, &Dbacc::execACC_SCANREQ);
  addRecSignal(GSN_ACCMINUPDATE, &Dbacc::execACCMINUPDATE);
  addRecSignal(GSN_ACC_COMMITREQ, &Dbacc::execACC_COMMITREQ);
  addRecSignal(GSN_ACC_TO_REQ, &Dbacc::execACC_TO_REQ);
  addRecSignal(GSN_ACC_LOCKREQ, &Dbacc::execACC_LOCKREQ);
  addRecSignal(GSN_NDB_STTOR, &Dbacc::execNDB_STTOR);
  addRecSignal(GSN_DROP_TAB_REQ, &Dbacc::execDROP_TAB_REQ);
  addRecSignal(GSN_READ_CONFIG_REQ, &Dbacc::execREAD_CONFIG_REQ, true);
  addRecSignal(GSN_DROP_FRAG_REQ, &Dbacc::execDROP_FRAG_REQ);

  addRecSignal(GSN_DBINFO_SCANREQ, &Dbacc::execDBINFO_SCANREQ);

  initData();

#ifdef VM_TRACE
  {
    void* tmp[] = { &expDirRangePtr,
		    &gnsDirRangePtr,
		    &newDirRangePtr,
		    &rdDirRangePtr,
		    &nciOverflowrangeptr,
                    &expDirptr,
                    &rdDirptr,
                    &sdDirptr,
                    &nciOverflowDirptr,
                    &fragrecptr,
                    &operationRecPtr,
                    &idrOperationRecPtr,
                    &mlpqOperPtr,
                    &queOperPtr,
                    &readWriteOpPtr,
                    &iopOverflowRecPtr,
                    &tfoOverflowRecPtr,
                    &porOverflowRecPtr,
                    &priOverflowRecPtr,
                    &rorOverflowRecPtr,
                    &sorOverflowRecPtr,
                    &troOverflowRecPtr,
                    &ancPageptr,
                    &colPageptr,
                    &ccoPageptr,
                    &datapageptr,
                    &delPageptr,
                    &excPageptr,
                    &expPageptr,
                    &gdiPageptr,
                    &gePageptr,
                    &gflPageptr,
                    &idrPageptr,
                    &ilcPageptr,
                    &inpPageptr,
                    &iopPageptr,
                    &lastPageptr,
                    &lastPrevpageptr,
                    &lcnPageptr,
                    &lcnCopyPageptr,
                    &lupPageptr,
                    &ciPageidptr,
                    &gsePageidptr,
                    &isoPageptr,
                    &nciPageidptr,
                    &rsbPageidptr,
                    &rscPageidptr,
                    &slPageidptr,
                    &sscPageidptr,
                    &rlPageptr,
                    &rlpPageptr,
                    &ropPageptr,
                    &rpPageptr,
                    &slPageptr,
                    &spPageptr,
                    &scanPtr,
                    &tabptr
    };
    init_globals_list(tmp, sizeof(tmp)/sizeof(tmp[0]));
  }
#endif
}//Dbacc::Dbacc()

Dbacc::~Dbacc() 
{
  deallocRecord((void **)&dirRange, "DirRange",
		sizeof(DirRange), 
		cdirrangesize);
  
  deallocRecord((void **)&directoryarray, "Directoryarray",
		sizeof(Directoryarray), 
		cdirarraysize);
  
  deallocRecord((void **)&fragmentrec, "Fragmentrec",
		sizeof(Fragmentrec), 
		cfragmentsize);
  
  deallocRecord((void **)&operationrec, "Operationrec",
		sizeof(Operationrec),
		coprecsize);
  
  deallocRecord((void **)&overflowRecord, "OverflowRecord",
		sizeof(OverflowRecord),
		coverflowrecsize);

  deallocRecord((void **)&scanRec, "ScanRec",
		sizeof(ScanRec), 
		cscanRecSize);
  
  deallocRecord((void **)&tabrec, "Tabrec",
		sizeof(Tabrec),
		ctablesize);
  }//Dbacc::~Dbacc()

BLOCK_FUNCTIONS(Dbacc)
