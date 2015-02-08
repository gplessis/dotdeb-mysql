/*
   Copyright (c) 2004, 2011, Oracle and/or its affiliates. All rights reserved.

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

#define DBSPJ_C
#include "Dbspj.hpp"

#include <SectionReader.hpp>
#include <signaldata/LqhKey.hpp>
#include <signaldata/QueryTree.hpp>
#include <signaldata/TcKeyRef.hpp>
#include <signaldata/RouteOrd.hpp>
#include <signaldata/TransIdAI.hpp>
#include <signaldata/DiGetNodes.hpp>
#include <signaldata/DihScanTab.hpp>
#include <signaldata/AttrInfo.hpp>
#include <Interpreter.hpp>
#include <AttributeHeader.hpp>
#include <AttributeDescriptor.hpp>
#include <KeyDescriptor.hpp>
#include <md5_hash.hpp>
#include <signaldata/TcKeyConf.hpp>

#include <signaldata/NodeFailRep.hpp>
#include <signaldata/ReadNodesConf.hpp>

// Use DEBUG to print messages that should be
// seen only when we debug the product

#ifdef VM_TRACE

#define DEBUG(x) ndbout << "DBSPJ: "<< x << endl;
#define DEBUG_LQHKEYREQ
#define DEBUG_SCAN_FRAGREQ

#else

#define DEBUG(x)

#endif

#if 1
#define DEBUG_CRASH() ndbrequire(false)
#else
#define DEBUG_CRASH()
#endif

#if 1
#undef DEBUG
#define DEBUG(x)
#undef DEBUG_LQHKEYREQ
#undef DEBUG_SCAN_FRAGREQ
#endif

const Ptr<Dbspj::TreeNode> Dbspj::NullTreeNodePtr = { 0, RNIL };
const Dbspj::RowRef Dbspj::NullRowRef = { RNIL, GLOBAL_PAGE_SIZE_WORDS, { 0 } };

/** A noop for now.*/
void Dbspj::execREAD_CONFIG_REQ(Signal* signal)
{
  jamEntry();
  const ReadConfigReq req =
    *reinterpret_cast<const ReadConfigReq*>(signal->getDataPtr());

  Pool_context pc;
  pc.m_block = this;

  DEBUG("execREAD_CONFIG_REQ");
  DEBUG("sizeof(Request): " << sizeof(Request) <<
        " sizeof(TreeNode): " << sizeof(TreeNode));

  m_arenaAllocator.init(1024, RT_SPJ_ARENA_BLOCK, pc);
  m_request_pool.arena_pool_init(&m_arenaAllocator, RT_SPJ_REQUEST, pc);
  m_treenode_pool.arena_pool_init(&m_arenaAllocator, RT_SPJ_TREENODE, pc);
  m_scanfraghandle_pool.arena_pool_init(&m_arenaAllocator, RT_SPJ_SCANFRAG, pc);
  m_lookup_request_hash.setSize(16);
  m_scan_request_hash.setSize(16);
  void* ptr = m_ctx.m_mm.get_memroot();
  m_page_pool.set((RowPage*)ptr, (Uint32)~0);

  Record_info ri;
  Dependency_map::createRecordInfo(ri, RT_SPJ_DATABUFFER);
  m_dependency_map_pool.init(&m_arenaAllocator, ri, pc);

  ReadConfigConf* const conf =
    reinterpret_cast<ReadConfigConf*>(signal->getDataPtrSend());
  conf->senderRef = reference();
  conf->senderData = req.senderData;

  sendSignal(req.senderRef, GSN_READ_CONFIG_CONF, signal,
             ReadConfigConf::SignalLength, JBB);
}//Dbspj::execREAD_CONF_REQ()

static Uint32 f_STTOR_REF = 0;

void Dbspj::execSTTOR(Signal* signal)
{
//#define UNIT_TEST_DATABUFFER2

  jamEntry();
  /* START CASE */
  const Uint16 tphase = signal->theData[1];
  f_STTOR_REF = signal->getSendersBlockRef();

  ndbout << "Dbspj::execSTTOR() inst:" << instance()
         << " phase=" << tphase << endl;

  if (tphase == 1)
  {
    jam();
    signal->theData[0] = 0;
    sendSignalWithDelay(reference(), GSN_CONTINUEB, signal, 1000, 1);
  }

  if (tphase == 4)
  {
    jam();

    signal->theData[0] = reference();
    sendSignal(NDBCNTR_REF, GSN_READ_NODESREQ, signal, 1, JBB);
    return;
  }

  sendSTTORRY(signal);

#ifdef UNIT_TEST_DATABUFFER2
  if (tphase == 120)
  {
    ndbout_c("basic test of ArenaPool / DataBuffer2");

    for (Uint32 i = 0; i<100; i++)
    {
      ArenaHead ah;
      if (!m_arenaAllocator.seize(ah))
      {
        ndbout_c("Failed to allocate arena");
        break;
      }

      ndbout_c("*** LOOP %u", i);
      Uint32 sum = 0;
      Dependency_map::Head head;
      LocalArenaPoolImpl pool(ah, m_dependency_map_pool);
      for (Uint32 j = 0; j<100; j++)
      {
        Uint32 sz = rand() % 1000;
        if (0)
          ndbout_c("adding %u", sz);
        Local_dependency_map list(pool, head);
        for (Uint32 i = 0; i<sz; i++)
          signal->theData[i] = sum + i;
        list.append(signal->theData, sz);
        sum += sz;
      }

      {
        ndbrequire(head.getSize() == sum);
        Local_dependency_map list(pool, head);
        Dependency_map::ConstDataBufferIterator it;
        Uint32 cnt = 0;
        for (list.first(it); !it.isNull(); list.next(it))
        {
          ndbrequire(* it.data == cnt);
          cnt++;
        }

        ndbrequire(cnt == sum);
      }

      Resource_limit rl;
      if (m_ctx.m_mm.get_resource_limit(7, rl))
      {
        ndbout_c("Resource %d min: %d max: %d curr: %d",
                 7, rl.m_min, rl.m_max, rl.m_curr);
      }

      {
        ndbout_c("release map");
        Local_dependency_map list(pool, head);
        list.release();
      }

      ndbout_c("release all");
      m_arenaAllocator.release(ah);
      ndbout_c("*** LOOP %u sum: %u", i, sum);
    }
  }
#endif
}//Dbspj::execSTTOR()

void
Dbspj::sendSTTORRY(Signal* signal)
{
  signal->theData[0] = 0;
  signal->theData[1] = 0;    /* BLOCK CATEGORY */
  signal->theData[2] = 0;    /* SIGNAL VERSION NUMBER */
  signal->theData[3] = 4;
#ifdef UNIT_TEST_DATABUFFER2
  signal->theData[4] = 120;  /* Start phase end*/
#else
  signal->theData[4] = 255;
#endif
  signal->theData[5] = 255;
  sendSignal(f_STTOR_REF, GSN_STTORRY, signal, 6, JBB);
}

void
Dbspj::execREAD_NODESCONF(Signal* signal)
{
  jamEntry();

  ReadNodesConf * const conf = (ReadNodesConf *)signal->getDataPtr();

  if (getNodeState().getNodeRestartInProgress())
  {
    jam();
    c_alive_nodes.assign(NdbNodeBitmask::Size, conf->startedNodes);
    c_alive_nodes.set(getOwnNodeId());
  }
  else
  {
    jam();
    c_alive_nodes.assign(NdbNodeBitmask::Size, conf->startingNodes);
    NdbNodeBitmask tmp;
    tmp.assign(NdbNodeBitmask::Size, conf->startedNodes);
    c_alive_nodes.bitOR(tmp);
  }

  sendSTTORRY(signal);
}

void
Dbspj::execINCL_NODEREQ(Signal* signal)
{
  jamEntry();
  const Uint32 senderRef = signal->theData[0];
  const Uint32 nodeId  = signal->theData[1];

  ndbrequire(!c_alive_nodes.get(nodeId));
  c_alive_nodes.set(nodeId);

  signal->theData[0] = nodeId;
  signal->theData[1] = reference();
  sendSignal(senderRef, GSN_INCL_NODECONF, signal, 2, JBB);
}

void
Dbspj::execNODE_FAILREP(Signal* signal)
{
  jamEntry();

  const NodeFailRep * rep = (NodeFailRep*)signal->getDataPtr();
  NdbNodeBitmask failed;
  failed.assign(NdbNodeBitmask::Size, rep->theNodes);

  c_alive_nodes.bitANDC(failed);

  signal->theData[0] = 1;
  signal->theData[1] = 0;
  failed.copyto(NdbNodeBitmask::Size, signal->theData + 2);
  sendSignal(reference(), GSN_CONTINUEB, signal, 2 + NdbNodeBitmask::Size,
             JBB);
}

void
Dbspj::execAPI_FAILREQ(Signal* signal)
{
  jamEntry();
  Uint32 failedApiNode = signal->theData[0];
  ndbrequire(signal->theData[1] == QMGR_REF); // As callback hard-codes QMGR

  /**
   * We only need to care about lookups
   *   as SCAN's are aborted by DBTC
   */

  signal->theData[0] = failedApiNode;
  signal->theData[1] = reference();
  sendSignal(QMGR_REF, GSN_API_FAILCONF, signal, 2, JBB);
}

void
Dbspj::execCONTINUEB(Signal* signal)
{
  jamEntry();
  switch(signal->theData[0]) {
  case 0:
    releaseGlobal(signal);
    return;
  case 1:
    nodeFail_checkRequests(signal);
    return;
  case 2:
    nodeFail_checkRequests(signal);
    return;
  }

  ndbrequire(false);
}

void
Dbspj::nodeFail_checkRequests(Signal* signal)
{
  jam();
  const Uint32 type = signal->theData[0];
  const Uint32 bucket = signal->theData[1];

  NdbNodeBitmask failed;
  failed.assign(NdbNodeBitmask::Size, signal->theData+2);

  Request_iterator iter;
  Request_hash * hash;
  switch(type){
  case 1:
    hash = &m_lookup_request_hash;
    break;
  case 2:
    hash = &m_scan_request_hash;
    break;
  }
  hash->next(bucket, iter);

  const Uint32 RT_BREAK = 64;
  for(Uint32 i = 0; (i<RT_BREAK || iter.bucket == bucket) &&
        !iter.curr.isNull(); i++)
  {
    jam();

    Ptr<Request> requestPtr = iter.curr;
    hash->next(iter);
    i += nodeFail(signal, requestPtr, failed);
  }

  if (!iter.curr.isNull())
  {
    jam();
    signal->theData[0] = type;
    signal->theData[1] = bucket;
    failed.copyto(NdbNodeBitmask::Size, signal->theData+2);
    sendSignal(reference(), GSN_CONTINUEB, signal, 2 + NdbNodeBitmask::Size,
               JBB);
  }
  else if (type == 1)
  {
    jam();
    signal->theData[0] = 2;
    signal->theData[1] = 0;
    failed.copyto(NdbNodeBitmask::Size, signal->theData+2);
    sendSignal(reference(), GSN_CONTINUEB, signal, 2 + NdbNodeBitmask::Size,
               JBB);
  }
  else if (type == 2)
  {
    jam();
    ndbout_c("Finished with handling node-failure");
  }
}

/**
 * MODULE LQHKEYREQ
 */
void Dbspj::execLQHKEYREQ(Signal* signal)
{
  jamEntry();
  c_Counters.incr_counter(CI_READS_RECEIVED, 1);

  const LqhKeyReq* req = reinterpret_cast<const LqhKeyReq*>(signal->getDataPtr());

  /**
   * #0 - KEYINFO contains key for first operation (used for hash in TC)
   * #1 - ATTRINFO contains tree + parameters
   *      (unless StoredProcId is set, when only paramters are sent,
   *       but this is not yet implemented)
   */
  SectionHandle handle = SectionHandle(this, signal);
  SegmentedSectionPtr ssPtr;
  handle.getSection(ssPtr, LqhKeyReq::AttrInfoSectionNum);

  Uint32 err;
  Ptr<Request> requestPtr = { 0, RNIL };
  do
  {
    ArenaHead ah;
    err = DbspjErr::OutOfQueryMemory;
    if (unlikely(!m_arenaAllocator.seize(ah)))
      break;


    m_request_pool.seize(ah, requestPtr);

    new (requestPtr.p) Request(ah);
    do_init(requestPtr.p, req, signal->getSendersBlockRef());

    Uint32 len_cnt;

    {
      SectionReader r0(ssPtr, getSectionSegmentPool());

      err = DbspjErr::ZeroLengthQueryTree;
      if (unlikely(!r0.getWord(&len_cnt)))
        break;
    }

    Uint32 len = QueryTree::getLength(len_cnt);
    Uint32 cnt = QueryTree::getNodeCnt(len_cnt);

    {
      SectionReader treeReader(ssPtr, getSectionSegmentPool());
      SectionReader paramReader(ssPtr, getSectionSegmentPool());
      paramReader.step(len); // skip over tree to parameters

      Build_context ctx;
      ctx.m_resultRef = req->variableData[0];
      ctx.m_savepointId = req->savePointId;
      ctx.m_scanPrio = 1;
      ctx.m_start_signal = signal;
      ctx.m_keyPtr.i = handle.m_ptr[LqhKeyReq::KeyInfoSectionNum].i;
      ctx.m_senderRef = signal->getSendersBlockRef();

      err = build(ctx, requestPtr, treeReader, paramReader);
      if (unlikely(err != 0))
        break;
    }

    /**
     * a query being shipped as a LQHKEYREQ may only return finite rows
     *   i.e be a (multi-)lookup
     */
    ndbassert(requestPtr.p->isLookup());
    ndbassert(requestPtr.p->m_node_cnt == cnt);
    err = DbspjErr::InvalidRequest;
    if (unlikely(!requestPtr.p->isLookup() || requestPtr.p->m_node_cnt != cnt))
      break;

    /**
     * Store request in list(s)/hash(es)
     */
    store_lookup(requestPtr);

    release(ssPtr);
    handle.clear();

    start(signal, requestPtr);
    return;
  } while (0);

  /**
   * Error handling below,
   *  'err' may contain error code.
   */
  if (!requestPtr.isNull())
  {
    jam();
    m_request_pool.release(requestPtr);
  }
  releaseSections(handle);
  handle_early_lqhkey_ref(signal, req, err);
}

void
Dbspj::do_init(Request* requestP, const LqhKeyReq* req, Uint32 senderRef)
{
  requestP->m_bits = 0;
  requestP->m_errCode = 0;
  requestP->m_state = Request::RS_BUILDING;
  requestP->m_node_cnt = 0;
  requestP->m_cnt_active = 0;
  requestP->m_rows = 0;
  requestP->m_active_nodes.clear();
  requestP->m_outstanding = 0;
  requestP->m_transId[0] = req->transId1;
  requestP->m_transId[1] = req->transId2;
  bzero(requestP->m_lookup_node_data, sizeof(requestP->m_lookup_node_data));
#ifdef SPJ_TRACE_TIME
  requestP->m_cnt_batches = 0;
  requestP->m_sum_rows = 0;
  requestP->m_sum_running = 0;
  requestP->m_sum_waiting = 0;
  requestP->m_save_time = spj_now();
#endif
  const Uint32 reqInfo = req->requestInfo;
  Uint32 tmp = req->clientConnectPtr;
  if (LqhKeyReq::getDirtyFlag(reqInfo) &&
      LqhKeyReq::getOperation(reqInfo) == ZREAD)
  {
    jam();

    ndbrequire(LqhKeyReq::getApplicationAddressFlag(reqInfo));
    //const Uint32 apiRef   = lqhKeyReq->variableData[0];
    //const Uint32 apiOpRec = lqhKeyReq->variableData[1];
    tmp = req->variableData[1];
    requestP->m_senderData = tmp;
    requestP->m_senderRef = senderRef;
  }
  else
  {
    if (LqhKeyReq::getSameClientAndTcFlag(reqInfo) == 1)
    {
      if (LqhKeyReq::getApplicationAddressFlag(reqInfo))
        tmp = req->variableData[2];
      else
        tmp = req->variableData[0];
    }
    requestP->m_senderData = tmp;
    requestP->m_senderRef = senderRef;
  }
  requestP->m_rootResultData = tmp;
}

void
Dbspj::store_lookup(Ptr<Request> requestPtr)
{
  ndbassert(requestPtr.p->isLookup());
  Ptr<Request> tmp;
  bool found = m_lookup_request_hash.find(tmp, *requestPtr.p);
  ndbrequire(found == false);
  m_lookup_request_hash.add(requestPtr);
}

void
Dbspj::handle_early_lqhkey_ref(Signal* signal,
                               const LqhKeyReq * lqhKeyReq,
                               Uint32 err)
{
  /**
   * Error path...
   */
  ndbrequire(err);
  const Uint32 reqInfo = lqhKeyReq->requestInfo;
  const Uint32 transid[2] = { lqhKeyReq->transId1, lqhKeyReq->transId2 };

  if (LqhKeyReq::getDirtyFlag(reqInfo) &&
      LqhKeyReq::getOperation(reqInfo) == ZREAD)
  {
    jam();
    /* Dirty read sends TCKEYREF direct to client, and nothing to TC */
    ndbrequire(LqhKeyReq::getApplicationAddressFlag(reqInfo));
    const Uint32 apiRef   = lqhKeyReq->variableData[0];
    const Uint32 apiOpRec = lqhKeyReq->variableData[1];

    TcKeyRef* const tcKeyRef = reinterpret_cast<TcKeyRef*>(signal->getDataPtrSend());

    tcKeyRef->connectPtr = apiOpRec;
    tcKeyRef->transId[0] = transid[0];
    tcKeyRef->transId[1] = transid[1];
    tcKeyRef->errorCode = err;
    sendTCKEYREF(signal, apiRef, signal->getSendersBlockRef());
  }
  else
  {
    jam();
    const Uint32 returnref = signal->getSendersBlockRef();
    const Uint32 clientPtr = lqhKeyReq->clientConnectPtr;

    Uint32 TcOprec = clientPtr;
    if (LqhKeyReq::getSameClientAndTcFlag(reqInfo) == 1)
    {
      if (LqhKeyReq::getApplicationAddressFlag(reqInfo))
        TcOprec = lqhKeyReq->variableData[2];
      else
        TcOprec = lqhKeyReq->variableData[0];
    }

    LqhKeyRef* const ref = reinterpret_cast<LqhKeyRef*>(signal->getDataPtrSend());
    ref->userRef = clientPtr;
    ref->connectPtr = TcOprec;
    ref->errorCode = err;
    ref->transId1 = transid[0];
    ref->transId2 = transid[1];
    sendSignal(returnref, GSN_LQHKEYREF, signal,
               LqhKeyRef::SignalLength, JBB);
  }
}

void
Dbspj::sendTCKEYREF(Signal* signal, Uint32 ref, Uint32 routeRef)
{
  const Uint32 nodeId = refToNode(ref);
  const bool connectedToNode = getNodeInfo(nodeId).m_connected;

  if (likely(connectedToNode))
  {
    jam();
    sendSignal(ref, GSN_TCKEYREF, signal, TcKeyRef::SignalLength, JBB);
  }
  else
  {
    jam();
    memmove(signal->theData+25, signal->theData, 4*TcKeyRef::SignalLength);
    RouteOrd* ord = (RouteOrd*)signal->getDataPtrSend();
    ord->dstRef = ref;
    ord->srcRef = reference();
    ord->gsn = GSN_TCKEYREF;
    ord->cnt = 0;
    LinearSectionPtr ptr[3];
    ptr[0].p = signal->theData+25;
    ptr[0].sz = TcKeyRef::SignalLength;
    sendSignal(routeRef, GSN_ROUTE_ORD, signal, RouteOrd::SignalLength, JBB,
               ptr, 1);
  }
}

void
Dbspj::sendTCKEYCONF(Signal* signal, Uint32 len, Uint32 ref, Uint32 routeRef)
{
  const Uint32 nodeId = refToNode(ref);
  const bool connectedToNode = getNodeInfo(nodeId).m_connected;

  if (likely(connectedToNode))
  {
    jam();
    sendSignal(ref, GSN_TCKEYCONF, signal, len, JBB);
  }
  else
  {
    jam();
    memmove(signal->theData+25, signal->theData, 4*len);
    RouteOrd* ord = (RouteOrd*)signal->getDataPtrSend();
    ord->dstRef = ref;
    ord->srcRef = reference();
    ord->gsn = GSN_TCKEYCONF;
    ord->cnt = 0;
    LinearSectionPtr ptr[3];
    ptr[0].p = signal->theData+25;
    ptr[0].sz = len;
    sendSignal(routeRef, GSN_ROUTE_ORD, signal, RouteOrd::SignalLength, JBB,
               ptr, 1);
  }
}

/**
 * END - MODULE LQHKEYREQ
 */


/**
 * MODULE SCAN_FRAGREQ
 */
void
Dbspj::execSCAN_FRAGREQ(Signal* signal)
{
  jamEntry();

  /* Reassemble if the request was fragmented */
  if (!assembleFragments(signal))
  {
    jam();
    return;
  }

  const ScanFragReq * req = (ScanFragReq *)&signal->theData[0];

#ifdef DEBUG_SCAN_FRAGREQ
  ndbout_c("Incomming SCAN_FRAGREQ ");
  printSCAN_FRAGREQ(stdout, signal->getDataPtrSend(),
                    ScanFragReq::SignalLength + 2,
                    DBLQH);
#endif

  /**
   * #0 - ATTRINFO contains tree + parameters
   *      (unless StoredProcId is set, when only paramters are sent,
   *       but this is not yet implemented)
   * #1 - KEYINFO if first op is index scan - contains bounds for first scan
   *              if first op is lookup - contains keyinfo for lookup
   */
  SectionHandle handle = SectionHandle(this, signal);
  SegmentedSectionPtr ssPtr;
  handle.getSection(ssPtr, ScanFragReq::AttrInfoSectionNum);

  Uint32 err;
  Ptr<Request> requestPtr = { 0, RNIL };
  do
  {
    ArenaHead ah;
    err = DbspjErr::OutOfQueryMemory;
    if (unlikely(!m_arenaAllocator.seize(ah)))
      break;

    m_request_pool.seize(ah, requestPtr);

    new (requestPtr.p) Request(ah);
    do_init(requestPtr.p, req, signal->getSendersBlockRef());

    Uint32 len_cnt;
    {
      SectionReader r0(ssPtr, getSectionSegmentPool());
      err = DbspjErr::ZeroLengthQueryTree;
      if (unlikely(!r0.getWord(&len_cnt)))
        break;
    }

    Uint32 len = QueryTree::getLength(len_cnt);
    Uint32 cnt = QueryTree::getNodeCnt(len_cnt);

    {
      SectionReader treeReader(ssPtr, getSectionSegmentPool());
      SectionReader paramReader(ssPtr, getSectionSegmentPool());
      paramReader.step(len); // skip over tree to parameters

      Build_context ctx;
      ctx.m_resultRef = req->resultRef;
      ctx.m_scanPrio = ScanFragReq::getScanPrio(req->requestInfo);
      ctx.m_savepointId = req->savePointId;
      ctx.m_batch_size_rows = req->batch_size_rows;
      ctx.m_start_signal = signal;
      ctx.m_senderRef = signal->getSendersBlockRef();

      if (handle.m_cnt > 1)
      {
        jam();
        ctx.m_keyPtr.i = handle.m_ptr[ScanFragReq::KeyInfoSectionNum].i;
      }
      else
      {
        jam();
        ctx.m_keyPtr.i = RNIL;
      }

      err = build(ctx, requestPtr, treeReader, paramReader);
      if (unlikely(err != 0))
        break;
    }

    ndbassert(requestPtr.p->isScan());
    ndbassert(requestPtr.p->m_node_cnt == cnt);
    err = DbspjErr::InvalidRequest;
    if (unlikely(!requestPtr.p->isScan() || requestPtr.p->m_node_cnt != cnt))
      break;

    /**
     * Store request in list(s)/hash(es)
     */
    store_scan(requestPtr);

    release(ssPtr);
    handle.clear();

    start(signal, requestPtr);
    return;
  } while (0);

  if (!requestPtr.isNull())
  {
    jam();
    m_request_pool.release(requestPtr);
  }
  releaseSections(handle);
  handle_early_scanfrag_ref(signal, req, err);
}

void
Dbspj::do_init(Request* requestP, const ScanFragReq* req, Uint32 senderRef)
{
  requestP->m_bits = 0;
  requestP->m_errCode = 0;
  requestP->m_state = Request::RS_BUILDING;
  requestP->m_node_cnt = 0;
  requestP->m_cnt_active = 0;
  requestP->m_rows = 0;
  requestP->m_active_nodes.clear();
  requestP->m_outstanding = 0;
  requestP->m_senderRef = senderRef;
  requestP->m_senderData = req->senderData;
  requestP->m_transId[0] = req->transId1;
  requestP->m_transId[1] = req->transId2;
  requestP->m_rootResultData = req->resultData;
  bzero(requestP->m_lookup_node_data, sizeof(requestP->m_lookup_node_data));
#ifdef SPJ_TRACE_TIME
  requestP->m_cnt_batches = 0;
  requestP->m_sum_rows = 0;
  requestP->m_sum_running = 0;
  requestP->m_sum_waiting = 0;
  requestP->m_save_time = spj_now();
#endif
}

void
Dbspj::store_scan(Ptr<Request> requestPtr)
{
  ndbassert(requestPtr.p->isScan());
  Ptr<Request> tmp;
  bool found = m_scan_request_hash.find(tmp, *requestPtr.p);
  ndbrequire(found == false);
  m_scan_request_hash.add(requestPtr);
}

void
Dbspj::handle_early_scanfrag_ref(Signal* signal,
                                 const ScanFragReq * _req,
                                 Uint32 err)
{
  ScanFragReq req = *_req;
  Uint32 senderRef = signal->getSendersBlockRef();

  ScanFragRef * ref = (ScanFragRef*)&signal->theData[0];
  ref->senderData = req.senderData;
  ref->transId1 = req.transId1;
  ref->transId2 = req.transId2;
  ref->errorCode = err;
  sendSignal(senderRef, GSN_SCAN_FRAGREF, signal,
             ScanFragRef::SignalLength, JBB);
}

/**
 * END - MODULE SCAN_FRAGREQ
 */

/**
 * MODULE GENERIC
 */
Uint32
Dbspj::build(Build_context& ctx,
             Ptr<Request> requestPtr,
             SectionReader & tree,
             SectionReader & param)
{
  Uint32 tmp0, tmp1;
  Uint32 err = DbspjErr::ZeroLengthQueryTree;
  ctx.m_cnt = 0;
  ctx.m_scan_cnt = 0;

  tree.getWord(&tmp0);
  Uint32 loop = QueryTree::getNodeCnt(tmp0);

  DEBUG("::build()");
  err = DbspjErr::InvalidTreeNodeCount;
  if (loop == 0 || loop > NDB_SPJ_MAX_TREE_NODES)
  {
    DEBUG_CRASH();
    goto error;
  }

  while (ctx.m_cnt < loop)
  {
    DEBUG(" - loop " << ctx.m_cnt << " pos: " << tree.getPos().currPos);
    tree.peekWord(&tmp0);
    param.peekWord(&tmp1);
    Uint32 node_op = QueryNode::getOpType(tmp0);
    Uint32 node_len = QueryNode::getLength(tmp0);
    Uint32 param_op = QueryNodeParameters::getOpType(tmp1);
    Uint32 param_len = QueryNodeParameters::getLength(tmp1);

    err = DbspjErr::QueryNodeTooBig;
    if (unlikely(node_len >= NDB_ARRAY_SIZE(m_buffer0)))
    {
      DEBUG_CRASH();
      goto error;
    }

    err = DbspjErr::QueryNodeParametersTooBig;
    if (unlikely(param_len >= NDB_ARRAY_SIZE(m_buffer1)))
    {
      DEBUG_CRASH();
      goto error;
    }

    err = DbspjErr::InvalidTreeNodeSpecification;
    if (unlikely(tree.getWords(m_buffer0, node_len) == false))
    {
      DEBUG_CRASH();
      goto error;
    }

    err = DbspjErr::InvalidTreeParametersSpecification;
    if (unlikely(param.getWords(m_buffer1, param_len) == false))
    {
      DEBUG_CRASH();
      goto error;
    }

#if defined(DEBUG_LQHKEYREQ) || defined(DEBUG_SCAN_FRAGREQ)
    printf("node: ");
    for (Uint32 i = 0; i<node_len; i++)
      printf("0x%.8x ", m_buffer0[i]);
    printf("\n");

    printf("param: ");
    for (Uint32 i = 0; i<param_len; i++)
      printf("0x%.8x ", m_buffer1[i]);
    printf("\n");
#endif

    err = DbspjErr::UnknowQueryOperation;
    if (unlikely(node_op != param_op))
    {
      DEBUG_CRASH();
      goto error;
    }

    const OpInfo* info = getOpInfo(node_op);
    if (unlikely(info == 0))
    {
      DEBUG_CRASH();
      goto error;
    }

    QueryNode* qn = (QueryNode*)m_buffer0;
    QueryNodeParameters * qp = (QueryNodeParameters*)m_buffer1;
    qn->len = node_len;
    qp->len = param_len;
    err = (this->*(info->m_build))(ctx, requestPtr, qn, qp);
    if (unlikely(err != 0))
    {
      DEBUG_CRASH();
      goto error;
    }

    /**
     * only first node gets access to signal
     */
    ctx.m_start_signal = 0;

    /**
     * TODO handle error, by aborting request
     */
    ndbrequire(ctx.m_cnt < NDB_ARRAY_SIZE(ctx.m_node_list));
    ctx.m_cnt++;
  }
  requestPtr.p->m_node_cnt = ctx.m_cnt;

  /**
   * Init ROW_BUFFERS for those TreeNodes requiring either
   * T_ROW_BUFFER or T_ROW_BUFFER_MAP.
   */
  if (requestPtr.p->m_bits & Request::RT_ROW_BUFFERS)
  {
    Ptr<TreeNode> treeNodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(treeNodePtr); !treeNodePtr.isNull(); list.next(treeNodePtr))
    {
      if (treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER_MAP)
      {
        jam();
        treeNodePtr.p->m_row_map.init();
      }
      else if (treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER)
      {
        jam();
        treeNodePtr.p->m_row_list.init();
      }
    }
  }

  if (ctx.m_scan_cnt > 1)
  {
    jam();
    requestPtr.p->m_bits |= Request::RT_MULTI_SCAN;

    /**
     * Iff, multi-scan is non-bushy (normal case)
     *   we don't strictly need RT_VAR_ALLOC for RT_ROW_BUFFERS
     *   but could instead pop-row stack frame,
     *     however this is not implemented...
     *
     * so, use RT_VAR_ALLOC
     */
    if (requestPtr.p->m_bits & Request::RT_ROW_BUFFERS)
    {
      jam();
      requestPtr.p->m_bits |= Request::RT_VAR_ALLOC;
    }
  }

  return 0;

error:
  jam();
  return err;
}

Uint32
Dbspj::createNode(Build_context& ctx, Ptr<Request> requestPtr,
                  Ptr<TreeNode> & treeNodePtr)
{
  /**
   * In the future, we can have different TreeNode-allocation strategies
   *   that can be setup using the Build_context
   *
   */
  if (m_treenode_pool.seize(requestPtr.p->m_arena, treeNodePtr))
  {
    DEBUG("createNode - seize -> ptrI: " << treeNodePtr.i);
    new (treeNodePtr.p) TreeNode(requestPtr.i);
    ctx.m_node_list[ctx.m_cnt] = treeNodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    list.addLast(treeNodePtr);
    treeNodePtr.p->m_node_no = ctx.m_cnt;
    return 0;
  }
  return DbspjErr::OutOfOperations;
}

void
Dbspj::start(Signal* signal,
             Ptr<Request> requestPtr)
{
  if (requestPtr.p->m_bits & Request::RT_NEED_PREPARE)
  {
    jam();
    requestPtr.p->m_outstanding = 0;
    requestPtr.p->m_state = Request::RS_PREPARING;

    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
    {
      jam();
      ndbrequire(nodePtr.p->m_info != 0);
      if (nodePtr.p->m_info->m_prepare != 0)
      {
        jam();
        (this->*(nodePtr.p->m_info->m_prepare))(signal, requestPtr, nodePtr);
      }
    }

    /**
     * preferably RT_NEED_PREPARE should only be set if blocking
     * calls are used, in which case m_outstanding should have been increased
     */
    ndbassert(requestPtr.p->m_outstanding);
  }

  checkPrepareComplete(signal, requestPtr, 0);
}

void
Dbspj::checkPrepareComplete(Signal * signal, Ptr<Request> requestPtr,
                            Uint32 cnt)
{
  ndbrequire(requestPtr.p->m_outstanding >= cnt);
  requestPtr.p->m_outstanding -= cnt;

  if (requestPtr.p->m_outstanding == 0)
  {
    jam();

    if (unlikely((requestPtr.p->m_state & Request::RS_ABORTING) != 0))
    {
      jam();
      batchComplete(signal, requestPtr);
      return;
    }

    requestPtr.p->m_state = Request::RS_RUNNING;
    Ptr<TreeNode> nodePtr;
    {
      Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
      ndbrequire(list.first(nodePtr));
    }
    ndbrequire(nodePtr.p->m_info != 0 && nodePtr.p->m_info->m_start != 0);
    (this->*(nodePtr.p->m_info->m_start))(signal, requestPtr, nodePtr);
  }
}

void
Dbspj::checkBatchComplete(Signal * signal, Ptr<Request> requestPtr,
                          Uint32 cnt)
{
  ndbrequire(requestPtr.p->m_outstanding >= cnt);
  requestPtr.p->m_outstanding -= cnt;

  if (requestPtr.p->m_outstanding == 0)
  {
    jam();
    batchComplete(signal, requestPtr);
  }
}

void
Dbspj::batchComplete(Signal* signal, Ptr<Request> requestPtr)
{
  ndbrequire(requestPtr.p->m_outstanding == 0); // "definition" of batchComplete

  bool is_complete = requestPtr.p->m_cnt_active == 0;
  bool need_complete_phase = requestPtr.p->m_bits & Request::RT_NEED_COMPLETE;

  if (requestPtr.p->isLookup())
  {
    ndbassert(requestPtr.p->m_cnt_active == 0);
  }

  if (!is_complete || (is_complete && need_complete_phase == false))
  {
    /**
     * one batch complete, and either
     *   - request not complete
     *   - or not complete_phase needed
     */
    jam();

    if ((requestPtr.p->m_state & Request::RS_ABORTING) != 0)
    {
      ndbassert(is_complete);
    }

    prepareNextBatch(signal, requestPtr);
    sendConf(signal, requestPtr, is_complete);
  }
  else if (is_complete && need_complete_phase)
  {
    jam();
    /**
     * run complete-phase
     */
    complete(signal, requestPtr);
    return;
  }

  if (requestPtr.p->m_cnt_active == 0)
  {
    jam();
    /**
     * request completed
     */
    cleanup(requestPtr);
  }
  else if ((requestPtr.p->m_bits & Request::RT_MULTI_SCAN) != 0)
  {
    jam();
    /**
     * release unneeded buffers and position cursor for SCAN_NEXTREQ
     */
    releaseScanBuffers(requestPtr);
  }
  else if ((requestPtr.p->m_bits & Request::RT_ROW_BUFFERS) != 0)
  {
    jam();
    /**
     * if not multiple scans in request, simply release all pages allocated
     * for row buffers (all rows will be released anyway)
     */
    releaseRequestBuffers(requestPtr, true);
  }
}

/**
 * Locate next TreeNode(s) to retrieve more rows from.
 *
 *   Calculate set of the 'm_active_nodes' we will receive from in NEXTREQ.
 *   Add these TreeNodes to the cursor list to be iterated.
 */
void
Dbspj::prepareNextBatch(Signal* signal, Ptr<Request> requestPtr)
{
  requestPtr.p->m_cursor_nodes.init();
  requestPtr.p->m_active_nodes.clear();

  if (requestPtr.p->m_cnt_active == 0)
  {
    jam();
    return;
  }

  if (requestPtr.p->m_bits & Request::RT_REPEAT_SCAN_RESULT)
  {
    /**
     * If REPEAT_SCAN_RESULT we handle bushy scans by return more *new* rows
     * from only one of the active child scans. If there are multiple 
     * bushy scans not being able to return their current result set in 
     * a single batch, result sets from the other child scans are repeated
     * until all rows has been returned to the API client.
     *
     * Hence, the cross joined results from the bushy scans are partly
     * produced within the SPJ block on a 'batchsize granularity', 
     * and partly is the responsibility of the API-client by iterating
     * the result rows within the current result batches.
     * (Opposed to non-REPEAT_SCAN_RESULT, the client only have to care about 
     *  the current batched rows - no buffering is required)
     */
    jam();
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);

    /**
     * Locate last 'TN_ACTIVE' TreeNode which is the only one choosen 
     * to return more *new* rows.
     */
    for (list.last(nodePtr); !nodePtr.isNull(); list.prev(nodePtr))
    {
      if (nodePtr.p->m_state == TreeNode::TN_ACTIVE)
      {
        jam();
        DEBUG("Will fetch more from 'active' m_node_no: " << nodePtr.p->m_node_no);
        /**
         * A later NEXTREQ will request a *new* batch of rows from this TreeNode.
         */
        registerActiveCursor(requestPtr, nodePtr);
        break;
      }
    }

    /**
     *  Restart/repeat other (index scan) child batches which:
     *    - Being 'after' nodePtr located above.
     *    - Not being an ancestor of (depends on) any 'active' TreeNode.
     *      (As these scans are started when rows from these parent nodes
     *      arrives.)
     */
    if (!nodePtr.isNull())
    {
      jam();
      DEBUG("Calculate 'active', w/ cursor on m_node_no: " << nodePtr.p->m_node_no);

      /* Restart any partial index-scans after this 'TN_ACTIVE' TreeNode */
      for (list.next(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
      {
        jam();
        if (!nodePtr.p->m_ancestors.overlaps (requestPtr.p->m_active_nodes))
        {
          jam();
          ndbrequire(nodePtr.p->m_state != TreeNode::TN_ACTIVE);
          ndbrequire(nodePtr.p->m_info != 0);
          if (nodePtr.p->m_info->m_parent_batch_repeat != 0)
          {
            jam();
            (this->*(nodePtr.p->m_info->m_parent_batch_repeat))(signal,
                                                                requestPtr,
                                                                nodePtr);
          }
        }
      }
    } // if (!nodePtr.isNull()
  }
  else  // not 'RT_REPEAT_SCAN_RESULT'
  {
    /**
     * If not REPEAT_SCAN_RESULT multiple active TreeNodes may return their 
     * remaining result simultaneously. In case of bushy-scans, these
     * concurrent result streams are cross joins of each other
     * in SQL terms. In order to produce the cross joined result, it is
     * the responsibility of the API-client to buffer these streams and
     * iterate them to produce the cross join.
     */
    jam();
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    TreeNodeBitMask ancestors_of_active;

    for (list.last(nodePtr); !nodePtr.isNull(); list.prev(nodePtr))
    {
      /**
       * If we are active (i.e not consumed all rows originating
       *   from parent rows) and we are not in the set of parents 
       *   for any active child:
       *
       * Then, this is a position that execSCAN_NEXTREQ should continue
       */
      if (nodePtr.p->m_state == TreeNode::TN_ACTIVE &&
         !ancestors_of_active.get (nodePtr.p->m_node_no))
      {
        jam();
        DEBUG("Add 'active' m_node_no: " << nodePtr.p->m_node_no);
        registerActiveCursor(requestPtr, nodePtr);
        ancestors_of_active.bitOR(nodePtr.p->m_ancestors);
      }
    }
  } // if (RT_REPEAT_SCAN_RESULT)

  DEBUG("Calculated 'm_active_nodes': " << requestPtr.p->m_active_nodes.rep.data[0]);
}

void
Dbspj::sendConf(Signal* signal, Ptr<Request> requestPtr, bool is_complete)
{
  if (requestPtr.p->isScan())
  {
    if (unlikely((requestPtr.p->m_state & Request::RS_WAITING) != 0))
    {
      jam();
      /**
       * We aborted request ourselves (due to node-failure ?)
       *   but TC haven't contacted us...so we can't reply yet...
       */
      ndbrequire(is_complete);
      ndbrequire((requestPtr.p->m_state & Request::RS_ABORTING) != 0);
      return;
    }

    if (requestPtr.p->m_errCode == 0)
    {
      jam();
      ScanFragConf * conf=
        reinterpret_cast<ScanFragConf*>(signal->getDataPtrSend());
      conf->senderData = requestPtr.p->m_senderData;
      conf->transId1 = requestPtr.p->m_transId[0];
      conf->transId2 = requestPtr.p->m_transId[1];
      conf->completedOps = requestPtr.p->m_rows;
      conf->fragmentCompleted = is_complete ? 1 : 0;
      conf->total_len = requestPtr.p->m_active_nodes.rep.data[0];

      c_Counters.incr_counter(CI_SCAN_BATCHES_RETURNED, 1);
      c_Counters.incr_counter(CI_SCAN_ROWS_RETURNED, requestPtr.p->m_rows);

#ifdef SPJ_TRACE_TIME
      Uint64 now = spj_now();
      Uint64 then = requestPtr.p->m_save_time;

      requestPtr.p->m_sum_rows += requestPtr.p->m_rows;
      requestPtr.p->m_sum_running += Uint32(now - then);
      requestPtr.p->m_cnt_batches++;
      requestPtr.p->m_save_time = now;

      if (is_complete)
      {
        Uint32 cnt = requestPtr.p->m_cnt_batches;
        ndbout_c("batches: %u avg_rows: %u avg_running: %u avg_wait: %u",
                 cnt,
                 (requestPtr.p->m_sum_rows / cnt),
                 (requestPtr.p->m_sum_running / cnt),
                 cnt == 1 ? 0 : requestPtr.p->m_sum_waiting / (cnt - 1));
      }
#endif

      /**
       * reset for next batch
       */
      requestPtr.p->m_rows = 0;
      if (!is_complete)
      {
        jam();
        requestPtr.p->m_state |= Request::RS_WAITING;
      }
#ifdef DEBUG_SCAN_FRAGREQ
      ndbout_c("Dbspj::sendConf() sending SCAN_FRAGCONF ");
      printSCAN_FRAGCONF(stdout, signal->getDataPtrSend(),
                         conf->total_len,
                         DBLQH);
#endif
      sendSignal(requestPtr.p->m_senderRef, GSN_SCAN_FRAGCONF, signal,
                 ScanFragConf::SignalLength, JBB);
    }
    else
    {
      jam();
      ndbrequire(is_complete);
      ScanFragRef * ref=
        reinterpret_cast<ScanFragRef*>(signal->getDataPtrSend());
      ref->senderData = requestPtr.p->m_senderData;
      ref->transId1 = requestPtr.p->m_transId[0];
      ref->transId2 = requestPtr.p->m_transId[1];
      ref->errorCode = requestPtr.p->m_errCode;

      sendSignal(requestPtr.p->m_senderRef, GSN_SCAN_FRAGREF, signal,
                 ScanFragRef::SignalLength, JBB);
    }
  }
  else
  {
    ndbassert(is_complete);
    if (requestPtr.p->m_errCode)
    {
      jam();
      Uint32 resultRef = getResultRef(requestPtr);
      TcKeyRef* ref = (TcKeyRef*)signal->getDataPtr();
      ref->connectPtr = requestPtr.p->m_senderData;
      ref->transId[0] = requestPtr.p->m_transId[0];
      ref->transId[1] = requestPtr.p->m_transId[1];
      ref->errorCode = requestPtr.p->m_errCode;
      ref->errorData = 0;

      sendTCKEYREF(signal, resultRef, requestPtr.p->m_senderRef);
    }
  }
}

Uint32
Dbspj::getResultRef(Ptr<Request> requestPtr)
{
  Ptr<TreeNode> nodePtr;
  Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
  for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
  {
    if (nodePtr.p->m_info == &g_LookupOpInfo)
    {
      jam();
      return nodePtr.p->m_lookup_data.m_api_resultRef;
    }
  }
  ndbrequire(false);
  return 0;
}

void
Dbspj::releaseScanBuffers(Ptr<Request> requestPtr)
{
  Ptr<TreeNode> treeNodePtr;
  Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
  TreeNodeBitMask ancestors_of_active;

  for (list.last(treeNodePtr); !treeNodePtr.isNull(); list.prev(treeNodePtr))
  {
    /**
     * If there are no active children,
     *   then we can cleanup in our sub-branch
     */
    if (!ancestors_of_active.get(treeNodePtr.p->m_node_no))
    {
      if (treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER)
      {
        jam();
        releaseNodeRows(requestPtr, treeNodePtr);
      }
      
      /**
       * Cleanup ACTIVE nodes fetching more rows in a NEXTREQ,
       * or nodes being in 'm_active_nodes' as they will 'repeat'.
       * (and then become active)
       */
      if (treeNodePtr.p->m_state == TreeNode::TN_ACTIVE ||
          requestPtr.p->m_active_nodes.get(treeNodePtr.p->m_node_no))
      {
        jam();
        cleanupChildBranch(requestPtr, treeNodePtr);
      }
    }

    /**
      * Collect ancestors of all nodes which are, or will
      * become active in NEXTREQ (possibly repeated)
      */
    if (treeNodePtr.p->m_state == TreeNode::TN_ACTIVE ||
        requestPtr.p->m_active_nodes.get(treeNodePtr.p->m_node_no))
    {
      ancestors_of_active.bitOR(treeNodePtr.p->m_ancestors);
    }
  }
  /**
   * Needs to be atleast 1 active otherwise we should have
   *   taken the cleanup "path" in batchComplete
   */
  ndbrequire(requestPtr.p->m_cnt_active >= 1);
}

void
Dbspj::registerActiveCursor(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr)
{
  Uint32 bit = treeNodePtr.p->m_node_no;
  ndbrequire(!requestPtr.p->m_active_nodes.get(bit));
  requestPtr.p->m_active_nodes.set(bit);

  Local_TreeNodeCursor_list list(m_treenode_pool, requestPtr.p->m_cursor_nodes);
#ifdef VM_TRACE
  {
    Ptr<TreeNode> nodePtr;
    for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
    {
      ndbrequire(nodePtr.i != treeNodePtr.i);
    }
  }
#endif
  list.add(treeNodePtr);
}

void
Dbspj::cleanupChildBranch(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr)
{
  LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
  Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
  Dependency_map::ConstDataBufferIterator it;
  for (list.first(it); !it.isNull(); list.next(it))
  {
    jam();
    Ptr<TreeNode> childPtr;
    m_treenode_pool.getPtr(childPtr, *it.data);
    if (childPtr.p->m_info->m_parent_batch_cleanup != 0)
    {
      jam();
      (this->*(childPtr.p->m_info->m_parent_batch_cleanup))(requestPtr,
                                                            childPtr);
    }
    cleanupChildBranch(requestPtr,childPtr);
  }
}

void
Dbspj::releaseNodeRows(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr)
{
  /**
   * Release all rows associated with tree node
   */

  // only when var-alloc, or else stack will be popped wo/ consideration
  // to individual rows
  ndbassert(requestPtr.p->m_bits & Request::RT_VAR_ALLOC);
  ndbassert(treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER);

  /**
   * Two ways to iterate...
   */
  if ((treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER_MAP) == 0)
  {
    jam();
    Uint32 cnt = 0;
    SLFifoRowListIterator iter;
    for (first(requestPtr, treeNodePtr, iter); !iter.isNull(); )
    {
      jam();
      RowRef pos = iter.m_ref;
      next(iter);
      releaseRow(requestPtr, pos);
      cnt ++;
    }
    treeNodePtr.p->m_row_list.init();
    DEBUG("SLFifoRowListIterator: released " << cnt << " rows!");
  }
  else
  {
    jam();
    Uint32 cnt = 0;
    RowMapIterator iter;
    for (first(requestPtr, treeNodePtr, iter); !iter.isNull(); )
    {
      jam();
      RowRef pos = iter.m_ref;
      // this could be made more efficient by not actually seting up m_row_ptr
      next(iter);
      releaseRow(requestPtr, pos);
      cnt++;
    }
    treeNodePtr.p->m_row_map.init();
    DEBUG("RowMapIterator: released " << cnt << " rows!");
  }
}

void
Dbspj::releaseRow(Ptr<Request> requestPtr, RowRef pos)
{
  ndbassert(requestPtr.p->m_bits & Request::RT_VAR_ALLOC);
  ndbassert(pos.m_allocator == 1);
  Ptr<RowPage> ptr;
  m_page_pool.getPtr(ptr, pos.m_page_id);
  ((Var_page*)ptr.p)->free_record(pos.m_page_pos, Var_page::CHAIN);
  Uint32 free_space = ((Var_page*)ptr.p)->free_space;
  if (free_space == 0)
  {
    jam();
    LocalDLFifoList<RowPage> list(m_page_pool,
                                  requestPtr.p->m_rowBuffer.m_page_list);
    list.remove(ptr);
    releasePage(ptr);
  }
  else if (free_space > requestPtr.p->m_rowBuffer.m_var.m_free)
  {
    LocalDLFifoList<RowPage> list(m_page_pool,
                                  requestPtr.p->m_rowBuffer.m_page_list);
    list.remove(ptr);
    list.addLast(ptr);
    requestPtr.p->m_rowBuffer.m_var.m_free = free_space;
  }
}

void
Dbspj::releaseRequestBuffers(Ptr<Request> requestPtr, bool reset)
{
  /**
   * Release all pages for request
   */
  {
    {
      LocalDLFifoList<RowPage> list(m_page_pool,
                                    requestPtr.p->m_rowBuffer.m_page_list);
      if (!list.isEmpty())
      {
        jam();
        Ptr<RowPage> first, last;
        list.first(first);
        list.last(last);
        releasePages(first.i, last);
        list.remove();
      }
    }
    requestPtr.p->m_rowBuffer.stack_init();
  }

  if (reset)
  {
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
    {
      jam();
      if (nodePtr.p->m_bits & TreeNode::T_ROW_BUFFER)
      {
        jam();
        if (nodePtr.p->m_bits & TreeNode::T_ROW_BUFFER_MAP)
        {
          jam();
          nodePtr.p->m_row_map.init();
        }
        else
        {
          nodePtr.p->m_row_list.init();
        }
      }
    }
  }
}

void
Dbspj::reportBatchComplete(Signal * signal, Ptr<Request> requestPtr,
                           Ptr<TreeNode> treeNodePtr)
{
  LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
  Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
  Dependency_map::ConstDataBufferIterator it;
  for (list.first(it); !it.isNull(); list.next(it))
  {
    jam();
    Ptr<TreeNode> childPtr;
    m_treenode_pool.getPtr(childPtr, * it.data);
    if (childPtr.p->m_bits & TreeNode::T_NEED_REPORT_BATCH_COMPLETED)
    {
      jam();
      ndbrequire(childPtr.p->m_info != 0 &&
                 childPtr.p->m_info->m_parent_batch_complete !=0 );
      (this->*(childPtr.p->m_info->m_parent_batch_complete))(signal,
                                                             requestPtr,
                                                             childPtr);
    }
  }
}

void
Dbspj::abort(Signal* signal, Ptr<Request> requestPtr, Uint32 errCode)
{
  jam();

  if ((requestPtr.p->m_state & Request::RS_ABORTING) != 0)
  {
    jam();
    goto checkcomplete;
  }

  requestPtr.p->m_state |= Request::RS_ABORTING;
  requestPtr.p->m_errCode = errCode;

  {
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
    {
      jam();
      /**
       * clear T_REPORT_BATCH_COMPLETE so that child nodes don't get confused
       *   during abort
       */
      nodePtr.p->m_bits &= ~Uint32(TreeNode::T_REPORT_BATCH_COMPLETE);

      ndbrequire(nodePtr.p->m_info != 0);
      if (nodePtr.p->m_info->m_abort != 0)
      {
        jam();
        (this->*(nodePtr.p->m_info->m_abort))(signal, requestPtr, nodePtr);
      }
    }
  }

checkcomplete:
  checkBatchComplete(signal, requestPtr, 0);
}

Uint32
Dbspj::nodeFail(Signal* signal, Ptr<Request> requestPtr,
                NdbNodeBitmask nodes)
{
  Uint32 cnt = 0;
  Uint32 iter = 0;

  {
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
    {
      jam();
      ndbrequire(nodePtr.p->m_info != 0);
      if (nodePtr.p->m_info->m_execNODE_FAILREP != 0)
      {
        jam();
        iter ++;
        cnt += (this->*(nodePtr.p->m_info->m_execNODE_FAILREP))(signal,
                                                                requestPtr,
                                                                nodePtr, nodes);
      }
    }
  }

  if (cnt == 0)
  {
    jam();
    /**
     * None of the operations needed NodeFailRep "action"
     *   check if our TC has died...but...only needed in
     *   scan case...for lookup...not so...
     */
    if (requestPtr.p->isScan() &&
        nodes.get(refToNode(requestPtr.p->m_senderRef)))
    {
      jam();
      abort(signal, requestPtr, DbspjErr::NodeFailure);
    }
  }
  else
  {
    jam();
    abort(signal, requestPtr, DbspjErr::NodeFailure);
  }

  return cnt + iter;
}

void
Dbspj::complete(Signal* signal, Ptr<Request> requestPtr)
{
  /**
   * we need to run complete-phase before sending last SCAN_FRAGCONF
   */
  Uint32 flags = requestPtr.p->m_state &
    (Request::RS_ABORTING | Request::RS_WAITING);

  requestPtr.p->m_state = Request::RS_COMPLETING | flags;

  // clear bit so that next batchComplete()
  // will continue to cleanup
  ndbassert((requestPtr.p->m_bits & Request::RT_NEED_COMPLETE) != 0);
  requestPtr.p->m_bits &= ~(Uint32)Request::RT_NEED_COMPLETE;
  requestPtr.p->m_outstanding = 0;
  {
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(nodePtr); !nodePtr.isNull(); list.next(nodePtr))
    {
      jam();
      ndbrequire(nodePtr.p->m_info != 0);
      if (nodePtr.p->m_info->m_complete != 0)
      {
        jam();
        (this->*(nodePtr.p->m_info->m_complete))(signal, requestPtr, nodePtr);
      }
    }

    /**
     * preferably RT_NEED_COMPLETE should only be set if blocking
     * calls are used, in which case m_outstanding should have been increased
     *
     * BUT: scanIndex does DIH_SCAN_TAB_COMPLETE_REP which does not send reply
     *      so it not really "blocking"
     *      i.e remove assert
     */
    //ndbassert(requestPtr.p->m_outstanding);
  }
  checkBatchComplete(signal, requestPtr, 0);
}

void
Dbspj::cleanup(Ptr<Request> requestPtr)
{
  ndbrequire(requestPtr.p->m_cnt_active == 0);
  {
    Ptr<TreeNode> nodePtr;
    Local_TreeNode_list list(m_treenode_pool, requestPtr.p->m_nodes);
    for (list.first(nodePtr); !nodePtr.isNull(); )
    {
      jam();
      ndbrequire(nodePtr.p->m_info != 0 && nodePtr.p->m_info->m_cleanup != 0);
      (this->*(nodePtr.p->m_info->m_cleanup))(requestPtr, nodePtr);

      Ptr<TreeNode> tmp = nodePtr;
      list.next(nodePtr);
      m_treenode_pool.release(tmp);
    }
    list.remove();
  }
  if (requestPtr.p->isScan())
  {
    jam();

    if (unlikely((requestPtr.p->m_state & Request::RS_WAITING) != 0))
    {
      jam();
      requestPtr.p->m_state = Request::RS_ABORTED;
      return;
    }

#ifdef VM_TRACE
    {
      Request key;
      key.m_transId[0] = requestPtr.p->m_transId[0];
      key.m_transId[1] = requestPtr.p->m_transId[1];
      key.m_senderData = requestPtr.p->m_senderData;
      Ptr<Request> tmp;
      ndbrequire(m_scan_request_hash.find(tmp, key));
    }
#endif
    m_scan_request_hash.remove(requestPtr);
  }
  else
  {
    jam();
#ifdef VM_TRACE
    {
      Request key;
      key.m_transId[0] = requestPtr.p->m_transId[0];
      key.m_transId[1] = requestPtr.p->m_transId[1];
      key.m_senderData = requestPtr.p->m_senderData;
      Ptr<Request> tmp;
      ndbrequire(m_lookup_request_hash.find(tmp, key));
    }
#endif
    m_lookup_request_hash.remove(requestPtr);
  }
  releaseRequestBuffers(requestPtr, false);
  ArenaHead ah = requestPtr.p->m_arena;
  m_request_pool.release(requestPtr);
  m_arenaAllocator.release(ah);
}

void
Dbspj::cleanup_common(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr)
{
  jam();

  LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
  {
    Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
    list.release();
  }

  {
    Local_pattern_store pattern(pool, treeNodePtr.p->m_keyPattern);
    pattern.release();
  }

  {
    Local_pattern_store pattern(pool, treeNodePtr.p->m_attrParamPattern);
    pattern.release();
  }

  if (treeNodePtr.p->m_send.m_keyInfoPtrI != RNIL)
  {
    jam();
    releaseSection(treeNodePtr.p->m_send.m_keyInfoPtrI);
  }

  if (treeNodePtr.p->m_send.m_attrInfoPtrI != RNIL)
  {
    jam();
    releaseSection(treeNodePtr.p->m_send.m_attrInfoPtrI);
  }
}

/**
 * Processing of signals from LQH
 */
void
Dbspj::execLQHKEYREF(Signal* signal)
{
  jamEntry();

  const LqhKeyRef* ref = reinterpret_cast<const LqhKeyRef*>(signal->getDataPtr());

  DEBUG("execLQHKEYREF, errorCode:" << ref->errorCode);
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, ref->connectPtr);

  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  ndbrequire(treeNodePtr.p->m_info && treeNodePtr.p->m_info->m_execLQHKEYREF);
  (this->*(treeNodePtr.p->m_info->m_execLQHKEYREF))(signal,
                                                    requestPtr,
                                                    treeNodePtr);
}

void
Dbspj::execLQHKEYCONF(Signal* signal)
{
  jamEntry();

  DEBUG("execLQHKEYCONF");

  const LqhKeyConf* conf = reinterpret_cast<const LqhKeyConf*>(signal->getDataPtr());
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, conf->opPtr);

  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  ndbrequire(treeNodePtr.p->m_info && treeNodePtr.p->m_info->m_execLQHKEYCONF);
  (this->*(treeNodePtr.p->m_info->m_execLQHKEYCONF))(signal,
                                                     requestPtr,
                                                     treeNodePtr);
}

void
Dbspj::execSCAN_FRAGREF(Signal* signal)
{
  jamEntry();
  const ScanFragRef* ref = reinterpret_cast<const ScanFragRef*>(signal->getDataPtr());

  DEBUG("execSCAN_FRAGREF, errorCode:" << ref->errorCode);

  Ptr<ScanFragHandle> scanFragHandlePtr;
  m_scanfraghandle_pool.getPtr(scanFragHandlePtr, ref->senderData);
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, scanFragHandlePtr.p->m_treeNodePtrI);
  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  ndbrequire(treeNodePtr.p->m_info&&treeNodePtr.p->m_info->m_execSCAN_FRAGREF);
  (this->*(treeNodePtr.p->m_info->m_execSCAN_FRAGREF))(signal,
                                                       requestPtr,
                                                       treeNodePtr,
                                                       scanFragHandlePtr);
}

void
Dbspj::execSCAN_HBREP(Signal* signal)
{
  jamEntry();

  Uint32 senderData = signal->theData[0];
  //Uint32 transId[2] = { signal->theData[1], signal->theData[2] };

  Ptr<ScanFragHandle> scanFragHandlePtr;
  m_scanfraghandle_pool.getPtr(scanFragHandlePtr, senderData);
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, scanFragHandlePtr.p->m_treeNodePtrI);
  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  Uint32 ref = requestPtr.p->m_senderRef;
  signal->theData[0] = requestPtr.p->m_senderData;
  sendSignal(ref, GSN_SCAN_HBREP, signal, 3, JBB);
}

void
Dbspj::execSCAN_FRAGCONF(Signal* signal)
{
  jamEntry();
  DEBUG("execSCAN_FRAGCONF");

  const ScanFragConf* conf = reinterpret_cast<const ScanFragConf*>(signal->getDataPtr());

#ifdef DEBUG_SCAN_FRAGREQ
  ndbout_c("Dbspj::execSCAN_FRAGCONF() receiveing SCAN_FRAGCONF ");
  printSCAN_FRAGCONF(stdout, signal->getDataPtrSend(),
                     conf->total_len,
                     DBLQH);
#endif

  Ptr<ScanFragHandle> scanFragHandlePtr;
  m_scanfraghandle_pool.getPtr(scanFragHandlePtr, conf->senderData);
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, scanFragHandlePtr.p->m_treeNodePtrI);
  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  ndbrequire(treeNodePtr.p->m_info&&treeNodePtr.p->m_info->m_execSCAN_FRAGCONF);
  (this->*(treeNodePtr.p->m_info->m_execSCAN_FRAGCONF))(signal,
                                                        requestPtr,
                                                        treeNodePtr,
                                                        scanFragHandlePtr);
}

void
Dbspj::execSCAN_NEXTREQ(Signal* signal)
{
  jamEntry();
  const ScanFragNextReq * req = (ScanFragNextReq*)&signal->theData[0];

  DEBUG("Incomming SCAN_NEXTREQ");
#ifdef DEBUG_SCAN_FRAGREQ
  printSCANFRAGNEXTREQ(stdout, &signal->theData[0],
                       ScanFragNextReq::SignalLength, DBLQH);
#endif

  Request key;
  key.m_transId[0] = req->transId1;
  key.m_transId[1] = req->transId2;
  key.m_senderData = req->senderData;

  Ptr<Request> requestPtr;
  if (unlikely(!m_scan_request_hash.find(requestPtr, key)))
  {
    jam();
    ndbrequire(req->requestInfo == ScanFragNextReq::ZCLOSE);
    return;
  }

#ifdef SPJ_TRACE_TIME
  Uint64 now = spj_now();
  Uint64 then = requestPtr.p->m_save_time;
  requestPtr.p->m_sum_waiting += Uint32(now - then);
  requestPtr.p->m_save_time = now;
#endif

  Uint32 state = requestPtr.p->m_state;
  requestPtr.p->m_state = state & ~Uint32(Request::RS_WAITING);

  if (unlikely(state == Request::RS_ABORTED))
  {
    jam();
    batchComplete(signal, requestPtr);
    return;
  }

  if (unlikely((state & Request::RS_ABORTING) != 0))
  {
    jam();
    /**
     * abort is already in progress...
     *   since RS_WAITING is cleared...it will end this request
     */
    return;
  }

  if (req->requestInfo == ScanFragNextReq::ZCLOSE)  // Requested close scan
  {
    jam();
    abort(signal, requestPtr, 0);
    return;
  }

  ndbrequire((state & Request::RS_WAITING) != 0);
  ndbrequire(requestPtr.p->m_outstanding == 0);

  {
    /**
     * Scroll all relevant cursors...
     */
    Ptr<TreeNode> treeNodePtr;
    Local_TreeNodeCursor_list list(m_treenode_pool,
                                   requestPtr.p->m_cursor_nodes);
    Uint32 cnt_active = 0;

    for (list.first(treeNodePtr); !treeNodePtr.isNull(); list.next(treeNodePtr))
    {
      if (treeNodePtr.p->m_state == TreeNode::TN_ACTIVE)
      {
        jam();
        DEBUG("SCAN_NEXTREQ on TreeNode: " << treeNodePtr.i
           << ",  m_node_no: " << treeNodePtr.p->m_node_no
           << ", w/ m_parentPtrI: " << treeNodePtr.p->m_parentPtrI);

        ndbrequire(treeNodePtr.p->m_info != 0 &&
                   treeNodePtr.p->m_info->m_execSCAN_NEXTREQ != 0);
        (this->*(treeNodePtr.p->m_info->m_execSCAN_NEXTREQ))(signal,
                                                             requestPtr,
                                                             treeNodePtr);
        cnt_active++;
      }
      else
      {
        /**
         * Restart any other scans not being 'TN_ACTIVE'
         * (Only effective if 'RT_REPEAT_SCAN_RESULT')
         */
        jam();
        ndbrequire(requestPtr.p->m_bits & Request::RT_REPEAT_SCAN_RESULT);
        DEBUG("  Restart TreeNode: " << treeNodePtr.i
           << ",  m_node_no: " << treeNodePtr.p->m_node_no
           << ", w/ m_parentPtrI: " << treeNodePtr.p->m_parentPtrI);

        ndbrequire(treeNodePtr.p->m_info != 0 &&
                   treeNodePtr.p->m_info->m_parent_batch_complete !=0 );
        (this->*(treeNodePtr.p->m_info->m_parent_batch_complete))(signal,
                                                                  requestPtr,
                                                                  treeNodePtr);
      }
    }
    /* Expected only a single ACTIVE TreeNode among the cursors */
    ndbrequire(cnt_active == 1 ||
               !(requestPtr.p->m_bits & Request::RT_REPEAT_SCAN_RESULT));
  }
}

void
Dbspj::execTRANSID_AI(Signal* signal)
{
  jamEntry();
  DEBUG("execTRANSID_AI");
  TransIdAI * req = (TransIdAI *)signal->getDataPtr();
  Uint32 ptrI = req->connectPtr;
  //Uint32 transId[2] = { req->transId[0], req->transId[1] };

  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, ptrI);
  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  ndbrequire(signal->getNoOfSections() != 0); // TODO check if this can happen

  SegmentedSectionPtr dataPtr;
  {
    SectionHandle handle(this, signal);
    handle.getSection(dataPtr, 0);
    handle.clear();
  }

#if defined(DEBUG_LQHKEYREQ) || defined(DEBUG_SCAN_FRAGREQ)
  printf("execTRANSID_AI: ");
  print(dataPtr, stdout);
#endif

  /**
   * build easy-access-array for row
   */
  Uint32 tmp[2+MAX_ATTRIBUTES_IN_TABLE];
  RowPtr::Header* header = CAST_PTR(RowPtr::Header, &tmp[0]);

  Uint32 cnt = buildRowHeader(header, dataPtr);
  ndbassert(header->m_len < NDB_ARRAY_SIZE(tmp));

  struct RowPtr row;
  row.m_type = RowPtr::RT_SECTION;
  row.m_src_node_ptrI = treeNodePtr.i;
  row.m_row_data.m_section.m_header = header;
  row.m_row_data.m_section.m_dataPtr.assign(dataPtr);

  getCorrelationData(row.m_row_data.m_section,
                     cnt - 1,
                     row.m_src_correlation);

  if (treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER)
  {
    jam();
    Uint32 err = storeRow(requestPtr, treeNodePtr, row);
    ndbrequire(err == 0);
  }

  ndbrequire(treeNodePtr.p->m_info&&treeNodePtr.p->m_info->m_execTRANSID_AI);

  (this->*(treeNodePtr.p->m_info->m_execTRANSID_AI))(signal,
                                                     requestPtr,
                                                     treeNodePtr,
                                                     row);
  release(dataPtr);
}

Uint32
Dbspj::storeRow(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr, RowPtr &row)
{
  ndbassert(row.m_type == RowPtr::RT_SECTION);
  SegmentedSectionPtr dataPtr = row.m_row_data.m_section.m_dataPtr;
  Uint32 * headptr = (Uint32*)row.m_row_data.m_section.m_header;
  Uint32 headlen = 1 + row.m_row_data.m_section.m_header->m_len;

  /**
   * If rows are not in map, then they are kept in linked list
   */
  Uint32 linklen = (treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER_MAP)?
    0 : 2;

  Uint32 totlen = 0;
  totlen += dataPtr.sz;
  totlen += headlen;
  totlen += linklen;

  RowRef ref;
  Uint32 * dstptr = 0;
  if ((requestPtr.p->m_bits & Request::RT_VAR_ALLOC) == 0)
  {
    jam();
    dstptr = stackAlloc(requestPtr.p->m_rowBuffer, ref, totlen);
  }
  else
  {
    jam();
    dstptr = varAlloc(requestPtr.p->m_rowBuffer, ref, totlen);
  }

  if (unlikely(dstptr == 0))
  {
    jam();
    return DbspjErr::OutOfRowMemory;
  }

  row.m_type = RowPtr::RT_LINEAR;
  row.m_row_data.m_linear.m_row_ref = ref;
  row.m_row_data.m_linear.m_header = (RowPtr::Header*)(dstptr + linklen);
  row.m_row_data.m_linear.m_data = dstptr + linklen + headlen;

  memcpy(dstptr + linklen, headptr, 4 * headlen);
  copy(dstptr + linklen + headlen, dataPtr);

  if (linklen)
  {
    jam();
    NullRowRef.copyto_link(dstptr); // Null terminate list...
    add_to_list(treeNodePtr.p->m_row_list, ref);
  }
  else
  {
    jam();
    return add_to_map(requestPtr, treeNodePtr, row.m_src_correlation, ref);
  }

  return 0;
}

void
Dbspj::setupRowPtr(Ptr<TreeNode> treeNodePtr,
                   RowPtr& row, RowRef ref, const Uint32 * src)
{
  Uint32 linklen = (treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER_MAP)?
    0 : 2;
  const RowPtr::Header * headptr = (RowPtr::Header*)(src + linklen);
  Uint32 headlen = 1 + headptr->m_len;

  row.m_type = RowPtr::RT_LINEAR;
  row.m_row_data.m_linear.m_row_ref = ref;
  row.m_row_data.m_linear.m_header = headptr;
  row.m_row_data.m_linear.m_data = (Uint32*)headptr + headlen;
}

void
Dbspj::add_to_list(SLFifoRowList & list, RowRef rowref)
{
  if (list.isNull())
  {
    jam();
    list.m_first_row_page_id = rowref.m_page_id;
    list.m_first_row_page_pos = rowref.m_page_pos;
  }
  else
  {
    jam();
    /**
     * add last to list
     */
    RowRef last;
    last.m_allocator = rowref.m_allocator;
    last.m_page_id = list.m_last_row_page_id;
    last.m_page_pos = list.m_last_row_page_pos;
    Uint32 * rowptr;
    if (rowref.m_allocator == 0)
    {
      jam();
      rowptr = get_row_ptr_stack(last);
    }
    else
    {
      jam();
      rowptr = get_row_ptr_var(last);
    }
    rowref.copyto_link(rowptr);
  }

  list.m_last_row_page_id = rowref.m_page_id;
  list.m_last_row_page_pos = rowref.m_page_pos;
}

Uint32 *
Dbspj::get_row_ptr_stack(RowRef pos)
{
  ndbassert(pos.m_allocator == 0);
  Ptr<RowPage> ptr;
  m_page_pool.getPtr(ptr, pos.m_page_id);
  return ptr.p->m_data + pos.m_page_pos;
}

Uint32 *
Dbspj::get_row_ptr_var(RowRef pos)
{
  ndbassert(pos.m_allocator == 1);
  Ptr<RowPage> ptr;
  m_page_pool.getPtr(ptr, pos.m_page_id);
  return ((Var_page*)ptr.p)->get_ptr(pos.m_page_pos);
}

bool
Dbspj::first(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr,
             SLFifoRowListIterator& iter)
{
  Uint32 var = (requestPtr.p->m_bits & Request::RT_VAR_ALLOC) != 0;
  SLFifoRowList & list = treeNodePtr.p->m_row_list;
  if (list.isNull())
  {
    jam();
    iter.setNull();
    return false;
  }

  iter.m_ref.m_allocator = var;
  iter.m_ref.m_page_id = list.m_first_row_page_id;
  iter.m_ref.m_page_pos = list.m_first_row_page_pos;
  if (var == 0)
  {
    jam();
    iter.m_row_ptr = get_row_ptr_stack(iter.m_ref);
  }
  else
  {
    jam();
    iter.m_row_ptr = get_row_ptr_var(iter.m_ref);
  }

  return true;
}

bool
Dbspj::next(SLFifoRowListIterator& iter)
{
  iter.m_ref.assign_from_link(iter.m_row_ptr);
  if (iter.m_ref.isNull())
  {
    jam();
    return false;
  }

  if (iter.m_ref.m_allocator == 0)
  {
    jam();
    iter.m_row_ptr = get_row_ptr_stack(iter.m_ref);
  }
  else
  {
    jam();
    iter.m_row_ptr = get_row_ptr_var(iter.m_ref);
  }
  return true;
}

bool
Dbspj::next(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr,
            SLFifoRowListIterator& iter, SLFifoRowListIteratorPtr start)
{
  Uint32 var = (requestPtr.p->m_bits & Request::RT_VAR_ALLOC) != 0;
  (void)var;
  ndbassert(var == iter.m_ref.m_allocator);
  if (iter.m_ref.m_allocator == 0)
  {
    jam();
    iter.m_row_ptr = get_row_ptr_stack(start.m_ref);
  }
  else
  {
    jam();
    iter.m_row_ptr = get_row_ptr_var(start.m_ref);
  }
  return next(iter);
}

Uint32
Dbspj::add_to_map(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr,
                  Uint32 corrVal, RowRef rowref)
{
  Uint32 * mapptr;
  RowMap& map = treeNodePtr.p->m_row_map;
  if (map.isNull())
  {
    jam();
    Uint16 batchsize = treeNodePtr.p->m_batch_size;
    Uint32 sz16 = RowMap::MAP_SIZE_PER_REF_16 * batchsize;
    Uint32 sz32 = (sz16 + 1) / 2;
    RowRef ref;
    if ((requestPtr.p->m_bits & Request::RT_VAR_ALLOC) == 0)
    {
      jam();
      mapptr = stackAlloc(requestPtr.p->m_rowBuffer, ref, sz32);
    }
    else
    {
      jam();
      mapptr = varAlloc(requestPtr.p->m_rowBuffer, ref, sz32);
    }
    if (unlikely(mapptr == 0))
    {
      jam();
      return DbspjErr::OutOfRowMemory;
    }
    map.assign(ref);
    map.m_elements = 0;
    map.m_size = batchsize;
    map.clear(mapptr);
  }
  else
  {
    jam();
    RowRef ref;
    map.copyto(ref);
    if (ref.m_allocator == 0)
    {
      jam();
      mapptr = get_row_ptr_stack(ref);
    }
    else
    {
      jam();
      mapptr = get_row_ptr_var(ref);
    }
  }

  Uint32 pos = corrVal & 0xFFFF;
  ndbrequire(pos < map.m_size);
  ndbrequire(map.m_elements < map.m_size);

  if (1)
  {
    /**
     * Check that *pos* is empty
     */
    RowRef check;
    map.load(mapptr, pos, check);
    ndbrequire(check.m_page_pos == 0xFFFF);
  }

  map.store(mapptr, pos, rowref);

  return 0;
}

bool
Dbspj::first(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr,
             RowMapIterator & iter)
{
  Uint32 var = (requestPtr.p->m_bits & Request::RT_VAR_ALLOC) != 0;
  RowMap& map = treeNodePtr.p->m_row_map;
  if (map.isNull())
  {
    jam();
    iter.setNull();
    return false;
  }

  if (var == 0)
  {
    jam();
    iter.m_map_ptr = get_row_ptr_stack(map.m_map_ref);
  }
  else
  {
    jam();
    iter.m_map_ptr = get_row_ptr_var(map.m_map_ref);
  }
  iter.m_size = map.m_size;
  iter.m_ref.m_allocator = var;

  Uint32 pos = 0;
  while (RowMap::isNull(iter.m_map_ptr, pos) && pos < iter.m_size)
    pos++;

  if (pos == iter.m_size)
  {
    jam();
    iter.setNull();
    return false;
  }
  else
  {
    jam();
    RowMap::load(iter.m_map_ptr, pos, iter.m_ref);
    iter.m_element_no = pos;
    if (var == 0)
    {
      jam();
      iter.m_row_ptr = get_row_ptr_stack(iter.m_ref);
    }
    else
    {
      jam();
      iter.m_row_ptr = get_row_ptr_var(iter.m_ref);
    }
    return true;
  }
}

bool
Dbspj::next(RowMapIterator & iter)
{
  Uint32 pos = iter.m_element_no + 1;
  while (RowMap::isNull(iter.m_map_ptr, pos) && pos < iter.m_size)
    pos++;

  if (pos == iter.m_size)
  {
    jam();
    iter.setNull();
    return false;
  }
  else
  {
    jam();
    RowMap::load(iter.m_map_ptr, pos, iter.m_ref);
    iter.m_element_no = pos;
    if (iter.m_ref.m_allocator == 0)
    {
      jam();
      iter.m_row_ptr = get_row_ptr_stack(iter.m_ref);
    }
    else
    {
      jam();
      iter.m_row_ptr = get_row_ptr_var(iter.m_ref);
    }
    return true;
  }
}

bool
Dbspj::next(Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr,
            RowMapIterator & iter, RowMapIteratorPtr start)
{
  Uint32 var = (requestPtr.p->m_bits & Request::RT_VAR_ALLOC) != 0;
  RowMap& map = treeNodePtr.p->m_row_map;
  ndbrequire(!map.isNull());

  if (var == 0)
  {
    jam();
    iter.m_map_ptr = get_row_ptr_stack(map.m_map_ref);
  }
  else
  {
    jam();
    iter.m_map_ptr = get_row_ptr_var(map.m_map_ref);
  }
  iter.m_size = map.m_size;

  RowMap::load(iter.m_map_ptr, start.m_element_no, iter.m_ref);
  iter.m_element_no = start.m_element_no;
  return next(iter);
}

Uint32 *
Dbspj::stackAlloc(RowBuffer & buffer, RowRef& dst, Uint32 sz)
{
  Ptr<RowPage> ptr;
  LocalDLFifoList<RowPage> list(m_page_pool, buffer.m_page_list);

  Uint32 pos = buffer.m_stack.m_pos;
  const Uint32 SIZE = RowPage::SIZE;
  if (list.isEmpty() || (pos + sz) > SIZE)
  {
    jam();
    bool ret = allocPage(ptr);
    if (unlikely(ret == false))
    {
      jam();
      return 0;
    }

    pos = 0;
    list.addLast(ptr);
  }
  else
  {
    list.last(ptr);
  }

  dst.m_page_id = ptr.i;
  dst.m_page_pos = pos;
  dst.m_allocator = 0;
  buffer.m_stack.m_pos = pos + sz;
  return ptr.p->m_data + pos;
}

Uint32 *
Dbspj::varAlloc(RowBuffer & buffer, RowRef& dst, Uint32 sz)
{
  Ptr<RowPage> ptr;
  LocalDLFifoList<RowPage> list(m_page_pool, buffer.m_page_list);

  Uint32 free_space = buffer.m_var.m_free;
  if (list.isEmpty() || free_space < (sz + 1))
  {
    jam();
    bool ret = allocPage(ptr);
    if (unlikely(ret == false))
    {
      jam();
      return 0;
    }

    list.addLast(ptr);
    ((Var_page*)ptr.p)->init();
  }
  else
  {
    jam();
    list.last(ptr);
  }

  Var_page * vp = (Var_page*)ptr.p;
  Uint32 pos = vp->alloc_record(sz, (Var_page*)m_buffer0, Var_page::CHAIN);

  dst.m_page_id = ptr.i;
  dst.m_page_pos = pos;
  dst.m_allocator = 1;
  buffer.m_var.m_free = vp->free_space;
  return vp->get_ptr(pos);
}

bool
Dbspj::allocPage(Ptr<RowPage> & ptr)
{
  if (m_free_page_list.firstItem == RNIL)
  {
    jam();
    ptr.p = (RowPage*)m_ctx.m_mm.alloc_page(RT_SPJ_DATABUFFER,
                                            &ptr.i,
                                            Ndbd_mem_manager::NDB_ZONE_ANY);
    if (ptr.p == 0)
    {
      return false;
    }
    return true;
  }
  else
  {
    jam();
    LocalSLList<RowPage> list(m_page_pool, m_free_page_list);
    bool ret = list.remove_front(ptr);
    ndbrequire(ret);
    return ret;
  }
}

void
Dbspj::releasePage(Ptr<RowPage> ptr)
{
  LocalSLList<RowPage> list(m_page_pool, m_free_page_list);
  list.add(ptr);
}

void
Dbspj::releasePages(Uint32 first, Ptr<RowPage> last)
{
  LocalSLList<RowPage> list(m_page_pool, m_free_page_list);
  list.add(first, last);
}

void
Dbspj::releaseGlobal(Signal * signal)
{
  Uint32 delay = 100;
  LocalSLList<RowPage> list(m_page_pool, m_free_page_list);
  if (list.empty())
  {
    jam();
    delay = 300;
  }
  else
  {
    Ptr<RowPage> ptr;
    list.remove_front(ptr);
    m_ctx.m_mm.release_page(RT_SPJ_DATABUFFER, ptr.i);
  }

  signal->theData[0] = 0;
  sendSignalWithDelay(reference(), GSN_CONTINUEB, signal, delay, 1);
}

/**
 * END - MODULE GENERIC
 */

/**
 * MODULE LOOKUP
 */
const Dbspj::OpInfo
Dbspj::g_LookupOpInfo =
{
  &Dbspj::lookup_build,
  0, // prepare
  &Dbspj::lookup_start,
  &Dbspj::lookup_execTRANSID_AI,
  &Dbspj::lookup_execLQHKEYREF,
  &Dbspj::lookup_execLQHKEYCONF,
  0, // execSCAN_FRAGREF
  0, // execSCAN_FRAGCONF
  &Dbspj::lookup_parent_row,
  &Dbspj::lookup_parent_batch_complete,
  0, // Dbspj::lookup_parent_batch_repeat,
  0, // Dbspj::lookup_parent_batch_cleanup,
  0, // Dbspj::lookup_execSCAN_NEXTREQ
  0, // Dbspj::lookup_complete
  &Dbspj::lookup_abort,
  &Dbspj::lookup_execNODE_FAILREP,
  &Dbspj::lookup_cleanup
};

Uint32
Dbspj::lookup_build(Build_context& ctx,
                    Ptr<Request> requestPtr,
                    const QueryNode* qn,
                    const QueryNodeParameters* qp)
{
  Uint32 err = 0;
  Ptr<TreeNode> treeNodePtr;
  const QN_LookupNode * node = (const QN_LookupNode*)qn;
  const QN_LookupParameters * param = (const QN_LookupParameters*)qp;
  do
  {
    err = createNode(ctx, requestPtr, treeNodePtr);
    if (unlikely(err != 0))
    {
      DEBUG_CRASH();
      break;
    }

    treeNodePtr.p->m_info = &g_LookupOpInfo;
    Uint32 transId1 = requestPtr.p->m_transId[0];
    Uint32 transId2 = requestPtr.p->m_transId[1];
    Uint32 savePointId = ctx.m_savepointId;

    Uint32 treeBits = node->requestInfo;
    Uint32 paramBits = param->requestInfo;
    //ndbout_c("Dbspj::lookup_build() treeBits=%.8x paramBits=%.8x",
    //         treeBits, paramBits);
    LqhKeyReq* dst = (LqhKeyReq*)treeNodePtr.p->m_lookup_data.m_lqhKeyReq;
    {
      /**
       * static variables
       */
      dst->tcBlockref = reference();
      dst->clientConnectPtr = treeNodePtr.i;

      /**
       * TODO reference()+treeNodePtr.i is passed twice
       *   this can likely be optimized using the requestInfo-bits
       * UPDATE: This can be accomplished by *not* setApplicationAddressFlag
       *         and patch LQH to then instead use tcBlockref/clientConnectPtr
       */
      dst->transId1 = transId1;
      dst->transId2 = transId2;
      dst->savePointId = savePointId;
      dst->scanInfo = 0;
      dst->attrLen = 0;
      /** Initialy set reply ref to client, do_send will set SPJ refs if non-LEAF */
      dst->variableData[0] = ctx.m_resultRef;
      dst->variableData[1] = param->resultData;
      Uint32 requestInfo = 0;
      LqhKeyReq::setOperation(requestInfo, ZREAD);
      LqhKeyReq::setApplicationAddressFlag(requestInfo, 1);
      LqhKeyReq::setDirtyFlag(requestInfo, 1);
      LqhKeyReq::setSimpleFlag(requestInfo, 1);
      LqhKeyReq::setNormalProtocolFlag(requestInfo, 0);  // Assume T_LEAF
      LqhKeyReq::setCorrFactorFlag(requestInfo, 1);
      LqhKeyReq::setNoDiskFlag(requestInfo,
                               (treeBits & DABits::NI_LINKED_DISK) == 0 &&
                               (paramBits & DABits::PI_DISK_ATTR) == 0);
      dst->requestInfo = requestInfo;
    }

    err = DbspjErr::InvalidTreeNodeSpecification;
    if (unlikely(node->len < QN_LookupNode::NodeSize))
    {
      DEBUG_CRASH();
      break;
    }

    if (treeBits & QN_LookupNode::L_UNIQUE_INDEX)
    {
      jam();
      treeNodePtr.p->m_bits |= TreeNode::T_UNIQUE_INDEX_LOOKUP;
    }

    Uint32 tableId = node->tableId;
    Uint32 schemaVersion = node->tableVersion;

    Uint32 tableSchemaVersion = tableId + ((schemaVersion << 16) & 0xFFFF0000);
    dst->tableSchemaVersion = tableSchemaVersion;

    err = DbspjErr::InvalidTreeParametersSpecification;
    DEBUG("param len: " << param->len);
    if (unlikely(param->len < QN_LookupParameters::NodeSize))
    {
      DEBUG_CRASH();
      break;
    }

    ctx.m_resultData = param->resultData;
    treeNodePtr.p->m_lookup_data.m_api_resultRef = ctx.m_resultRef;
    treeNodePtr.p->m_lookup_data.m_api_resultData = param->resultData;
    treeNodePtr.p->m_lookup_data.m_outstanding = 0;
    treeNodePtr.p->m_lookup_data.m_parent_batch_complete = false;

    /**
     * Parse stuff common lookup/scan-frag
     */
    struct DABuffer nodeDA, paramDA;
    nodeDA.ptr = node->optional;
    nodeDA.end = nodeDA.ptr + (node->len - QN_LookupNode::NodeSize);
    paramDA.ptr = param->optional;
    paramDA.end = paramDA.ptr + (param->len - QN_LookupParameters::NodeSize);
    err = parseDA(ctx, requestPtr, treeNodePtr,
                  nodeDA, treeBits, paramDA, paramBits);
    if (unlikely(err != 0))
    {
      DEBUG_CRASH();
      break;
    }

    if (treeNodePtr.p->m_bits & TreeNode::T_ATTR_INTERPRETED)
    {
      jam();
      LqhKeyReq::setInterpretedFlag(dst->requestInfo, 1);
    }

    /**
     * Inherit batch size from parent
     */
    treeNodePtr.p->m_batch_size = 1;
    if (treeNodePtr.p->m_parentPtrI != RNIL)
    {
      jam();
      Ptr<TreeNode> parentPtr;
      m_treenode_pool.getPtr(parentPtr, treeNodePtr.p->m_parentPtrI);
      treeNodePtr.p->m_batch_size = parentPtr.p->m_batch_size;
    }

    if (ctx.m_start_signal)
    {
      jam();
      Signal * signal = ctx.m_start_signal;
      const LqhKeyReq* src = (const LqhKeyReq*)signal->getDataPtr();
#if NOT_YET
      Uint32 instanceNo =
        blockToInstance(signal->header.theReceiversBlockNumber);
      treeNodePtr.p->m_send.m_ref = numberToRef(DBLQH,
                                                instanceNo, getOwnNodeId());
#else
      treeNodePtr.p->m_send.m_ref =
        numberToRef(DBLQH, getInstanceKey(src->tableSchemaVersion & 0xFFFF,
                                          src->fragmentData & 0xFFFF),
                    getOwnNodeId());
#endif

      Uint32 hashValue = src->hashValue;
      Uint32 fragId = src->fragmentData;
      Uint32 requestInfo = src->requestInfo;
      Uint32 attrLen = src->attrLen; // fragdist-key is in here

      /**
       * assertions
       */
      ndbassert(LqhKeyReq::getAttrLen(attrLen) == 0);         // Only long
      ndbassert(LqhKeyReq::getScanTakeOverFlag(attrLen) == 0);// Not supported
      ndbassert(LqhKeyReq::getReorgFlag(attrLen) == 0);       // Not supported
      ndbassert(LqhKeyReq::getOperation(requestInfo) == ZREAD);
      ndbassert(LqhKeyReq::getKeyLen(requestInfo) == 0);      // Only long
      ndbassert(LqhKeyReq::getMarkerFlag(requestInfo) == 0);  // Only read
      ndbassert(LqhKeyReq::getAIInLqhKeyReq(requestInfo) == 0);
      ndbassert(LqhKeyReq::getSeqNoReplica(requestInfo) == 0);
      ndbassert(LqhKeyReq::getLastReplicaNo(requestInfo) == 0);
      ndbassert(LqhKeyReq::getApplicationAddressFlag(requestInfo) != 0);
      ndbassert(LqhKeyReq::getSameClientAndTcFlag(requestInfo) == 0);

#if TODO
      /**
       * Handle various lock-modes
       */
      static Uint8 getDirtyFlag(const UintR & requestInfo);
      static Uint8 getSimpleFlag(const UintR & requestInfo);
#endif

      Uint32 dst_requestInfo = dst->requestInfo;
      ndbassert(LqhKeyReq::getInterpretedFlag(requestInfo) ==
                LqhKeyReq::getInterpretedFlag(dst_requestInfo));
      ndbassert(LqhKeyReq::getNoDiskFlag(requestInfo) ==
                LqhKeyReq::getNoDiskFlag(dst_requestInfo));

      dst->hashValue = hashValue;
      dst->fragmentData = fragId;
      dst->attrLen = attrLen; // fragdist is in here

      treeNodePtr.p->m_send.m_keyInfoPtrI = ctx.m_keyPtr.i;
      treeNodePtr.p->m_bits |= TreeNode::T_ONE_SHOT;
    }
    return 0;
  } while (0);

  return err;
}

void
Dbspj::lookup_start(Signal* signal,
                    Ptr<Request> requestPtr,
                    Ptr<TreeNode> treeNodePtr)
{
  lookup_send(signal, requestPtr, treeNodePtr);
}

void
Dbspj::lookup_send(Signal* signal,
                   Ptr<Request> requestPtr,
                   Ptr<TreeNode> treeNodePtr)
{
  jam();

  Uint32 cnt = 2;
  if (treeNodePtr.p->isLeaf())
  {
    jam();
    if (requestPtr.p->isLookup())
    {
      jam();
      cnt = 0;
    }
    else
    {
      jam();
      cnt = 1;
    }
  }

  LqhKeyReq* req = reinterpret_cast<LqhKeyReq*>(signal->getDataPtrSend());

  memcpy(req, treeNodePtr.p->m_lookup_data.m_lqhKeyReq,
         sizeof(treeNodePtr.p->m_lookup_data.m_lqhKeyReq));
  req->variableData[2] = treeNodePtr.p->m_send.m_correlation;
  req->variableData[3] = requestPtr.p->m_rootResultData;

  if (!(requestPtr.p->isLookup() && treeNodePtr.p->isLeaf()))
  {
    // Non-LEAF want reply to SPJ instead of ApiClient.
    LqhKeyReq::setNormalProtocolFlag(req->requestInfo, 1);
    req->variableData[0] = reference();
    req->variableData[1] = treeNodePtr.i;
  }
  else
  {
    jam();
    /**
     * Fake that TC sent this request,
     *   so that it can route a maybe TCKEYREF
     */
    req->tcBlockref = requestPtr.p->m_senderRef;
  }

  SectionHandle handle(this);

  Uint32 ref = treeNodePtr.p->m_send.m_ref;
  Uint32 keyInfoPtrI = treeNodePtr.p->m_send.m_keyInfoPtrI;
  Uint32 attrInfoPtrI = treeNodePtr.p->m_send.m_attrInfoPtrI;

  if (treeNodePtr.p->m_bits & TreeNode::T_ONE_SHOT)
  {
    jam();
    /**
     * Pass sections to send
     */
    treeNodePtr.p->m_send.m_attrInfoPtrI = RNIL;
    treeNodePtr.p->m_send.m_keyInfoPtrI = RNIL;
  }
  else
  {
    if ((treeNodePtr.p->m_bits & TreeNode::T_KEYINFO_CONSTRUCTED) == 0)
    {
      jam();
      Uint32 tmp = RNIL;
      ndbrequire(dupSection(tmp, keyInfoPtrI)); // TODO handle error
      keyInfoPtrI = tmp;
    }
    else
    {
      jam();
      treeNodePtr.p->m_send.m_keyInfoPtrI = RNIL;
    }

    if ((treeNodePtr.p->m_bits & TreeNode::T_ATTRINFO_CONSTRUCTED) == 0)
    {
      jam();
      Uint32 tmp = RNIL;
      ndbrequire(dupSection(tmp, attrInfoPtrI)); // TODO handle error
      attrInfoPtrI = tmp;
    }
    else
    {
      jam();
      treeNodePtr.p->m_send.m_attrInfoPtrI = RNIL;
    }
  }

  getSection(handle.m_ptr[0], keyInfoPtrI);
  getSection(handle.m_ptr[1], attrInfoPtrI);
  handle.m_cnt = 2;

#if defined DEBUG_LQHKEYREQ
  ndbout_c("LQHKEYREQ to %x", ref);
  printLQHKEYREQ(stdout, signal->getDataPtrSend(),
                 NDB_ARRAY_SIZE(treeNodePtr.p->m_lookup_data.m_lqhKeyReq),
                 DBLQH);
  printf("KEYINFO: ");
  print(handle.m_ptr[0], stdout);
  printf("ATTRINFO: ");
  print(handle.m_ptr[1], stdout);
#endif

  Uint32 Tnode = refToNode(ref);
  if (Tnode == getOwnNodeId())
  {
    c_Counters.incr_counter(CI_LOCAL_READS_SENT, 1);
  }
  else
  {
    c_Counters.incr_counter(CI_REMOTE_READS_SENT, 1);
  }

  if (unlikely(!c_alive_nodes.get(Tnode)))
  {
    jam();
    releaseSections(handle);
    abort(signal, requestPtr, DbspjErr::NodeFailure);
    return;
  }
  else if (! (treeNodePtr.p->isLeaf() && requestPtr.p->isLookup()))
  {
    jam();
    ndbassert(Tnode < NDB_ARRAY_SIZE(requestPtr.p->m_lookup_node_data));
    requestPtr.p->m_outstanding += cnt;
    requestPtr.p->m_lookup_node_data[Tnode] += cnt;
    // number wrapped
    ndbrequire(! (requestPtr.p->m_lookup_node_data[Tnode] == 0));
  }

  sendSignal(ref, GSN_LQHKEYREQ, signal,
             NDB_ARRAY_SIZE(treeNodePtr.p->m_lookup_data.m_lqhKeyReq),
             JBB, &handle);

  treeNodePtr.p->m_lookup_data.m_outstanding += cnt;
  if (requestPtr.p->isLookup() && treeNodePtr.p->isLeaf())
  {
    jam();
    /**
     * Send TCKEYCONF with DirtyReadBit + Tnode,
     *   so that API can discover if Tnode while waiting for result
     */
    Uint32 resultRef = req->variableData[0];
    Uint32 resultData = req->variableData[1];

    TcKeyConf* conf = (TcKeyConf*)signal->getDataPtrSend();
    conf->apiConnectPtr = RNIL; // lookup transaction from operations...
    conf->confInfo = 0;
    TcKeyConf::setNoOfOperations(conf->confInfo, 1);
    conf->transId1 = requestPtr.p->m_transId[0];
    conf->transId2 = requestPtr.p->m_transId[1];
    conf->operations[0].apiOperationPtr = resultData;
    conf->operations[0].attrInfoLen = TcKeyConf::DirtyReadBit | Tnode;
    Uint32 sigLen = TcKeyConf::StaticLength + TcKeyConf::OperationLength;
    sendTCKEYCONF(signal, sigLen, resultRef, requestPtr.p->m_senderRef);
  }
}

void
Dbspj::lookup_execTRANSID_AI(Signal* signal,
                             Ptr<Request> requestPtr,
                             Ptr<TreeNode> treeNodePtr,
                             const RowPtr & rowRef)
{
  jam();

  Uint32 Tnode = refToNode(signal->getSendersBlockRef());

  {
    LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
    Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
    Dependency_map::ConstDataBufferIterator it;
    for (list.first(it); !it.isNull(); list.next(it))
    {
      jam();
      Ptr<TreeNode> childPtr;
      m_treenode_pool.getPtr(childPtr, * it.data);
      ndbrequire(childPtr.p->m_info != 0&&childPtr.p->m_info->m_parent_row!=0);
      (this->*(childPtr.p->m_info->m_parent_row))(signal,
                                                  requestPtr, childPtr,rowRef);
    }
  }
  ndbrequire(!(requestPtr.p->isLookup() && treeNodePtr.p->isLeaf()));

  ndbassert(requestPtr.p->m_lookup_node_data[Tnode] >= 1);
  requestPtr.p->m_lookup_node_data[Tnode] -= 1;

  treeNodePtr.p->m_lookup_data.m_outstanding--;

  if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE
      && treeNodePtr.p->m_lookup_data.m_parent_batch_complete
      && treeNodePtr.p->m_lookup_data.m_outstanding == 0)
  {
    jam();
    // We have received all rows for this operation in this batch.
    reportBatchComplete(signal, requestPtr, treeNodePtr);

    // Prepare for next batch.
    treeNodePtr.p->m_lookup_data.m_parent_batch_complete = false;
    treeNodePtr.p->m_lookup_data.m_outstanding = 0;
  }

  checkBatchComplete(signal, requestPtr, 1);
}

void
Dbspj::lookup_execLQHKEYREF(Signal* signal,
                            Ptr<Request> requestPtr,
                            Ptr<TreeNode> treeNodePtr)
{
  const LqhKeyRef * rep = (LqhKeyRef*)signal->getDataPtr();
  Uint32 errCode = rep->errorCode;
  Uint32 Tnode = refToNode(signal->getSendersBlockRef());

  c_Counters.incr_counter(CI_READS_NOT_FOUND, 1);

  if (requestPtr.p->isLookup())
  {
    jam();

    /* CONF/REF not requested for lookup-Leaf: */
    ndbrequire(!treeNodePtr.p->isLeaf());

    /**
     * Scan-request does not need to
     *   send TCKEYREF...
     */
    /**
     * Return back to api...
     *   NOTE: assume that signal is tampered with
     */
    Uint32 resultRef = treeNodePtr.p->m_lookup_data.m_api_resultRef;
    Uint32 resultData = treeNodePtr.p->m_lookup_data.m_api_resultData;
    TcKeyRef* ref = (TcKeyRef*)signal->getDataPtr();
    ref->connectPtr = resultData;
    ref->transId[0] = requestPtr.p->m_transId[0];
    ref->transId[1] = requestPtr.p->m_transId[1];
    ref->errorCode = errCode;
    ref->errorData = 0;

    DEBUG("lookup_execLQHKEYREF, errorCode:" << errCode);

    sendTCKEYREF(signal, resultRef, requestPtr.p->m_senderRef);

    if (treeNodePtr.p->m_bits & TreeNode::T_UNIQUE_INDEX_LOOKUP)
    {
      /**
       * If this is a "leaf" unique index lookup
       *   emit extra TCKEYCONF as would have been done with ordinary
       *   operation
       */
      LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
      Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
      Dependency_map::ConstDataBufferIterator it;
      ndbrequire(list.first(it));
      ndbrequire(list.getSize() == 1); // should only be 1 child
      Ptr<TreeNode> childPtr;
      m_treenode_pool.getPtr(childPtr, * it.data);
      if (childPtr.p->m_bits & TreeNode::T_LEAF)
      {
        jam();
        Uint32 resultRef = childPtr.p->m_lookup_data.m_api_resultRef;
        Uint32 resultData = childPtr.p->m_lookup_data.m_api_resultData;
        TcKeyConf* conf = (TcKeyConf*)signal->getDataPtr();
        conf->apiConnectPtr = RNIL;
        conf->confInfo = 0;
        conf->gci_hi = 0;
        TcKeyConf::setNoOfOperations(conf->confInfo, 1);
        conf->transId1 = requestPtr.p->m_transId[0];
        conf->transId2 = requestPtr.p->m_transId[1];
        conf->operations[0].apiOperationPtr = resultData;
        conf->operations[0].attrInfoLen =
          TcKeyConf::DirtyReadBit |getOwnNodeId();
        sendTCKEYCONF(signal, TcKeyConf::StaticLength + 2, resultRef, requestPtr.p->m_senderRef);
      }
    }
  }
  else
  {
    jam();
    switch(errCode){
    case 626: // Row not found
    case 899: // Interpreter_exit_nok
      jam();
      break;
    default:
      jam();
      abort(signal, requestPtr, errCode);
    }
  }

  Uint32 cnt = 2;
  if (treeNodePtr.p->isLeaf())  // Can't be a lookup-Leaf, asserted above
    cnt = 1;

  ndbassert(requestPtr.p->m_lookup_node_data[Tnode] >= cnt);
  requestPtr.p->m_lookup_node_data[Tnode] -= cnt;

  treeNodePtr.p->m_lookup_data.m_outstanding -= cnt;

  if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE
      && treeNodePtr.p->m_lookup_data.m_parent_batch_complete
      && treeNodePtr.p->m_lookup_data.m_outstanding == 0)
  {
    jam();
    // We have received all rows for this operation in this batch.
    reportBatchComplete(signal, requestPtr, treeNodePtr);

    // Prepare for next batch.
    treeNodePtr.p->m_lookup_data.m_parent_batch_complete = false;
    treeNodePtr.p->m_lookup_data.m_outstanding = 0;
  }

  checkBatchComplete(signal, requestPtr, cnt);
}

void
Dbspj::lookup_execLQHKEYCONF(Signal* signal,
                             Ptr<Request> requestPtr,
                             Ptr<TreeNode> treeNodePtr)
{
  ndbrequire(!(requestPtr.p->isLookup() && treeNodePtr.p->isLeaf()));

  Uint32 Tnode = refToNode(signal->getSendersBlockRef());

  if (treeNodePtr.p->m_bits & TreeNode::T_USER_PROJECTION)
  {
    jam();
    requestPtr.p->m_rows++;
  }

  ndbassert(requestPtr.p->m_lookup_node_data[Tnode] >= 1);
  requestPtr.p->m_lookup_node_data[Tnode] -= 1;

  treeNodePtr.p->m_lookup_data.m_outstanding--;

  if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE
      && treeNodePtr.p->m_lookup_data.m_parent_batch_complete
      && treeNodePtr.p->m_lookup_data.m_outstanding == 0)
  {
    jam();
    // We have received all rows for this operation in this batch.
    reportBatchComplete(signal, requestPtr, treeNodePtr);

    // Prepare for next batch.
    treeNodePtr.p->m_lookup_data.m_parent_batch_complete = false;
    treeNodePtr.p->m_lookup_data.m_outstanding = 0;
  }

  checkBatchComplete(signal, requestPtr, 1);
}

void
Dbspj::lookup_parent_row(Signal* signal,
                         Ptr<Request> requestPtr,
                         Ptr<TreeNode> treeNodePtr,
                         const RowPtr & rowRef)
{
  /**
   * Here we need to...
   *   1) construct a key
   *   2) compute hash     (normally TC)
   *   3) get node for row (normally TC)
   */
  Uint32 err;
  const LqhKeyReq* src = (LqhKeyReq*)treeNodePtr.p->m_lookup_data.m_lqhKeyReq;
  const Uint32 tableId = LqhKeyReq::getTableId(src->tableSchemaVersion);
  const Uint32 corrVal = rowRef.m_src_correlation;

  DEBUG("::lookup_parent_row");

  do
  {
    Uint32 ptrI = RNIL;
    if (treeNodePtr.p->m_bits & TreeNode::T_KEYINFO_CONSTRUCTED)
    {
      jam();
      DEBUG("parent_row w/ T_KEYINFO_CONSTRUCTED");
      /**
       * Get key-pattern
       */
      LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
      Local_pattern_store pattern(pool, treeNodePtr.p->m_keyPattern);

      bool keyIsNull;
      err = expand(ptrI, pattern, rowRef, keyIsNull);
      if (unlikely(err != 0))
        break;

      if (keyIsNull)
      {
        jam();
        DEBUG("Key contain NULL values");
        /**
         * When the key contains NULL values, an EQ-match is impossible!
         * Entire lookup request can therefore be eliminate as it is known
         * to be REFused with errorCode = 626 (Row not found).
         * Different handling is required depening of request being a
         * scan or lookup:
         */
        if (requestPtr.p->isScan())
        {
          /**
           * Scan request: We can simply ignore lookup operation:
           * As rowCount in SCANCONF will not include this KEYREQ,
           * we dont have to send a KEYREF either.
           */
          jam();
          DEBUG("..Ignore impossible KEYREQ");
          if (ptrI != RNIL)
          {
            releaseSection(ptrI);
          }
          return;  // Bailout, KEYREQ would have returned KEYREF(626) anyway
        }
        else  // isLookup()
        {
          /**
           * Ignored lookup request need a faked KEYREF for the lookup operation.
           * Furthermore, if this is a leaf treeNode, a KEYCONF is also
           * expected by the API.
           *
           * TODO: Not implemented yet as we believe
           *       elimination of NULL key access for scan request
           *       will have the most performance impact.
           */
          jam();
        }
      } // keyIsNull

      /**
       * NOTE:
       *    The logic below contradicts 'keyIsNull' logic above and should
       *    be removed.
       *    However, it's likely that scanIndex should have similar
       *    logic as 'Null as wildcard' may make sense for a range bound.
       * NOTE2:
       *    Until 'keyIsNull' also cause bailout for request->isLookup()
       *    createEmptySection *is* require to avoid crash due to empty keys.
       */
      if (ptrI == RNIL)  // TODO: remove when keyIsNull is completely handled
      {
        jam();
        /**
         * We constructed a null-key...construct a zero-length key (even if we don't support it *now*)
         *
         *   (we actually did prior to joining mysql where null was treated as any other
         *   value in a key). But mysql treats null in unique key as *wildcard*
         *   which we don't support so well...and do nasty tricks in handler
         *
         * NOTE: should be *after* check for error
         */
        err = createEmptySection(ptrI);
        if (unlikely(err != 0))
          break;
      }

      treeNodePtr.p->m_send.m_keyInfoPtrI = ptrI;
    }

    BuildKeyReq tmp;
    err = computeHash(signal, tmp, tableId, treeNodePtr.p->m_send.m_keyInfoPtrI);
    if (unlikely(err != 0))
      break;

    err = getNodes(signal, tmp, tableId);
    if (unlikely(err != 0))
      break;

    Uint32 attrInfoPtrI = treeNodePtr.p->m_send.m_attrInfoPtrI;
    if (treeNodePtr.p->m_bits & TreeNode::T_ATTRINFO_CONSTRUCTED)
    {
      jam();
      Uint32 tmp = RNIL;
      ndbrequire(dupSection(tmp, attrInfoPtrI)); // TODO handle error

      Uint32 org_size;
      {
        SegmentedSectionPtr ptr;
        getSection(ptr, tmp);
        org_size = ptr.sz;
      }

      bool hasNull;
      LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
      Local_pattern_store pattern(pool, treeNodePtr.p->m_attrParamPattern);
      err = expand(tmp, pattern, rowRef, hasNull);
      if (unlikely(err != 0))
        break;
//    ndbrequire(!hasNull);

      /**
       * Update size of subsrouting section, which contains arguments
       */
      SegmentedSectionPtr ptr;
      getSection(ptr, tmp);
      Uint32 new_size = ptr.sz;
      Uint32 * sectionptrs = ptr.p->theData;
      sectionptrs[4] = new_size - org_size;

      treeNodePtr.p->m_send.m_attrInfoPtrI = tmp;
    }

    /**
     * Now send...
     */

    /**
     * TODO merge better with lookup_start (refactor)
     */
    {
      /* We set the upper half word of m_correlation to the tuple ID
       * of the parent, such that the API can match this tuple with its
       * parent.
       * Then we re-use the tuple ID of the parent as the
       * tuple ID for this tuple also. Since the tuple ID
       * is unique within this batch and SPJ block for the parent operation,
       * it must also be unique for this operation.
       * This ensures that lookup operations with no user projection will
       * work, since such operations will have the same tuple ID as their
       * parents. The API will then be able to match a tuple with its
       * grandparent, even if it gets no tuple for the parent operation.*/
      treeNodePtr.p->m_send.m_correlation =
        (corrVal << 16) + (corrVal & 0xffff);

      treeNodePtr.p->m_send.m_ref = tmp.receiverRef;
      LqhKeyReq * dst = (LqhKeyReq*)treeNodePtr.p->m_lookup_data.m_lqhKeyReq;
      dst->hashValue = tmp.hashInfo[0];
      dst->fragmentData = tmp.fragId;
      Uint32 attrLen = 0;
      LqhKeyReq::setDistributionKey(attrLen, tmp.fragDistKey);
      dst->attrLen = attrLen;
      lookup_send(signal, requestPtr, treeNodePtr);

      if (treeNodePtr.p->m_bits & TreeNode::T_ATTRINFO_CONSTRUCTED)
      {
        jam();
        // restore
        treeNodePtr.p->m_send.m_attrInfoPtrI = attrInfoPtrI;
      }
    }
    return;
  } while (0);

  ndbrequire(false);
}

void
Dbspj::lookup_parent_batch_complete(Signal* signal,
                                    Ptr<Request> requestPtr,
                                    Ptr<TreeNode> treeNodePtr)
{
  jam();

  /**
   * lookups are performed directly...so we're not really interested in
   *   parent_batch_complete...we only pass-through
   */

  /**
   * but this method should only be called if we have T_REPORT_BATCH_COMPLETE
   */
  ndbassert(treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE);

  ndbassert(!treeNodePtr.p->m_lookup_data.m_parent_batch_complete);
  treeNodePtr.p->m_lookup_data.m_parent_batch_complete = true;
  if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE
      && treeNodePtr.p->m_lookup_data.m_outstanding == 0)
  {
    jam();
    // We have received all rows for this operation in this batch.
    reportBatchComplete(signal, requestPtr, treeNodePtr);

    // Prepare for next batch.
    treeNodePtr.p->m_lookup_data.m_parent_batch_complete = false;
    treeNodePtr.p->m_lookup_data.m_outstanding = 0;
  }
}

void
Dbspj::lookup_abort(Signal* signal,
                    Ptr<Request> requestPtr,
                    Ptr<TreeNode> treeNodePtr)
{
  jam();
}

Uint32
Dbspj::lookup_execNODE_FAILREP(Signal* signal,
                               Ptr<Request> requestPtr,
                               Ptr<TreeNode> treeNodePtr,
                               NdbNodeBitmask mask)
{
  jam();
  Uint32 node = 0;
  Uint32 sum = 0;
  while (requestPtr.p->m_outstanding &&
         ((node = mask.find(node + 1)) != NdbNodeBitmask::NotFound))
  {
    Uint32 cnt = requestPtr.p->m_lookup_node_data[node];
    sum += cnt;
    requestPtr.p->m_lookup_node_data[node] = 0;
  }

  if (sum)
  {
    jam();
    ndbrequire(requestPtr.p->m_outstanding >= sum);
    requestPtr.p->m_outstanding -= sum;
  }

  return sum;
}

void
Dbspj::lookup_cleanup(Ptr<Request> requestPtr,
                      Ptr<TreeNode> treeNodePtr)
{
  cleanup_common(requestPtr, treeNodePtr);
}


Uint32
Dbspj::handle_special_hash(Uint32 tableId, Uint32 dstHash[4],
                           const Uint64* src,
                           Uint32 srcLen,       // Len in #32bit words
                           const KeyDescriptor* desc)
{
  const Uint32 MAX_KEY_SIZE_IN_LONG_WORDS=
    (MAX_KEY_SIZE_IN_WORDS + 1) / 2;
  Uint64 alignedWorkspace[MAX_KEY_SIZE_IN_LONG_WORDS * MAX_XFRM_MULTIPLY];
  const bool hasVarKeys = desc->noOfVarKeys > 0;
  const bool hasCharAttr = desc->hasCharAttr;
  const bool compute_distkey = desc->noOfDistrKeys > 0;

  const Uint64 *hashInput = 0;
  Uint32 inputLen = 0;
  Uint32 keyPartLen[MAX_ATTRIBUTES_IN_INDEX];
  Uint32 * keyPartLenPtr;

  /* Normalise KeyInfo into workspace if necessary */
  if (hasCharAttr || (compute_distkey && hasVarKeys))
  {
    hashInput = alignedWorkspace;
    keyPartLenPtr = keyPartLen;
    inputLen = xfrm_key(tableId,
                        (Uint32*)src,
                        (Uint32*)alignedWorkspace,
                        sizeof(alignedWorkspace) >> 2,
                        keyPartLenPtr);
    if (unlikely(inputLen == 0))
    {
      return 290;  // 'Corrupt key in TC, unable to xfrm'
    }
  }
  else
  {
    /* Keyinfo already suitable for hash */
    hashInput = src;
    inputLen = srcLen;
    keyPartLenPtr = 0;
  }

  /* Calculate primary key hash */
  md5_hash(dstHash, hashInput, inputLen);

  /* If the distribution key != primary key then we have to
   * form a distribution key from the primary key and calculate
   * a separate distribution hash based on this
   */
  if (compute_distkey)
  {
    jam();

    Uint32 distrKeyHash[4];
    /* Reshuffle primary key columns to get just distribution key */
    Uint32 len = create_distr_key(tableId, (Uint32*)hashInput, (Uint32*)alignedWorkspace, keyPartLenPtr);
    /* Calculate distribution key hash */
    md5_hash(distrKeyHash, alignedWorkspace, len);

    /* Just one word used for distribution */
    dstHash[1] = distrKeyHash[1];
  }
  return 0;
}

Uint32
Dbspj::computeHash(Signal* signal,
                   BuildKeyReq& dst, Uint32 tableId, Uint32 ptrI)
{
  /**
   * Essentially the same code as in Dbtc::hash().
   * The code for user defined partitioning has been removed though.
   */
  SegmentedSectionPtr ptr;
  getSection(ptr, ptrI);

  /* NOTE:  md5_hash below require 64-bit alignment
   */
  const Uint32 MAX_KEY_SIZE_IN_LONG_WORDS=
    (MAX_KEY_SIZE_IN_WORDS + 1) / 2;
  Uint64 tmp64[MAX_KEY_SIZE_IN_LONG_WORDS];
  Uint32 *tmp32 = (Uint32*)tmp64;
  copy(tmp32, ptr);

  const KeyDescriptor* desc = g_key_descriptor_pool.getPtr(tableId);
  ndbrequire(desc != NULL);

  bool need_special_hash = desc->hasCharAttr | (desc->noOfDistrKeys > 0);
  if (need_special_hash)
  {
    jam();
    return handle_special_hash(tableId, dst.hashInfo, tmp64, ptr.sz, desc);
  }
  else
  {
    jam();
    md5_hash(dst.hashInfo, tmp64, ptr.sz);
    return 0;
  }
}

/**
 * This function differs from computeHash in that *ptrI*
 * only contains partition key (packed) and not full primary key
 */
Uint32
Dbspj::computePartitionHash(Signal* signal,
                            BuildKeyReq& dst, Uint32 tableId, Uint32 ptrI)
{
  SegmentedSectionPtr ptr;
  getSection(ptr, ptrI);

  /* NOTE:  md5_hash below require 64-bit alignment
   */
  const Uint32 MAX_KEY_SIZE_IN_LONG_WORDS=
    (MAX_KEY_SIZE_IN_WORDS + 1) / 2;
  Uint64 _space[MAX_KEY_SIZE_IN_LONG_WORDS];
  Uint64 *tmp64 = _space;
  Uint32 *tmp32 = (Uint32*)tmp64;
  Uint32 sz = ptr.sz;
  copy(tmp32, ptr);

  const KeyDescriptor* desc = g_key_descriptor_pool.getPtr(tableId);
  ndbrequire(desc != NULL);

  bool need_xfrm = desc->hasCharAttr || desc->noOfVarKeys;
  if (need_xfrm)
  {
    jam();
    /**
     * xfrm distribution key
     */
    Uint32 srcPos = 0;
    Uint32 dstPos = 0;
    Uint32 * src = tmp32;
    Uint32 * dst = signal->theData+24;
    for (Uint32 i = 0; i < desc->noOfKeyAttr; i++)
    {
      const KeyDescriptor::KeyAttr& keyAttr = desc->keyAttr[i];
      if (AttributeDescriptor::getDKey(keyAttr.attributeDescriptor))
      {
        xfrm_attr(keyAttr.attributeDescriptor, keyAttr.charsetInfo,
                  src, srcPos, dst, dstPos,
                  NDB_ARRAY_SIZE(signal->theData) - 24);
      }
    }
    tmp64 = (Uint64*)dst;
    sz = dstPos;
  }

  md5_hash(dst.hashInfo, tmp64, sz);
  return 0;
}

Uint32
Dbspj::getNodes(Signal* signal, BuildKeyReq& dst, Uint32 tableId)
{
  Uint32 err;
  DiGetNodesReq * req = (DiGetNodesReq *)&signal->theData[0];
  req->tableId = tableId;
  req->hashValue = dst.hashInfo[1];
  req->distr_key_indicator = 0; // userDefinedPartitioning not supported!
  * (EmulatedJamBuffer**)req->jamBuffer = jamBuffer();

#if 1
  EXECUTE_DIRECT(DBDIH, GSN_DIGETNODESREQ, signal,
                 DiGetNodesReq::SignalLength, 0);
#else
  sendSignal(DBDIH_REF, GSN_DIGETNODESREQ, signal,
             DiGetNodesReq::SignalLength, JBB);
  jamEntry();

#endif

  DiGetNodesConf * conf = (DiGetNodesConf *)&signal->theData[0];
  err = signal->theData[0];
  Uint32 Tdata2 = conf->reqinfo;
  Uint32 nodeId = conf->nodes[0];
  Uint32 instanceKey = (Tdata2 >> 24) & 127;

  DEBUG("HASH to nodeId:" << nodeId << ", instanceKey:" << instanceKey);

  jamEntry();
  if (unlikely(err != 0))
    goto error;

  dst.fragId = conf->fragId;
  dst.fragDistKey = (Tdata2 >> 16) & 255;
  dst.receiverRef = numberToRef(DBLQH, instanceKey, nodeId);

  return 0;

error:
  /**
   * TODO handle error
   */
  ndbrequire(false);
  return err;
}

/**
 * END - MODULE LOOKUP
 */

/**
 * MODULE SCAN FRAG
 *
 * NOTE: This may only be root node
 */
const Dbspj::OpInfo
Dbspj::g_ScanFragOpInfo =
{
  &Dbspj::scanFrag_build,
  0, // prepare
  &Dbspj::scanFrag_start,
  &Dbspj::scanFrag_execTRANSID_AI,
  0, // execLQHKEYREF
  0, // execLQHKEYCONF
  &Dbspj::scanFrag_execSCAN_FRAGREF,
  &Dbspj::scanFrag_execSCAN_FRAGCONF,
  0, // parent row
  0, // parent batch complete
  0, // parent batch repeat
  0, // Dbspj::scanFrag_parent_batch_cleanup,
  &Dbspj::scanFrag_execSCAN_NEXTREQ,
  0, // Dbspj::scanFrag_complete
  &Dbspj::scanFrag_abort,
  0, // execNODE_FAILREP,
  &Dbspj::scanFrag_cleanup
};

Uint32
Dbspj::scanFrag_build(Build_context& ctx,
                      Ptr<Request> requestPtr,
                      const QueryNode* qn,
                      const QueryNodeParameters* qp)
{
  Uint32 err = 0;
  Ptr<TreeNode> treeNodePtr;
  const QN_ScanFragNode * node = (const QN_ScanFragNode*)qn;
  const QN_ScanFragParameters * param = (const QN_ScanFragParameters*)qp;

  do
  {
    err = createNode(ctx, requestPtr, treeNodePtr);
    if (unlikely(err != 0))
      break;

    treeNodePtr.p->m_scanfrag_data.m_scanFragHandlePtrI = RNIL;
    Ptr<ScanFragHandle> scanFragHandlePtr;
    if (unlikely(m_scanfraghandle_pool.seize(requestPtr.p->m_arena,
                                             scanFragHandlePtr) != true))
    {
      err = DbspjErr::OutOfQueryMemory;
      break;
    }

    scanFragHandlePtr.p->m_treeNodePtrI = treeNodePtr.i;
    scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_NOT_STARTED;
    treeNodePtr.p->m_scanfrag_data.m_scanFragHandlePtrI = scanFragHandlePtr.i;

    requestPtr.p->m_bits |= Request::RT_SCAN;
    treeNodePtr.p->m_info = &g_ScanFragOpInfo;
    treeNodePtr.p->m_bits |= TreeNode::T_ATTR_INTERPRETED;
    treeNodePtr.p->m_batch_size = ctx.m_batch_size_rows;

    ScanFragReq*dst=(ScanFragReq*)treeNodePtr.p->m_scanfrag_data.m_scanFragReq;
    dst->senderData = scanFragHandlePtr.i;
    dst->resultRef = reference();
    dst->resultData = treeNodePtr.i;
    dst->savePointId = ctx.m_savepointId;

    Uint32 transId1 = requestPtr.p->m_transId[0];
    Uint32 transId2 = requestPtr.p->m_transId[1];
    dst->transId1 = transId1;
    dst->transId2 = transId2;

    Uint32 treeBits = node->requestInfo;
    Uint32 paramBits = param->requestInfo;
    //ndbout_c("Dbspj::scanFrag_build() treeBits=%.8x paramBits=%.8x",
    //         treeBits, paramBits);
    Uint32 requestInfo = 0;
    ScanFragReq::setReadCommittedFlag(requestInfo, 1);
    ScanFragReq::setScanPrio(requestInfo, ctx.m_scanPrio);
    ScanFragReq::setCorrFactorFlag(requestInfo, 1);
    ScanFragReq::setNoDiskFlag(requestInfo,
                               (treeBits & DABits::NI_LINKED_DISK) == 0 &&
                               (paramBits & DABits::PI_DISK_ATTR) == 0);
    dst->requestInfo = requestInfo;

    err = DbspjErr::InvalidTreeNodeSpecification;
    DEBUG("scanFrag_build: len=" << node->len);
    if (unlikely(node->len < QN_ScanFragNode::NodeSize))
      break;

    dst->tableId = node->tableId;
    dst->schemaVersion = node->tableVersion;

    err = DbspjErr::InvalidTreeParametersSpecification;
    DEBUG("param len: " << param->len);
    if (unlikely(param->len < QN_ScanFragParameters::NodeSize))
    {
      jam();
      DEBUG_CRASH();
      break;
    }

    ctx.m_resultData = param->resultData;

    /**
     * Parse stuff common lookup/scan-frag
     */
    struct DABuffer nodeDA, paramDA;
    nodeDA.ptr = node->optional;
    nodeDA.end = nodeDA.ptr + (node->len - QN_ScanFragNode::NodeSize);
    paramDA.ptr = param->optional;
    paramDA.end = paramDA.ptr + (param->len - QN_ScanFragParameters::NodeSize);
    err = parseDA(ctx, requestPtr, treeNodePtr,
                  nodeDA, treeBits, paramDA, paramBits);
    if (unlikely(err != 0))
    {
      jam();
      DEBUG_CRASH();
      break;
    }

    ctx.m_scan_cnt++;
    ctx.m_scans.set(treeNodePtr.p->m_node_no);

    if (ctx.m_start_signal)
    {
      jam();
      Signal* signal = ctx.m_start_signal;
      const ScanFragReq* src = (const ScanFragReq*)(signal->getDataPtr());

#if NOT_YET
      Uint32 instanceNo =
        blockToInstance(signal->header.theReceiversBlockNumber);
      treeNodePtr.p->m_send.m_ref = numberToRef(DBLQH,
                                                instanceNo, getOwnNodeId());
#else
      treeNodePtr.p->m_send.m_ref =
        numberToRef(DBLQH, getInstanceKey(src->tableId,
                                          src->fragmentNoKeyLen),
                    getOwnNodeId());
#endif

      Uint32 fragId = src->fragmentNoKeyLen;
      Uint32 requestInfo = src->requestInfo;
      Uint32 batch_size_bytes = src->batch_size_bytes;
      Uint32 batch_size_rows = src->batch_size_rows;

#ifdef VM_TRACE
      Uint32 savePointId = src->savePointId;
      Uint32 tableId = src->tableId;
      Uint32 schemaVersion = src->schemaVersion;
      Uint32 transId1 = src->transId1;
      Uint32 transId2 = src->transId2;
#endif
      ndbassert(ScanFragReq::getLockMode(requestInfo) == 0);
      ndbassert(ScanFragReq::getHoldLockFlag(requestInfo) == 0);
      ndbassert(ScanFragReq::getKeyinfoFlag(requestInfo) == 0);
      ndbassert(ScanFragReq::getReadCommittedFlag(requestInfo) == 1);
      ndbassert(ScanFragReq::getLcpScanFlag(requestInfo) == 0);
      //ScanFragReq::getAttrLen(requestInfo); // ignore
      ndbassert(ScanFragReq::getReorgFlag(requestInfo) == 0);

      Uint32 tupScanFlag = ScanFragReq::getTupScanFlag(requestInfo);
      Uint32 rangeScanFlag = ScanFragReq::getRangeScanFlag(requestInfo);
      Uint32 descendingFlag = ScanFragReq::getDescendingFlag(requestInfo);
      Uint32 scanPrio = ScanFragReq::getScanPrio(requestInfo);

      Uint32 dst_requestInfo = dst->requestInfo;

      ScanFragReq::setTupScanFlag(dst_requestInfo,tupScanFlag);
      ScanFragReq::setRangeScanFlag(dst_requestInfo,rangeScanFlag);
      ScanFragReq::setDescendingFlag(dst_requestInfo,descendingFlag);
      ScanFragReq::setScanPrio(dst_requestInfo,scanPrio);

      /**
       * 'NoDiskFlag' should agree with information in treeNode
       */
      ndbassert(ScanFragReq::getNoDiskFlag(requestInfo) ==
                ScanFragReq::getNoDiskFlag(dst_requestInfo));

      dst->fragmentNoKeyLen = fragId;
      dst->requestInfo = dst_requestInfo;
      dst->batch_size_bytes = batch_size_bytes;
      dst->batch_size_rows = batch_size_rows;

#ifdef VM_TRACE
      ndbassert(dst->savePointId == savePointId);
      ndbassert(dst->tableId == tableId);
      ndbassert(dst->schemaVersion == schemaVersion);
      ndbassert(dst->transId1 == transId1);
      ndbassert(dst->transId2 == transId2);
#endif

      treeNodePtr.p->m_send.m_keyInfoPtrI = ctx.m_keyPtr.i;
      treeNodePtr.p->m_bits |= TreeNode::T_ONE_SHOT;

      if (rangeScanFlag)
      {
        c_Counters.incr_counter(CI_RANGE_SCANS_RECEIVED, 1);
      }
      else
      {
        c_Counters.incr_counter(CI_TABLE_SCANS_RECEIVED, 1);
      }
    }
    else
    {
      ndbrequire(false);
    }

    return 0;
  } while (0);

  return err;
}

void
Dbspj::scanFrag_start(Signal* signal,
                      Ptr<Request> requestPtr,
                      Ptr<TreeNode> treeNodePtr)
{
  scanFrag_send(signal, requestPtr, treeNodePtr);
}

void
Dbspj::scanFrag_send(Signal* signal,
                     Ptr<Request> requestPtr,
                     Ptr<TreeNode> treeNodePtr)
{
  jam();

  requestPtr.p->m_outstanding++;
  requestPtr.p->m_cnt_active++;
  treeNodePtr.p->m_state = TreeNode::TN_ACTIVE;
  Ptr<ScanFragHandle> scanFragHandlePtr;
  m_scanfraghandle_pool.getPtr(scanFragHandlePtr, treeNodePtr.p->
                               m_scanfrag_data.m_scanFragHandlePtrI);

  ScanFragReq* req = reinterpret_cast<ScanFragReq*>(signal->getDataPtrSend());

  memcpy(req, treeNodePtr.p->m_scanfrag_data.m_scanFragReq,
         sizeof(treeNodePtr.p->m_scanfrag_data.m_scanFragReq));
  req->variableData[0] = treeNodePtr.p->m_send.m_correlation;
  req->variableData[1] = requestPtr.p->m_rootResultData;

  SectionHandle handle(this);

  Uint32 ref = treeNodePtr.p->m_send.m_ref;
  Uint32 keyInfoPtrI = treeNodePtr.p->m_send.m_keyInfoPtrI;
  Uint32 attrInfoPtrI = treeNodePtr.p->m_send.m_attrInfoPtrI;

  /**
   * ScanFrag may only be used as root-node, i.e T_ONE_SHOT
   */
  ndbrequire(treeNodePtr.p->m_bits & TreeNode::T_ONE_SHOT);

  /**
   * Pass sections to send
   */
  treeNodePtr.p->m_send.m_attrInfoPtrI = RNIL;
  treeNodePtr.p->m_send.m_keyInfoPtrI = RNIL;

  getSection(handle.m_ptr[0], attrInfoPtrI);
  handle.m_cnt = 1;

  if (keyInfoPtrI != RNIL)
  {
    jam();
    getSection(handle.m_ptr[1], keyInfoPtrI);
    handle.m_cnt = 2;
  }

#ifdef DEBUG_SCAN_FRAGREQ
  ndbout_c("SCAN_FRAGREQ to %x", ref);
  printSCAN_FRAGREQ(stdout, signal->getDataPtrSend(),
                    NDB_ARRAY_SIZE(treeNodePtr.p->m_scanfrag_data.m_scanFragReq),
                    DBLQH);
  printf("ATTRINFO: ");
  print(handle.m_ptr[0], stdout);
  if (handle.m_cnt > 1)
  {
    printf("KEYINFO: ");
    print(handle.m_ptr[1], stdout);
  }
#endif

  if (ScanFragReq::getRangeScanFlag(req->requestInfo))
  {
    c_Counters.incr_counter(CI_LOCAL_RANGE_SCANS_SENT, 1);
  }
  else
  {
    c_Counters.incr_counter(CI_LOCAL_TABLE_SCANS_SENT, 1);
  }

  ndbrequire(refToNode(ref) == getOwnNodeId());
  sendSignal(ref, GSN_SCAN_FRAGREQ, signal,
             NDB_ARRAY_SIZE(treeNodePtr.p->m_scanfrag_data.m_scanFragReq),
             JBB, &handle);

  scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_SCANNING;
  treeNodePtr.p->m_scanfrag_data.m_rows_received = 0;
  treeNodePtr.p->m_scanfrag_data.m_rows_expecting = ~Uint32(0);
}

void
Dbspj::scanFrag_execTRANSID_AI(Signal* signal,
                               Ptr<Request> requestPtr,
                               Ptr<TreeNode> treeNodePtr,
                               const RowPtr & rowRef)
{
  jam();
  treeNodePtr.p->m_scanfrag_data.m_rows_received++;

  LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
  Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
  Dependency_map::ConstDataBufferIterator it;

  {
    for (list.first(it); !it.isNull(); list.next(it))
    {
      jam();
      Ptr<TreeNode> childPtr;
      m_treenode_pool.getPtr(childPtr, * it.data);
      ndbrequire(childPtr.p->m_info != 0&&childPtr.p->m_info->m_parent_row!=0);
      (this->*(childPtr.p->m_info->m_parent_row))(signal,
                                                  requestPtr, childPtr,rowRef);
    }
  }

  if (treeNodePtr.p->m_scanfrag_data.m_rows_received ==
      treeNodePtr.p->m_scanfrag_data.m_rows_expecting)
  {
    jam();

    if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE)
    {
      jam();
      reportBatchComplete(signal, requestPtr, treeNodePtr);
    }

    checkBatchComplete(signal, requestPtr, 1);
    return;
  }
}

void
Dbspj::scanFrag_execSCAN_FRAGREF(Signal* signal,
                                 Ptr<Request> requestPtr,
                                 Ptr<TreeNode> treeNodePtr,
                                 Ptr<ScanFragHandle> scanFragHandlePtr)
{
  const ScanFragRef* rep =
    reinterpret_cast<const ScanFragRef*>(signal->getDataPtr());
  Uint32 errCode = rep->errorCode;

  DEBUG("scanFrag_execSCAN_FRAGREF, rep->senderData:" << rep->senderData
        << ", requestPtr.p->m_senderData:" << requestPtr.p->m_senderData);
  scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_COMPLETE;
  ndbrequire(treeNodePtr.p->m_state == TreeNode::TN_ACTIVE);
  ndbrequire(requestPtr.p->m_cnt_active);
  requestPtr.p->m_cnt_active--;
  ndbrequire(requestPtr.p->m_outstanding);
  requestPtr.p->m_outstanding--;
  treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;

  abort(signal, requestPtr, errCode);
}


void
Dbspj::scanFrag_execSCAN_FRAGCONF(Signal* signal,
                                  Ptr<Request> requestPtr,
                                  Ptr<TreeNode> treeNodePtr,
                                  Ptr<ScanFragHandle> scanFragHandlePtr)
{
  const ScanFragConf * conf =
    reinterpret_cast<const ScanFragConf*>(signal->getDataPtr());
  Uint32 rows = conf->completedOps;
  Uint32 done = conf->fragmentCompleted;

  Uint32 state = scanFragHandlePtr.p->m_state;
  if (state == ScanFragHandle::SFH_WAIT_CLOSE && done == 0)
  {
    jam();
    /**
     * We sent an explicit close request...ignore this...a close will come later
     */
    return;
  }

  ndbrequire(done <= 2); // 0, 1, 2 (=ZSCAN_FRAG_CLOSED)

  ndbassert(treeNodePtr.p->m_scanfrag_data.m_rows_expecting == ~Uint32(0));
  treeNodePtr.p->m_scanfrag_data.m_rows_expecting = rows;
  if (treeNodePtr.p->isLeaf())
  {
    /**
     * If this is a leaf node, then no rows will be sent to the SPJ block,
     * as there are no child operations to instantiate.
     */
    treeNodePtr.p->m_scanfrag_data.m_rows_received = rows;
  }

  requestPtr.p->m_rows += rows;
  if (done)
  {
    jam();

    ndbrequire(requestPtr.p->m_cnt_active);
    requestPtr.p->m_cnt_active--;
    treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;
    scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_COMPLETE;
  }
  else
  {
    jam();
    scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_WAIT_NEXTREQ;
  }

  if (treeNodePtr.p->m_scanfrag_data.m_rows_expecting ==
      treeNodePtr.p->m_scanfrag_data.m_rows_received ||
      (state == ScanFragHandle::SFH_WAIT_CLOSE))
  {
    jam();

    if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE)
    {
      jam();
      reportBatchComplete(signal, requestPtr, treeNodePtr);
    }

    checkBatchComplete(signal, requestPtr, 1);
    return;
  }
}

void
Dbspj::scanFrag_execSCAN_NEXTREQ(Signal* signal,
                                 Ptr<Request> requestPtr,
                                 Ptr<TreeNode> treeNodePtr)
{
  jamEntry();

  Ptr<ScanFragHandle> scanFragHandlePtr;
  m_scanfraghandle_pool.getPtr(scanFragHandlePtr, treeNodePtr.p->
                               m_scanfrag_data.m_scanFragHandlePtrI);

  const ScanFragReq * org =
    (ScanFragReq*)treeNodePtr.p->m_scanfrag_data.m_scanFragReq;

  ScanFragNextReq* req =
    reinterpret_cast<ScanFragNextReq*>(signal->getDataPtrSend());
  req->senderData = treeNodePtr.p->m_scanfrag_data.m_scanFragHandlePtrI;
  req->requestInfo = 0;
  req->transId1 = requestPtr.p->m_transId[0];
  req->transId2 = requestPtr.p->m_transId[1];
  req->batch_size_rows = org->batch_size_rows;
  req->batch_size_bytes = org->batch_size_bytes;

  DEBUG("scanFrag_execSCAN_NEXTREQ to: " << hex << treeNodePtr.p->m_send.m_ref
        << ", senderData: " << req->senderData);
#ifdef DEBUG_SCAN_FRAGREQ
  printSCANFRAGNEXTREQ(stdout, &signal->theData[0],
                       ScanFragNextReq::SignalLength, DBLQH);
#endif

  sendSignal(treeNodePtr.p->m_send.m_ref,
             GSN_SCAN_NEXTREQ,
             signal,
             ScanFragNextReq::SignalLength,
             JBB);

  treeNodePtr.p->m_scanfrag_data.m_rows_received = 0;
  treeNodePtr.p->m_scanfrag_data.m_rows_expecting = ~Uint32(0);
  requestPtr.p->m_outstanding++;
  scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_SCANNING;
}//Dbspj::scanFrag_execSCAN_NEXTREQ()

void
Dbspj::scanFrag_abort(Signal* signal,
                      Ptr<Request> requestPtr,
                      Ptr<TreeNode> treeNodePtr)
{
  jam();

  Ptr<ScanFragHandle> scanFragHandlePtr;
  m_scanfraghandle_pool.getPtr(scanFragHandlePtr, treeNodePtr.p->
                               m_scanfrag_data.m_scanFragHandlePtrI);
  if (treeNodePtr.p->m_state == TreeNode::TN_ACTIVE)
  {
    jam();

    switch(scanFragHandlePtr.p->m_state){
    case ScanFragHandle::SFH_NOT_STARTED:
    case ScanFragHandle::SFH_COMPLETE:
      ndbrequire(false); // we shouldnt be TN_ACTIVE then...

    case ScanFragHandle::SFH_WAIT_CLOSE:
      jam();
      // close already sent
      return;
    case ScanFragHandle::SFH_WAIT_NEXTREQ:
      jam();
      // we were idle
      requestPtr.p->m_outstanding++;
      break;
    case ScanFragHandle::SFH_SCANNING:
      jam();
      break;
    }

    treeNodePtr.p->m_scanfrag_data.m_rows_expecting = ~Uint32(0);
    scanFragHandlePtr.p->m_state = ScanFragHandle::SFH_WAIT_CLOSE;

    ScanFragNextReq* req =
      reinterpret_cast<ScanFragNextReq*>(signal->getDataPtrSend());
    req->senderData = treeNodePtr.p->m_scanfrag_data.m_scanFragHandlePtrI;
    req->requestInfo = ScanFragNextReq::ZCLOSE;
    req->transId1 = requestPtr.p->m_transId[0];
    req->transId2 = requestPtr.p->m_transId[1];
    req->batch_size_rows = 0;
    req->batch_size_bytes = 0;

    sendSignal(treeNodePtr.p->m_send.m_ref,
               GSN_SCAN_NEXTREQ,
               signal,
               ScanFragNextReq::SignalLength,
               JBB);
  }
}


void
Dbspj::scanFrag_cleanup(Ptr<Request> requestPtr,
                        Ptr<TreeNode> treeNodePtr)
{
  Uint32 ptrI = treeNodePtr.p->m_scanfrag_data.m_scanFragHandlePtrI;
  if (ptrI != RNIL)
  {
    m_scanfraghandle_pool.release(ptrI);
  }
  cleanup_common(requestPtr, treeNodePtr);
}

/**
 * END - MODULE SCAN FRAG
 */

/**
 * MODULE SCAN INDEX
 *
 * NOTE: This may not be root-node
 */
const Dbspj::OpInfo
Dbspj::g_ScanIndexOpInfo =
{
  &Dbspj::scanIndex_build,
  &Dbspj::scanIndex_prepare,
  0, // start
  &Dbspj::scanIndex_execTRANSID_AI,
  0, // execLQHKEYREF
  0, // execLQHKEYCONF
  &Dbspj::scanIndex_execSCAN_FRAGREF,
  &Dbspj::scanIndex_execSCAN_FRAGCONF,
  &Dbspj::scanIndex_parent_row,
  &Dbspj::scanIndex_parent_batch_complete,
  &Dbspj::scanIndex_parent_batch_repeat,
  &Dbspj::scanIndex_parent_batch_cleanup,
  &Dbspj::scanIndex_execSCAN_NEXTREQ,
  &Dbspj::scanIndex_complete,
  &Dbspj::scanIndex_abort,
  &Dbspj::scanIndex_execNODE_FAILREP,
  &Dbspj::scanIndex_cleanup
};

Uint32
Dbspj::scanIndex_build(Build_context& ctx,
                       Ptr<Request> requestPtr,
                       const QueryNode* qn,
                       const QueryNodeParameters* qp)
{
  Uint32 err = 0;
  Ptr<TreeNode> treeNodePtr;
  const QN_ScanIndexNode * node = (const QN_ScanIndexNode*)qn;
  const QN_ScanIndexParameters * param = (const QN_ScanIndexParameters*)qp;

  do
  {
    err = createNode(ctx, requestPtr, treeNodePtr);
    if (unlikely(err != 0))
      break;

    Uint32 batchSize = param->batchSize;

    requestPtr.p->m_bits |= Request::RT_SCAN;
    requestPtr.p->m_bits |= Request::RT_NEED_PREPARE;
    requestPtr.p->m_bits |= Request::RT_NEED_COMPLETE;
    treeNodePtr.p->m_info = &g_ScanIndexOpInfo;
    treeNodePtr.p->m_bits |= TreeNode::T_ATTR_INTERPRETED;
    treeNodePtr.p->m_bits |= TreeNode::T_NEED_REPORT_BATCH_COMPLETED;
    treeNodePtr.p->m_batch_size = 
      batchSize & ~(0xFFFFFFFF << QN_ScanIndexParameters::BatchRowBits);

    ScanFragReq*dst=(ScanFragReq*)treeNodePtr.p->m_scanindex_data.m_scanFragReq;
    dst->senderData = treeNodePtr.i;
    dst->resultRef = reference();
    dst->resultData = treeNodePtr.i;
    dst->savePointId = ctx.m_savepointId;
    dst->batch_size_rows  = 
      batchSize & ~(0xFFFFFFFF << QN_ScanIndexParameters::BatchRowBits);
    dst->batch_size_bytes = batchSize >> QN_ScanIndexParameters::BatchRowBits;

    Uint32 transId1 = requestPtr.p->m_transId[0];
    Uint32 transId2 = requestPtr.p->m_transId[1];
    dst->transId1 = transId1;
    dst->transId2 = transId2;

    Uint32 treeBits = node->requestInfo;
    Uint32 paramBits = param->requestInfo;
    Uint32 requestInfo = 0;
    ScanFragReq::setRangeScanFlag(requestInfo, 1);
    ScanFragReq::setReadCommittedFlag(requestInfo, 1);
    ScanFragReq::setScanPrio(requestInfo, ctx.m_scanPrio);
    ScanFragReq::setNoDiskFlag(requestInfo,
                               (treeBits & DABits::NI_LINKED_DISK) == 0 &&
                               (paramBits & DABits::PI_DISK_ATTR) == 0);
    ScanFragReq::setCorrFactorFlag(requestInfo, 1);
    dst->requestInfo = requestInfo;

    err = DbspjErr::InvalidTreeNodeSpecification;
    DEBUG("scanIndex_build: len=" << node->len);
    if (unlikely(node->len < QN_ScanIndexNode::NodeSize))
      break;

    dst->tableId = node->tableId;
    dst->schemaVersion = node->tableVersion;

    err = DbspjErr::InvalidTreeParametersSpecification;
    DEBUG("param len: " << param->len);
    if (unlikely(param->len < QN_ScanIndexParameters::NodeSize))
    {
      jam();
      DEBUG_CRASH();
      break;
    }

    ctx.m_resultData = param->resultData;

    /**
     * Parse stuff
     */
    struct DABuffer nodeDA, paramDA;
    nodeDA.ptr = node->optional;
    nodeDA.end = nodeDA.ptr + (node->len - QN_ScanIndexNode::NodeSize);
    paramDA.ptr = param->optional;
    paramDA.end = paramDA.ptr + (param->len - QN_ScanIndexParameters::NodeSize);

    err = parseScanIndex(ctx, requestPtr, treeNodePtr,
                         nodeDA, treeBits, paramDA, paramBits);

    if (unlikely(err != 0))
    {
      jam();
      DEBUG_CRASH();
      break;
    }

    /**
     * Since we T_NEED_REPORT_BATCH_COMPLETED, we set
     *   this on all our parents...
     */
    Ptr<TreeNode> nodePtr;
    nodePtr.i = treeNodePtr.p->m_parentPtrI;
    while (nodePtr.i != RNIL)
    {
      jam();
      m_treenode_pool.getPtr(nodePtr);
      nodePtr.p->m_bits |= TreeNode::T_REPORT_BATCH_COMPLETE;
      nodePtr.p->m_bits |= TreeNode::T_NEED_REPORT_BATCH_COMPLETED;
      nodePtr.i = nodePtr.p->m_parentPtrI;
    }

    /**
     * If there exists other scan TreeNodes not being among 
     * my ancestors, results from this scanIndex may be repeated 
     * as part of an X-scan.
     *
     * NOTE: The scan nodes being along the left deep ancestor chain
     *       are not 'repeatable' as they are driving the
     *       repeated X-scan and are thus not repeated themself.
     */
    if (requestPtr.p->m_bits & Request::RT_REPEAT_SCAN_RESULT &&
       !treeNodePtr.p->m_ancestors.contains(ctx.m_scans))
    {
      treeNodePtr.p->m_bits |= TreeNode::T_SCAN_REPEATABLE;
    }

    ctx.m_scan_cnt++;
    ctx.m_scans.set(treeNodePtr.p->m_node_no);

    return 0;
  } while (0);

  return err;
}

Uint32
Dbspj::parseScanIndex(Build_context& ctx,
                      Ptr<Request> requestPtr,
                      Ptr<TreeNode> treeNodePtr,
                      DABuffer tree, Uint32 treeBits,
                      DABuffer param, Uint32 paramBits)
{
  Uint32 err = 0;

  typedef QN_ScanIndexNode Node;
  typedef QN_ScanIndexParameters Params;

  do
  {
    jam();

    ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
    data.m_fragments.init();
    data.m_frags_outstanding = 0;
    data.m_frags_complete = 0;
    data.m_frags_not_started = 0;
    data.m_parallelismStat.init();
    data.m_firstExecution = true;
    data.m_batch_chunks = 0;

    err = parseDA(ctx, requestPtr, treeNodePtr,
                  tree, treeBits, param, paramBits);
    if (unlikely(err != 0))
      break;

    if (treeBits & Node::SI_PRUNE_PATTERN)
    {
      Uint32 len_cnt = * tree.ptr ++;
      Uint32 len = len_cnt & 0xFFFF; // length of pattern in words
      Uint32 cnt = len_cnt >> 16;    // no of parameters

      LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
      ndbrequire((cnt==0) == ((treeBits & Node::SI_PRUNE_PARAMS) ==0));
      ndbrequire((cnt==0) == ((paramBits & Params::SIP_PRUNE_PARAMS)==0));

      if (treeBits & Node::SI_PRUNE_LINKED)
      {
        jam();
        DEBUG("LINKED-PRUNE PATTERN w/ " << cnt << " PARAM values");

        data.m_prunePattern.init();
        Local_pattern_store pattern(pool, data.m_prunePattern);

        /**
         * Expand pattern into a new pattern (with linked values)
         */
        err = expand(pattern, treeNodePtr, tree, len, param, cnt);
        if (unlikely(err != 0))
          break;

        treeNodePtr.p->m_bits |= TreeNode::T_PRUNE_PATTERN;
        c_Counters.incr_counter(CI_PRUNED_RANGE_SCANS_RECEIVED, 1);
      }
      else
      {
        jam();
        DEBUG("FIXED-PRUNE w/ " << cnt << " PARAM values");

        /**
         * Expand pattern directly into
         *   This means a "fixed" pruning from here on
         *   i.e guaranteed single partition
         */
        Uint32 prunePtrI = RNIL;
        bool hasNull;
        err = expand(prunePtrI, tree, len, param, cnt, hasNull);
        if (unlikely(err != 0))
          break;

        if (unlikely(hasNull))
        {
          /* API should have elliminated requests w/ const-NULL keys */
          jam();
          DEBUG("BEWARE: T_CONST_PRUNE-key contain NULL values");
//        treeNodePtr.p->m_bits |= TreeNode::T_NULL_PRUNE;
//        break;
          ndbrequire(false);
        }
        ndbrequire(prunePtrI != RNIL);  /* todo: can we allow / take advantage of NULLs in range scan? */
        data.m_constPrunePtrI = prunePtrI;

        /**
         * We may not compute the partition for the hash-key here
         *   as we have not yet opened a read-view
         */
        treeNodePtr.p->m_bits |= TreeNode::T_CONST_PRUNE;
        c_Counters.incr_counter(CI_CONST_PRUNED_RANGE_SCANS_RECEIVED, 1);
      }
    } //SI_PRUNE_PATTERN

    if ((treeNodePtr.p->m_bits & TreeNode::T_CONST_PRUNE) == 0 &&
        ((treeBits & Node::SI_PARALLEL) ||
         ((paramBits & Params::SIP_PARALLEL))))
    {
      jam();
      treeNodePtr.p->m_bits |= TreeNode::T_SCAN_PARALLEL;
    }

    return 0;
  } while(0);

  DEBUG_CRASH();
  return err;
}

void
Dbspj::scanIndex_prepare(Signal * signal,
                         Ptr<Request> requestPtr, Ptr<TreeNode> treeNodePtr)
{
  jam();

  treeNodePtr.p->m_state = TreeNode::TN_PREPARING;
  ScanFragReq*dst=(ScanFragReq*)treeNodePtr.p->m_scanindex_data.m_scanFragReq;

  DihScanTabReq * req = (DihScanTabReq*)signal->getDataPtrSend();
  req->senderRef = reference();
  req->senderData = treeNodePtr.i;
  req->tableId = dst->tableId;
  req->schemaTransId = 0;
  sendSignal(DBDIH_REF, GSN_DIH_SCAN_TAB_REQ, signal,
             DihScanTabReq::SignalLength, JBB);

  requestPtr.p->m_outstanding++;
}

void
Dbspj::execDIH_SCAN_TAB_REF(Signal* signal)
{
  jamEntry();
  ndbrequire(false);
}

void
Dbspj::execDIH_SCAN_TAB_CONF(Signal* signal)
{
  jamEntry();
  DihScanTabConf * conf = (DihScanTabConf*)signal->getDataPtr();

  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, conf->senderData);
  ndbrequire(treeNodePtr.p->m_info == &g_ScanIndexOpInfo);

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;

  Uint32 cookie = conf->scanCookie;
  Uint32 fragCount = conf->fragmentCount;
  ScanFragReq * dst = (ScanFragReq*)data.m_scanFragReq;

  if (conf->reorgFlag)
  {
    jam();
    ScanFragReq::setReorgFlag(dst->requestInfo, 1);
  }

  data.m_fragCount = fragCount;
  data.m_scanCookie = cookie;

  const Uint32 prunemask = TreeNode::T_PRUNE_PATTERN | TreeNode::T_CONST_PRUNE;
  bool pruned = (treeNodePtr.p->m_bits & prunemask) != 0;

  Ptr<Request> requestPtr;
  m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);

  Ptr<ScanFragHandle> fragPtr;
  Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
  if (likely(m_scanfraghandle_pool.seize(requestPtr.p->m_arena, fragPtr)))
  {
    jam();
    fragPtr.p->init(0);
    fragPtr.p->m_treeNodePtrI = treeNodePtr.i;
    list.addLast(fragPtr);
  }
  else
  {
    jam();
    goto error1;
  }

  if (treeNodePtr.p->m_bits & TreeNode::T_CONST_PRUNE)
  {
    jam();

    // TODO we need a different variant of computeHash here,
    // since m_constPrunePtrI does not contain full primary key
    // but only parts in distribution key

    BuildKeyReq tmp;
    Uint32 indexId = dst->tableId;
    Uint32 tableId = g_key_descriptor_pool.getPtr(indexId)->primaryTableId;
    Uint32 err = computePartitionHash(signal, tmp, tableId, data.m_constPrunePtrI);
    if (unlikely(err != 0))
      goto error;

    releaseSection(data.m_constPrunePtrI);
    data.m_constPrunePtrI = RNIL;

    err = getNodes(signal, tmp, tableId);
    if (unlikely(err != 0))
      goto error;

    fragPtr.p->m_fragId = tmp.fragId;
    fragPtr.p->m_ref = tmp.receiverRef;
    data.m_fragCount = 1;
  }
  else if (fragCount == 1)
  {
    jam();
    /**
     * This is roughly equivalent to T_CONST_PRUNE
     *   pretend that it is const-pruned
     */
    if (treeNodePtr.p->m_bits & TreeNode::T_PRUNE_PATTERN)
    {
      jam();
      LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
      Local_pattern_store pattern(pool, data.m_prunePattern);
      pattern.release();
    }
    data.m_constPrunePtrI = RNIL;
    Uint32 clear = TreeNode::T_PRUNE_PATTERN | TreeNode::T_SCAN_PARALLEL;
    treeNodePtr.p->m_bits &= ~clear;
    treeNodePtr.p->m_bits |= TreeNode::T_CONST_PRUNE;

    /**
     * We must get fragPtr.p->m_ref...so set pruned=false
     */
    pruned = false;
  }
  else
  {
    for (Uint32 i = 1; i<fragCount; i++)
    {
      jam();
      Ptr<ScanFragHandle> fragPtr;
      if (likely(m_scanfraghandle_pool.seize(requestPtr.p->m_arena, fragPtr)))
      {
        jam();
        fragPtr.p->init(i);
        fragPtr.p->m_treeNodePtrI = treeNodePtr.i;
        list.addLast(fragPtr);
      }
      else
      {
        goto error1;
      }
    }
  }
  data.m_frags_complete = data.m_fragCount;

  if (!pruned)
  {
    jam();
    Uint32 tableId = ((ScanFragReq*)data.m_scanFragReq)->tableId;
    DihScanGetNodesReq * req = (DihScanGetNodesReq*)signal->getDataPtrSend();
    req->senderRef = reference();
    req->tableId = tableId;
    req->scanCookie = cookie;

    Uint32 cnt = 0;
    for (list.first(fragPtr); !fragPtr.isNull(); list.next(fragPtr))
    {
      jam();
      req->senderData = fragPtr.i;
      req->fragId = fragPtr.p->m_fragId;
      sendSignal(DBDIH_REF, GSN_DIH_SCAN_GET_NODES_REQ, signal,
                 DihScanGetNodesReq::SignalLength, JBB);
      cnt++;
    }
    data.m_frags_outstanding = cnt;
    requestPtr.p->m_outstanding++;
  }
  else
  {
    jam();
    treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;
  }
  checkPrepareComplete(signal, requestPtr, 1);

  return;

error1:
error:
  ndbrequire(false);
}

void
Dbspj::execDIH_SCAN_GET_NODES_REF(Signal* signal)
{
  jamEntry();
  ndbrequire(false);
}

void
Dbspj::execDIH_SCAN_GET_NODES_CONF(Signal* signal)
{
  jamEntry();

  DihScanGetNodesConf * conf = (DihScanGetNodesConf*)signal->getDataPtr();

  Uint32 senderData = conf->senderData;
  Uint32 node = conf->nodes[0];
  Uint32 instanceKey = conf->instanceKey;

  Ptr<ScanFragHandle> fragPtr;
  m_scanfraghandle_pool.getPtr(fragPtr, senderData);
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, fragPtr.p->m_treeNodePtrI);
  ndbrequire(treeNodePtr.p->m_info == &g_ScanIndexOpInfo);
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  ndbrequire(data.m_frags_outstanding > 0);
  data.m_frags_outstanding--;

  fragPtr.p->m_ref = numberToRef(DBLQH, instanceKey, node);

  if (data.m_frags_outstanding == 0)
  {
    jam();

    treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;

    Ptr<Request> requestPtr;
    m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);
    checkPrepareComplete(signal, requestPtr, 1);
  }
}

Uint32
Dbspj::scanIndex_findFrag(Local_ScanFragHandle_list & list,
                          Ptr<ScanFragHandle> & fragPtr, Uint32 fragId)
{
  for (list.first(fragPtr); !fragPtr.isNull(); list.next(fragPtr))
  {
    jam();
    if (fragPtr.p->m_fragId == fragId)
    {
      jam();
      return 0;
    }
  }

  return 99; // TODO
}

void
Dbspj::scanIndex_parent_row(Signal* signal,
                            Ptr<Request> requestPtr,
                            Ptr<TreeNode> treeNodePtr,
                            const RowPtr & rowRef)
{
  jam();

  Uint32 err;
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;

  /**
   * Construct range definition,
   *   and if prune pattern enabled
   *   stuff it onto correct scanindexFrag
   */
  do
  {
    Ptr<ScanFragHandle> fragPtr;
    Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
    LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
    if (treeNodePtr.p->m_bits & TreeNode::T_PRUNE_PATTERN)
    {
      jam();

      /**
       * TODO: Expand into linear memory instead
       *       of expanding into sections, and then copy
       *       section into linear
       */
      Local_pattern_store pattern(pool, data.m_prunePattern);
      Uint32 pruneKeyPtrI = RNIL;
      bool hasNull;
      err = expand(pruneKeyPtrI, pattern, rowRef, hasNull);
      if (unlikely(err != 0))
      {
        DEBUG_CRASH();
        break;
      }

      if (unlikely(hasNull))
      {
        jam();
        DEBUG("T_PRUNE_PATTERN-key contain NULL values");

        // Ignore this request as 'NULL == <column>' will never give a match
        if (pruneKeyPtrI != RNIL)
        {
          releaseSection(pruneKeyPtrI);
        }
        return;  // Bailout, SCANREQ would have returned 0 rows anyway
      }

      // TODO we need a different variant of computeHash here,
      // since pruneKeyPtrI does not contain full primary key
      // but only parts in distribution key

      BuildKeyReq tmp;
      ScanFragReq * dst = (ScanFragReq*)data.m_scanFragReq;
      Uint32 indexId = dst->tableId;
      Uint32 tableId = g_key_descriptor_pool.getPtr(indexId)->primaryTableId;
      err = computePartitionHash(signal, tmp, tableId, pruneKeyPtrI);
      releaseSection(pruneKeyPtrI); // see ^ TODO
      if (unlikely(err != 0))
      {
        DEBUG_CRASH();
        break;
      }

      err = getNodes(signal, tmp, tableId);
      if (unlikely(err != 0))
      {
        DEBUG_CRASH();
        break;
      }

      err = scanIndex_findFrag(list, fragPtr, tmp.fragId);
      if (unlikely(err != 0))
      {
        DEBUG_CRASH();
        break;
      }

      /**
       * NOTE: We can get different receiverRef's here
       *       for different keys. E.g during node-recovery where
       *       primary-fragment is switched.
       *
       *       Use latest that we receive
       *
       * TODO: Also double check table-reorg
       */
      fragPtr.p->m_ref = tmp.receiverRef;
    }
    else
    {
      jam();
      /**
       * If const prune, or no-prune, store on first fragment,
       * and send to 1 or all resp.
       */
      list.first(fragPtr);
    }

    Uint32 ptrI = fragPtr.p->m_rangePtrI;
    bool hasNull;
    if (treeNodePtr.p->m_bits & TreeNode::T_KEYINFO_CONSTRUCTED)
    {
      jam();
      Local_pattern_store pattern(pool, treeNodePtr.p->m_keyPattern);
      err = expand(ptrI, pattern, rowRef, hasNull);
      if (unlikely(err != 0))
      {
        DEBUG_CRASH();
        break;
      }
    }
    else
    {
      jam();
      // Fixed key...fix later...
      ndbrequire(false);
    }
//  ndbrequire(!hasNull);  // FIXME, can't ignore request as we already added it to keyPattern
    fragPtr.p->m_rangePtrI = ptrI;
    scanIndex_fixupBound(fragPtr, ptrI, rowRef.m_src_correlation);

    if (treeNodePtr.p->m_bits & TreeNode::T_ONE_SHOT)
    {
      jam();
      /**
       * We being a T_ONE_SHOT means that we're only be called
       *   with parent_row once, i.e batch is complete
       */
      scanIndex_parent_batch_complete(signal, requestPtr, treeNodePtr);
    }

    return;
  } while (0);

  ndbrequire(false);
}


void
Dbspj::scanIndex_fixupBound(Ptr<ScanFragHandle> fragPtr,
                            Uint32 ptrI, Uint32 corrVal)
{
  /**
   * Index bounds...need special tender and care...
   *
   * 1) Set #bound no, bound-size, and renumber attributes
   */
  SectionReader r0(ptrI, getSectionSegmentPool());
  ndbrequire(r0.step(fragPtr.p->m_range_builder.m_range_size));
  Uint32 boundsz = r0.getSize() - fragPtr.p->m_range_builder.m_range_size;
  Uint32 boundno = fragPtr.p->m_range_builder.m_range_cnt + 1;

  Uint32 tmp;
  ndbrequire(r0.peekWord(&tmp));
  tmp |= (boundsz << 16) | ((corrVal & 0xFFF) << 4);
  ndbrequire(r0.updateWord(tmp));
  ndbrequire(r0.step(1));    // Skip first BoundType

  // TODO: Renumbering below assume there are only EQ-bounds !!
  Uint32 id = 0;
  Uint32 len32;
  do
  {
    ndbrequire(r0.peekWord(&tmp));
    AttributeHeader ah(tmp);
    Uint32 len = ah.getByteSize();
    AttributeHeader::init(&tmp, id++, len);
    ndbrequire(r0.updateWord(tmp));
    len32 = (len + 3) >> 2;
  } while (r0.step(2 + len32));  // Skip AttributeHeader(1) + Attribute(len32) + next BoundType(1)

  fragPtr.p->m_range_builder.m_range_cnt = boundno;
  fragPtr.p->m_range_builder.m_range_size = r0.getSize();
}

void
Dbspj::scanIndex_parent_batch_complete(Signal* signal,
                                       Ptr<Request> requestPtr,
                                       Ptr<TreeNode> treeNodePtr)
{
  jam();

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  data.m_rows_received = 0;
  data.m_rows_expecting = 0;
  ndbassert(data.m_frags_outstanding == 0);
  ndbassert(data.m_frags_complete == data.m_fragCount);
  data.m_frags_complete = 0;

  Ptr<ScanFragHandle> fragPtr;
  {
    Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
    list.first(fragPtr);

    if ((treeNodePtr.p->m_bits & TreeNode::T_PRUNE_PATTERN) == 0)
    {
      if (fragPtr.p->m_rangePtrI == RNIL)
      {
        // No keys found
        jam();
        data.m_frags_complete = data.m_fragCount;
      }
    }
    else
    {
      while(!fragPtr.isNull())
      {
        if (fragPtr.p->m_rangePtrI == RNIL)
        {
          jam();
          /**
           * This is a pruned scan, so we must scan those fragments that
           * some distribution key hashed to.
           */
          fragPtr.p->m_state = ScanFragHandle::SFH_COMPLETE;
          data.m_frags_complete++;
        }
        list.next(fragPtr);
      }
    }
  }
  data.m_frags_not_started = data.m_fragCount - data.m_frags_complete;

  if (data.m_frags_complete == data.m_fragCount)
  {
    jam();
    /**
     * No keys was produced...
     */
    return;
  }

  /**
   * When parent's batch is complete, we send our batch
   */
  const ScanFragReq * org = (const ScanFragReq*)data.m_scanFragReq;
  ndbrequire(org->batch_size_rows > 0);

  if (treeNodePtr.p->m_bits & TreeNode::T_SCAN_PARALLEL)
  {
    jam();
    data.m_parallelism = MIN(data.m_fragCount - data.m_frags_complete, 
                             org->batch_size_rows);
  }
  else if (data.m_firstExecution)
  {
    /**
     * Having a high parallelism would allow us to fetch data from many
     * fragments in parallel and thus reduce the number of round trips.
     * On the other hand, we should set parallelism so low that we can fetch
     * all data from a fragment in one batch if possible.
     * Since this is the first execution, we do not know how many rows or bytes
     * this operation is likely to return. Therefore we set parallelism to 1,
     * since this gives the lowest penalty if our guess is wrong.
     */
    jam();
    data.m_parallelism = 1;
  }
  else
  {
    jam();
    /**
     * Use statistics from earlier runs of this operation to estimate the
     * initial parallelism. We use the mean minus two times the standard
     * deviation to have a low risk of setting parallelism to high (as erring
     * in the other direction is more costly).
     */
    Int32 parallelism = 
      static_cast<Int32>(MIN(data.m_parallelismStat.getMean()
                             - 2 * data.m_parallelismStat.getStdDev(),
                             org->batch_size_rows));

    if (parallelism < 1)
    {
      jam();
      parallelism = 1;
    }
    else if ((data.m_fragCount - data.m_frags_complete) % parallelism != 0)
    {
      jam();
      /**
       * Set parallelism such that we can expect to have similar
       * parallelism in each batch. For example if there are 8 remaining
       * fragments, then we should fecth 2 times 4 fragments rather than
       * 7+1.
       */
      const Int32 roundTrips =
        1 + (data.m_fragCount - data.m_frags_complete) / parallelism;
      parallelism = (data.m_fragCount - data.m_frags_complete) / roundTrips;
    }

    data.m_parallelism = static_cast<Uint32>(parallelism);

#ifdef DEBUG_SCAN_FRAGREQ
    DEBUG("::scanIndex_send() starting index scan with parallelism="
          << data.m_parallelism);
#endif
  }
  ndbrequire(data.m_parallelism > 0);

  const Uint32 bs_rows = org->batch_size_rows/ data.m_parallelism;
  const Uint32 bs_bytes = org->batch_size_bytes / data.m_parallelism;
  ndbassert(bs_rows > 0);
  ndbassert(bs_bytes > 0);

  data.m_largestBatchRows = 0;
  data.m_largestBatchBytes = 0;
  data.m_totalRows = 0;
  data.m_totalBytes = 0;

  {
    Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
    Ptr<ScanFragHandle> fragPtr;
    list.first(fragPtr);

    while(!fragPtr.isNull())
    {
      ndbassert(fragPtr.p->m_state == ScanFragHandle::SFH_NOT_STARTED ||
                fragPtr.p->m_state == ScanFragHandle::SFH_COMPLETE);
      fragPtr.p->m_state = ScanFragHandle::SFH_NOT_STARTED;
      list.next(fragPtr);
    }
  }

  Uint32 batchRange = 0;
  scanIndex_send(signal,
                 requestPtr,
                 treeNodePtr,
                 data.m_parallelism,
                 bs_bytes,
                 bs_rows,
                 batchRange);

  data.m_firstExecution = false;

  ndbrequire(static_cast<Uint32>(data.m_frags_outstanding + 
                                 data.m_frags_complete) <=
             data.m_fragCount);

  data.m_batch_chunks = 1;
  requestPtr.p->m_cnt_active++;
  requestPtr.p->m_outstanding++;
  treeNodePtr.p->m_state = TreeNode::TN_ACTIVE;
}

void
Dbspj::scanIndex_parent_batch_repeat(Signal* signal,
                                      Ptr<Request> requestPtr,
                                      Ptr<TreeNode> treeNodePtr)
{
  jam();
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;

  DEBUG("scanIndex_parent_batch_repeat(), m_node_no: " << treeNodePtr.p->m_node_no
        << ", m_batch_chunks: " << data.m_batch_chunks);
  
  ndbassert(treeNodePtr.p->m_bits & TreeNode::T_SCAN_REPEATABLE);

  /**
   * Register index-scans to be restarted if we didn't get all
   * previously fetched parent related child rows in a single batch.
   */
  if (data.m_batch_chunks > 1)
  {
    jam();
    DEBUG("Register TreeNode for restart, m_node_no: " << treeNodePtr.p->m_node_no);
    ndbrequire(treeNodePtr.p->m_state != TreeNode::TN_ACTIVE);
    registerActiveCursor(requestPtr, treeNodePtr);
    data.m_batch_chunks = 0;
  }
}

/**
 * Ask for the first batch for a number of fragments.
 */
void
Dbspj::scanIndex_send(Signal* signal,
                      Ptr<Request> requestPtr,
                      Ptr<TreeNode> treeNodePtr,
                      Uint32 noOfFrags,
                      Uint32 bs_bytes,
                      Uint32 bs_rows,
                      Uint32& batchRange)
{
  /**
   * if (m_bits & prunemask):
   * - Range keys sliced out to each ScanFragHandle
   * - Else, range keys kept on first (and only) ScanFragHandle
   */
  const bool prune = treeNodePtr.p->m_bits &
    (TreeNode::T_PRUNE_PATTERN | TreeNode::T_CONST_PRUNE);

  /**
   * If scan is repeatable, we must make sure not to release range keys so
   * that we canuse them again in the next repetition.
   */
  const bool repeatable =
    (treeNodePtr.p->m_bits & TreeNode::T_SCAN_REPEATABLE) != 0;

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  ndbassert(noOfFrags > 0);
  ndbassert(data.m_frags_not_started >= noOfFrags);
  ScanFragReq* const req =
    reinterpret_cast<ScanFragReq*>(signal->getDataPtrSend());
  const ScanFragReq * const org
    = reinterpret_cast<ScanFragReq*>(data.m_scanFragReq);
  memcpy(req, org, sizeof(data.m_scanFragReq));
  // req->variableData[0] // set below
  req->variableData[1] = requestPtr.p->m_rootResultData;
  req->batch_size_bytes = bs_bytes;
  req->batch_size_rows = bs_rows;

  Uint32 requestsSent = 0;
  Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
  Ptr<ScanFragHandle> fragPtr;
  list.first(fragPtr);
  Uint32 keyInfoPtrI = fragPtr.p->m_rangePtrI;
  ndbrequire(prune || keyInfoPtrI != RNIL);
  /**
   * Iterate over the list of fragments until we have sent as many
   * SCAN_FRAGREQs as we should.
   */
  while (requestsSent < noOfFrags)
  {
    jam();
    ndbassert(!fragPtr.isNull());

    if (fragPtr.p->m_state != ScanFragHandle::SFH_NOT_STARTED)
    {
      // Skip forward to the frags that we should send.
      jam();
      list.next(fragPtr);
      continue;
    }

    const Uint32 ref = fragPtr.p->m_ref;

    if (noOfFrags==1 && !prune &&
        data.m_frags_not_started == data.m_fragCount &&
        refToNode(ref) != getOwnNodeId() &&
        list.hasNext(fragPtr))
    {
      /**
       * If we are doing a scan with adaptive parallelism and start with
       * parallelism=1 then it makes sense to fetch a batch from a fragment on
       * the local data node. The reason for this is that if that fragment
       * contains few rows, we may be able to read from several fragments in
       * parallel. Then we minimize the total number of round trips (to remote
       * data nodes) if we fetch the first fragment batch locally.
       */
      jam();
      list.next(fragPtr);
      continue;
    }

    SectionHandle handle(this);

    Uint32 attrInfoPtrI = treeNodePtr.p->m_send.m_attrInfoPtrI;

    /**
     * Set data specific for this fragment
     */
    req->senderData = fragPtr.i;
    req->fragmentNoKeyLen = fragPtr.p->m_fragId;

    if (prune)
    {
      jam();
      keyInfoPtrI = fragPtr.p->m_rangePtrI;
      if (keyInfoPtrI == RNIL)
      {
        /**
         * Since we use pruning, we can see that no parent rows would hash
         * to this fragment.
         */
        jam();
        fragPtr.p->m_state = ScanFragHandle::SFH_COMPLETE;
        list.next(fragPtr);
        continue;
      }

      if (!repeatable)
      {
        /**
         * If we'll use sendSignal() and we need to send the attrInfo several
         * times, we need to copy them. (For repeatable or unpruned scans
         * we use sendSignalNoRelease(), so then we do not need to copy.)
         */
        jam();
        Uint32 tmp = RNIL;
        ndbrequire(dupSection(tmp, attrInfoPtrI)); // TODO handle error
        attrInfoPtrI = tmp;
      }
    }

    req->variableData[0] = batchRange;
    getSection(handle.m_ptr[0], attrInfoPtrI);
    getSection(handle.m_ptr[1], keyInfoPtrI);
    handle.m_cnt = 2;

#if defined DEBUG_SCAN_FRAGREQ
    ndbout_c("SCAN_FRAGREQ to %x", ref);
    printSCAN_FRAGREQ(stdout, signal->getDataPtrSend(),
                      NDB_ARRAY_SIZE(treeNodePtr.p->m_scanfrag_data.m_scanFragReq),
                      DBLQH);
    printf("ATTRINFO: ");
    print(handle.m_ptr[0], stdout);
    printf("KEYINFO: ");
    print(handle.m_ptr[1], stdout);
#endif

    if (refToNode(ref) == getOwnNodeId())
    {
      c_Counters.incr_counter(CI_LOCAL_RANGE_SCANS_SENT, 1);
    }
    else
    {
      c_Counters.incr_counter(CI_REMOTE_RANGE_SCANS_SENT, 1);
    }

    if (prune && !repeatable)
    {
      /**
       * For a non-repeatable pruned scan, key info is unique for each
       * fragment and therefore cannot be reused, so we release key info
       * right away.
       */
      jam();
      sendSignal(ref, GSN_SCAN_FRAGREQ, signal,
                 NDB_ARRAY_SIZE(data.m_scanFragReq), JBB, &handle);
      fragPtr.p->m_rangePtrI = RNIL;
      fragPtr.p->reset_ranges();
    }
    else
    {
      /**
       * Reuse key info for multiple fragments and/or multiple repetitions
       * of the scan.
       */
      jam();
      sendSignalNoRelease(ref, GSN_SCAN_FRAGREQ, signal,
                          NDB_ARRAY_SIZE(data.m_scanFragReq), JBB, &handle);
    }
    handle.clear();

    fragPtr.p->m_state = ScanFragHandle::SFH_SCANNING; // running
    data.m_frags_outstanding++;
    batchRange += bs_rows;
    requestsSent++;
    list.next(fragPtr);
  } // while (requestsSent < noOfFrags)

  data.m_frags_not_started -= requestsSent;
}

void
Dbspj::scanIndex_execTRANSID_AI(Signal* signal,
                                Ptr<Request> requestPtr,
                                Ptr<TreeNode> treeNodePtr,
                                const RowPtr & rowRef)
{
  jam();

  LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
  Local_dependency_map list(pool, treeNodePtr.p->m_dependent_nodes);
  Dependency_map::ConstDataBufferIterator it;

  {
    for (list.first(it); !it.isNull(); list.next(it))
    {
      jam();
      Ptr<TreeNode> childPtr;
      m_treenode_pool.getPtr(childPtr, * it.data);
      ndbrequire(childPtr.p->m_info != 0&&childPtr.p->m_info->m_parent_row!=0);
      (this->*(childPtr.p->m_info->m_parent_row))(signal,
                                                  requestPtr, childPtr,rowRef);
    }
  }

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  data.m_rows_received++;

  if (data.m_frags_outstanding == 0 &&
      data.m_rows_received == data.m_rows_expecting)
  {
    jam();
    /**
     * Finished...
     */
    if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE)
    {
      jam();
      reportBatchComplete(signal, requestPtr, treeNodePtr);
    }

    checkBatchComplete(signal, requestPtr, 1);
    return;
  }
}

void
Dbspj::scanIndex_execSCAN_FRAGCONF(Signal* signal,
                                   Ptr<Request> requestPtr,
                                   Ptr<TreeNode> treeNodePtr,
                                   Ptr<ScanFragHandle> fragPtr)
{
  jam();

  const ScanFragConf * conf = (const ScanFragConf*)(signal->getDataPtr());

  Uint32 rows = conf->completedOps;
  Uint32 done = conf->fragmentCompleted;

  Uint32 state = fragPtr.p->m_state;
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;

  if (state == ScanFragHandle::SFH_WAIT_CLOSE && done == 0)
  {
    jam();
    /**
     * We sent an explicit close request...ignore this...a close will come later
     */
    return;
  }

  requestPtr.p->m_rows += rows;
  data.m_totalRows += rows;
  data.m_totalBytes += conf->total_len;
  data.m_largestBatchRows = MAX(data.m_largestBatchRows, rows);
  data.m_largestBatchBytes = MAX(data.m_largestBatchBytes, conf->total_len);

  if (!treeNodePtr.p->isLeaf())
  {
    jam();
    data.m_rows_expecting += rows;
  }
  ndbrequire(data.m_frags_outstanding);
  ndbrequire(state == ScanFragHandle::SFH_SCANNING ||
             state == ScanFragHandle::SFH_WAIT_CLOSE);

  data.m_frags_outstanding--;
  fragPtr.p->m_state = ScanFragHandle::SFH_WAIT_NEXTREQ;

  if (done)
  {
    jam();
    fragPtr.p->m_state = ScanFragHandle::SFH_COMPLETE;
    ndbrequire(data.m_frags_complete < data.m_fragCount);
    data.m_frags_complete++;

    if (data.m_frags_complete == data.m_fragCount ||
        ((requestPtr.p->m_state & Request::RS_ABORTING) != 0 &&
         data.m_fragCount == (data.m_frags_complete + data.m_frags_not_started)))
    {
      jam();
      ndbrequire(requestPtr.p->m_cnt_active);
      requestPtr.p->m_cnt_active--;
      treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;
    }
  }


  if (data.m_frags_outstanding == 0)
  {
    const ScanFragReq * const org
      = reinterpret_cast<const ScanFragReq*>(data.m_scanFragReq);

    if (data.m_frags_complete == data.m_fragCount)
    {
      jam();
      /**
       * Calculate what would have been the optimal parallelism for the
       * scan instance that we have just completed, and update
       * 'parallelismStat' with this value. We then use this statistics to set
       * the initial parallelism for the next instance of this operation.
       */
      double parallelism = data.m_fragCount;
      if (data.m_totalRows > 0)
      {
        parallelism = MIN(parallelism,
                          double(org->batch_size_rows) / data.m_totalRows);
      }
      if (data.m_totalBytes > 0)
      {
        parallelism = MIN(parallelism,
                          double(org->batch_size_bytes) / data.m_totalBytes);
      }
      data.m_parallelismStat.update(parallelism);
    }

    /**
     * Don't reportBatchComplete to children if we're aborting...
     */
    if (state == ScanFragHandle::SFH_WAIT_CLOSE)
    {
      jam();
      ndbrequire((requestPtr.p->m_state & Request::RS_ABORTING) != 0);
    }
    else if (! (data.m_rows_received == data.m_rows_expecting))
    {
      jam();
      return;
    }
    else
    {
      if (treeNodePtr.p->m_bits & TreeNode::T_REPORT_BATCH_COMPLETE)
      {
        jam();
        reportBatchComplete(signal, requestPtr, treeNodePtr);
      }
    }

    checkBatchComplete(signal, requestPtr, 1);
    return;
  }
}

void
Dbspj::scanIndex_execSCAN_FRAGREF(Signal* signal,
                                  Ptr<Request> requestPtr,
                                  Ptr<TreeNode> treeNodePtr,
                                  Ptr<ScanFragHandle> fragPtr)
{
  jam();

  const ScanFragRef * rep = CAST_CONSTPTR(ScanFragRef, signal->getDataPtr());
  const Uint32 errCode = rep->errorCode;

  Uint32 state = fragPtr.p->m_state;
  ndbrequire(state == ScanFragHandle::SFH_SCANNING ||
             state == ScanFragHandle::SFH_WAIT_CLOSE);

  fragPtr.p->m_state = ScanFragHandle::SFH_COMPLETE;

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  ndbrequire(data.m_frags_complete < data.m_fragCount);
  data.m_frags_complete++;
  ndbrequire(data.m_frags_outstanding > 0);
  data.m_frags_outstanding--;

  if (data.m_fragCount == (data.m_frags_complete + data.m_frags_not_started))
  {
    jam();
    ndbrequire(requestPtr.p->m_cnt_active);
    requestPtr.p->m_cnt_active--;
    treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;
  }

  if (data.m_frags_outstanding == 0)
  {
    jam();
    ndbrequire(requestPtr.p->m_outstanding);
    requestPtr.p->m_outstanding--;
  }

  abort(signal, requestPtr, errCode);
}

void
Dbspj::scanIndex_execSCAN_NEXTREQ(Signal* signal,
                                  Ptr<Request> requestPtr,
                                  Ptr<TreeNode> treeNodePtr)
{
  jam();

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  const ScanFragReq * org = (const ScanFragReq*)data.m_scanFragReq;

  data.m_rows_received = 0;
  data.m_rows_expecting = 0;
  ndbassert(data.m_frags_outstanding == 0);

  ndbrequire(data.m_frags_complete < data.m_fragCount);
  if ((treeNodePtr.p->m_bits & TreeNode::T_SCAN_PARALLEL) == 0)
  {
    jam();
    /**
     * Since fetching few but large batches is more efficient, we
     * set parallelism to the lowest value where we can still expect each
     * batch to be full.
     */
    if (data.m_largestBatchRows < org->batch_size_rows/data.m_parallelism &&
        data.m_largestBatchBytes < org->batch_size_bytes/data.m_parallelism)
    {
      jam();
      data.m_parallelism = MIN(data.m_fragCount - data.m_frags_complete,
                               org->batch_size_rows);
      if (data.m_largestBatchRows > 0)
      {
        jam();
        data.m_parallelism =
          MIN(org->batch_size_rows / data.m_largestBatchRows,
              data.m_parallelism);
      }
      if (data.m_largestBatchBytes > 0)
      {
        jam();
        data.m_parallelism =
          MIN(data.m_parallelism,
              org->batch_size_bytes/data.m_largestBatchBytes);
      }
      if (data.m_frags_complete == 0 &&
          data.m_frags_not_started % data.m_parallelism != 0)
      {
        jam();
        /**
         * Set parallelism such that we can expect to have similar
         * parallelism in each batch. For example if there are 8 remaining
         * fragments, then we should fecth 2 times 4 fragments rather than
         * 7+1.
         */
        const Uint32 roundTrips =
          1 + data.m_frags_not_started / data.m_parallelism;
        data.m_parallelism = data.m_frags_not_started / roundTrips;
      }
    }
    else
    {
      jam();
      // We get full batches, so we should lower parallelism.
      data.m_parallelism = MIN(data.m_fragCount - data.m_frags_complete,
                               MAX(1, data.m_parallelism/2));
    }
    ndbassert(data.m_parallelism > 0);
#ifdef DEBUG_SCAN_FRAGREQ
    DEBUG("::scanIndex_execSCAN_NEXTREQ() Asking for new batches from " <<
          data.m_parallelism <<
          " fragments with " << org->batch_size_rows/data.m_parallelism <<
          " rows and " << org->batch_size_bytes/data.m_parallelism <<
          " bytes.");
#endif
  }
  else
  {
    jam();
    data.m_parallelism = MIN(data.m_fragCount - data.m_frags_complete,
                             org->batch_size_rows);
  }

  const Uint32 bs_rows = org->batch_size_rows/data.m_parallelism;
  ndbassert(bs_rows > 0);
  ScanFragNextReq* req =
    reinterpret_cast<ScanFragNextReq*>(signal->getDataPtrSend());
  req->requestInfo = 0;
  ScanFragNextReq::setCorrFactorFlag(req->requestInfo);
  req->transId1 = requestPtr.p->m_transId[0];
  req->transId2 = requestPtr.p->m_transId[1];
  req->batch_size_rows = bs_rows;
  req->batch_size_bytes = org->batch_size_bytes/data.m_parallelism;

  Uint32 batchRange = 0;
  Ptr<ScanFragHandle> fragPtr;
  Uint32 sentFragCount = 0;
  {
    /**
     * First, ask for more data from fragments that are already started.
     */
  Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
  list.first(fragPtr);
    while (sentFragCount < data.m_parallelism && !fragPtr.isNull())
  {
    jam();
      ndbassert(fragPtr.p->m_state == ScanFragHandle::SFH_WAIT_NEXTREQ ||
                fragPtr.p->m_state == ScanFragHandle::SFH_COMPLETE ||
                fragPtr.p->m_state == ScanFragHandle::SFH_NOT_STARTED);
    if (fragPtr.p->m_state == ScanFragHandle::SFH_WAIT_NEXTREQ)
    {
      jam();

      data.m_frags_outstanding++;
      req->variableData[0] = batchRange;
      fragPtr.p->m_state = ScanFragHandle::SFH_SCANNING;
      batchRange += bs_rows;

      DEBUG("scanIndex_execSCAN_NEXTREQ to: " << hex
            << treeNodePtr.p->m_send.m_ref
              << ", m_node_no=" << treeNodePtr.p->m_node_no
            << ", senderData: " << req->senderData);

#ifdef DEBUG_SCAN_FRAGREQ
      printSCANFRAGNEXTREQ(stdout, &signal->theData[0],
                           ScanFragNextReq:: SignalLength + 1, DBLQH);
#endif

      req->senderData = fragPtr.i;
      sendSignal(fragPtr.p->m_ref, GSN_SCAN_NEXTREQ, signal,
                 ScanFragNextReq::SignalLength + 1,
                 JBB);
        sentFragCount++;
      }
      list.next(fragPtr);
    }
  }

  if (sentFragCount < data.m_parallelism)
  {
    /**
     * Then start new fragments until we reach data.m_parallelism.
     */
    jam();
    ndbassert(data.m_frags_not_started != 0);
    scanIndex_send(signal,
                   requestPtr,
                   treeNodePtr,
                   data.m_parallelism - sentFragCount,
                   org->batch_size_bytes/data.m_parallelism,
                   bs_rows,
                   batchRange);
  }
  /**
   * cursor should not have been positioned here...
   *   unless we actually had something more to send.
   *   so require that we did actually send something
   */
  ndbrequire(data.m_frags_outstanding > 0);
  ndbrequire(data.m_batch_chunks > 0);
  data.m_batch_chunks++;

  requestPtr.p->m_outstanding++;
  ndbassert(treeNodePtr.p->m_state == TreeNode::TN_ACTIVE);
}

void
Dbspj::scanIndex_complete(Signal* signal,
                          Ptr<Request> requestPtr,
                          Ptr<TreeNode> treeNodePtr)
{
  jam();
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  ScanFragReq*dst=(ScanFragReq*)treeNodePtr.p->m_scanindex_data.m_scanFragReq;
  if (!data.m_fragments.isEmpty())
  {
    jam();
    DihScanTabCompleteRep* rep=(DihScanTabCompleteRep*)signal->getDataPtrSend();
    rep->tableId = dst->tableId;
    rep->scanCookie = data.m_scanCookie;
    sendSignal(DBDIH_REF, GSN_DIH_SCAN_TAB_COMPLETE_REP,
               signal, DihScanTabCompleteRep::SignalLength, JBB);
  }
}

void
Dbspj::scanIndex_abort(Signal* signal,
                       Ptr<Request> requestPtr,
                       Ptr<TreeNode> treeNodePtr)
{
  jam();

  switch(treeNodePtr.p->m_state){
  case TreeNode::TN_BUILDING:
  case TreeNode::TN_PREPARING:
  case TreeNode::TN_INACTIVE:
  case TreeNode::TN_COMPLETING:
  case TreeNode::TN_END:
    ndbout_c("H'%.8x H'%.8x scanIndex_abort state: %u",
             requestPtr.p->m_transId[0],
             requestPtr.p->m_transId[1],
             treeNodePtr.p->m_state);
    return;

  case TreeNode::TN_ACTIVE:
    jam();
    break;
  }

  ScanFragNextReq* req = CAST_PTR(ScanFragNextReq, signal->getDataPtrSend());
  req->requestInfo = ScanFragNextReq::ZCLOSE;
  req->transId1 = requestPtr.p->m_transId[0];
  req->transId2 = requestPtr.p->m_transId[1];
  req->batch_size_rows = 0;
  req->batch_size_bytes = 0;

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
  Ptr<ScanFragHandle> fragPtr;

  Uint32 cnt_waiting = 0;
  Uint32 cnt_scanning = 0;
  for (list.first(fragPtr); !fragPtr.isNull(); list.next(fragPtr))
  {
    switch(fragPtr.p->m_state){
    case ScanFragHandle::SFH_NOT_STARTED:
    case ScanFragHandle::SFH_COMPLETE:
    case ScanFragHandle::SFH_WAIT_CLOSE:
      jam();
      break;
    case ScanFragHandle::SFH_WAIT_NEXTREQ:
      jam();
      cnt_waiting++;              // was idle...
      data.m_frags_outstanding++; // is closing
      goto do_abort;
    case ScanFragHandle::SFH_SCANNING:
      jam();
      cnt_scanning++;
      goto do_abort;
    do_abort:
      req->senderData = fragPtr.i;
      sendSignal(fragPtr.p->m_ref, GSN_SCAN_NEXTREQ, signal,
                 ScanFragNextReq::SignalLength, JBB);

      fragPtr.p->m_state = ScanFragHandle::SFH_WAIT_CLOSE;
      break;
    }
  }

  if (cnt_scanning == 0)
  {
    if (cnt_waiting > 0)
    {
      /**
       * If all were waiting...this should increase m_outstanding
       */
      jam();
      requestPtr.p->m_outstanding++;
    }
    else
    {
      /**
       * All fragments are either complete or not yet started, so there is
       * nothing to abort.
       */
      jam();
      ndbassert(data.m_frags_not_started > 0);
      ndbrequire(requestPtr.p->m_cnt_active);
      requestPtr.p->m_cnt_active--;
      treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;
    }
  }
}

Uint32
Dbspj::scanIndex_execNODE_FAILREP(Signal* signal,
                                  Ptr<Request> requestPtr,
                                  Ptr<TreeNode> treeNodePtr,
                                  NdbNodeBitmask nodes)
{
  jam();

  switch(treeNodePtr.p->m_state){
  case TreeNode::TN_PREPARING:
  case TreeNode::TN_INACTIVE:
    return 1;

  case TreeNode::TN_BUILDING:
  case TreeNode::TN_COMPLETING:
  case TreeNode::TN_END:
    return 0;

  case TreeNode::TN_ACTIVE:
    jam();
    break;
  }


  Uint32 sum = 0;
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
  Ptr<ScanFragHandle> fragPtr;

  Uint32 save0 = data.m_frags_outstanding;
  Uint32 save1 = data.m_frags_complete;

  for (list.first(fragPtr); !fragPtr.isNull(); list.next(fragPtr))
  {
    if (nodes.get(refToNode(fragPtr.p->m_ref)) == false)
    {
      jam();
      /**
       * No action needed
       */
      continue;
    }

    switch(fragPtr.p->m_state){
    case ScanFragHandle::SFH_NOT_STARTED:
      jam();
      ndbrequire(data.m_frags_complete < data.m_fragCount);
      data.m_frags_complete++;
      ndbrequire(data.m_frags_not_started > 0);
      data.m_frags_not_started--;
      // fall through
    case ScanFragHandle::SFH_COMPLETE:
      jam();
      sum++; // indicate that we should abort
      /**
       * we could keep list of all fragments...
       *   or execute DIGETNODES again...
       *   but for now, we don't
       */
      break;
    case ScanFragHandle::SFH_WAIT_CLOSE:
    case ScanFragHandle::SFH_SCANNING:
      jam();
      ndbrequire(data.m_frags_outstanding > 0);
      data.m_frags_outstanding--;
      // fall through
    case ScanFragHandle::SFH_WAIT_NEXTREQ:
      jam();
      sum++;
      ndbrequire(data.m_frags_complete < data.m_fragCount);
      data.m_frags_complete++;
      break;
    }
    fragPtr.p->m_ref = 0;
    fragPtr.p->m_state = ScanFragHandle::SFH_COMPLETE;
  }

  if (save0 != 0 && data.m_frags_outstanding == 0)
  {
    jam();
    ndbrequire(requestPtr.p->m_outstanding);
    requestPtr.p->m_outstanding--;
  }

  if (save1 != 0 &&
      data.m_fragCount == (data.m_frags_complete + data.m_frags_not_started))
  {
    jam();
    ndbrequire(requestPtr.p->m_cnt_active);
    requestPtr.p->m_cnt_active--;
    treeNodePtr.p->m_state = TreeNode::TN_INACTIVE;
  }

  return sum;
}

void
Dbspj::scanIndex_release_rangekeys(Ptr<Request> requestPtr,
                                   Ptr<TreeNode> treeNodePtr)
{
  jam();
  DEBUG("scanIndex_release_rangekeys(), tree node " << treeNodePtr.i
          << " m_node_no: " << treeNodePtr.p->m_node_no);

  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
  Ptr<ScanFragHandle> fragPtr;

  if (treeNodePtr.p->m_bits & TreeNode::T_PRUNE_PATTERN)
  {
    jam();
    for (list.first(fragPtr); !fragPtr.isNull(); list.next(fragPtr))
    {
      if (fragPtr.p->m_rangePtrI != RNIL)
      {
        releaseSection(fragPtr.p->m_rangePtrI);
        fragPtr.p->m_rangePtrI = RNIL;
      }
      fragPtr.p->reset_ranges();
    }
  }
  else
  {
    jam();
    list.first(fragPtr);
    if (fragPtr.p->m_rangePtrI != RNIL)
    {
      releaseSection(fragPtr.p->m_rangePtrI);
      fragPtr.p->m_rangePtrI = RNIL;
    }
    fragPtr.p->reset_ranges();
  }
}

/**
 * Parent batch has completed, and will not refetch (X-joined) results
 * from its childs. Release & reset range keys which are unsent or we
 * have kept for possible resubmits.
 */
void
Dbspj::scanIndex_parent_batch_cleanup(Ptr<Request> requestPtr,
                                      Ptr<TreeNode> treeNodePtr)
{
  DEBUG("scanIndex_parent_batch_cleanup");
  scanIndex_release_rangekeys(requestPtr,treeNodePtr);
}

void
Dbspj::scanIndex_cleanup(Ptr<Request> requestPtr,
                         Ptr<TreeNode> treeNodePtr)
{
  ScanIndexData& data = treeNodePtr.p->m_scanindex_data;
  DEBUG("scanIndex_cleanup");

  /**
   * Range keys has been collected wherever there are uncompleted
   * parent batches...release them to avoid memleak.
   */
  scanIndex_release_rangekeys(requestPtr,treeNodePtr);

  {
    Local_ScanFragHandle_list list(m_scanfraghandle_pool, data.m_fragments);
    list.remove();
  }
  if (treeNodePtr.p->m_bits & TreeNode::T_PRUNE_PATTERN)
  {
    jam();
    LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
    Local_pattern_store pattern(pool, data.m_prunePattern);
    pattern.release();
  }
  else if (treeNodePtr.p->m_bits & TreeNode::T_CONST_PRUNE)
  {
    jam();
    if (data.m_constPrunePtrI != RNIL)
    {
      jam();
      releaseSection(data.m_constPrunePtrI);
      data.m_constPrunePtrI = RNIL;
    }
  }

  cleanup_common(requestPtr, treeNodePtr);
}

/**
 * END - MODULE SCAN INDEX
 */

/**
 * Static OpInfo handling
 */
const Dbspj::OpInfo*
Dbspj::getOpInfo(Uint32 op)
{
  DEBUG("getOpInfo(" << op << ")");
  switch(op){
  case QueryNode::QN_LOOKUP:
    return &Dbspj::g_LookupOpInfo;
  case QueryNode::QN_SCAN_FRAG:
    return &Dbspj::g_ScanFragOpInfo;
  case QueryNode::QN_SCAN_INDEX:
    return &Dbspj::g_ScanIndexOpInfo;
  default:
    return 0;
  }
}

/**
 * MODULE COMMON PARSE/UNPACK
 */

/**
 *  @returns dstLen + 1 on error
 */
static
Uint32
unpackList(Uint32 dstLen, Uint32 * dst, Dbspj::DABuffer & buffer)
{
  const Uint32 * ptr = buffer.ptr;
  if (likely(ptr != buffer.end))
  {
    Uint32 tmp = * ptr++;
    Uint32 cnt = tmp & 0xFFFF;

    * dst ++ = (tmp >> 16); // Store first
    DEBUG("cnt: " << cnt << " first: " << (tmp >> 16));

    if (cnt > 1)
    {
      Uint32 len = cnt / 2;
      if (unlikely(cnt >= dstLen || (ptr + len > buffer.end)))
        goto error;

      cnt --; // subtract item stored in header

      for (Uint32 i = 0; i < cnt/2; i++)
      {
        * dst++ = (* ptr) & 0xFFFF;
        * dst++ = (* ptr) >> 16;
        ptr++;
      }

      if (cnt & 1)
      {
        * dst ++ = * ptr & 0xFFFF;
        ptr++;
      }

      cnt ++; // readd item stored in header
    }
    buffer.ptr = ptr;
    return cnt;
  }
  return 0;

error:
  return dstLen + 1;
}

/**
 * This fuctions takes an array of attrinfo, and builds "header"
 *   which can be used to do random access inside the row
 */
Uint32
Dbspj::buildRowHeader(RowPtr::Header * header, SegmentedSectionPtr ptr)
{
  Uint32 tmp, len;
  Uint32 * dst = header->m_offset;
  const Uint32 * const save = dst;
  SectionReader r0(ptr, getSectionSegmentPool());
  Uint32 offset = 0;
  do
  {
    * dst++ = offset;
    r0.getWord(&tmp);
    len = AttributeHeader::getDataSize(tmp);
    offset += 1 + len;
  } while (r0.step(len));

  return header->m_len = static_cast<Uint32>(dst - save);
}

/**
 * This fuctions takes an array of attrinfo, and builds "header"
 *   which can be used to do random access inside the row
 */
Uint32
Dbspj::buildRowHeader(RowPtr::Header * header, const Uint32 *& src, Uint32 len)
{
  Uint32 * dst = header->m_offset;
  const Uint32 * save = dst;
  Uint32 offset = 0;
  for (Uint32 i = 0; i<len; i++)
  {
    * dst ++ = offset;
    Uint32 tmp = * src++;
    Uint32 tmp_len = AttributeHeader::getDataSize(tmp);
    offset += 1 + tmp_len;
    src += tmp_len;
  }

  return header->m_len = static_cast<Uint32>(dst - save);
}

Uint32
Dbspj::appendToPattern(Local_pattern_store & pattern,
                       DABuffer & tree, Uint32 len)
{
  if (unlikely(tree.ptr + len > tree.end))
    return DbspjErr::InvalidTreeNodeSpecification;

  if (unlikely(pattern.append(tree.ptr, len)==0))
    return  DbspjErr::OutOfQueryMemory;

  tree.ptr += len;
  return 0;
}

Uint32
Dbspj::appendParamToPattern(Local_pattern_store& dst,
                            const RowPtr::Linear & row, Uint32 col)
{
  /**
   * TODO handle errors
   */
  Uint32 offset = row.m_header->m_offset[col];
  const Uint32 * ptr = row.m_data + offset;
  Uint32 len = AttributeHeader::getDataSize(* ptr ++);
  /* Param COL's converted to DATA when appended to pattern */
  Uint32 info = QueryPattern::data(len);
  return dst.append(&info,1) && dst.append(ptr,len) ? 0 : DbspjErr::OutOfQueryMemory;
}

Uint32
Dbspj::appendParamHeadToPattern(Local_pattern_store& dst,
                                const RowPtr::Linear & row, Uint32 col)
{
  /**
   * TODO handle errors
   */
  Uint32 offset = row.m_header->m_offset[col];
  const Uint32 * ptr = row.m_data + offset;
  Uint32 len = AttributeHeader::getDataSize(*ptr);
  /* Param COL's converted to DATA when appended to pattern */
  Uint32 info = QueryPattern::data(len+1);
  return dst.append(&info,1) && dst.append(ptr,len+1) ? 0 : DbspjErr::OutOfQueryMemory;
}

Uint32
Dbspj::appendTreeToSection(Uint32 & ptrI, SectionReader & tree, Uint32 len)
{
  /**
   * TODO handle errors
   */
  Uint32 SZ = 16;
  Uint32 tmp[16];
  while (len > SZ)
  {
    jam();
    tree.getWords(tmp, SZ);
    ndbrequire(appendToSection(ptrI, tmp, SZ));
    len -= SZ;
  }

  tree.getWords(tmp, len);
  return appendToSection(ptrI, tmp, len) ? 0 : /** todo error code */ 1;
#if TODO
err:
  return 1;
#endif
}

void
Dbspj::getCorrelationData(const RowPtr::Section & row,
                          Uint32 col,
                          Uint32& correlationNumber)
{
  /**
   * TODO handle errors
   */
  SegmentedSectionPtr ptr(row.m_dataPtr);
  SectionReader reader(ptr, getSectionSegmentPool());
  Uint32 offset = row.m_header->m_offset[col];
  ndbrequire(reader.step(offset));
  Uint32 tmp;
  ndbrequire(reader.getWord(&tmp));
  Uint32 len = AttributeHeader::getDataSize(tmp);
  ndbrequire(len == 1);
  ndbrequire(AttributeHeader::getAttributeId(tmp) == AttributeHeader::CORR_FACTOR32);
  ndbrequire(reader.getWord(&correlationNumber));
}

void
Dbspj::getCorrelationData(const RowPtr::Linear & row,
                          Uint32 col,
                          Uint32& correlationNumber)
{
  /**
   * TODO handle errors
   */
  Uint32 offset = row.m_header->m_offset[col];
  Uint32 tmp = row.m_data[offset];
  Uint32 len = AttributeHeader::getDataSize(tmp);
  ndbrequire(len == 1);
  ndbrequire(AttributeHeader::getAttributeId(tmp) == AttributeHeader::CORR_FACTOR32);
  correlationNumber = row.m_data[offset+1];
}

Uint32
Dbspj::appendColToSection(Uint32 & dst, const RowPtr::Section & row,
                          Uint32 col, bool& hasNull)
{
  /**
   * TODO handle errors
   */
  SegmentedSectionPtr ptr(row.m_dataPtr);
  SectionReader reader(ptr, getSectionSegmentPool());
  Uint32 offset = row.m_header->m_offset[col];
  ndbrequire(reader.step(offset));
  Uint32 tmp;
  ndbrequire(reader.getWord(&tmp));
  Uint32 len = AttributeHeader::getDataSize(tmp);
  if (unlikely(len==0))
  {
    jam();
    hasNull = true;  // NULL-value in key
    return 0;
  }
  return appendTreeToSection(dst, reader, len);
}

Uint32
Dbspj::appendColToSection(Uint32 & dst, const RowPtr::Linear & row,
                          Uint32 col, bool& hasNull)
{
  /**
   * TODO handle errors
   */
  Uint32 offset = row.m_header->m_offset[col];
  const Uint32 * ptr = row.m_data + offset;
  Uint32 len = AttributeHeader::getDataSize(* ptr ++);
  if (unlikely(len==0))
  {
    jam();
    hasNull = true;  // NULL-value in key
    return 0;
  }
  return appendToSection(dst, ptr, len) ? 0 : DbspjErr::InvalidPattern;
}

Uint32
Dbspj::appendAttrinfoToSection(Uint32 & dst, const RowPtr::Linear & row,
                               Uint32 col, bool& hasNull)
{
  /**
   * TODO handle errors
   */
  Uint32 offset = row.m_header->m_offset[col];
  const Uint32 * ptr = row.m_data + offset;
  Uint32 len = AttributeHeader::getDataSize(* ptr);
  if (unlikely(len==0))
  {
    jam();
    hasNull = true;  // NULL-value in key
  }
  return appendToSection(dst, ptr, 1 + len) ? 0 : DbspjErr::InvalidPattern;
}

Uint32
Dbspj::appendAttrinfoToSection(Uint32 & dst, const RowPtr::Section & row,
                               Uint32 col, bool& hasNull)
{
  /**
   * TODO handle errors
   */
  SegmentedSectionPtr ptr(row.m_dataPtr);
  SectionReader reader(ptr, getSectionSegmentPool());
  Uint32 offset = row.m_header->m_offset[col];
  ndbrequire(reader.step(offset));
  Uint32 tmp;
  ndbrequire(reader.peekWord(&tmp));
  Uint32 len = AttributeHeader::getDataSize(tmp);
  if (unlikely(len==0))
  {
    jam();
    hasNull = true;  // NULL-value in key
  }
  return appendTreeToSection(dst, reader, 1 + len);
}

/**
 * 'PkCol' is the composite NDB$PK column in an unique index consisting of
 * a fragment id and the composite PK value (all PK columns concatenated)
 */
Uint32
Dbspj::appendPkColToSection(Uint32 & dst, const RowPtr::Section & row, Uint32 col)
{
  /**
   * TODO handle errors
   */
  SegmentedSectionPtr ptr(row.m_dataPtr);
  SectionReader reader(ptr, getSectionSegmentPool());
  Uint32 offset = row.m_header->m_offset[col];
  ndbrequire(reader.step(offset));
  Uint32 tmp;
  ndbrequire(reader.getWord(&tmp));
  Uint32 len = AttributeHeader::getDataSize(tmp);
  ndbrequire(len>1);  // NULL-value in PkKey is an error
  ndbrequire(reader.step(1)); // Skip fragid
  return appendTreeToSection(dst, reader, len-1);
}

/**
 * 'PkCol' is the composite NDB$PK column in an unique index consisting of
 * a fragment id and the composite PK value (all PK columns concatenated)
 */
Uint32
Dbspj::appendPkColToSection(Uint32 & dst, const RowPtr::Linear & row, Uint32 col)
{
  Uint32 offset = row.m_header->m_offset[col];
  Uint32 tmp = row.m_data[offset];
  Uint32 len = AttributeHeader::getDataSize(tmp);
  ndbrequire(len>1);  // NULL-value in PkKey is an error
  return appendToSection(dst, row.m_data+offset+2, len - 1) ? 0 : /** todo error code */ 1;
}

Uint32
Dbspj::appendFromParent(Uint32 & dst, Local_pattern_store& pattern,
                        Local_pattern_store::ConstDataBufferIterator& it,
                        Uint32 levels, const RowPtr & rowptr,
                        bool& hasNull)
{
  Ptr<TreeNode> treeNodePtr;
  m_treenode_pool.getPtr(treeNodePtr, rowptr.m_src_node_ptrI);
  Uint32 corrVal = rowptr.m_src_correlation;
  RowPtr targetRow;
  while (levels--)
  {
    jam();
    if (unlikely(treeNodePtr.p->m_parentPtrI == RNIL))
    {
      DEBUG_CRASH();
      return DbspjErr::InvalidPattern;
    }
    m_treenode_pool.getPtr(treeNodePtr, treeNodePtr.p->m_parentPtrI);
    if (unlikely((treeNodePtr.p->m_bits & TreeNode::T_ROW_BUFFER_MAP) == 0))
    {
      DEBUG_CRASH();
      return DbspjErr::InvalidPattern;
    }

    RowRef ref;
    treeNodePtr.p->m_row_map.copyto(ref);
    Uint32 allocator = ref.m_allocator;
    const Uint32 * mapptr;
    if (allocator == 0)
    {
      jam();
      mapptr = get_row_ptr_stack(ref);
    }
    else
    {
      jam();
      mapptr = get_row_ptr_var(ref);
    }

    Uint32 pos = corrVal >> 16; // parent corr-val
    if (unlikely(! (pos < treeNodePtr.p->m_row_map.m_size)))
    {
      DEBUG_CRASH();
      return DbspjErr::InvalidPattern;
    }

    // load ref to parent row
    treeNodePtr.p->m_row_map.load(mapptr, pos, ref);

    const Uint32 * rowptr;
    if (allocator == 0)
    {
      jam();
      rowptr = get_row_ptr_stack(ref);
    }
    else
    {
      jam();
      rowptr = get_row_ptr_var(ref);
    }
    setupRowPtr(treeNodePtr, targetRow, ref, rowptr);

    if (levels)
    {
      jam();
      getCorrelationData(targetRow.m_row_data.m_linear,
                         targetRow.m_row_data.m_linear.m_header->m_len - 1,
                         corrVal);
    }
  }

  if (unlikely(it.isNull()))
  {
    DEBUG_CRASH();
    return DbspjErr::InvalidPattern;
  }

  Uint32 info = *it.data;
  Uint32 type = QueryPattern::getType(info);
  Uint32 val = QueryPattern::getLength(info);
  pattern.next(it);
  switch(type){
  case QueryPattern::P_COL:
    jam();
    return appendColToSection(dst, targetRow.m_row_data.m_linear, val, hasNull);
    break;
  case QueryPattern::P_UNQ_PK:
    jam();
    return appendPkColToSection(dst, targetRow.m_row_data.m_linear, val);
    break;
  case QueryPattern::P_ATTRINFO:
    jam();
    return appendAttrinfoToSection(dst, targetRow.m_row_data.m_linear, val, hasNull);
    break;
  case QueryPattern::P_DATA:
    jam();
    // retreiving DATA from parent...is...an error
    break;
  case QueryPattern::P_PARENT:
    jam();
    // no point in nesting P_PARENT...an error
    break;
  case QueryPattern::P_PARAM:
  case QueryPattern::P_PARAM_HEADER:
    jam();
    // should have been expanded during build
    break;
  }

  DEBUG_CRASH();
  return DbspjErr::InvalidPattern;
}

Uint32
Dbspj::appendDataToSection(Uint32 & ptrI,
                           Local_pattern_store& pattern,
                           Local_pattern_store::ConstDataBufferIterator& it,
                           Uint32 len, bool& hasNull)
{
  if (unlikely(len==0))
  {
    jam();
    hasNull = true;
    return 0;
  }

#if 0
  /**
   * TODO handle errors
   */
  Uint32 tmp[NDB_SECTION_SEGMENT_SZ];
  while (len > NDB_SECTION_SEGMENT_SZ)
  {
    pattern.copyout(tmp, NDB_SECTION_SEGMENT_SZ, it);
    appendToSection(ptrI, tmp, NDB_SECTION_SEGMENT_SZ);
    len -= NDB_SECTION_SEGMENT_SZ;
  }

  pattern.copyout(tmp, len, it);
  appendToSection(ptrI, tmp, len);
  return 0;
#else
  Uint32 remaining = len;
  Uint32 dstIdx = 0;
  Uint32 tmp[NDB_SECTION_SEGMENT_SZ];

  while (remaining > 0 && !it.isNull())
  {
    tmp[dstIdx] = *it.data;
    remaining--;
    dstIdx++;
    pattern.next(it);
    if (dstIdx == NDB_SECTION_SEGMENT_SZ || remaining == 0)
    {
      if (!appendToSection(ptrI, tmp, dstIdx))
      {
        DEBUG_CRASH();
        return DbspjErr::InvalidPattern;
      }
      dstIdx = 0;
    }
  }
  if (remaining > 0)
  {
    DEBUG_CRASH();
    return DbspjErr::InvalidPattern;
  }
  else
  {
    return 0;
  }
#endif
}

Uint32
Dbspj::createEmptySection(Uint32 & dst)
{
  Uint32 tmp;
  SegmentedSectionPtr ptr;
  if (likely(import(ptr, &tmp, 0)))
  {
    jam();
    dst = ptr.i;
    return 0;
  }

  jam();
  return DbspjErr::OutOfSectionMemory;
}

/**
 * This function takes a pattern and a row and expands it into a section
 */
Uint32
Dbspj::expandS(Uint32 & _dst, Local_pattern_store& pattern,
               const RowPtr & row, bool& hasNull)
{
  Uint32 err;
  Uint32 dst = _dst;
  hasNull = false;
  Local_pattern_store::ConstDataBufferIterator it;
  pattern.first(it);
  while (!it.isNull())
  {
    Uint32 info = *it.data;
    Uint32 type = QueryPattern::getType(info);
    Uint32 val = QueryPattern::getLength(info);
    pattern.next(it);
    switch(type){
    case QueryPattern::P_COL:
      jam();
      err = appendColToSection(dst, row.m_row_data.m_section, val, hasNull);
      break;
    case QueryPattern::P_UNQ_PK:
      jam();
      err = appendPkColToSection(dst, row.m_row_data.m_section, val);
      break;
    case QueryPattern::P_ATTRINFO:
      jam();
      err = appendAttrinfoToSection(dst, row.m_row_data.m_section, val, hasNull);
      break;
    case QueryPattern::P_DATA:
      jam();
      err = appendDataToSection(dst, pattern, it, val, hasNull);
      break;
    case QueryPattern::P_PARENT:
      jam();
      // P_PARENT is a prefix to another pattern token
      // that permits code to access rows from earlier than immediate parent.
      // val is no of levels to move up the tree
      err = appendFromParent(dst, pattern, it, val, row, hasNull);
      break;
      // PARAM's was converted to DATA by ::expand(pattern...)
    case QueryPattern::P_PARAM:
    case QueryPattern::P_PARAM_HEADER:
    default:
      jam();
      err = DbspjErr::InvalidPattern;
      DEBUG_CRASH();
    }
    if (unlikely(err != 0))
    {
      jam();
      DEBUG_CRASH();
      goto error;
    }
  }

  _dst = dst;
  return 0;
error:
  jam();
  return err;
}

/**
 * This function takes a pattern and a row and expands it into a section
 */
Uint32
Dbspj::expandL(Uint32 & _dst, Local_pattern_store& pattern,
               const RowPtr & row, bool& hasNull)
{
  Uint32 err;
  Uint32 dst = _dst;
  hasNull = false;
  Local_pattern_store::ConstDataBufferIterator it;
  pattern.first(it);
  while (!it.isNull())
  {
    Uint32 info = *it.data;
    Uint32 type = QueryPattern::getType(info);
    Uint32 val = QueryPattern::getLength(info);
    pattern.next(it);
    switch(type){
    case QueryPattern::P_COL:
      jam();
      err = appendColToSection(dst, row.m_row_data.m_linear, val, hasNull);
      break;
    case QueryPattern::P_UNQ_PK:
      jam();
      err = appendPkColToSection(dst, row.m_row_data.m_linear, val);
      break;
    case QueryPattern::P_ATTRINFO:
      jam();
      err = appendAttrinfoToSection(dst, row.m_row_data.m_linear, val, hasNull);
      break;
    case QueryPattern::P_DATA:
      jam();
      err = appendDataToSection(dst, pattern, it, val, hasNull);
      break;
    case QueryPattern::P_PARENT:
      jam();
      // P_PARENT is a prefix to another pattern token
      // that permits code to access rows from earlier than immediate parent
      // val is no of levels to move up the tree
      err = appendFromParent(dst, pattern, it, val, row, hasNull);
      break;
      // PARAM's was converted to DATA by ::expand(pattern...)
    case QueryPattern::P_PARAM:
    case QueryPattern::P_PARAM_HEADER:
    default:
      jam();
      err = DbspjErr::InvalidPattern;
      DEBUG_CRASH();
    }
    if (unlikely(err != 0))
    {
      jam();
      DEBUG_CRASH();
      goto error;
    }
  }

  _dst = dst;
  return 0;
error:
  jam();
  return err;
}

Uint32
Dbspj::expand(Uint32 & ptrI, DABuffer& pattern, Uint32 len,
              DABuffer& param, Uint32 paramCnt, bool& hasNull)
{
  /**
   * TODO handle error
   */
  Uint32 err;
  Uint32 tmp[1+MAX_ATTRIBUTES_IN_TABLE];
  struct RowPtr::Linear row;
  row.m_data = param.ptr;
  row.m_header = CAST_PTR(RowPtr::Header, &tmp[0]);
  buildRowHeader(CAST_PTR(RowPtr::Header, &tmp[0]), param.ptr, paramCnt);

  Uint32 dst = ptrI;
  const Uint32 * ptr = pattern.ptr;
  const Uint32 * end = ptr + len;
  hasNull = false;

  for (; ptr < end; )
  {
    Uint32 info = * ptr++;
    Uint32 type = QueryPattern::getType(info);
    Uint32 val = QueryPattern::getLength(info);
    switch(type){
    case QueryPattern::P_PARAM:
      jam();
      ndbassert(val < paramCnt);
      err = appendColToSection(dst, row, val, hasNull);
      break;
    case QueryPattern::P_PARAM_HEADER:
      jam();
      ndbassert(val < paramCnt);
      err = appendAttrinfoToSection(dst, row, val, hasNull);
      break;
    case QueryPattern::P_DATA:
      if (unlikely(val==0))
      {
        jam();
        hasNull = true;
        err = 0;
      }
      else if (likely(appendToSection(dst, ptr, val)))
      {
        jam();
        err = 0;
      }
      else
      {
        jam();
        err = DbspjErr::InvalidPattern;
      }
      ptr += val;
      break;
    case QueryPattern::P_COL:    // (linked) COL's not expected here
    case QueryPattern::P_PARENT: // Prefix to P_COL
    case QueryPattern::P_ATTRINFO:
    case QueryPattern::P_UNQ_PK:
    default:
      jam();
      jamLine(type);
      err = DbspjErr::InvalidPattern;
      DEBUG_CRASH();
    }
    if (unlikely(err != 0))
    {
      jam();
      DEBUG_CRASH();
      goto error;
    }
  }

  /**
   * Iterate forward
   */
  pattern.ptr = end;

error:
  jam();
  ptrI = dst;
  return err;
}

Uint32
Dbspj::expand(Local_pattern_store& dst, Ptr<TreeNode> treeNodePtr,
              DABuffer& pattern, Uint32 len,
              DABuffer& param, Uint32 paramCnt)
{
  /**
   * TODO handle error
   */
  Uint32 err;
  Uint32 tmp[1+MAX_ATTRIBUTES_IN_TABLE];
  struct RowPtr::Linear row;
  row.m_header = CAST_PTR(RowPtr::Header, &tmp[0]);
  row.m_data = param.ptr;
  buildRowHeader(CAST_PTR(RowPtr::Header, &tmp[0]), param.ptr, paramCnt);

  const Uint32 * end = pattern.ptr + len;
  for (; pattern.ptr < end; )
  {
    Uint32 info = *pattern.ptr;
    Uint32 type = QueryPattern::getType(info);
    Uint32 val = QueryPattern::getLength(info);
    switch(type){
    case QueryPattern::P_COL:
    case QueryPattern::P_UNQ_PK:
    case QueryPattern::P_ATTRINFO:
      jam();
      err = appendToPattern(dst, pattern, 1);
      break;
    case QueryPattern::P_DATA:
      jam();
      err = appendToPattern(dst, pattern, val+1);
      break;
    case QueryPattern::P_PARAM:
      jam();
      // NOTE: Converted to P_DATA by appendParamToPattern
      ndbassert(val < paramCnt);
      err = appendParamToPattern(dst, row, val);
      pattern.ptr++;
      break;
    case QueryPattern::P_PARAM_HEADER:
      jam();
      // NOTE: Converted to P_DATA by appendParamHeadToPattern
      ndbassert(val < paramCnt);
      err = appendParamHeadToPattern(dst, row, val);
      pattern.ptr++;
      break;
    case QueryPattern::P_PARENT: // Prefix to P_COL
    {
      jam();
      err = appendToPattern(dst, pattern, 1);

      // Locate requested grandparent and request it to
      // T_ROW_BUFFER its result rows
      Ptr<TreeNode> parentPtr;
      m_treenode_pool.getPtr(parentPtr, treeNodePtr.p->m_parentPtrI);
      while (val--)
      {
        jam();
        ndbassert(parentPtr.p->m_parentPtrI != RNIL);
        m_treenode_pool.getPtr(parentPtr, parentPtr.p->m_parentPtrI);
        parentPtr.p->m_bits |= TreeNode::T_ROW_BUFFER;
        parentPtr.p->m_bits |= TreeNode::T_ROW_BUFFER_MAP;
      }
      Ptr<Request> requestPtr;
      m_request_pool.getPtr(requestPtr, treeNodePtr.p->m_requestPtrI);
      requestPtr.p->m_bits |= Request::RT_ROW_BUFFERS;
      break;
    }
    default:
      jam();
      err = DbspjErr::InvalidPattern;
      DEBUG_CRASH();
    }

    if (unlikely(err != 0))
    {
      DEBUG_CRASH();
      goto error;
    }
  }
  return 0;

error:
  jam();
  return err;
}

Uint32
Dbspj::parseDA(Build_context& ctx,
               Ptr<Request> requestPtr,
               Ptr<TreeNode> treeNodePtr,
               DABuffer& tree, Uint32 treeBits,
               DABuffer& param, Uint32 paramBits)
{
  Uint32 err;
  Uint32 attrInfoPtrI = RNIL;
  Uint32 attrParamPtrI = RNIL;

  do
  {
    if (treeBits & DABits::NI_REPEAT_SCAN_RESULT)
    {
      jam();
      DEBUG("use REPEAT_SCAN_RESULT when returning results");
      requestPtr.p->m_bits |= Request::RT_REPEAT_SCAN_RESULT;
    } // DABits::NI_HAS_PARENT

    if (treeBits & DABits::NI_HAS_PARENT)
    {
      jam();
      DEBUG("NI_HAS_PARENT");
      /**
       * OPTIONAL PART 1:
       *
       * Parent nodes are stored first in optional part
       *   this is a list of 16-bit numbers refering to
       *   *earlier* nodes in tree
       *   the list stores length of list as first 16-bit
       */
      err = DbspjErr::InvalidTreeNodeSpecification;
      Uint32 dst[63];
      Uint32 cnt = unpackList(NDB_ARRAY_SIZE(dst), dst, tree);
      if (unlikely(cnt > NDB_ARRAY_SIZE(dst)))
      {
        DEBUG_CRASH();
        break;
      }

      err = 0;

      if (unlikely(cnt!=1))
      {
        /**
         * Only a single parent supported for now, i.e only trees
         */
        DEBUG_CRASH();
      }

      for (Uint32 i = 0; i<cnt; i++)
      {
        DEBUG("adding " << dst[i] << " as parent");
        Ptr<TreeNode> parentPtr = ctx.m_node_list[dst[i]];
        LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
        Local_dependency_map map(pool, parentPtr.p->m_dependent_nodes);
        if (unlikely(!map.append(&treeNodePtr.i, 1)))
        {
          err = DbspjErr::OutOfQueryMemory;
          DEBUG_CRASH();
          break;
        }
        parentPtr.p->m_bits &= ~(Uint32)TreeNode::T_LEAF;
        treeNodePtr.p->m_parentPtrI = parentPtr.i;

        // Build Bitmask of all ancestors to treeNode
        treeNodePtr.p->m_ancestors = parentPtr.p->m_ancestors;
        treeNodePtr.p->m_ancestors.set(parentPtr.p->m_node_no);
      }

      if (unlikely(err != 0))
        break;
    } // DABits::NI_HAS_PARENT

    err = DbspjErr::InvalidTreeParametersSpecificationKeyParamBitsMissmatch;
    if (unlikely( ((treeBits  & DABits::NI_KEY_PARAMS)==0) !=
                  ((paramBits & DABits::PI_KEY_PARAMS)==0)))
    {
      DEBUG_CRASH();
      break;
    }

    if (treeBits & (DABits::NI_KEY_PARAMS
                    | DABits::NI_KEY_LINKED
                    | DABits::NI_KEY_CONSTS))
    {
      jam();
      DEBUG("NI_KEY_PARAMS | NI_KEY_LINKED | NI_KEY_CONSTS");

      /**
       * OPTIONAL PART 2:
       *
       * If keys are parametrized or linked
       *   DATA0[LO/HI] - Length of key pattern/#parameters to key
       */
      Uint32 len_cnt = * tree.ptr ++;
      Uint32 len = len_cnt & 0xFFFF; // length of pattern in words
      Uint32 cnt = len_cnt >> 16;    // no of parameters

      LocalArenaPoolImpl pool(requestPtr.p->m_arena, m_dependency_map_pool);
      Local_pattern_store pattern(pool, treeNodePtr.p->m_keyPattern);

      err = DbspjErr::InvalidTreeParametersSpecificationIncorrectKeyParamCount;
      if (unlikely( ((cnt==0) != ((treeBits & DABits::NI_KEY_PARAMS) == 0)) ||
                    ((cnt==0) != ((paramBits & DABits::PI_KEY_PARAMS) == 0))))
      {
        DEBUG_CRASH();
        break;
      }

      if (treeBits & DABits::NI_KEY_LINKED)
      {
        jam();
        DEBUG("LINKED-KEY PATTERN w/ " << cnt << " PARAM values");
        /**
         * Expand pattern into a new pattern (with linked values)
         */
        err = expand(pattern, treeNodePtr, tree, len, param, cnt);

        /**
         * This node constructs a new key for each send
         */
        treeNodePtr.p->m_bits |= TreeNode::T_KEYINFO_CONSTRUCTED;
      }
      else
      {
        jam();
        DEBUG("FIXED-KEY w/ " << cnt << " PARAM values");
        /**
         * Expand pattern directly into keyinfo
         *   This means a "fixed" key from here on
         */
        bool hasNull;
        Uint32 keyInfoPtrI = RNIL;
        err = expand(keyInfoPtrI, tree, len, param, cnt, hasNull);
        if (unlikely(hasNull))
        {
          /* API should have elliminated requests w/ const-NULL keys */
          jam();
          DEBUG("BEWARE: FIXED-key contain NULL values");
//        treeNodePtr.p->m_bits |= TreeNode::T_NULL_PRUNE;
//        break;
          ndbrequire(false);
        }
        treeNodePtr.p->m_send.m_keyInfoPtrI = keyInfoPtrI;
      }

      if (unlikely(err != 0))
      {
        DEBUG_CRASH();
        break;
      }
    } // DABits::NI_KEY_...

    const Uint32 mask =
      DABits::NI_LINKED_ATTR | DABits::NI_ATTR_INTERPRET |
      DABits::NI_ATTR_LINKED | DABits::NI_ATTR_PARAMS;

    if (((treeBits & mask) | (paramBits & DABits::PI_ATTR_LIST)) != 0)
    {
      jam();
      /**
       * OPTIONAL PART 3: attrinfo handling
       * - NI_LINKED_ATTR - these are attributes to be passed to children
       * - PI_ATTR_LIST   - this is "user-columns" (passed as parameters)

       * - NI_ATTR_INTERPRET - tree contains interpreted program
       * - NI_ATTR_LINKED - means that the attr-info contains linked-values
       * - NI_ATTR_PARAMS - means that the attr-info is parameterized
       *   PI_ATTR_PARAMS - means that the parameters contains attr parameters
       *
       * IF NI_ATTR_INTERPRET
       *   DATA0[LO/HI] = Length of program / total #arguments to program
       *   DATA1..N     = Program
       *
       * IF NI_ATTR_PARAMS
       *   DATA0[LO/HI] = Length / #param
       *   DATA1..N     = PARAM-0...PARAM-M
       *
       * IF PI_ATTR_INTERPRET
       *   DATA0[LO/HI] = Length of program / Length of subroutine-part
       *   DATA1..N     = Program (scan filter)
       *
       * IF NI_ATTR_LINKED
       *   DATA0[LO/HI] = Length / #
       *
       *
       */
      Uint32 sections[5] = { 0, 0, 0, 0, 0 };
      Uint32 * sectionptrs = 0;

      bool interpreted =
        (treeBits & DABits::NI_ATTR_INTERPRET) ||
        (paramBits & DABits::PI_ATTR_INTERPRET) ||
        (treeNodePtr.p->m_bits & TreeNode::T_ATTR_INTERPRETED);

      if (interpreted)
      {
        /**
         * Add section headers for interpreted execution
         *   and create pointer so that they can be updated later
         */
        jam();
        err = DbspjErr::OutOfSectionMemory;
        if (unlikely(!appendToSection(attrInfoPtrI, sections, 5)))
        {
          DEBUG_CRASH();
          break;
        }

        SegmentedSectionPtr ptr;
        getSection(ptr, attrInfoPtrI);
        sectionptrs = ptr.p->theData;

        if (treeBits & DABits::NI_ATTR_INTERPRET)
        {
          jam();

          /**
           * Having two interpreter programs is an error.
           */
          err = DbspjErr::BothTreeAndParametersContainInterpretedProgram;
          if (unlikely(paramBits & DABits::PI_ATTR_INTERPRET))
          {
            DEBUG_CRASH();
            break;
          }

          treeNodePtr.p->m_bits |= TreeNode::T_ATTR_INTERPRETED;
          Uint32 len2 = * tree.ptr++;
          Uint32 len_prg = len2 & 0xFFFF; // Length of interpret program
          Uint32 len_pattern = len2 >> 16;// Length of attr param pattern
          err = DbspjErr::OutOfSectionMemory;
          if (unlikely(!appendToSection(attrInfoPtrI, tree.ptr, len_prg)))
          {
            DEBUG_CRASH();
            break;
          }

          tree.ptr += len_prg;
          sectionptrs[1] = len_prg; // size of interpret program

          Uint32 tmp = * tree.ptr ++; // attr-pattern header
          Uint32 cnt = tmp & 0xFFFF;

          if (treeBits & DABits::NI_ATTR_LINKED)
          {
            jam();
            /**
             * Expand pattern into a new pattern (with linked values)
             */
            LocalArenaPoolImpl pool(requestPtr.p->m_arena,
                                    m_dependency_map_pool);
            Local_pattern_store pattern(pool,treeNodePtr.p->m_attrParamPattern);
            err = expand(pattern, treeNodePtr, tree, len_pattern, param, cnt);
            if (unlikely(err))
            {
              DEBUG_CRASH();
              break;
            }
            /**
             * This node constructs a new attr-info for each send
             */
            treeNodePtr.p->m_bits |= TreeNode::T_ATTRINFO_CONSTRUCTED;
          }
          else
          {
            jam();
            /**
             * Expand pattern directly into attr-info param
             *   This means a "fixed" attr-info param from here on
             */
            bool hasNull;
            err = expand(attrParamPtrI, tree, len_pattern, param, cnt, hasNull);
            if (unlikely(err))
            {
              DEBUG_CRASH();
              break;
            }
//          ndbrequire(!hasNull);
          }
        }
        else // if (treeBits & DABits::NI_ATTR_INTERPRET)
        {
          jam();
          /**
           * Only relevant for interpreted stuff
           */
          ndbrequire((treeBits & DABits::NI_ATTR_PARAMS) == 0);
          ndbrequire((paramBits & DABits::PI_ATTR_PARAMS) == 0);
          ndbrequire((treeBits & DABits::NI_ATTR_LINKED) == 0);

          treeNodePtr.p->m_bits |= TreeNode::T_ATTR_INTERPRETED;

          if (! (paramBits & DABits::PI_ATTR_INTERPRET))
          {
            jam();

            /**
             * Tree node has interpreted execution,
             *   but no interpreted program specified
             *   auto-add Exit_ok (i.e return each row)
             */
            Uint32 tmp = Interpreter::ExitOK();
            err = DbspjErr::OutOfSectionMemory;
            if (unlikely(!appendToSection(attrInfoPtrI, &tmp, 1)))
            {
              DEBUG_CRASH();
              break;
            }
            sectionptrs[1] = 1;
          }
        } // if (treeBits & DABits::NI_ATTR_INTERPRET)
      } // if (interpreted)

      if (paramBits & DABits::PI_ATTR_INTERPRET)
      {
        jam();

        /**
         * Add the interpreted code that represents the scan filter.
         */
        const Uint32 len2 = * param.ptr++;
        Uint32 program_len = len2 & 0xFFFF;
        Uint32 subroutine_len = len2 >> 16;
        err = DbspjErr::OutOfSectionMemory;
        if (unlikely(!appendToSection(attrInfoPtrI, param.ptr, program_len)))
        {
          DEBUG_CRASH();
          break;
        }
        /**
         * The interpreted code is added is in the "Interpreted execute region"
         * of the attrinfo (see Dbtup::interpreterStartLab() for details).
         * It will thus execute before reading the attributes that constitutes
         * the projections.
         */
        sectionptrs[1] = program_len;
        param.ptr += program_len;

        if (subroutine_len)
        {
          if (unlikely(!appendToSection(attrParamPtrI,
                                        param.ptr, subroutine_len)))
          {
            DEBUG_CRASH();
            break;
          }
          sectionptrs[4] = subroutine_len;
          param.ptr += subroutine_len;
        }
        treeNodePtr.p->m_bits |= TreeNode::T_ATTR_INTERPRETED;
      }

      Uint32 sum_read = 0;
      Uint32 dst[MAX_ATTRIBUTES_IN_TABLE + 2];

      if (paramBits & DABits::PI_ATTR_LIST)
      {
        jam();
        Uint32 len = * param.ptr++;
        DEBUG("PI_ATTR_LIST");

        treeNodePtr.p->m_bits |= TreeNode::T_USER_PROJECTION;
        err = DbspjErr::OutOfSectionMemory;
        if (!appendToSection(attrInfoPtrI, param.ptr, len))
        {
          DEBUG_CRASH();
          break;
        }

        param.ptr += len;

        /**
         * Insert a flush of this partial result set
         */
        Uint32 flush[4];
        flush[0] = AttributeHeader::FLUSH_AI << 16;
        flush[1] = ctx.m_resultRef;
        flush[2] = ctx.m_resultData;
        flush[3] = ctx.m_senderRef; // RouteRef
        if (!appendToSection(attrInfoPtrI, flush, 4))
        {
          DEBUG_CRASH();
          break;
        }

        sum_read += len + 4;
      }

      if (treeBits & DABits::NI_LINKED_ATTR)
      {
        jam();
        DEBUG("NI_LINKED_ATTR");
        err = DbspjErr::InvalidTreeNodeSpecification;
        Uint32 cnt = unpackList(MAX_ATTRIBUTES_IN_TABLE, dst, tree);
        if (unlikely(cnt > MAX_ATTRIBUTES_IN_TABLE))
        {
          DEBUG_CRASH();
          break;
        }

        /**
         * AttributeHeader contains attrId in 16-higher bits
         */
        for (Uint32 i = 0; i<cnt; i++)
          dst[i] <<= 16;

        /**
         * Read correlation factor
         */
        dst[cnt++] = AttributeHeader::CORR_FACTOR32 << 16;

        err = DbspjErr::OutOfSectionMemory;
        if (!appendToSection(attrInfoPtrI, dst, cnt))
        {
          DEBUG_CRASH();
          break;
        }

        sum_read += cnt;
      }

      if (interpreted)
      {
        jam();
        /**
         * Let reads be performed *after* interpreted program
         *   i.e in "final read"-section
         */
        sectionptrs[3] = sum_read;

        if (attrParamPtrI != RNIL)
        {
          jam();
          ndbrequire(!(treeNodePtr.p->m_bits&TreeNode::T_ATTRINFO_CONSTRUCTED));

          SegmentedSectionPtr ptr;
          getSection(ptr, attrParamPtrI);
          {
            SectionReader r0(ptr, getSectionSegmentPool());
            err = appendTreeToSection(attrInfoPtrI, r0, ptr.sz);
            sectionptrs[4] = ptr.sz;
            if (unlikely(err != 0))
            {
              DEBUG_CRASH();
              break;
            }
          }
          releaseSection(attrParamPtrI);
        }
      }

      treeNodePtr.p->m_send.m_attrInfoPtrI = attrInfoPtrI;
    } // if (((treeBits & mask) | (paramBits & DABits::PI_ATTR_LIST)) != 0)

    return 0;
  } while (0);

  return err;
}

/**
 * END - MODULE COMMON PARSE/UNPACK
 */

/**
 * Process a scan request for an ndb$info table. (These are used for monitoring
 * purposes and do not contain application data.)
 */
void Dbspj::execDBINFO_SCANREQ(Signal *signal)
{
  DbinfoScanReq req= * CAST_PTR(DbinfoScanReq, &signal->theData[0]);
  const Ndbinfo::ScanCursor* cursor =
    CAST_CONSTPTR(Ndbinfo::ScanCursor, DbinfoScan::getCursorPtr(&req));
  Ndbinfo::Ratelimit rl;

  jamEntry();

  switch(req.tableId){

    // The SPJ block only implements the ndbinfo.counters table.
  case Ndbinfo::COUNTERS_TABLEID:
  {
    Ndbinfo::counter_entry counters[] = {
      { Ndbinfo::SPJ_READS_RECEIVED_COUNTER,
        c_Counters.get_counter(CI_READS_RECEIVED) },
      { Ndbinfo::SPJ_LOCAL_READS_SENT_COUNTER,
        c_Counters.get_counter(CI_LOCAL_READS_SENT) },
      { Ndbinfo::SPJ_REMOTE_READS_SENT_COUNTER,
        c_Counters.get_counter(CI_REMOTE_READS_SENT) },
      { Ndbinfo::SPJ_READS_NOT_FOUND_COUNTER,
        c_Counters.get_counter(CI_READS_NOT_FOUND) },
      { Ndbinfo::SPJ_TABLE_SCANS_RECEIVED_COUNTER,
        c_Counters.get_counter(CI_TABLE_SCANS_RECEIVED) },
      { Ndbinfo::SPJ_LOCAL_TABLE_SCANS_SENT_COUNTER,
        c_Counters.get_counter(CI_LOCAL_TABLE_SCANS_SENT) },
      { Ndbinfo::SPJ_RANGE_SCANS_RECEIVED_COUNTER,
        c_Counters.get_counter(CI_RANGE_SCANS_RECEIVED) },
      { Ndbinfo::SPJ_LOCAL_RANGE_SCANS_SENT_COUNTER,
        c_Counters.get_counter(CI_LOCAL_RANGE_SCANS_SENT) },
      { Ndbinfo::SPJ_REMOTE_RANGE_SCANS_SENT_COUNTER,
        c_Counters.get_counter(CI_REMOTE_RANGE_SCANS_SENT) },
      { Ndbinfo::SPJ_SCAN_BATCHES_RETURNED_COUNTER,
        c_Counters.get_counter(CI_SCAN_BATCHES_RETURNED) },
      { Ndbinfo::SPJ_SCAN_ROWS_RETURNED_COUNTER,
        c_Counters.get_counter(CI_SCAN_ROWS_RETURNED) },
      { Ndbinfo::SPJ_PRUNED_RANGE_SCANS_RECEIVED_COUNTER,
        c_Counters.get_counter(CI_PRUNED_RANGE_SCANS_RECEIVED) },
      { Ndbinfo::SPJ_CONST_PRUNED_RANGE_SCANS_RECEIVED_COUNTER,
        c_Counters.get_counter(CI_CONST_PRUNED_RANGE_SCANS_RECEIVED) }
    };
    const size_t num_counters = sizeof(counters) / sizeof(counters[0]);

    Uint32 i = cursor->data[0];
    const BlockNumber bn = blockToMain(number());
    while(i < num_counters)
    {
      jam();
      Ndbinfo::Row row(signal, req);
      row.write_uint32(getOwnNodeId());
      row.write_uint32(bn);           // block number
      row.write_uint32(instance());   // block instance
      row.write_uint32(counters[i].id);

      row.write_uint64(counters[i].val);
      ndbinfo_send_row(signal, req, row, rl);
      i++;
      if (rl.need_break(req))
      {
        jam();
        ndbinfo_send_scan_break(signal, req, rl, i);
        return;
      }
    }
    break;
  }

  default:
    break;
  }

  ndbinfo_send_scan_conf(signal, req, rl);
} // Dbspj::execDBINFO_SCANREQ(Signal *signal)

void Dbspj::IncrementalStatistics::update(double sample)
{
  // Prevent wrap-around
  if(m_noOfSamples < 0xffffffff)
  {
    m_noOfSamples++;
    const double delta = sample - m_mean;
    m_mean += delta/m_noOfSamples;
    m_sumSquare +=  delta * (sample - m_mean);
  }
}
