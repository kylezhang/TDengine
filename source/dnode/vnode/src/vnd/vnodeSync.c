/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include "vnd.h"

#define BATCH_DISABLE 1

static inline bool vnodeIsMsgBlock(tmsg_t type) {
  return (type == TDMT_VND_CREATE_TABLE) || (type == TDMT_VND_ALTER_TABLE) || (type == TDMT_VND_DROP_TABLE) ||
         (type == TDMT_VND_UPDATE_TAG_VAL);
}

static inline bool vnodeIsMsgWeak(tmsg_t type) { return false; }

static inline void vnodeWaitBlockMsg(SVnode *pVnode, const SRpcMsg *pMsg) {
  if (vnodeIsMsgBlock(pMsg->msgType)) {
    const STraceId *trace = &pMsg->info.traceId;
    taosThreadMutexLock(&pVnode->lock);
    if (!pVnode->blocked) {
      vGTrace("vgId:%d, msg:%p wait block, type:%s", pVnode->config.vgId, pMsg, TMSG_INFO(pMsg->msgType));
      pVnode->blocked = true;
      taosThreadMutexUnlock(&pVnode->lock);
      tsem_wait(&pVnode->syncSem);
    } else {
      taosThreadMutexUnlock(&pVnode->lock);
    }
  }
}

static inline void vnodePostBlockMsg(SVnode *pVnode, const SRpcMsg *pMsg) {
  if (vnodeIsMsgBlock(pMsg->msgType)) {
    const STraceId *trace = &pMsg->info.traceId;
    taosThreadMutexLock(&pVnode->lock);
    if (pVnode->blocked) {
      vGTrace("vgId:%d, msg:%p post block, type:%s", pVnode->config.vgId, pMsg, TMSG_INFO(pMsg->msgType));
      pVnode->blocked = false;
      tsem_post(&pVnode->syncSem);
    }
    taosThreadMutexUnlock(&pVnode->lock);
  }
}

void vnodeRedirectRpcMsg(SVnode *pVnode, SRpcMsg *pMsg) {
  SEpSet newEpSet = {0};
  syncGetRetryEpSet(pVnode->sync, &newEpSet);

  const STraceId *trace = &pMsg->info.traceId;
  vGTrace("vgId:%d, msg:%p is redirect since not leader, numOfEps:%d inUse:%d", pVnode->config.vgId, pMsg,
          newEpSet.numOfEps, newEpSet.inUse);
  for (int32_t i = 0; i < newEpSet.numOfEps; ++i) {
    vGTrace("vgId:%d, msg:%p redirect:%d ep:%s:%u", pVnode->config.vgId, pMsg, i, newEpSet.eps[i].fqdn,
            newEpSet.eps[i].port);
  }
  pMsg->info.hasEpSet = 1;

  SRpcMsg rsp = {.code = TSDB_CODE_RPC_REDIRECT, .info = pMsg->info, .msgType = pMsg->msgType + 1};
  tmsgSendRedirectRsp(&rsp, &newEpSet);
}

static void inline vnodeHandleWriteMsg(SVnode *pVnode, SRpcMsg *pMsg) {
  SRpcMsg rsp = {.code = pMsg->code, .info = pMsg->info};
  if (vnodeProcessWriteMsg(pVnode, pMsg, pMsg->info.conn.applyIndex, &rsp) < 0) {
    rsp.code = terrno;
    const STraceId *trace = &pMsg->info.traceId;
    vGError("vgId:%d, msg:%p failed to apply right now since %s", pVnode->config.vgId, pMsg, terrstr());
  }
  if (rsp.info.handle != NULL) {
    tmsgSendRsp(&rsp);
  } else {
    if (rsp.pCont) {
      rpcFreeCont(rsp.pCont);
    }
  }
}

static void vnodeHandleProposeError(SVnode *pVnode, SRpcMsg *pMsg, int32_t code) {
  if (code == TSDB_CODE_SYN_NOT_LEADER) {
    vnodeRedirectRpcMsg(pVnode, pMsg);
  } else {
    const STraceId *trace = &pMsg->info.traceId;
    vGError("vgId:%d, msg:%p failed to propose since %s, code:0x%x", pVnode->config.vgId, pMsg, tstrerror(code), code);
    SRpcMsg rsp = {.code = code, .info = pMsg->info};
    if (rsp.info.handle != NULL) {
      tmsgSendRsp(&rsp);
    }
  }
}

static void inline vnodeProposeBatchMsg(SVnode *pVnode, SRpcMsg **pMsgArr, bool *pIsWeakArr, int32_t *arrSize) {
  if (*arrSize <= 0) return;

#if BATCH_DISABLE
  int32_t code = syncPropose(pVnode->sync, pMsgArr[0], pIsWeakArr[0]);
#else
  int32_t code = syncProposeBatch(pVnode->sync, pMsgArr, pIsWeakArr, *arrSize);
#endif

  if (code > 0) {
    for (int32_t i = 0; i < *arrSize; ++i) {
      vnodeHandleWriteMsg(pVnode, pMsgArr[i]);
    }
  } else if (code == 0) {
    vnodeWaitBlockMsg(pVnode, pMsgArr[*arrSize - 1]);
  } else {
    if (terrno != 0) code = terrno;
    for (int32_t i = 0; i < *arrSize; ++i) {
      vnodeHandleProposeError(pVnode, pMsgArr[i], code);
    }
  }

  for (int32_t i = 0; i < *arrSize; ++i) {
    SRpcMsg        *pMsg = pMsgArr[i];
    const STraceId *trace = &pMsg->info.traceId;
    vGTrace("vgId:%d, msg:%p is freed, code:0x%x", pVnode->config.vgId, pMsg, code);
    rpcFreeCont(pMsg->pCont);
    taosFreeQitem(pMsg);
  }

  *arrSize = 0;
}

void vnodeProposeWriteMsg(SQueueInfo *pInfo, STaosQall *qall, int32_t numOfMsgs) {
  SVnode   *pVnode = pInfo->ahandle;
  int32_t   vgId = pVnode->config.vgId;
  int32_t   code = 0;
  SRpcMsg  *pMsg = NULL;
  int32_t   arrayPos = 0;
  SRpcMsg **pMsgArr = taosMemoryCalloc(numOfMsgs, sizeof(SRpcMsg *));
  bool     *pIsWeakArr = taosMemoryCalloc(numOfMsgs, sizeof(bool));
  vTrace("vgId:%d, get %d msgs from vnode-write queue", vgId, numOfMsgs);

  for (int32_t msg = 0; msg < numOfMsgs; msg++) {
    if (taosGetQitem(qall, (void **)&pMsg) == 0) continue;
    bool isWeak = vnodeIsMsgWeak(pMsg->msgType);
    bool isBlock = vnodeIsMsgBlock(pMsg->msgType);

    const STraceId *trace = &pMsg->info.traceId;
    vGTrace("vgId:%d, msg:%p get from vnode-write queue, weak:%d block:%d msg:%d:%d pos:%d, handle:%p", vgId, pMsg,
            isWeak, isBlock, msg, numOfMsgs, arrayPos, pMsg->info.handle);

    if (!pVnode->restored) {
      vGError("vgId:%d, msg:%p failed to process since restore not finished", vgId, pMsg);
      terrno = TSDB_CODE_APP_NOT_READY;
      vnodeHandleProposeError(pVnode, pMsg, TSDB_CODE_APP_NOT_READY);
      rpcFreeCont(pMsg->pCont);
      taosFreeQitem(pMsg);
      continue;
    }

    if (pMsgArr == NULL || pIsWeakArr == NULL) {
      vGError("vgId:%d, msg:%p failed to process since out of memory", vgId, pMsg);
      terrno = TSDB_CODE_OUT_OF_MEMORY;
      vnodeHandleProposeError(pVnode, pMsg, terrno);
      rpcFreeCont(pMsg->pCont);
      taosFreeQitem(pMsg);
      continue;
    }

    code = vnodePreProcessWriteMsg(pVnode, pMsg);
    if (code != 0) {
      vGError("vgId:%d, msg:%p failed to pre-process since %s", vgId, pMsg, terrstr());
      rpcFreeCont(pMsg->pCont);
      taosFreeQitem(pMsg);
      continue;
    }

    if (isBlock || BATCH_DISABLE) {
      vnodeProposeBatchMsg(pVnode, pMsgArr, pIsWeakArr, &arrayPos);
    }

    pMsgArr[arrayPos] = pMsg;
    pIsWeakArr[arrayPos] = isWeak;
    arrayPos++;

    if (isBlock || msg == numOfMsgs - 1 || BATCH_DISABLE) {
      vnodeProposeBatchMsg(pVnode, pMsgArr, pIsWeakArr, &arrayPos);
    }
  }

  taosMemoryFree(pMsgArr);
  taosMemoryFree(pIsWeakArr);
}

void vnodeApplyWriteMsg(SQueueInfo *pInfo, STaosQall *qall, int32_t numOfMsgs) {
  SVnode  *pVnode = pInfo->ahandle;
  int32_t  vgId = pVnode->config.vgId;
  int32_t  code = 0;
  SRpcMsg *pMsg = NULL;

  for (int32_t i = 0; i < numOfMsgs; ++i) {
    if (taosGetQitem(qall, (void **)&pMsg) == 0) continue;
    const STraceId *trace = &pMsg->info.traceId;
    vGTrace("vgId:%d, msg:%p get from vnode-apply queue, type:%s handle:%p index:%" PRId64, vgId, pMsg,
            TMSG_INFO(pMsg->msgType), pMsg->info.handle, pMsg->info.conn.applyIndex);

    SRpcMsg rsp = {.code = pMsg->code, .info = pMsg->info};
    if (rsp.code == 0) {
      if (vnodeProcessWriteMsg(pVnode, pMsg, pMsg->info.conn.applyIndex, &rsp) < 0) {
        rsp.code = terrno;
        vGError("vgId:%d, msg:%p failed to apply since %s, index:%" PRId64, vgId, pMsg, terrstr(),
                pMsg->info.conn.applyIndex);
      }
    }

    vnodePostBlockMsg(pVnode, pMsg);
    if (rsp.info.handle != NULL) {
      tmsgSendRsp(&rsp);
    } else {
      if (rsp.pCont) {
        rpcFreeCont(rsp.pCont);
      }
    }

    vGTrace("vgId:%d, msg:%p is freed, code:0x%x index:%" PRId64, vgId, pMsg, rsp.code, pMsg->info.conn.applyIndex);
    rpcFreeCont(pMsg->pCont);
    taosFreeQitem(pMsg);
  }
}

int32_t vnodeProcessSyncCtrlMsg(SVnode *pVnode, SRpcMsg *pMsg, SRpcMsg **pRsp) {
  int32_t         code = 0;
  const STraceId *trace = &pMsg->info.traceId;

  if (!syncEnvIsStart()) {
    vGError("vgId:%d, msg:%p failed to process since sync env not start", pVnode->config.vgId, pMsg);
    terrno = TSDB_CODE_APP_ERROR;
    return -1;
  }

  SSyncNode *pSyncNode = syncNodeAcquire(pVnode->sync);
  if (pSyncNode == NULL) {
    vGError("vgId:%d, msg:%p failed to process since invalid sync node", pVnode->config.vgId, pMsg);
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }

  vGTrace("vgId:%d, sync msg:%p will be processed, type:%s", pVnode->config.vgId, pMsg, TMSG_INFO(pMsg->msgType));

  if (pMsg->msgType == TDMT_SYNC_HEARTBEAT) {
    SyncHeartbeat *pSyncMsg = syncHeartbeatFromRpcMsg2(pMsg);
    code = syncNodeOnHeartbeat(pSyncNode, pSyncMsg);
    syncHeartbeatDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_HEARTBEAT_REPLY) {
    SyncHeartbeatReply *pSyncMsg = syncHeartbeatReplyFromRpcMsg2(pMsg);
    code = syncNodeOnHeartbeatReply(pSyncNode, pSyncMsg);
    syncHeartbeatReplyDestroy(pSyncMsg);

  } else {
    vGError("vgId:%d, msg:%p failed to process since error msg type:%d", pVnode->config.vgId, pMsg, pMsg->msgType);
    code = -1;
  }

  vTrace("vgId:%d, sync msg:%p is processed, type:%s code:0x%x", pVnode->config.vgId, pMsg, TMSG_INFO(pMsg->msgType),
         code);
  syncNodeRelease(pSyncNode);
  if (code != 0 && terrno == 0) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
  }
  return code;
}

int32_t vnodeProcessSyncMsg(SVnode *pVnode, SRpcMsg *pMsg, SRpcMsg **pRsp) {
  int32_t         code = 0;
  const STraceId *trace = &pMsg->info.traceId;

  if (!syncEnvIsStart()) {
    vGError("vgId:%d, msg:%p failed to process since sync env not start", pVnode->config.vgId, pMsg);
    terrno = TSDB_CODE_APP_ERROR;
    return -1;
  }

  SSyncNode *pSyncNode = syncNodeAcquire(pVnode->sync);
  if (pSyncNode == NULL) {
    vGError("vgId:%d, msg:%p failed to process since invalid sync node", pVnode->config.vgId, pMsg);
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }

  vGTrace("vgId:%d, sync msg:%p will be processed, type:%s", pVnode->config.vgId, pMsg, TMSG_INFO(pMsg->msgType));

  if (pMsg->msgType == TDMT_SYNC_TIMEOUT) {
    SyncTimeout *pSyncMsg = syncTimeoutFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnTimer(pSyncNode, pSyncMsg);
    syncTimeoutDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_PING) {
    SyncPing *pSyncMsg = syncPingFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnPing(pSyncNode, pSyncMsg);
    syncPingDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_PING_REPLY) {
    SyncPingReply *pSyncMsg = syncPingReplyFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnPingReply(pSyncNode, pSyncMsg);
    syncPingReplyDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_CLIENT_REQUEST) {
    SyncClientRequest *pSyncMsg = syncClientRequestFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnClientRequest(pSyncNode, pSyncMsg, NULL);
    syncClientRequestDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_REQUEST_VOTE) {
    SyncRequestVote *pSyncMsg = syncRequestVoteFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnRequestVote(pSyncNode, pSyncMsg);
    syncRequestVoteDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_REQUEST_VOTE_REPLY) {
    SyncRequestVoteReply *pSyncMsg = syncRequestVoteReplyFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnRequestVoteReply(pSyncNode, pSyncMsg);
    syncRequestVoteReplyDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_APPEND_ENTRIES) {
    SyncAppendEntries *pSyncMsg = syncAppendEntriesFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnAppendEntries(pSyncNode, pSyncMsg);
    syncAppendEntriesDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_APPEND_ENTRIES_REPLY) {
    SyncAppendEntriesReply *pSyncMsg = syncAppendEntriesReplyFromRpcMsg2(pMsg);
    ASSERT(pSyncMsg != NULL);
    code = syncNodeOnAppendEntriesReply(pSyncNode, pSyncMsg);
    syncAppendEntriesReplyDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_SNAPSHOT_SEND) {
    SyncSnapshotSend *pSyncMsg = syncSnapshotSendFromRpcMsg2(pMsg);
    code = syncNodeOnSnapshot(pSyncNode, pSyncMsg);
    syncSnapshotSendDestroy(pSyncMsg);

  } else if (pMsg->msgType == TDMT_SYNC_SNAPSHOT_RSP) {
    SyncSnapshotRsp *pSyncMsg = syncSnapshotRspFromRpcMsg2(pMsg);
    code = syncNodeOnSnapshotReply(pSyncNode, pSyncMsg);
    syncSnapshotRspDestroy(pSyncMsg);

  } else {
    vGError("vgId:%d, msg:%p failed to process since error msg type:%d", pVnode->config.vgId, pMsg, pMsg->msgType);
    code = -1;
  }

  vTrace("vgId:%d, sync msg:%p is processed, type:%s code:0x%x", pVnode->config.vgId, pMsg, TMSG_INFO(pMsg->msgType),
         code);
  syncNodeRelease(pSyncNode);
  if (code != 0 && terrno == 0) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
  }
  return code;
}

static int32_t vnodeSyncEqCtrlMsg(const SMsgCb *msgcb, SRpcMsg *pMsg) {
  if (msgcb == NULL) {
    return -1;
  }

  int32_t code = tmsgPutToQueue(msgcb, SYNC_CTRL_QUEUE, pMsg);
  if (code != 0) {
    rpcFreeCont(pMsg->pCont);
    pMsg->pCont = NULL;
  }
  return code;
}

static int32_t vnodeSyncEqMsg(const SMsgCb *msgcb, SRpcMsg *pMsg) {
  if (msgcb == NULL) {
    return -1;
  }

  int32_t code = tmsgPutToQueue(msgcb, SYNC_QUEUE, pMsg);
  if (code != 0) {
    rpcFreeCont(pMsg->pCont);
    pMsg->pCont = NULL;
  }
  return code;
}

static int32_t vnodeSyncSendMsg(const SEpSet *pEpSet, SRpcMsg *pMsg) {
  int32_t code = tmsgSendReq(pEpSet, pMsg);
  if (code != 0) {
    rpcFreeCont(pMsg->pCont);
    pMsg->pCont = NULL;
  }
  return code;
}

static int32_t vnodeSyncGetSnapshot(SSyncFSM *pFsm, SSnapshot *pSnapshot) {
  vnodeGetSnapshot(pFsm->data, pSnapshot);
  return 0;
}

static void vnodeSyncReconfig(struct SSyncFSM *pFsm, const SRpcMsg *pMsg, SReConfigCbMeta *cbMeta) {}

static void vnodeSyncCommitMsg(SSyncFSM *pFsm, const SRpcMsg *pMsg, SFsmCbMeta cbMeta) {
  if (cbMeta.isWeak == 0) {
    SVnode *pVnode = pFsm->data;

    if (cbMeta.code == 0) {
      SRpcMsg rpcMsg = {.msgType = pMsg->msgType, .contLen = pMsg->contLen};
      rpcMsg.pCont = rpcMallocCont(rpcMsg.contLen);
      memcpy(rpcMsg.pCont, pMsg->pCont, pMsg->contLen);
      syncGetAndDelRespRpc(pVnode->sync, cbMeta.seqNum, &rpcMsg.info);
      rpcMsg.info.conn.applyIndex = cbMeta.index;
      rpcMsg.info.conn.applyTerm = cbMeta.term;

      vInfo("vgId:%d, commit-cb is excuted, fsm:%p, index:%" PRId64 ", term:%" PRIu64 ", msg-index:%" PRId64
            ", weak:%d, code:%d, state:%d %s, type:%s",
            syncGetVgId(pVnode->sync), pFsm, cbMeta.index, cbMeta.term, rpcMsg.info.conn.applyIndex, cbMeta.isWeak,
            cbMeta.code, cbMeta.state, syncUtilState2String(cbMeta.state), TMSG_INFO(pMsg->msgType));

      tmsgPutToQueue(&pVnode->msgCb, APPLY_QUEUE, &rpcMsg);
    } else {
      SRpcMsg rsp = {.code = cbMeta.code, .info = pMsg->info};
      vError("vgId:%d, commit-cb execute error, type:%s, index:%" PRId64 ", error:0x%x %s", syncGetVgId(pVnode->sync),
             TMSG_INFO(pMsg->msgType), cbMeta.index, cbMeta.code, tstrerror(cbMeta.code));
      if (rsp.info.handle != NULL) {
        tmsgSendRsp(&rsp);
      }
    }
  }
}

static void vnodeSyncPreCommitMsg(SSyncFSM *pFsm, const SRpcMsg *pMsg, SFsmCbMeta cbMeta) {
  if (cbMeta.isWeak == 1) {
    SVnode *pVnode = pFsm->data;
    vTrace("vgId:%d, pre-commit-cb is excuted, fsm:%p, index:%" PRId64 ", weak:%d, code:%d, state:%d %s, type:%s",
           syncGetVgId(pVnode->sync), pFsm, cbMeta.index, cbMeta.isWeak, cbMeta.code, cbMeta.state,
           syncUtilState2String(cbMeta.state), TMSG_INFO(pMsg->msgType));

    if (cbMeta.code == 0) {
      SRpcMsg rpcMsg = {.msgType = pMsg->msgType, .contLen = pMsg->contLen};
      rpcMsg.pCont = rpcMallocCont(rpcMsg.contLen);
      memcpy(rpcMsg.pCont, pMsg->pCont, pMsg->contLen);
      syncGetAndDelRespRpc(pVnode->sync, cbMeta.seqNum, &rpcMsg.info);
      rpcMsg.info.conn.applyIndex = cbMeta.index;
      rpcMsg.info.conn.applyTerm = cbMeta.term;
      tmsgPutToQueue(&pVnode->msgCb, APPLY_QUEUE, &rpcMsg);
    } else {
      SRpcMsg rsp = {.code = cbMeta.code, .info = pMsg->info};
      vError("vgId:%d, pre-commit-cb execute error, type:%s, error:0x%x %s", syncGetVgId(pVnode->sync),
             TMSG_INFO(pMsg->msgType), cbMeta.code, tstrerror(cbMeta.code));
      if (rsp.info.handle != NULL) {
        tmsgSendRsp(&rsp);
      }
    }
  }
}

static void vnodeSyncRollBackMsg(SSyncFSM *pFsm, const SRpcMsg *pMsg, SFsmCbMeta cbMeta) {
  SVnode *pVnode = pFsm->data;
  vTrace("vgId:%d, rollback-cb is excuted, fsm:%p, index:%" PRId64 ", weak:%d, code:%d, state:%d %s, type:%s",
         syncGetVgId(pVnode->sync), pFsm, cbMeta.index, cbMeta.isWeak, cbMeta.code, cbMeta.state,
         syncUtilState2String(cbMeta.state), TMSG_INFO(pMsg->msgType));
}

#define USE_TSDB_SNAPSHOT

static int32_t vnodeSnapshotStartRead(struct SSyncFSM *pFsm, void *pParam, void **ppReader) {
#ifdef USE_TSDB_SNAPSHOT
  SVnode         *pVnode = pFsm->data;
  SSnapshotParam *pSnapshotParam = pParam;
  int32_t code = vnodeSnapReaderOpen(pVnode, pSnapshotParam->start, pSnapshotParam->end, (SVSnapReader **)ppReader);
  return code;
#else
  *ppReader = taosMemoryMalloc(32);
  return 0;
#endif
}

static int32_t vnodeSnapshotStopRead(struct SSyncFSM *pFsm, void *pReader) {
#ifdef USE_TSDB_SNAPSHOT
  SVnode *pVnode = pFsm->data;
  int32_t code = vnodeSnapReaderClose(pReader);
  return code;
#else
  taosMemoryFree(pReader);
  return 0;
#endif
}

static int32_t vnodeSnapshotDoRead(struct SSyncFSM *pFsm, void *pReader, void **ppBuf, int32_t *len) {
#ifdef USE_TSDB_SNAPSHOT
  SVnode *pVnode = pFsm->data;
  int32_t code = vnodeSnapRead(pReader, (uint8_t **)ppBuf, len);
  return code;
#else
  static int32_t times = 0;
  if (times++ < 5) {
    *len = 64;
    *ppBuf = taosMemoryMalloc(*len);
    snprintf(*ppBuf, *len, "snapshot block %d", times);
  } else {
    *len = 0;
    *ppBuf = NULL;
  }
  return 0;
#endif
}

static int32_t vnodeSnapshotStartWrite(struct SSyncFSM *pFsm, void *pParam, void **ppWriter) {
#ifdef USE_TSDB_SNAPSHOT
  SVnode         *pVnode = pFsm->data;
  SSnapshotParam *pSnapshotParam = pParam;

  do {
    int32_t itemSize = tmsgGetQueueSize(&pVnode->msgCb, pVnode->config.vgId, APPLY_QUEUE);
    if (itemSize == 0) {
      vInfo("vgId:%d, start write vnode snapshot since apply queue is empty", pVnode->config.vgId);
      break;
    } else {
      vInfo("vgId:%d, write vnode snapshot later since %d items in apply queue", pVnode->config.vgId, itemSize);
      taosMsleep(10);
    }
  } while (true);

  int32_t code = vnodeSnapWriterOpen(pVnode, pSnapshotParam->start, pSnapshotParam->end, (SVSnapWriter **)ppWriter);
  return code;
#else
  *ppWriter = taosMemoryMalloc(32);
  return 0;
#endif
}

static int32_t vnodeSnapshotStopWrite(struct SSyncFSM *pFsm, void *pWriter, bool isApply, SSnapshot *pSnapshot) {
#ifdef USE_TSDB_SNAPSHOT
  SVnode *pVnode = pFsm->data;
  vInfo("vgId:%d, stop write vnode snapshot, apply:%d, index:%" PRId64 " term:%" PRIu64 " config:%" PRId64,
        pVnode->config.vgId, isApply, pSnapshot->lastApplyIndex, pSnapshot->lastApplyTerm, pSnapshot->lastConfigIndex);

  int32_t code = vnodeSnapWriterClose(pWriter, !isApply, pSnapshot);
  vInfo("vgId:%d, apply vnode snapshot finished, code:0x%x", pVnode->config.vgId, code);
  return code;
#else
  taosMemoryFree(pWriter);
  return 0;
#endif
}

static int32_t vnodeSnapshotDoWrite(struct SSyncFSM *pFsm, void *pWriter, void *pBuf, int32_t len) {
#ifdef USE_TSDB_SNAPSHOT
  SVnode *pVnode = pFsm->data;
  vDebug("vgId:%d, continue write vnode snapshot, len:%d", pVnode->config.vgId, len);
  int32_t code = vnodeSnapWrite(pWriter, pBuf, len);
  vDebug("vgId:%d, continue write vnode snapshot finished, len:%d", pVnode->config.vgId, len);
  return code;
#else
  return 0;
#endif
}

static void vnodeLeaderTransfer(struct SSyncFSM *pFsm, const SRpcMsg *pMsg, SFsmCbMeta cbMeta) {}

static void vnodeRestoreFinish(struct SSyncFSM *pFsm) {
  SVnode *pVnode = pFsm->data;

  do {
    int32_t itemSize = tmsgGetQueueSize(&pVnode->msgCb, pVnode->config.vgId, APPLY_QUEUE);
    if (itemSize == 0) {
      vInfo("vgId:%d, apply queue is empty, restore finish", pVnode->config.vgId);
      break;
    } else {
      vInfo("vgId:%d, restore not finish since %d items in apply queue", pVnode->config.vgId, itemSize);
      taosMsleep(10);
    }
  } while (true);

  walApplyVer(pVnode->pWal, pVnode->state.applied);

  pVnode->restored = true;
  vDebug("vgId:%d, sync restore finished", pVnode->config.vgId);
}

static void vnodeBecomeFollower(struct SSyncFSM *pFsm) {
  SVnode *pVnode = pFsm->data;
  vDebug("vgId:%d, become follower", pVnode->config.vgId);

  // clear old leader resource
  taosThreadMutexLock(&pVnode->lock);
  if (pVnode->blocked) {
    pVnode->blocked = false;
    vDebug("vgId:%d, become follower and post block", pVnode->config.vgId);
    tsem_post(&pVnode->syncSem);
  }
  taosThreadMutexUnlock(&pVnode->lock);
}

static void vnodeBecomeLeader(struct SSyncFSM *pFsm) {
  SVnode *pVnode = pFsm->data;
  vDebug("vgId:%d, become leader", pVnode->config.vgId);

#if 0
  taosThreadMutexLock(&pVnode->lock);
  if (pVnode->blocked) {
    pVnode->blocked = false;
    tsem_post(&pVnode->syncSem);
  }
  taosThreadMutexUnlock(&pVnode->lock);
#endif
}

static SSyncFSM *vnodeSyncMakeFsm(SVnode *pVnode) {
  SSyncFSM *pFsm = taosMemoryCalloc(1, sizeof(SSyncFSM));
  pFsm->data = pVnode;
  pFsm->FpCommitCb = vnodeSyncCommitMsg;
  pFsm->FpPreCommitCb = vnodeSyncPreCommitMsg;
  pFsm->FpRollBackCb = vnodeSyncRollBackMsg;
  pFsm->FpGetSnapshotInfo = vnodeSyncGetSnapshot;
  pFsm->FpRestoreFinishCb = vnodeRestoreFinish;
  pFsm->FpLeaderTransferCb = vnodeLeaderTransfer;
  pFsm->FpBecomeLeaderCb = vnodeBecomeLeader;
  pFsm->FpBecomeFollowerCb = vnodeBecomeFollower;
  pFsm->FpReConfigCb = vnodeSyncReconfig;
  pFsm->FpSnapshotStartRead = vnodeSnapshotStartRead;
  pFsm->FpSnapshotStopRead = vnodeSnapshotStopRead;
  pFsm->FpSnapshotDoRead = vnodeSnapshotDoRead;
  pFsm->FpSnapshotStartWrite = vnodeSnapshotStartWrite;
  pFsm->FpSnapshotStopWrite = vnodeSnapshotStopWrite;
  pFsm->FpSnapshotDoWrite = vnodeSnapshotDoWrite;

  return pFsm;
}

int32_t vnodeSyncOpen(SVnode *pVnode, char *path) {
  SSyncInfo syncInfo = {
      .snapshotStrategy = SYNC_STRATEGY_WAL_FIRST,
      .batchSize = 1,
      .vgId = pVnode->config.vgId,
      .syncCfg = pVnode->config.syncCfg,
      .pWal = pVnode->pWal,
      .msgcb = NULL,
      .FpSendMsg = vnodeSyncSendMsg,
      .FpEqMsg = vnodeSyncEqMsg,
      .FpEqCtrlMsg = vnodeSyncEqCtrlMsg,
  };

  snprintf(syncInfo.path, sizeof(syncInfo.path), "%s%ssync", path, TD_DIRSEP);
  syncInfo.pFsm = vnodeSyncMakeFsm(pVnode);

  SSyncCfg *pCfg = &syncInfo.syncCfg;
  vInfo("vgId:%d, start to open sync, replica:%d selfIndex:%d", pVnode->config.vgId, pCfg->replicaNum, pCfg->myIndex);
  for (int32_t i = 0; i < pCfg->replicaNum; ++i) {
    SNodeInfo *pNode = &pCfg->nodeInfo[i];
    vInfo("vgId:%d, index:%d ep:%s:%u", pVnode->config.vgId, i, pNode->nodeFqdn, pNode->nodePort);
  }

  pVnode->sync = syncOpen(&syncInfo);
  if (pVnode->sync <= 0) {
    vError("vgId:%d, failed to open sync since %s", pVnode->config.vgId, terrstr());
    return -1;
  }

  setPingTimerMS(pVnode->sync, 5000);
  setElectTimerMS(pVnode->sync, 4000);
  setHeartbeatTimerMS(pVnode->sync, 700);
  return 0;
}

void vnodeSyncStart(SVnode *pVnode) {
  vDebug("vgId:%d, start sync", pVnode->config.vgId);
  syncSetMsgCb(pVnode->sync, &pVnode->msgCb);
  syncStart(pVnode->sync);
}

void vnodeSyncClose(SVnode *pVnode) {
  vDebug("vgId:%d, close sync", pVnode->config.vgId);
  syncStop(pVnode->sync);
}

bool vnodeIsRoleLeader(SVnode *pVnode) { return syncGetMyRole(pVnode->sync) == TAOS_SYNC_STATE_LEADER; }

bool vnodeIsLeader(SVnode *pVnode) {
  if (!syncIsReady(pVnode->sync)) {
    vDebug("vgId:%d, vnode not ready, state:%s, restore:%d", pVnode->config.vgId, syncGetMyRoleStr(pVnode->sync),
           syncRestoreFinish(pVnode->sync));
    return false;
  }

  if (!pVnode->restored) {
    vDebug("vgId:%d, vnode not restored", pVnode->config.vgId);
    terrno = TSDB_CODE_APP_NOT_READY;
    return false;
  }

  return true;
}
