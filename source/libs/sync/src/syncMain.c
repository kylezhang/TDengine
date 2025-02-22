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

#include "sync.h"
#include "syncAppendEntries.h"
#include "syncAppendEntriesReply.h"
#include "syncCommit.h"
#include "syncElection.h"
#include "syncEnv.h"
#include "syncIndexMgr.h"
#include "syncInt.h"
#include "syncMessage.h"
#include "syncRaftCfg.h"
#include "syncRaftLog.h"
#include "syncRaftStore.h"
#include "syncReplication.h"
#include "syncRequestVote.h"
#include "syncRequestVoteReply.h"
#include "syncRespMgr.h"
#include "syncSnapshot.h"
#include "syncTimeout.h"
#include "syncUtil.h"
#include "syncVoteMgr.h"
#include "tref.h"

bool gRaftDetailLog = false;

static int32_t tsNodeRefId = -1;

// ------ local funciton ---------
// enqueue message ----
static void    syncNodeEqPingTimer(void* param, void* tmrId);
static void    syncNodeEqElectTimer(void* param, void* tmrId);
static void    syncNodeEqHeartbeatTimer(void* param, void* tmrId);
static int32_t syncNodeEqNoop(SSyncNode* ths);
static int32_t syncNodeAppendNoop(SSyncNode* ths);
static void    syncNodeEqPeerHeartbeatTimer(void* param, void* tmrId);

// process message ----
int32_t syncNodeOnPing(SSyncNode* ths, SyncPing* pMsg);
int32_t syncNodeOnPingReply(SSyncNode* ths, SyncPingReply* pMsg);

// ---------------------------------
static void syncNodeFreeCb(void* param) {
  syncNodeClose(param);
  param = NULL;
}

int32_t syncInit() {
  int32_t ret = 0;

  if (!syncEnvIsStart()) {
    tsNodeRefId = taosOpenRef(200, syncNodeFreeCb);
    if (tsNodeRefId < 0) {
      sError("failed to init node ref");
      syncCleanUp();
      ret = -1;
    } else {
      sDebug("sync rsetId:%d is open", tsNodeRefId);
      ret = syncEnvStart();
    }
  }

  return ret;
}

void syncCleanUp() {
  int32_t ret = syncEnvStop();
  ASSERT(ret == 0);

  if (tsNodeRefId != -1) {
    sDebug("sync rsetId:%d is closed", tsNodeRefId);
    taosCloseRef(tsNodeRefId);
    tsNodeRefId = -1;
  }
}

int64_t syncOpen(SSyncInfo* pSyncInfo) {
  SSyncNode* pSyncNode = syncNodeOpen(pSyncInfo);
  if (pSyncNode == NULL) {
    sError("vgId:%d, failed to open sync node", pSyncInfo->vgId);
    return -1;
  }

  pSyncNode->rid = taosAddRef(tsNodeRefId, pSyncNode);
  if (pSyncNode->rid < 0) {
    syncNodeClose(pSyncNode);
    pSyncNode = NULL;
    return -1;
  }

  sDebug("vgId:%d, sync rid:%" PRId64 " is added to rsetId:%d", pSyncInfo->vgId, pSyncNode->rid, tsNodeRefId);
  return pSyncNode->rid;
}

void syncStart(int64_t rid) {
  SSyncNode* pSyncNode = taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return;
  }

  if (pSyncNode->pRaftCfg->isStandBy) {
    syncNodeStartStandBy(pSyncNode);
  } else {
    syncNodeStart(pSyncNode);
  }

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

void syncStartNormal(int64_t rid) {
  SSyncNode* pSyncNode = taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return;
  }
  syncNodeStart(pSyncNode);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

void syncStartStandBy(int64_t rid) {
  SSyncNode* pSyncNode = taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return;
  }
  syncNodeStartStandBy(pSyncNode);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

void syncStop(int64_t rid) {
  SSyncNode* pSyncNode = taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) return;
  int32_t vgId = pSyncNode->vgId;
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);

  taosRemoveRef(tsNodeRefId, rid);
  sDebug("vgId:%d, sync rid:%" PRId64 " is removed from rsetId:%d", vgId, rid, tsNodeRefId);
}

int32_t syncSetStandby(int64_t rid) {
  SSyncNode* pSyncNode = taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    sError("failed to set standby since accquire ref error, rid:%" PRId64, rid);
    return -1;
  }

  if (pSyncNode->state != TAOS_SYNC_STATE_FOLLOWER) {
    if (pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
      terrno = TSDB_CODE_SYN_IS_LEADER;
    } else {
      terrno = TSDB_CODE_SYN_STANDBY_NOT_READY;
    }
    sError("failed to set standby since it is not follower, state:%s rid:%" PRId64, syncStr(pSyncNode->state), rid);
    taosReleaseRef(tsNodeRefId, pSyncNode->rid);
    return -1;
  }

  // state change
  pSyncNode->state = TAOS_SYNC_STATE_FOLLOWER;
  syncNodeStopHeartbeatTimer(pSyncNode);

  // reset elect timer, long enough
  int32_t electMS = TIMER_MAX_MS;
  int32_t ret = syncNodeRestartElectTimer(pSyncNode, electMS);
  ASSERT(ret == 0);

  pSyncNode->pRaftCfg->isStandBy = 1;
  raftCfgPersist(pSyncNode->pRaftCfg);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  sInfo("vgId:%d, set to standby", pSyncNode->vgId);
  return 0;
}

bool syncNodeCheckNewConfig(SSyncNode* pSyncNode, const SSyncCfg* pNewCfg) {
  bool IamInNew = syncNodeInConfig(pSyncNode, pNewCfg);
  if (!IamInNew) {
    return false;
  }

  if (pNewCfg->replicaNum > pSyncNode->replicaNum + 1) {
    return false;
  }

  if (pNewCfg->replicaNum < pSyncNode->replicaNum - 1) {
    return false;
  }

  return true;
}

int32_t syncReconfigBuild(int64_t rid, const SSyncCfg* pNewCfg, SRpcMsg* pRpcMsg) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);
  int32_t ret = 0;

  if (!syncNodeCheckNewConfig(pSyncNode, pNewCfg)) {
    taosReleaseRef(tsNodeRefId, pSyncNode->rid);
    terrno = TSDB_CODE_SYN_NEW_CONFIG_ERROR;
    sError("invalid new config. vgId:%d", pSyncNode->vgId);
    return -1;
  }

  char* newconfig = syncCfg2Str((SSyncCfg*)pNewCfg);
  pRpcMsg->msgType = TDMT_SYNC_CONFIG_CHANGE;
  pRpcMsg->info.noResp = 1;
  pRpcMsg->contLen = strlen(newconfig) + 1;
  pRpcMsg->pCont = rpcMallocCont(pRpcMsg->contLen);
  snprintf(pRpcMsg->pCont, pRpcMsg->contLen, "%s", newconfig);
  taosMemoryFree(newconfig);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
}

int32_t syncReconfig(int64_t rid, SSyncCfg* pNewCfg) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  if (!syncNodeCheckNewConfig(pSyncNode, pNewCfg)) {
    taosReleaseRef(tsNodeRefId, pSyncNode->rid);
    terrno = TSDB_CODE_SYN_NEW_CONFIG_ERROR;
    sError("invalid new config. vgId:%d", pSyncNode->vgId);
    return -1;
  }

#if 0
  char*   newconfig = syncCfg2Str((SSyncCfg*)pNewCfg);
  int32_t ret = 0;

  SRpcMsg rpcMsg = {0};
  rpcMsg.msgType = TDMT_SYNC_CONFIG_CHANGE;
  rpcMsg.info.noResp = 1;
  rpcMsg.contLen = strlen(newconfig) + 1;
  rpcMsg.pCont = rpcMallocCont(rpcMsg.contLen);
  snprintf(rpcMsg.pCont, rpcMsg.contLen, "%s", newconfig);
  taosMemoryFree(newconfig);
  ret = syncNodePropose(pSyncNode, &rpcMsg, false);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
#else
  syncNodeUpdateNewConfigIndex(pSyncNode, pNewCfg);
  syncNodeDoConfigChange(pSyncNode, pNewCfg, SYNC_INDEX_INVALID);
  if (pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
    syncNodeStopHeartbeatTimer(pSyncNode);

    for (int32_t i = 0; i < TSDB_MAX_REPLICA; ++i) {
      syncHbTimerInit(pSyncNode, &(pSyncNode->peerHeartbeatTimerArr[i]), (pSyncNode->replicasId)[i]);
    }

    syncNodeStartHeartbeatTimer(pSyncNode);

    syncNodeReplicate(pSyncNode);
  }
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return 0;
#endif
}

int32_t syncLeaderTransfer(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  int32_t ret = syncNodeLeaderTransfer(pSyncNode);
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
}

int32_t syncLeaderTransferTo(int64_t rid, SNodeInfo newLeader) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  int32_t ret = syncNodeLeaderTransferTo(pSyncNode, newLeader);
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
}

SyncIndex syncMinMatchIndex(SSyncNode* pSyncNode) {
  SyncIndex minMatchIndex = SYNC_INDEX_INVALID;

  if (pSyncNode->peersNum > 0) {
    minMatchIndex = syncIndexMgrGetIndex(pSyncNode->pMatchIndex, &(pSyncNode->peersId[0]));
  }

  for (int32_t i = 1; i < pSyncNode->peersNum; ++i) {
    SyncIndex matchIndex = syncIndexMgrGetIndex(pSyncNode->pMatchIndex, &(pSyncNode->peersId[i]));
    if (matchIndex < minMatchIndex) {
      minMatchIndex = matchIndex;
    }
  }
  return minMatchIndex;
}

char* syncNodePeerState2Str(const SSyncNode* pSyncNode) {
  int32_t len = 128;
  int32_t useLen = 0;
  int32_t leftLen = len - useLen;
  char*   pStr = taosMemoryMalloc(len);
  memset(pStr, 0, len);

  char*   p = pStr;
  int32_t use = snprintf(p, leftLen, "{");
  useLen += use;
  leftLen -= use;

  for (int32_t i = 0; i < pSyncNode->replicaNum; ++i) {
    SPeerState* pState = syncNodeGetPeerState((SSyncNode*)pSyncNode, &(pSyncNode->replicasId[i]));
    if (pState == NULL) {
      sError("vgId:%d, replica maybe dropped", pSyncNode->vgId);
      break;
    }

    p = pStr + useLen;
    use = snprintf(p, leftLen, "%d:%" PRId64 " ,%" PRId64, i, pState->lastSendIndex, pState->lastSendTime);
    useLen += use;
    leftLen -= use;
  }

  p = pStr + useLen;
  use = snprintf(p, leftLen, "}");
  useLen += use;
  leftLen -= use;

  // sTrace("vgId:%d, ------------------ syncNodePeerState2Str:%s", pSyncNode->vgId, pStr);

  return pStr;
}

int32_t syncBeginSnapshot(int64_t rid, int64_t lastApplyIndex) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);
  int32_t code = 0;

  if (syncNodeIsMnode(pSyncNode)) {
    // mnode
    int64_t logRetention = SYNC_MNODE_LOG_RETENTION;

    SyncIndex beginIndex = pSyncNode->pLogStore->syncLogBeginIndex(pSyncNode->pLogStore);
    SyncIndex endIndex = pSyncNode->pLogStore->syncLogEndIndex(pSyncNode->pLogStore);
    int64_t   logNum = endIndex - beginIndex;
    bool      isEmpty = pSyncNode->pLogStore->syncLogIsEmpty(pSyncNode->pLogStore);

    if (isEmpty || (!isEmpty && logNum < logRetention)) {
      char logBuf[256];
      snprintf(logBuf, sizeof(logBuf),
               "new-snapshot-index:%" PRId64 ", log-num:%" PRId64 ", empty:%d, do not delete wal", lastApplyIndex,
               logNum, isEmpty);
      syncNodeEventLog(pSyncNode, logBuf);

      taosReleaseRef(tsNodeRefId, pSyncNode->rid);
      return 0;
    }

    goto _DEL_WAL;

  } else {
    // vnode
    if (pSyncNode->replicaNum > 1) {
      // multi replicas

      if (pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
        pSyncNode->minMatchIndex = syncMinMatchIndex(pSyncNode);

        for (int32_t i = 0; i < pSyncNode->peersNum; ++i) {
          int64_t matchIndex = syncIndexMgrGetIndex(pSyncNode->pMatchIndex, &(pSyncNode->peersId[i]));
          if (lastApplyIndex > matchIndex) {
            do {
              char     host[64];
              uint16_t port;
              syncUtilU642Addr(pSyncNode->peersId[i].addr, host, sizeof(host), &port);
              char logBuf[256];
              snprintf(logBuf, sizeof(logBuf),
                       "new-snapshot-index:%" PRId64 " is greater than match-index:%" PRId64
                       " of %s:%d, do not delete wal",
                       lastApplyIndex, matchIndex, host, port);
              syncNodeEventLog(pSyncNode, logBuf);
            } while (0);

            taosReleaseRef(tsNodeRefId, pSyncNode->rid);
            return 0;
          }
        }

      } else if (pSyncNode->state == TAOS_SYNC_STATE_FOLLOWER) {
        if (lastApplyIndex > pSyncNode->minMatchIndex) {
          char logBuf[256];
          snprintf(logBuf, sizeof(logBuf),
                   "new-snapshot-index:%" PRId64 " is greater than min-match-index:%" PRId64 ", do not delete wal",
                   lastApplyIndex, pSyncNode->minMatchIndex);
          syncNodeEventLog(pSyncNode, logBuf);

          taosReleaseRef(tsNodeRefId, pSyncNode->rid);
          return 0;
        }

      } else if (pSyncNode->state == TAOS_SYNC_STATE_CANDIDATE) {
        char logBuf[256];
        snprintf(logBuf, sizeof(logBuf), "new-snapshot-index:%" PRId64 " candidate, do not delete wal", lastApplyIndex);
        syncNodeEventLog(pSyncNode, logBuf);

        taosReleaseRef(tsNodeRefId, pSyncNode->rid);
        return 0;

      } else {
        char logBuf[256];
        snprintf(logBuf, sizeof(logBuf), "new-snapshot-index:%" PRId64 " unknown state, do not delete wal",
                 lastApplyIndex);
        syncNodeEventLog(pSyncNode, logBuf);

        taosReleaseRef(tsNodeRefId, pSyncNode->rid);
        return 0;
      }

      goto _DEL_WAL;

    } else {
      // one replica

      goto _DEL_WAL;
    }
  }

_DEL_WAL:

  do {
    SyncIndex snapshottingIndex = atomic_load_64(&pSyncNode->snapshottingIndex);

    if (snapshottingIndex == SYNC_INDEX_INVALID) {
      atomic_store_64(&pSyncNode->snapshottingIndex, lastApplyIndex);
      pSyncNode->snapshottingTime = taosGetTimestampMs();

      SSyncLogStoreData* pData = pSyncNode->pLogStore->data;
      code = walBeginSnapshot(pData->pWal, lastApplyIndex);
      if (code == 0) {
        char logBuf[256];
        snprintf(logBuf, sizeof(logBuf), "wal snapshot begin, index:%" PRId64 ", last apply index:%" PRId64,
                 pSyncNode->snapshottingIndex, lastApplyIndex);
        syncNodeEventLog(pSyncNode, logBuf);

      } else {
        char logBuf[256];
        snprintf(logBuf, sizeof(logBuf),
                 "wal snapshot begin error since:%s, index:%" PRId64 ", last apply index:%" PRId64, terrstr(terrno),
                 pSyncNode->snapshottingIndex, lastApplyIndex);
        syncNodeErrorLog(pSyncNode, logBuf);

        atomic_store_64(&pSyncNode->snapshottingIndex, SYNC_INDEX_INVALID);
      }

    } else {
      char logBuf[256];
      snprintf(logBuf, sizeof(logBuf),
               "snapshotting for %" PRId64 ", do not delete wal for new-snapshot-index:%" PRId64, snapshottingIndex,
               lastApplyIndex);
      syncNodeEventLog(pSyncNode, logBuf);
    }
  } while (0);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return code;
}

int32_t syncEndSnapshot(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  int32_t code = 0;
  if (atomic_load_64(&pSyncNode->snapshottingIndex) != SYNC_INDEX_INVALID) {
    SSyncLogStoreData* pData = pSyncNode->pLogStore->data;
    code = walEndSnapshot(pData->pWal);
    if (code != 0) {
      sError("vgId:%d, wal snapshot end error since:%s", pSyncNode->vgId, terrstr(terrno));

      taosReleaseRef(tsNodeRefId, pSyncNode->rid);
      return -1;
    } else {
      do {
        char logBuf[256];
        snprintf(logBuf, sizeof(logBuf), "wal snapshot end, index:%" PRId64,
                 atomic_load_64(&pSyncNode->snapshottingIndex));
        syncNodeEventLog(pSyncNode, logBuf);
      } while (0);

      atomic_store_64(&pSyncNode->snapshottingIndex, SYNC_INDEX_INVALID);
    }
  }

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return code;
}

int32_t syncNodeLeaderTransfer(SSyncNode* pSyncNode) {
  if (pSyncNode->peersNum == 0) {
    sDebug("only one replica, cannot leader transfer");
    terrno = TSDB_CODE_SYN_ONE_REPLICA;
    return -1;
  }

  SNodeInfo newLeader = (pSyncNode->peersNodeInfo)[0];
  int32_t   ret = syncNodeLeaderTransferTo(pSyncNode, newLeader);
  return ret;
}

int32_t syncNodeLeaderTransferTo(SSyncNode* pSyncNode, SNodeInfo newLeader) {
  int32_t ret = 0;

  if (pSyncNode->replicaNum == 1) {
    sDebug("only one replica, cannot leader transfer");
    terrno = TSDB_CODE_SYN_ONE_REPLICA;
    return -1;
  }

  do {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "begin leader transfer to %s:%u", newLeader.nodeFqdn, newLeader.nodePort);
    syncNodeEventLog(pSyncNode, logBuf);
  } while (0);

  SyncLeaderTransfer* pMsg = syncLeaderTransferBuild(pSyncNode->vgId);
  pMsg->newLeaderId.addr = syncUtilAddr2U64(newLeader.nodeFqdn, newLeader.nodePort);
  pMsg->newLeaderId.vgId = pSyncNode->vgId;
  pMsg->newNodeInfo = newLeader;
  ASSERT(pMsg != NULL);
  SRpcMsg rpcMsg = {0};
  syncLeaderTransfer2RpcMsg(pMsg, &rpcMsg);
  syncLeaderTransferDestroy(pMsg);

  ret = syncNodePropose(pSyncNode, &rpcMsg, false);
  return ret;
}

bool syncCanLeaderTransfer(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return false;
  }
  ASSERT(rid == pSyncNode->rid);

  if (pSyncNode->replicaNum == 1) {
    taosReleaseRef(tsNodeRefId, pSyncNode->rid);
    return false;
  }

  if (pSyncNode->state == TAOS_SYNC_STATE_FOLLOWER) {
    taosReleaseRef(tsNodeRefId, pSyncNode->rid);
    return true;
  }

  bool matchOK = true;
  if (pSyncNode->state == TAOS_SYNC_STATE_CANDIDATE || pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
    SyncIndex myCommitIndex = pSyncNode->commitIndex;
    for (int i = 0; i < pSyncNode->peersNum; ++i) {
      SyncIndex peerMatchIndex = syncIndexMgrGetIndex(pSyncNode->pMatchIndex, &(pSyncNode->peersId)[i]);
      if (peerMatchIndex < myCommitIndex) {
        matchOK = false;
      }
    }
  }

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return matchOK;
}

int32_t syncForwardToPeer(int64_t rid, SRpcMsg* pMsg, bool isWeak) {
  int32_t ret = syncPropose(rid, pMsg, isWeak);
  return ret;
}

ESyncState syncGetMyRole(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return TAOS_SYNC_STATE_ERROR;
  }
  ASSERT(rid == pSyncNode->rid);
  ESyncState state = pSyncNode->state;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return state;
}

bool syncIsReady(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return false;
  }
  ASSERT(rid == pSyncNode->rid);
  bool b = (pSyncNode->state == TAOS_SYNC_STATE_LEADER) && pSyncNode->restoreFinish;
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);

  // if false, set error code
  if (false == b) {
    if (pSyncNode->state != TAOS_SYNC_STATE_LEADER) {
      terrno = TSDB_CODE_SYN_NOT_LEADER;
    } else {
      terrno = TSDB_CODE_APP_NOT_READY;
    }
  }
  return b;
}

bool syncIsRestoreFinish(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return false;
  }
  ASSERT(rid == pSyncNode->rid);
  bool b = pSyncNode->restoreFinish;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return b;
}

int32_t syncGetSnapshotByIndex(int64_t rid, SyncIndex index, SSnapshot* pSnapshot) {
  if (index < SYNC_INDEX_BEGIN) {
    return -1;
  }

  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  SSyncRaftEntry* pEntry = NULL;
  int32_t         code = pSyncNode->pLogStore->syncLogGetEntry(pSyncNode->pLogStore, index, &pEntry);
  if (code != 0) {
    if (pEntry != NULL) {
      syncEntryDestory(pEntry);
    }
    taosReleaseRef(tsNodeRefId, pSyncNode->rid);
    return -1;
  }
  ASSERT(pEntry != NULL);

  pSnapshot->data = NULL;
  pSnapshot->lastApplyIndex = index;
  pSnapshot->lastApplyTerm = pEntry->term;
  pSnapshot->lastConfigIndex = syncNodeGetSnapshotConfigIndex(pSyncNode, index);

  syncEntryDestory(pEntry);
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return 0;
}

int32_t syncGetSnapshotMeta(int64_t rid, struct SSnapshotMeta* sMeta) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);
  sMeta->lastConfigIndex = pSyncNode->pRaftCfg->lastConfigIndex;

  sTrace("vgId:%d, get snapshot meta, lastConfigIndex:%" PRId64, pSyncNode->vgId, pSyncNode->pRaftCfg->lastConfigIndex);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return 0;
}

int32_t syncGetSnapshotMetaByIndex(int64_t rid, SyncIndex snapshotIndex, struct SSnapshotMeta* sMeta) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  ASSERT(pSyncNode->pRaftCfg->configIndexCount >= 1);
  SyncIndex lastIndex = (pSyncNode->pRaftCfg->configIndexArr)[0];

  for (int i = 0; i < pSyncNode->pRaftCfg->configIndexCount; ++i) {
    if ((pSyncNode->pRaftCfg->configIndexArr)[i] > lastIndex &&
        (pSyncNode->pRaftCfg->configIndexArr)[i] <= snapshotIndex) {
      lastIndex = (pSyncNode->pRaftCfg->configIndexArr)[i];
    }
  }
  sMeta->lastConfigIndex = lastIndex;
  sTrace("vgId:%d, get snapshot meta by index:%" PRId64 " lcindex:%" PRId64, pSyncNode->vgId, snapshotIndex,
         sMeta->lastConfigIndex);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return 0;
}

SyncIndex syncNodeGetSnapshotConfigIndex(SSyncNode* pSyncNode, SyncIndex snapshotLastApplyIndex) {
  ASSERT(pSyncNode->pRaftCfg->configIndexCount >= 1);
  SyncIndex lastIndex = (pSyncNode->pRaftCfg->configIndexArr)[0];

  for (int i = 0; i < pSyncNode->pRaftCfg->configIndexCount; ++i) {
    if ((pSyncNode->pRaftCfg->configIndexArr)[i] > lastIndex &&
        (pSyncNode->pRaftCfg->configIndexArr)[i] <= snapshotLastApplyIndex) {
      lastIndex = (pSyncNode->pRaftCfg->configIndexArr)[i];
    }
  }
  sTrace("vgId:%d, sync get last config index, index:%" PRId64 " lcindex:%" PRId64, pSyncNode->vgId,
         snapshotLastApplyIndex, lastIndex);

  return lastIndex;
}

const char* syncGetMyRoleStr(int64_t rid) {
  const char* s = syncUtilState2String(syncGetMyRole(rid));
  return s;
}

bool syncRestoreFinish(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return false;
  }
  ASSERT(rid == pSyncNode->rid);
  bool restoreFinish = pSyncNode->restoreFinish;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return restoreFinish;
}

SyncTerm syncGetMyTerm(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return TAOS_SYNC_STATE_ERROR;
  }
  ASSERT(rid == pSyncNode->rid);
  SyncTerm term = pSyncNode->pRaftStore->currentTerm;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return term;
}

SyncIndex syncGetLastIndex(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return SYNC_INDEX_INVALID;
  }
  ASSERT(rid == pSyncNode->rid);
  SyncIndex lastIndex = syncNodeGetLastIndex(pSyncNode);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return lastIndex;
}

SyncIndex syncGetCommitIndex(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return SYNC_INDEX_INVALID;
  }
  ASSERT(rid == pSyncNode->rid);
  SyncIndex cmtIndex = pSyncNode->commitIndex;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return cmtIndex;
}

SyncGroupId syncGetVgId(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return TAOS_SYNC_STATE_ERROR;
  }
  ASSERT(rid == pSyncNode->rid);
  SyncGroupId vgId = pSyncNode->vgId;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return vgId;
}

void syncGetEpSet(int64_t rid, SEpSet* pEpSet) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    memset(pEpSet, 0, sizeof(*pEpSet));
    return;
  }
  ASSERT(rid == pSyncNode->rid);
  pEpSet->numOfEps = 0;
  for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
    snprintf(pEpSet->eps[i].fqdn, sizeof(pEpSet->eps[i].fqdn), "%s", (pSyncNode->pRaftCfg->cfg.nodeInfo)[i].nodeFqdn);
    pEpSet->eps[i].port = (pSyncNode->pRaftCfg->cfg.nodeInfo)[i].nodePort;
    (pEpSet->numOfEps)++;
    sInfo("vgId:%d, sync get epset: index:%d %s:%d", pSyncNode->vgId, i, pEpSet->eps[i].fqdn, pEpSet->eps[i].port);
  }
  pEpSet->inUse = pSyncNode->pRaftCfg->cfg.myIndex;
  sInfo("vgId:%d, sync get epset in-use:%d", pSyncNode->vgId, pEpSet->inUse);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

void syncGetRetryEpSet(int64_t rid, SEpSet* pEpSet) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    memset(pEpSet, 0, sizeof(*pEpSet));
    return;
  }
  ASSERT(rid == pSyncNode->rid);
  pEpSet->numOfEps = 0;
  for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
    snprintf(pEpSet->eps[i].fqdn, sizeof(pEpSet->eps[i].fqdn), "%s", (pSyncNode->pRaftCfg->cfg.nodeInfo)[i].nodeFqdn);
    pEpSet->eps[i].port = (pSyncNode->pRaftCfg->cfg.nodeInfo)[i].nodePort;
    (pEpSet->numOfEps)++;
    sInfo("vgId:%d, sync get retry epset: index:%d %s:%d", pSyncNode->vgId, i, pEpSet->eps[i].fqdn,
          pEpSet->eps[i].port);
  }
  if (pEpSet->numOfEps > 0) {
    pEpSet->inUse = (pSyncNode->pRaftCfg->cfg.myIndex + 1) % pEpSet->numOfEps;
  }
  sInfo("vgId:%d, sync get retry epset in-use:%d", pSyncNode->vgId, pEpSet->inUse);

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

int32_t syncGetRespRpc(int64_t rid, uint64_t index, SRpcMsg* msg) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return TAOS_SYNC_STATE_ERROR;
  }
  ASSERT(rid == pSyncNode->rid);

  SRespStub stub;
  int32_t   ret = syncRespMgrGet(pSyncNode->pSyncRespMgr, index, &stub);
  if (ret == 1) {
    memcpy(msg, &(stub.rpcMsg), sizeof(SRpcMsg));
  }

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
}

int32_t syncGetAndDelRespRpc(int64_t rid, uint64_t index, SRpcHandleInfo* pInfo) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return TAOS_SYNC_STATE_ERROR;
  }
  ASSERT(rid == pSyncNode->rid);

  SRespStub stub;
  int32_t   ret = syncRespMgrGetAndDel(pSyncNode->pSyncRespMgr, index, &stub);
  if (ret == 1) {
    *pInfo = stub.rpcMsg.info;
  }

  sTrace("vgId:%d, get seq:%" PRIu64 " rpc handle:%p", pSyncNode->vgId, index, pInfo->handle);
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
}

void syncSetMsgCb(int64_t rid, const SMsgCb* msgcb) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    sTrace("syncSetQ get pSyncNode is NULL, rid:%" PRId64, rid);
    return;
  }
  ASSERT(rid == pSyncNode->rid);
  pSyncNode->msgcb = msgcb;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

char* sync2SimpleStr(int64_t rid) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    sTrace("syncSetRpc get pSyncNode is NULL, rid:%" PRId64, rid);
    return NULL;
  }
  ASSERT(rid == pSyncNode->rid);
  char* s = syncNode2SimpleStr(pSyncNode);
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);

  return s;
}

void setPingTimerMS(int64_t rid, int32_t pingTimerMS) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return;
  }
  ASSERT(rid == pSyncNode->rid);
  pSyncNode->pingBaseLine = pingTimerMS;
  pSyncNode->pingTimerMS = pingTimerMS;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

void setElectTimerMS(int64_t rid, int32_t electTimerMS) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return;
  }
  ASSERT(rid == pSyncNode->rid);
  pSyncNode->electBaseLine = electTimerMS;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

void setHeartbeatTimerMS(int64_t rid, int32_t hbTimerMS) {
  SSyncNode* pSyncNode = (SSyncNode*)taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    return;
  }
  ASSERT(rid == pSyncNode->rid);
  pSyncNode->hbBaseLine = hbTimerMS;
  pSyncNode->heartbeatTimerMS = hbTimerMS;

  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
}

int32_t syncPropose(int64_t rid, SRpcMsg* pMsg, bool isWeak) {
  SSyncNode* pSyncNode = taosAcquireRef(tsNodeRefId, rid);
  if (pSyncNode == NULL) {
    taosReleaseRef(tsNodeRefId, rid);
    terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
    return -1;
  }
  ASSERT(rid == pSyncNode->rid);

  int32_t ret = syncNodePropose(pSyncNode, pMsg, isWeak);
  taosReleaseRef(tsNodeRefId, pSyncNode->rid);
  return ret;
}

static bool syncNodeBatchOK(SRpcMsg** pMsgPArr, int32_t arrSize) {
  for (int32_t i = 0; i < arrSize; ++i) {
    if (pMsgPArr[i]->msgType == TDMT_SYNC_CONFIG_CHANGE) {
      return false;
    }

    if (pMsgPArr[i]->msgType == TDMT_SYNC_CONFIG_CHANGE_FINISH) {
      return false;
    }
  }

  return true;
}

int32_t syncNodePropose(SSyncNode* pSyncNode, SRpcMsg* pMsg, bool isWeak) {
  int32_t ret = 0;

  do {
    char eventLog[128];
    snprintf(eventLog, sizeof(eventLog), "propose message, type:%s", TMSG_INFO(pMsg->msgType));
    syncNodeEventLog(pSyncNode, eventLog);
  } while (0);

  if (pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
    if (pSyncNode->changing && pMsg->msgType != TDMT_SYNC_CONFIG_CHANGE_FINISH) {
      ret = -1;
      terrno = TSDB_CODE_SYN_PROPOSE_NOT_READY;
      sError("vgId:%d, failed to sync propose since not ready, type:%s", pSyncNode->vgId, TMSG_INFO(pMsg->msgType));
      goto _END;
    }

    // config change
    if (pMsg->msgType == TDMT_SYNC_CONFIG_CHANGE) {
      if (!syncNodeCanChange(pSyncNode)) {
        ret = -1;
        terrno = TSDB_CODE_SYN_RECONFIG_NOT_READY;
        sError("vgId:%d, failed to sync reconfig since not ready, type:%s", pSyncNode->vgId, TMSG_INFO(pMsg->msgType));
        goto _END;
      }

      ASSERT(!pSyncNode->changing);
      pSyncNode->changing = true;
    }

    // not restored, vnode enable
    if (!pSyncNode->restoreFinish && pSyncNode->vgId != 1) {
      ret = -1;
      terrno = TSDB_CODE_SYN_PROPOSE_NOT_READY;
      sError("vgId:%d, failed to sync propose since not ready, type:%s, last:%" PRId64 ", cmt:%" PRId64,
             pSyncNode->vgId, TMSG_INFO(pMsg->msgType), syncNodeGetLastIndex(pSyncNode), pSyncNode->commitIndex);
      goto _END;
    }

    SRespStub stub;
    stub.createTime = taosGetTimestampMs();
    stub.rpcMsg = *pMsg;
    uint64_t seqNum = syncRespMgrAdd(pSyncNode->pSyncRespMgr, &stub);

    SyncClientRequest* pSyncMsg = syncClientRequestBuild2(pMsg, seqNum, isWeak, pSyncNode->vgId);
    SRpcMsg            rpcMsg;
    syncClientRequest2RpcMsg(pSyncMsg, &rpcMsg);

    // optimized one replica
    if (syncNodeIsOptimizedOneReplica(pSyncNode, pMsg)) {
      SyncIndex retIndex;
      int32_t   code = syncNodeOnClientRequest(pSyncNode, pSyncMsg, &retIndex);
      if (code == 0) {
        pMsg->info.conn.applyIndex = retIndex;
        pMsg->info.conn.applyTerm = pSyncNode->pRaftStore->currentTerm;
        rpcFreeCont(rpcMsg.pCont);
        syncRespMgrDel(pSyncNode->pSyncRespMgr, seqNum);
        ret = 1;
        sDebug("vgId:%d, sync optimize index:%" PRId64 ", type:%s", pSyncNode->vgId, retIndex,
               TMSG_INFO(pMsg->msgType));
      } else {
        ret = -1;
        terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
        sError("vgId:%d, failed to sync optimize index:%" PRId64 ", type:%s", pSyncNode->vgId, retIndex,
               TMSG_INFO(pMsg->msgType));
      }

    } else {
      if (pSyncNode->FpEqMsg != NULL && (*pSyncNode->FpEqMsg)(pSyncNode->msgcb, &rpcMsg) == 0) {
        ret = 0;
      } else {
        ret = -1;
        terrno = TSDB_CODE_SYN_INTERNAL_ERROR;
        sError("vgId:%d, failed to enqueue msg since its null", pSyncNode->vgId);
      }
    }

    syncClientRequestDestroy(pSyncMsg);
    goto _END;

  } else {
    ret = -1;
    terrno = TSDB_CODE_SYN_NOT_LEADER;
    sError("vgId:%d, sync propose not leader, %s, type:%s", pSyncNode->vgId, syncUtilState2String(pSyncNode->state),
           TMSG_INFO(pMsg->msgType));
    goto _END;
  }

_END:
  return ret;
}

int32_t syncHbTimerInit(SSyncNode* pSyncNode, SSyncTimer* pSyncTimer, SRaftId destId) {
  pSyncTimer->pTimer = NULL;
  pSyncTimer->counter = 0;
  pSyncTimer->timerMS = pSyncNode->hbBaseLine;
  pSyncTimer->timerCb = syncNodeEqPeerHeartbeatTimer;
  pSyncTimer->destId = destId;
  atomic_store_64(&pSyncTimer->logicClock, 0);
  return 0;
}

int32_t syncHbTimerStart(SSyncNode* pSyncNode, SSyncTimer* pSyncTimer) {
  int32_t ret = 0;
  if (syncEnvIsStart()) {
    SSyncHbTimerData* pData = taosMemoryMalloc(sizeof(SSyncHbTimerData));
    pData->pSyncNode = pSyncNode;
    pData->pTimer = pSyncTimer;
    pData->destId = pSyncTimer->destId;
    pData->logicClock = pSyncTimer->logicClock;

    pSyncTimer->pData = pData;
    taosTmrReset(pSyncTimer->timerCb, pSyncTimer->timerMS, pData, gSyncEnv->pTimerManager, &pSyncTimer->pTimer);
  } else {
    sError("vgId:%d, start ctrl hb timer error, sync env is stop", pSyncNode->vgId);
  }
  return ret;
}

int32_t syncHbTimerStop(SSyncNode* pSyncNode, SSyncTimer* pSyncTimer) {
  int32_t ret = 0;
  atomic_add_fetch_64(&pSyncTimer->logicClock, 1);
  taosTmrStop(pSyncTimer->pTimer);
  pSyncTimer->pTimer = NULL;
  // taosMemoryFree(pSyncTimer->pData);
  return ret;
}

SSyncNode* syncNodeOpen(SSyncInfo* pSyncInfo) {
  SSyncNode* pSyncNode = taosMemoryCalloc(1, sizeof(SSyncNode));
  if (pSyncNode == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    goto _error;
  }

  if (!taosDirExist((char*)(pSyncInfo->path))) {
    if (taosMkDir(pSyncInfo->path) != 0) {
      terrno = TAOS_SYSTEM_ERROR(errno);
      sError("failed to create dir:%s since %s", pSyncInfo->path, terrstr());
      goto _error;
    }
  }

  snprintf(pSyncNode->configPath, sizeof(pSyncNode->configPath), "%s%sraft_config.json", pSyncInfo->path, TD_DIRSEP);
  if (!taosCheckExistFile(pSyncNode->configPath)) {
    // create a new raft config file
    SRaftCfgMeta meta = {0};
    meta.isStandBy = pSyncInfo->isStandBy;
    meta.snapshotStrategy = pSyncInfo->snapshotStrategy;
    meta.lastConfigIndex = SYNC_INDEX_INVALID;
    meta.batchSize = pSyncInfo->batchSize;
    if (raftCfgCreateFile(&pSyncInfo->syncCfg, meta, pSyncNode->configPath) != 0) {
      sError("vgId:%d, failed to create raft cfg file at %s", pSyncNode->vgId, pSyncNode->configPath);
      goto _error;
    }
    if (pSyncInfo->syncCfg.replicaNum == 0) {
      sInfo("vgId:%d, sync config not input", pSyncNode->vgId);
      pSyncInfo->syncCfg = pSyncNode->pRaftCfg->cfg;
    }
  } else {
    // update syncCfg by raft_config.json
    pSyncNode->pRaftCfg = raftCfgOpen(pSyncNode->configPath);
    if (pSyncNode->pRaftCfg == NULL) {
      sError("vgId:%d, failed to open raft cfg file at %s", pSyncNode->vgId, pSyncNode->configPath);
      goto _error;
    }
    if (pSyncInfo->syncCfg.replicaNum > 0 && pSyncInfo->syncCfg.replicaNum != pSyncNode->pRaftCfg->cfg.replicaNum) {
      sInfo("vgId:%d, use sync config from input options and write to cfg file", pSyncNode->vgId);
      pSyncNode->pRaftCfg->cfg = pSyncInfo->syncCfg;
      if (raftCfgPersist(pSyncNode->pRaftCfg) != 0) {
        sError("vgId:%d, failed to persist raft cfg file at %s", pSyncNode->vgId, pSyncNode->configPath);
        goto _error;
      }
    } else {
      sInfo("vgId:%d, use sync config from raft cfg file", pSyncNode->vgId);
      pSyncInfo->syncCfg = pSyncNode->pRaftCfg->cfg;
    }

    raftCfgClose(pSyncNode->pRaftCfg);
    pSyncNode->pRaftCfg = NULL;
  }

  // init by SSyncInfo
  pSyncNode->vgId = pSyncInfo->vgId;
  SSyncCfg* pCfg = &pSyncInfo->syncCfg;
  sDebug("vgId:%d, replica:%d selfIndex:%d", pSyncNode->vgId, pCfg->replicaNum, pCfg->myIndex);
  for (int32_t i = 0; i < pCfg->replicaNum; ++i) {
    SNodeInfo* pNode = &pCfg->nodeInfo[i];
    sDebug("vgId:%d, index:%d ep:%s:%u", pSyncNode->vgId, i, pNode->nodeFqdn, pNode->nodePort);
  }

  memcpy(pSyncNode->path, pSyncInfo->path, sizeof(pSyncNode->path));
  snprintf(pSyncNode->raftStorePath, sizeof(pSyncNode->raftStorePath), "%s%sraft_store.json", pSyncInfo->path,
           TD_DIRSEP);
  snprintf(pSyncNode->configPath, sizeof(pSyncNode->configPath), "%s%sraft_config.json", pSyncInfo->path, TD_DIRSEP);

  pSyncNode->pWal = pSyncInfo->pWal;
  pSyncNode->msgcb = pSyncInfo->msgcb;
  pSyncNode->FpSendMsg = pSyncInfo->FpSendMsg;
  pSyncNode->FpEqMsg = pSyncInfo->FpEqMsg;
  pSyncNode->FpEqCtrlMsg = pSyncInfo->FpEqCtrlMsg;

  // init raft config
  pSyncNode->pRaftCfg = raftCfgOpen(pSyncNode->configPath);
  if (pSyncNode->pRaftCfg == NULL) {
    sError("vgId:%d, failed to open raft cfg file at %s", pSyncNode->vgId, pSyncNode->configPath);
    goto _error;
  }

  // init internal
  pSyncNode->myNodeInfo = pSyncNode->pRaftCfg->cfg.nodeInfo[pSyncNode->pRaftCfg->cfg.myIndex];
  if (!syncUtilnodeInfo2raftId(&pSyncNode->myNodeInfo, pSyncNode->vgId, &pSyncNode->myRaftId)) {
    sError("vgId:%d, failed to determine my raft member id", pSyncNode->vgId);
    goto _error;
  }

  // init peersNum, peers, peersId
  pSyncNode->peersNum = pSyncNode->pRaftCfg->cfg.replicaNum - 1;
  int j = 0;
  for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
    if (i != pSyncNode->pRaftCfg->cfg.myIndex) {
      pSyncNode->peersNodeInfo[j] = pSyncNode->pRaftCfg->cfg.nodeInfo[i];
      j++;
    }
  }
  for (int i = 0; i < pSyncNode->peersNum; ++i) {
    if (!syncUtilnodeInfo2raftId(&pSyncNode->peersNodeInfo[i], pSyncNode->vgId, &pSyncNode->peersId[i])) {
      sError("vgId:%d, failed to determine raft member id, peer:%d", pSyncNode->vgId, i);
      goto _error;
    }
  }

  // init replicaNum, replicasId
  pSyncNode->replicaNum = pSyncNode->pRaftCfg->cfg.replicaNum;
  for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
    if (!syncUtilnodeInfo2raftId(&pSyncNode->pRaftCfg->cfg.nodeInfo[i], pSyncNode->vgId, &pSyncNode->replicasId[i])) {
      sError("vgId:%d, failed to determine raft member id, replica:%d", pSyncNode->vgId, i);
      goto _error;
    }
  }

  // init raft algorithm
  pSyncNode->pFsm = pSyncInfo->pFsm;
  pSyncInfo->pFsm = NULL;
  pSyncNode->quorum = syncUtilQuorum(pSyncNode->pRaftCfg->cfg.replicaNum);
  pSyncNode->leaderCache = EMPTY_RAFT_ID;

  // init life cycle outside

  // TLA+ Spec
  // InitHistoryVars == /\ elections = {}
  //                    /\ allLogs   = {}
  //                    /\ voterLog  = [i \in Server |-> [j \in {} |-> <<>>]]
  // InitServerVars == /\ currentTerm = [i \in Server |-> 1]
  //                   /\ state       = [i \in Server |-> Follower]
  //                   /\ votedFor    = [i \in Server |-> Nil]
  // InitCandidateVars == /\ votesResponded = [i \in Server |-> {}]
  //                      /\ votesGranted   = [i \in Server |-> {}]
  // \* The values nextIndex[i][i] and matchIndex[i][i] are never read, since the
  // \* leader does not send itself messages. It's still easier to include these
  // \* in the functions.
  // InitLeaderVars == /\ nextIndex  = [i \in Server |-> [j \in Server |-> 1]]
  //                   /\ matchIndex = [i \in Server |-> [j \in Server |-> 0]]
  // InitLogVars == /\ log          = [i \in Server |-> << >>]
  //                /\ commitIndex  = [i \in Server |-> 0]
  // Init == /\ messages = [m \in {} |-> 0]
  //         /\ InitHistoryVars
  //         /\ InitServerVars
  //         /\ InitCandidateVars
  //         /\ InitLeaderVars
  //         /\ InitLogVars
  //

  // init TLA+ server vars
  pSyncNode->state = TAOS_SYNC_STATE_FOLLOWER;
  pSyncNode->pRaftStore = raftStoreOpen(pSyncNode->raftStorePath);
  if (pSyncNode->pRaftStore == NULL) {
    sError("vgId:%d, failed to open raft store at path %s", pSyncNode->vgId, pSyncNode->raftStorePath);
    goto _error;
  }

  // init TLA+ candidate vars
  pSyncNode->pVotesGranted = voteGrantedCreate(pSyncNode);
  if (pSyncNode->pVotesGranted == NULL) {
    sError("vgId:%d, failed to create VotesGranted", pSyncNode->vgId);
    goto _error;
  }
  pSyncNode->pVotesRespond = votesRespondCreate(pSyncNode);
  if (pSyncNode->pVotesRespond == NULL) {
    sError("vgId:%d, failed to create VotesRespond", pSyncNode->vgId);
    goto _error;
  }

  // init TLA+ leader vars
  pSyncNode->pNextIndex = syncIndexMgrCreate(pSyncNode);
  if (pSyncNode->pNextIndex == NULL) {
    sError("vgId:%d, failed to create SyncIndexMgr", pSyncNode->vgId);
    goto _error;
  }
  pSyncNode->pMatchIndex = syncIndexMgrCreate(pSyncNode);
  if (pSyncNode->pMatchIndex == NULL) {
    sError("vgId:%d, failed to create SyncIndexMgr", pSyncNode->vgId);
    goto _error;
  }

  // init TLA+ log vars
  pSyncNode->pLogStore = logStoreCreate(pSyncNode);
  if (pSyncNode->pLogStore == NULL) {
    sError("vgId:%d, failed to create SyncLogStore", pSyncNode->vgId);
    goto _error;
  }

  SyncIndex commitIndex = SYNC_INDEX_INVALID;
  if (pSyncNode->pFsm != NULL && pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    SSnapshot snapshot = {0};
    int32_t   code = pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
    if (code != 0) {
      sError("vgId:%d, failed to get snapshot info, code:%d", pSyncNode->vgId, code);
      goto _error;
    }
    if (snapshot.lastApplyIndex > commitIndex) {
      commitIndex = snapshot.lastApplyIndex;
      syncNodeEventLog(pSyncNode, "reset commit index by snapshot");
    }
  }
  pSyncNode->commitIndex = commitIndex;

  // timer ms init
  pSyncNode->pingBaseLine = PING_TIMER_MS;
  pSyncNode->electBaseLine = ELECT_TIMER_MS_MIN;
  pSyncNode->hbBaseLine = HEARTBEAT_TIMER_MS;

  // init ping timer
  pSyncNode->pPingTimer = NULL;
  pSyncNode->pingTimerMS = pSyncNode->pingBaseLine;
  atomic_store_64(&pSyncNode->pingTimerLogicClock, 0);
  atomic_store_64(&pSyncNode->pingTimerLogicClockUser, 0);
  pSyncNode->FpPingTimerCB = syncNodeEqPingTimer;
  pSyncNode->pingTimerCounter = 0;

  // init elect timer
  pSyncNode->pElectTimer = NULL;
  pSyncNode->electTimerMS = syncUtilElectRandomMS(pSyncNode->electBaseLine, 2 * pSyncNode->electBaseLine);
  atomic_store_64(&pSyncNode->electTimerLogicClock, 0);
  pSyncNode->FpElectTimerCB = syncNodeEqElectTimer;
  pSyncNode->electTimerCounter = 0;

  // init heartbeat timer
  pSyncNode->pHeartbeatTimer = NULL;
  pSyncNode->heartbeatTimerMS = pSyncNode->hbBaseLine;
  atomic_store_64(&pSyncNode->heartbeatTimerLogicClock, 0);
  atomic_store_64(&pSyncNode->heartbeatTimerLogicClockUser, 0);
  pSyncNode->FpHeartbeatTimerCB = syncNodeEqHeartbeatTimer;
  pSyncNode->heartbeatTimerCounter = 0;

  // init peer heartbeat timer
  for (int32_t i = 0; i < TSDB_MAX_REPLICA; ++i) {
    syncHbTimerInit(pSyncNode, &(pSyncNode->peerHeartbeatTimerArr[i]), (pSyncNode->replicasId)[i]);
  }

  // init callback
  pSyncNode->FpOnPing = syncNodeOnPing;
  pSyncNode->FpOnPingReply = syncNodeOnPingReply;
  pSyncNode->FpOnClientRequest = syncNodeOnClientRequest;
  pSyncNode->FpOnTimeout = syncNodeOnTimer;
  pSyncNode->FpOnSnapshot = syncNodeOnSnapshot;
  pSyncNode->FpOnSnapshotReply = syncNodeOnSnapshotReply;
  pSyncNode->FpOnRequestVote = syncNodeOnRequestVote;
  pSyncNode->FpOnRequestVoteReply = syncNodeOnRequestVoteReply;
  pSyncNode->FpOnAppendEntries = syncNodeOnAppendEntries;
  pSyncNode->FpOnAppendEntriesReply = syncNodeOnAppendEntriesReply;

  // tools
  pSyncNode->pSyncRespMgr = syncRespMgrCreate(pSyncNode, SYNC_RESP_TTL_MS);
  if (pSyncNode->pSyncRespMgr == NULL) {
    sError("vgId:%d, failed to create SyncRespMgr", pSyncNode->vgId);
    goto _error;
  }

  // restore state
  pSyncNode->restoreFinish = false;

  // snapshot senders
  for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
    SSyncSnapshotSender* pSender = snapshotSenderCreate(pSyncNode, i);
    // ASSERT(pSender != NULL);
    (pSyncNode->senders)[i] = pSender;
  }

  // snapshot receivers
  pSyncNode->pNewNodeReceiver = snapshotReceiverCreate(pSyncNode, EMPTY_RAFT_ID);

  // is config changing
  pSyncNode->changing = false;

  // peer state
  syncNodePeerStateInit(pSyncNode);

  // min match index
  pSyncNode->minMatchIndex = SYNC_INDEX_INVALID;

  // start in syncNodeStart
  // start raft
  // syncNodeBecomeFollower(pSyncNode);

  int64_t timeNow = taosGetTimestampMs();
  pSyncNode->startTime = timeNow;
  pSyncNode->leaderTime = timeNow;
  pSyncNode->lastReplicateTime = timeNow;

  // snapshotting
  atomic_store_64(&pSyncNode->snapshottingIndex, SYNC_INDEX_INVALID);

  syncNodeEventLog(pSyncNode, "sync open");

  return pSyncNode;

_error:
  if (pSyncInfo->pFsm) {
    taosMemoryFree(pSyncInfo->pFsm);
    pSyncInfo->pFsm = NULL;
  }
  syncNodeClose(pSyncNode);
  pSyncNode = NULL;
  return NULL;
}

void syncNodeMaybeUpdateCommitBySnapshot(SSyncNode* pSyncNode) {
  if (pSyncNode->pFsm != NULL && pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    SSnapshot snapshot;
    int32_t   code = pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
    ASSERT(code == 0);
    if (snapshot.lastApplyIndex > pSyncNode->commitIndex) {
      pSyncNode->commitIndex = snapshot.lastApplyIndex;
    }
  }
}

void syncNodeStart(SSyncNode* pSyncNode) {
  // start raft
  if (pSyncNode->replicaNum == 1) {
    raftStoreNextTerm(pSyncNode->pRaftStore);
    syncNodeBecomeLeader(pSyncNode, "one replica start");

    // Raft 3.6.2 Committing entries from previous terms
    syncNodeAppendNoop(pSyncNode);
    syncMaybeAdvanceCommitIndex(pSyncNode);

  } else {
    syncNodeBecomeFollower(pSyncNode, "first start");
  }

  int32_t ret = 0;
  ret = syncNodeStartPingTimer(pSyncNode);
  ASSERT(ret == 0);
}

void syncNodeStartStandBy(SSyncNode* pSyncNode) {
  // state change
  pSyncNode->state = TAOS_SYNC_STATE_FOLLOWER;
  syncNodeStopHeartbeatTimer(pSyncNode);

  // reset elect timer, long enough
  int32_t electMS = TIMER_MAX_MS;
  int32_t ret = syncNodeRestartElectTimer(pSyncNode, electMS);
  ASSERT(ret == 0);

  ret = 0;
  ret = syncNodeStartPingTimer(pSyncNode);
  ASSERT(ret == 0);
}

void syncNodeClose(SSyncNode* pSyncNode) {
  if (pSyncNode == NULL) {
    return;
  }
  int32_t ret;

  syncNodeEventLog(pSyncNode, "sync close");

  ret = raftStoreClose(pSyncNode->pRaftStore);
  ASSERT(ret == 0);

  syncRespMgrDestroy(pSyncNode->pSyncRespMgr);
  pSyncNode->pSyncRespMgr = NULL;
  voteGrantedDestroy(pSyncNode->pVotesGranted);
  pSyncNode->pVotesGranted = NULL;
  votesRespondDestory(pSyncNode->pVotesRespond);
  pSyncNode->pVotesRespond = NULL;
  syncIndexMgrDestroy(pSyncNode->pNextIndex);
  pSyncNode->pNextIndex = NULL;
  syncIndexMgrDestroy(pSyncNode->pMatchIndex);
  pSyncNode->pMatchIndex = NULL;
  logStoreDestory(pSyncNode->pLogStore);
  pSyncNode->pLogStore = NULL;
  raftCfgClose(pSyncNode->pRaftCfg);
  pSyncNode->pRaftCfg = NULL;

  syncNodeStopPingTimer(pSyncNode);
  syncNodeStopElectTimer(pSyncNode);
  syncNodeStopHeartbeatTimer(pSyncNode);

  if (pSyncNode->pFsm != NULL) {
    taosMemoryFree(pSyncNode->pFsm);
  }

  for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
    if ((pSyncNode->senders)[i] != NULL) {
      snapshotSenderDestroy((pSyncNode->senders)[i]);
      (pSyncNode->senders)[i] = NULL;
    }
  }

  if (pSyncNode->pNewNodeReceiver != NULL) {
    snapshotReceiverDestroy(pSyncNode->pNewNodeReceiver);
    pSyncNode->pNewNodeReceiver = NULL;
  }

  taosMemoryFree(pSyncNode);
}

// option
// bool syncNodeSnapshotEnable(SSyncNode* pSyncNode) { return pSyncNode->pRaftCfg->snapshotEnable; }

ESyncStrategy syncNodeStrategy(SSyncNode* pSyncNode) { return pSyncNode->pRaftCfg->snapshotStrategy; }

// ping --------------
int32_t syncNodePing(SSyncNode* pSyncNode, const SRaftId* destRaftId, SyncPing* pMsg) {
  syncPingLog2((char*)"==syncNodePing==", pMsg);
  int32_t ret = 0;

  SRpcMsg rpcMsg;
  syncPing2RpcMsg(pMsg, &rpcMsg);
  syncRpcMsgLog2((char*)"==syncNodePing==", &rpcMsg);

  ret = syncNodeSendMsgById(destRaftId, pSyncNode, &rpcMsg);
  return ret;
}

int32_t syncNodePingSelf(SSyncNode* pSyncNode) {
  int32_t   ret = 0;
  SyncPing* pMsg = syncPingBuild3(&pSyncNode->myRaftId, &pSyncNode->myRaftId, pSyncNode->vgId);
  ret = syncNodePing(pSyncNode, &pMsg->destId, pMsg);
  ASSERT(ret == 0);

  syncPingDestroy(pMsg);
  return ret;
}

int32_t syncNodePingPeers(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  for (int i = 0; i < pSyncNode->peersNum; ++i) {
    SRaftId*  destId = &(pSyncNode->peersId[i]);
    SyncPing* pMsg = syncPingBuild3(&pSyncNode->myRaftId, destId, pSyncNode->vgId);
    ret = syncNodePing(pSyncNode, destId, pMsg);
    ASSERT(ret == 0);
    syncPingDestroy(pMsg);
  }
  return ret;
}

int32_t syncNodePingAll(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
    SRaftId*  destId = &(pSyncNode->replicasId[i]);
    SyncPing* pMsg = syncPingBuild3(&pSyncNode->myRaftId, destId, pSyncNode->vgId);
    ret = syncNodePing(pSyncNode, destId, pMsg);
    ASSERT(ret == 0);
    syncPingDestroy(pMsg);
  }
  return ret;
}

// timer control --------------
int32_t syncNodeStartPingTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  if (syncEnvIsStart()) {
    taosTmrReset(pSyncNode->FpPingTimerCB, pSyncNode->pingTimerMS, pSyncNode, gSyncEnv->pTimerManager,
                 &pSyncNode->pPingTimer);
    atomic_store_64(&pSyncNode->pingTimerLogicClock, pSyncNode->pingTimerLogicClockUser);
  } else {
    sError("vgId:%d, start ping timer error, sync env is stop", pSyncNode->vgId);
  }
  return ret;
}

int32_t syncNodeStopPingTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  atomic_add_fetch_64(&pSyncNode->pingTimerLogicClockUser, 1);
  taosTmrStop(pSyncNode->pPingTimer);
  pSyncNode->pPingTimer = NULL;
  return ret;
}

int32_t syncNodeStartElectTimer(SSyncNode* pSyncNode, int32_t ms) {
  int32_t ret = 0;
  if (syncEnvIsStart()) {
    pSyncNode->electTimerMS = ms;

    SElectTimer* pElectTimer = taosMemoryMalloc(sizeof(SElectTimer));
    pElectTimer->logicClock = pSyncNode->electTimerLogicClock;
    pElectTimer->pSyncNode = pSyncNode;
    pElectTimer->pData = NULL;

    taosTmrReset(pSyncNode->FpElectTimerCB, pSyncNode->electTimerMS, pElectTimer, gSyncEnv->pTimerManager,
                 &pSyncNode->pElectTimer);

  } else {
    sError("vgId:%d, start elect timer error, sync env is stop", pSyncNode->vgId);
  }
  return ret;
}

int32_t syncNodeStopElectTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  atomic_add_fetch_64(&pSyncNode->electTimerLogicClock, 1);
  taosTmrStop(pSyncNode->pElectTimer);
  pSyncNode->pElectTimer = NULL;

  return ret;
}

int32_t syncNodeRestartElectTimer(SSyncNode* pSyncNode, int32_t ms) {
  int32_t ret = 0;
  syncNodeStopElectTimer(pSyncNode);
  syncNodeStartElectTimer(pSyncNode, ms);
  return ret;
}

int32_t syncNodeResetElectTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  int32_t electMS;

  if (pSyncNode->pRaftCfg->isStandBy) {
    electMS = TIMER_MAX_MS;
  } else {
    electMS = syncUtilElectRandomMS(pSyncNode->electBaseLine, 2 * pSyncNode->electBaseLine);
  }
  ret = syncNodeRestartElectTimer(pSyncNode, electMS);

  do {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "reset elect timer, min:%d, max:%d, ms:%d", pSyncNode->electBaseLine,
             2 * pSyncNode->electBaseLine, electMS);
    syncNodeEventLog(pSyncNode, logBuf);
  } while (0);

  return ret;
}

static int32_t syncNodeDoStartHeartbeatTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;
  if (syncEnvIsStart()) {
    taosTmrReset(pSyncNode->FpHeartbeatTimerCB, pSyncNode->heartbeatTimerMS, pSyncNode, gSyncEnv->pTimerManager,
                 &pSyncNode->pHeartbeatTimer);
    atomic_store_64(&pSyncNode->heartbeatTimerLogicClock, pSyncNode->heartbeatTimerLogicClockUser);
  } else {
    sError("vgId:%d, start heartbeat timer error, sync env is stop", pSyncNode->vgId);
  }

  do {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "start heartbeat timer, ms:%d", pSyncNode->heartbeatTimerMS);
    syncNodeEventLog(pSyncNode, logBuf);
  } while (0);

  return ret;
}

int32_t syncNodeStartHeartbeatTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;

#if 0
  pSyncNode->heartbeatTimerMS = pSyncNode->hbBaseLine;
  ret = syncNodeDoStartHeartbeatTimer(pSyncNode);
#endif

  for (int i = 0; i < pSyncNode->peersNum; ++i) {
    SSyncTimer* pSyncTimer = syncNodeGetHbTimer(pSyncNode, &(pSyncNode->peersId[i]));
    if (pSyncTimer != NULL) {
      syncHbTimerStart(pSyncNode, pSyncTimer);
    }
  }

  return ret;
}

int32_t syncNodeStopHeartbeatTimer(SSyncNode* pSyncNode) {
  int32_t ret = 0;

#if 0
  atomic_add_fetch_64(&pSyncNode->heartbeatTimerLogicClockUser, 1);
  taosTmrStop(pSyncNode->pHeartbeatTimer);
  pSyncNode->pHeartbeatTimer = NULL;
#endif

  for (int i = 0; i < pSyncNode->peersNum; ++i) {
    SSyncTimer* pSyncTimer = syncNodeGetHbTimer(pSyncNode, &(pSyncNode->peersId[i]));
    if (pSyncTimer != NULL) {
      syncHbTimerStop(pSyncNode, pSyncTimer);
    }
  }

  return ret;
}

int32_t syncNodeRestartHeartbeatTimer(SSyncNode* pSyncNode) {
  syncNodeStopHeartbeatTimer(pSyncNode);
  syncNodeStartHeartbeatTimer(pSyncNode);
  return 0;
}

// utils --------------
int32_t syncNodeSendMsgById(const SRaftId* destRaftId, SSyncNode* pSyncNode, SRpcMsg* pMsg) {
  SEpSet epSet;
  syncUtilraftId2EpSet(destRaftId, &epSet);
  if (pSyncNode->FpSendMsg != NULL) {
    // htonl
    syncUtilMsgHtoN(pMsg->pCont);

    pMsg->info.noResp = 1;
    pSyncNode->FpSendMsg(&epSet, pMsg);
  } else {
    sError("vgId:%d, sync send msg by id error, fp-send-msg is null", pSyncNode->vgId);
    return -1;
  }

  return 0;
}

int32_t syncNodeSendMsgByInfo(const SNodeInfo* nodeInfo, SSyncNode* pSyncNode, SRpcMsg* pMsg) {
  SEpSet epSet;
  syncUtilnodeInfo2EpSet(nodeInfo, &epSet);
  if (pSyncNode->FpSendMsg != NULL) {
    // htonl
    syncUtilMsgHtoN(pMsg->pCont);

    pMsg->info.noResp = 1;
    pSyncNode->FpSendMsg(&epSet, pMsg);
  } else {
    sError("vgId:%d, sync send msg by info error, fp-send-msg is null", pSyncNode->vgId);
  }
  return 0;
}

cJSON* syncNode2Json(const SSyncNode* pSyncNode) {
  char   u64buf[128] = {0};
  cJSON* pRoot = cJSON_CreateObject();

  if (pSyncNode != NULL) {
    // init by SSyncInfo
    cJSON_AddNumberToObject(pRoot, "vgId", pSyncNode->vgId);
    cJSON_AddItemToObject(pRoot, "SRaftCfg", raftCfg2Json(pSyncNode->pRaftCfg));
    cJSON_AddStringToObject(pRoot, "path", pSyncNode->path);
    cJSON_AddStringToObject(pRoot, "raftStorePath", pSyncNode->raftStorePath);
    cJSON_AddStringToObject(pRoot, "configPath", pSyncNode->configPath);

    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->pWal);
    cJSON_AddStringToObject(pRoot, "pWal", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->msgcb);
    cJSON_AddStringToObject(pRoot, "rpcClient", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpSendMsg);
    cJSON_AddStringToObject(pRoot, "FpSendMsg", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->msgcb);
    cJSON_AddStringToObject(pRoot, "queue", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpEqMsg);
    cJSON_AddStringToObject(pRoot, "FpEqMsg", u64buf);

    // init internal
    cJSON* pMe = syncUtilNodeInfo2Json(&pSyncNode->myNodeInfo);
    cJSON_AddItemToObject(pRoot, "myNodeInfo", pMe);
    cJSON* pRaftId = syncUtilRaftId2Json(&pSyncNode->myRaftId);
    cJSON_AddItemToObject(pRoot, "myRaftId", pRaftId);

    cJSON_AddNumberToObject(pRoot, "peersNum", pSyncNode->peersNum);
    cJSON* pPeers = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "peersNodeInfo", pPeers);
    for (int i = 0; i < pSyncNode->peersNum; ++i) {
      cJSON_AddItemToArray(pPeers, syncUtilNodeInfo2Json(&pSyncNode->peersNodeInfo[i]));
    }
    cJSON* pPeersId = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "peersId", pPeersId);
    for (int i = 0; i < pSyncNode->peersNum; ++i) {
      cJSON_AddItemToArray(pPeersId, syncUtilRaftId2Json(&pSyncNode->peersId[i]));
    }

    cJSON_AddNumberToObject(pRoot, "replicaNum", pSyncNode->replicaNum);
    cJSON* pReplicasId = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "replicasId", pReplicasId);
    for (int i = 0; i < pSyncNode->replicaNum; ++i) {
      cJSON_AddItemToArray(pReplicasId, syncUtilRaftId2Json(&pSyncNode->replicasId[i]));
    }

    // raft algorithm
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->pFsm);
    cJSON_AddStringToObject(pRoot, "pFsm", u64buf);
    cJSON_AddNumberToObject(pRoot, "quorum", pSyncNode->quorum);
    cJSON* pLaderCache = syncUtilRaftId2Json(&pSyncNode->leaderCache);
    cJSON_AddItemToObject(pRoot, "leaderCache", pLaderCache);

    // life cycle
    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pSyncNode->rid);
    cJSON_AddStringToObject(pRoot, "rid", u64buf);

    // tla+ server vars
    cJSON_AddNumberToObject(pRoot, "state", pSyncNode->state);
    cJSON_AddStringToObject(pRoot, "state_str", syncUtilState2String(pSyncNode->state));
    cJSON_AddItemToObject(pRoot, "pRaftStore", raftStore2Json(pSyncNode->pRaftStore));

    // tla+ candidate vars
    cJSON_AddItemToObject(pRoot, "pVotesGranted", voteGranted2Json(pSyncNode->pVotesGranted));
    cJSON_AddItemToObject(pRoot, "pVotesRespond", votesRespond2Json(pSyncNode->pVotesRespond));

    // tla+ leader vars
    cJSON_AddItemToObject(pRoot, "pNextIndex", syncIndexMgr2Json(pSyncNode->pNextIndex));
    cJSON_AddItemToObject(pRoot, "pMatchIndex", syncIndexMgr2Json(pSyncNode->pMatchIndex));

    // tla+ log vars
    cJSON_AddItemToObject(pRoot, "pLogStore", logStore2Json(pSyncNode->pLogStore));
    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pSyncNode->commitIndex);
    cJSON_AddStringToObject(pRoot, "commitIndex", u64buf);

    // timer ms init
    cJSON_AddNumberToObject(pRoot, "pingBaseLine", pSyncNode->pingBaseLine);
    cJSON_AddNumberToObject(pRoot, "electBaseLine", pSyncNode->electBaseLine);
    cJSON_AddNumberToObject(pRoot, "hbBaseLine", pSyncNode->hbBaseLine);

    // ping timer
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->pPingTimer);
    cJSON_AddStringToObject(pRoot, "pPingTimer", u64buf);
    cJSON_AddNumberToObject(pRoot, "pingTimerMS", pSyncNode->pingTimerMS);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->pingTimerLogicClock);
    cJSON_AddStringToObject(pRoot, "pingTimerLogicClock", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->pingTimerLogicClockUser);
    cJSON_AddStringToObject(pRoot, "pingTimerLogicClockUser", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpPingTimerCB);
    cJSON_AddStringToObject(pRoot, "FpPingTimerCB", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->pingTimerCounter);
    cJSON_AddStringToObject(pRoot, "pingTimerCounter", u64buf);

    // elect timer
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->pElectTimer);
    cJSON_AddStringToObject(pRoot, "pElectTimer", u64buf);
    cJSON_AddNumberToObject(pRoot, "electTimerMS", pSyncNode->electTimerMS);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->electTimerLogicClock);
    cJSON_AddStringToObject(pRoot, "electTimerLogicClock", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpElectTimerCB);
    cJSON_AddStringToObject(pRoot, "FpElectTimerCB", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->electTimerCounter);
    cJSON_AddStringToObject(pRoot, "electTimerCounter", u64buf);

    // heartbeat timer
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->pHeartbeatTimer);
    cJSON_AddStringToObject(pRoot, "pHeartbeatTimer", u64buf);
    cJSON_AddNumberToObject(pRoot, "heartbeatTimerMS", pSyncNode->heartbeatTimerMS);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->heartbeatTimerLogicClock);
    cJSON_AddStringToObject(pRoot, "heartbeatTimerLogicClock", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->heartbeatTimerLogicClockUser);
    cJSON_AddStringToObject(pRoot, "heartbeatTimerLogicClockUser", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpHeartbeatTimerCB);
    cJSON_AddStringToObject(pRoot, "FpHeartbeatTimerCB", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pSyncNode->heartbeatTimerCounter);
    cJSON_AddStringToObject(pRoot, "heartbeatTimerCounter", u64buf);

    // callback
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnPing);
    cJSON_AddStringToObject(pRoot, "FpOnPing", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnPingReply);
    cJSON_AddStringToObject(pRoot, "FpOnPingReply", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnRequestVote);
    cJSON_AddStringToObject(pRoot, "FpOnRequestVote", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnRequestVoteReply);
    cJSON_AddStringToObject(pRoot, "FpOnRequestVoteReply", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnAppendEntries);
    cJSON_AddStringToObject(pRoot, "FpOnAppendEntries", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnAppendEntriesReply);
    cJSON_AddStringToObject(pRoot, "FpOnAppendEntriesReply", u64buf);
    snprintf(u64buf, sizeof(u64buf), "%p", pSyncNode->FpOnTimeout);
    cJSON_AddStringToObject(pRoot, "FpOnTimeout", u64buf);

    // restoreFinish
    cJSON_AddNumberToObject(pRoot, "restoreFinish", pSyncNode->restoreFinish);

    // snapshot senders
    cJSON* pSenders = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "senders", pSenders);
    for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
      cJSON_AddItemToArray(pSenders, snapshotSender2Json((pSyncNode->senders)[i]));
    }

    // snapshot receivers
    cJSON* pReceivers = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "receiver", snapshotReceiver2Json(pSyncNode->pNewNodeReceiver));

    // changing
    cJSON_AddNumberToObject(pRoot, "changing", pSyncNode->changing);
  }

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "SSyncNode", pRoot);
  return pJson;
}

char* syncNode2Str(const SSyncNode* pSyncNode) {
  cJSON* pJson = syncNode2Json(pSyncNode);
  char*  serialized = cJSON_Print(pJson);
  cJSON_Delete(pJson);
  return serialized;
}

inline void syncNodeEventLog(const SSyncNode* pSyncNode, char* str) {
  if (pSyncNode == NULL) {
    return;
  }

  SSnapshot snapshot = {.data = NULL, .lastApplyIndex = -1, .lastApplyTerm = 0};
  if (pSyncNode->pFsm != NULL && pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
  }

  SyncIndex logLastIndex = SYNC_INDEX_INVALID;
  SyncIndex logBeginIndex = SYNC_INDEX_INVALID;
  if (pSyncNode->pLogStore != NULL) {
    logLastIndex = pSyncNode->pLogStore->syncLogLastIndex(pSyncNode->pLogStore);
    logBeginIndex = pSyncNode->pLogStore->syncLogBeginIndex(pSyncNode->pLogStore);
  }

  char* pCfgStr = syncCfg2SimpleStr(&(pSyncNode->pRaftCfg->cfg));
  char* printStr = "";
  if (pCfgStr != NULL) {
    printStr = pCfgStr;
  }

  char*   peerStateStr = syncNodePeerState2Str(pSyncNode);
  int32_t userStrLen = strlen(str) + strlen(peerStateStr);

  if (userStrLen < 256) {
    char logBuf[256 + 256];
    if (pSyncNode != NULL && pSyncNode->pRaftCfg != NULL && pSyncNode->pRaftStore != NULL) {
      snprintf(logBuf, sizeof(logBuf),
               "vgId:%d, sync %s %s, tm:%" PRIu64 ", cmt:%" PRId64 ", fst:%" PRId64 ", lst:%" PRId64 ", min:%" PRId64
               ", snap:%" PRId64 ", snap-tm:%" PRIu64
               ", sby:%d, "
               "stgy:%d, bch:%d, "
               "r-num:%d, "
               "lcfg:%" PRId64 ", chging:%d, rsto:%d, dquorum:%d, elt:%" PRId64 ", hb:%" PRId64 ", %s, %s",
               pSyncNode->vgId, syncUtilState2String(pSyncNode->state), str, pSyncNode->pRaftStore->currentTerm,
               pSyncNode->commitIndex, logBeginIndex, logLastIndex, pSyncNode->minMatchIndex, snapshot.lastApplyIndex,
               snapshot.lastApplyTerm, pSyncNode->pRaftCfg->isStandBy, pSyncNode->pRaftCfg->snapshotStrategy,
               pSyncNode->pRaftCfg->batchSize, pSyncNode->replicaNum, pSyncNode->pRaftCfg->lastConfigIndex,
               pSyncNode->changing, pSyncNode->restoreFinish, syncNodeDynamicQuorum(pSyncNode),
               pSyncNode->electTimerLogicClock, pSyncNode->heartbeatTimerLogicClockUser, peerStateStr, printStr);
    } else {
      snprintf(logBuf, sizeof(logBuf), "%s", str);
    }
    // sDebug("%s", logBuf);
    // sInfo("%s", logBuf);
    sTrace("%s", logBuf);

  } else {
    int   len = 256 + userStrLen;
    char* s = (char*)taosMemoryMalloc(len);
    if (pSyncNode != NULL && pSyncNode->pRaftCfg != NULL && pSyncNode->pRaftStore != NULL) {
      snprintf(s, len,
               "vgId:%d, sync %s %s, tm:%" PRIu64 ", cmt:%" PRId64 ", fst:%" PRId64 ", lst:%" PRId64 ", min:%" PRId64
               ", snap:%" PRId64 ", snap-tm:%" PRIu64
               ", sby:%d, "
               "stgy:%d, bch:%d, "
               "r-num:%d, "
               "lcfg:%" PRId64 ", chging:%d, rsto:%d, dquorum:%d, elt:%" PRId64 ", hb:%" PRId64 ", %s, %s",
               pSyncNode->vgId, syncUtilState2String(pSyncNode->state), str, pSyncNode->pRaftStore->currentTerm,
               pSyncNode->commitIndex, logBeginIndex, logLastIndex, pSyncNode->minMatchIndex, snapshot.lastApplyIndex,
               snapshot.lastApplyTerm, pSyncNode->pRaftCfg->isStandBy, pSyncNode->pRaftCfg->snapshotStrategy,
               pSyncNode->pRaftCfg->batchSize, pSyncNode->replicaNum, pSyncNode->pRaftCfg->lastConfigIndex,
               pSyncNode->changing, pSyncNode->restoreFinish, syncNodeDynamicQuorum(pSyncNode),
               pSyncNode->electTimerLogicClock, pSyncNode->heartbeatTimerLogicClockUser, peerStateStr, printStr);
    } else {
      snprintf(s, len, "%s", str);
    }
    // sDebug("%s", s);
    // sInfo("%s", s);
    sTrace("%s", s);
    taosMemoryFree(s);
  }

  taosMemoryFree(peerStateStr);
  taosMemoryFree(pCfgStr);
}

inline void syncNodeErrorLog(const SSyncNode* pSyncNode, char* str) {
  if (pSyncNode == NULL) {
    return;
  }

  int32_t userStrLen = strlen(str);

  SSnapshot snapshot = {.data = NULL, .lastApplyIndex = -1, .lastApplyTerm = 0};
  if (pSyncNode->pFsm != NULL && pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
  }

  SyncIndex logLastIndex = SYNC_INDEX_INVALID;
  SyncIndex logBeginIndex = SYNC_INDEX_INVALID;
  if (pSyncNode->pLogStore != NULL) {
    logLastIndex = pSyncNode->pLogStore->syncLogLastIndex(pSyncNode->pLogStore);
    logBeginIndex = pSyncNode->pLogStore->syncLogBeginIndex(pSyncNode->pLogStore);
  }

  char* pCfgStr = syncCfg2SimpleStr(&(pSyncNode->pRaftCfg->cfg));
  char* printStr = "";
  if (pCfgStr != NULL) {
    printStr = pCfgStr;
  }

  if (userStrLen < 256) {
    char logBuf[256 + 256];
    if (pSyncNode != NULL && pSyncNode->pRaftCfg != NULL && pSyncNode->pRaftStore != NULL) {
      snprintf(logBuf, sizeof(logBuf),
               "vgId:%d, sync %s %s, tm:%" PRIu64 ", cmt:%" PRId64 ", fst:%" PRId64 ", lst:%" PRId64 ", min:%" PRId64
               ", snap:%" PRId64 ", snap-tm:%" PRIu64
               ", sby:%d, "
               "stgy:%d, bch:%d, "
               "r-num:%d, "
               "lcfg:%" PRId64 ", chging:%d, rsto:%d, dquorum:%d, elt:%" PRId64 ", hb:%" PRId64 ", %s",
               pSyncNode->vgId, syncUtilState2String(pSyncNode->state), str, pSyncNode->pRaftStore->currentTerm,
               pSyncNode->commitIndex, logBeginIndex, logLastIndex, pSyncNode->minMatchIndex, snapshot.lastApplyIndex,
               snapshot.lastApplyTerm, pSyncNode->pRaftCfg->isStandBy, pSyncNode->pRaftCfg->snapshotStrategy,
               pSyncNode->pRaftCfg->batchSize, pSyncNode->replicaNum, pSyncNode->pRaftCfg->lastConfigIndex,
               pSyncNode->changing, pSyncNode->restoreFinish, syncNodeDynamicQuorum(pSyncNode),
               pSyncNode->electTimerLogicClock, pSyncNode->heartbeatTimerLogicClockUser, printStr);
    } else {
      snprintf(logBuf, sizeof(logBuf), "%s", str);
    }
    sError("%s", logBuf);

  } else {
    int   len = 256 + userStrLen;
    char* s = (char*)taosMemoryMalloc(len);
    if (pSyncNode != NULL && pSyncNode->pRaftCfg != NULL && pSyncNode->pRaftStore != NULL) {
      snprintf(s, len,
               "vgId:%d, sync %s %s, tm:%" PRIu64 ", cmt:%" PRId64 ", fst:%" PRId64 ", lst:%" PRId64 ", min:%" PRId64
               ", snap:%" PRId64 ", snap-tm:%" PRIu64
               ", sby:%d, "
               "stgy:%d, bch:%d, "
               "r-num:%d, "
               "lcfg:%" PRId64 ", chging:%d, rsto:%d, dquorum:%d, elt:%" PRId64 ", hb:%" PRId64 ", %s",
               pSyncNode->vgId, syncUtilState2String(pSyncNode->state), str, pSyncNode->pRaftStore->currentTerm,
               pSyncNode->commitIndex, logBeginIndex, logLastIndex, pSyncNode->minMatchIndex, snapshot.lastApplyIndex,
               snapshot.lastApplyTerm, pSyncNode->pRaftCfg->isStandBy, pSyncNode->pRaftCfg->snapshotStrategy,
               pSyncNode->pRaftCfg->batchSize, pSyncNode->replicaNum, pSyncNode->pRaftCfg->lastConfigIndex,
               pSyncNode->changing, pSyncNode->restoreFinish, syncNodeDynamicQuorum(pSyncNode),
               pSyncNode->electTimerLogicClock, pSyncNode->heartbeatTimerLogicClockUser, printStr);
    } else {
      snprintf(s, len, "%s", str);
    }
    sError("%s", s);
    taosMemoryFree(s);
  }

  taosMemoryFree(pCfgStr);
}

inline char* syncNode2SimpleStr(const SSyncNode* pSyncNode) {
  int   len = 256;
  char* s = (char*)taosMemoryMalloc(len);

  SSnapshot snapshot = {.data = NULL, .lastApplyIndex = -1, .lastApplyTerm = 0};
  if (pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
  }
  SyncIndex logLastIndex = pSyncNode->pLogStore->syncLogLastIndex(pSyncNode->pLogStore);
  SyncIndex logBeginIndex = pSyncNode->pLogStore->syncLogBeginIndex(pSyncNode->pLogStore);

  snprintf(s, len,
           "vgId:%d, sync %s, tm:%" PRIu64 ", cmt:%" PRId64 ", fst:%" PRId64 ", lst:%" PRId64 ", snap:%" PRId64
           ", sby:%d, "
           "r-num:%d, "
           "lcfg:%" PRId64 ", chging:%d, rsto:%d",
           pSyncNode->vgId, syncUtilState2String(pSyncNode->state), pSyncNode->pRaftStore->currentTerm,
           pSyncNode->commitIndex, logBeginIndex, logLastIndex, snapshot.lastApplyIndex, pSyncNode->pRaftCfg->isStandBy,
           pSyncNode->replicaNum, pSyncNode->pRaftCfg->lastConfigIndex, pSyncNode->changing, pSyncNode->restoreFinish);

  return s;
}

inline bool syncNodeInConfig(SSyncNode* pSyncNode, const SSyncCfg* config) {
  bool b1 = false;
  bool b2 = false;

  for (int i = 0; i < config->replicaNum; ++i) {
    if (strcmp((config->nodeInfo)[i].nodeFqdn, pSyncNode->myNodeInfo.nodeFqdn) == 0 &&
        (config->nodeInfo)[i].nodePort == pSyncNode->myNodeInfo.nodePort) {
      b1 = true;
      break;
    }
  }

  for (int i = 0; i < config->replicaNum; ++i) {
    SRaftId raftId;
    raftId.addr = syncUtilAddr2U64((config->nodeInfo)[i].nodeFqdn, (config->nodeInfo)[i].nodePort);
    raftId.vgId = pSyncNode->vgId;

    if (syncUtilSameId(&raftId, &(pSyncNode->myRaftId))) {
      b2 = true;
      break;
    }
  }

  ASSERT(b1 == b2);
  return b1;
}

static bool syncIsConfigChanged(const SSyncCfg* pOldCfg, const SSyncCfg* pNewCfg) {
  if (pOldCfg->replicaNum != pNewCfg->replicaNum) return true;
  if (pOldCfg->myIndex != pNewCfg->myIndex) return true;
  for (int32_t i = 0; i < pOldCfg->replicaNum; ++i) {
    const SNodeInfo* pOldInfo = &pOldCfg->nodeInfo[i];
    const SNodeInfo* pNewInfo = &pNewCfg->nodeInfo[i];
    if (strcmp(pOldInfo->nodeFqdn, pNewInfo->nodeFqdn) != 0) return true;
    if (pOldInfo->nodePort != pNewInfo->nodePort) return true;
  }

  return false;
}

void syncNodeDoConfigChange(SSyncNode* pSyncNode, SSyncCfg* pNewConfig, SyncIndex lastConfigChangeIndex) {
  SSyncCfg oldConfig = pSyncNode->pRaftCfg->cfg;
#if 1
  if (!syncIsConfigChanged(&oldConfig, pNewConfig)) {
    sInfo("vgId:1, sync not reconfig since not changed");
    return;
  }
#endif
  pSyncNode->pRaftCfg->cfg = *pNewConfig;
  pSyncNode->pRaftCfg->lastConfigIndex = lastConfigChangeIndex;

  bool IamInOld = syncNodeInConfig(pSyncNode, &oldConfig);
  bool IamInNew = syncNodeInConfig(pSyncNode, pNewConfig);

  bool isDrop = false;
  bool isAdd = false;

  if (IamInOld && !IamInNew) {
    isDrop = true;
  } else {
    isDrop = false;
  }

  if (!IamInOld && IamInNew) {
    isAdd = true;
  } else {
    isAdd = false;
  }

  // log begin config change
  do {
    char  eventLog[256];
    char* pOldCfgStr = syncCfg2SimpleStr(&oldConfig);
    char* pNewCfgStr = syncCfg2SimpleStr(pNewConfig);
    snprintf(eventLog, sizeof(eventLog), "begin do config change, from %s to %s", pOldCfgStr, pNewCfgStr);
    syncNodeEventLog(pSyncNode, eventLog);
    taosMemoryFree(pOldCfgStr);
    taosMemoryFree(pNewCfgStr);
  } while (0);

  if (IamInNew) {
    pSyncNode->pRaftCfg->isStandBy = 0;  // change isStandBy to normal
  }
  if (isDrop) {
    pSyncNode->pRaftCfg->isStandBy = 1;  // set standby
  }

  // add last config index
  raftCfgAddConfigIndex(pSyncNode->pRaftCfg, lastConfigChangeIndex);

  if (IamInNew) {
    //-----------------------------------------
    int32_t ret = 0;

    // save snapshot senders
    int32_t oldReplicaNum = pSyncNode->replicaNum;
    SRaftId oldReplicasId[TSDB_MAX_REPLICA];
    memcpy(oldReplicasId, pSyncNode->replicasId, sizeof(oldReplicasId));
    SSyncSnapshotSender* oldSenders[TSDB_MAX_REPLICA];
    for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
      oldSenders[i] = (pSyncNode->senders)[i];

      char* eventLog = snapshotSender2SimpleStr(oldSenders[i], "snapshot sender save old");
      syncNodeEventLog(pSyncNode, eventLog);
      taosMemoryFree(eventLog);
    }

    // init internal
    pSyncNode->myNodeInfo = pSyncNode->pRaftCfg->cfg.nodeInfo[pSyncNode->pRaftCfg->cfg.myIndex];
    syncUtilnodeInfo2raftId(&pSyncNode->myNodeInfo, pSyncNode->vgId, &pSyncNode->myRaftId);

    // init peersNum, peers, peersId
    pSyncNode->peersNum = pSyncNode->pRaftCfg->cfg.replicaNum - 1;
    int j = 0;
    for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
      if (i != pSyncNode->pRaftCfg->cfg.myIndex) {
        pSyncNode->peersNodeInfo[j] = pSyncNode->pRaftCfg->cfg.nodeInfo[i];
        j++;
      }
    }
    for (int i = 0; i < pSyncNode->peersNum; ++i) {
      syncUtilnodeInfo2raftId(&pSyncNode->peersNodeInfo[i], pSyncNode->vgId, &pSyncNode->peersId[i]);
    }

    // init replicaNum, replicasId
    pSyncNode->replicaNum = pSyncNode->pRaftCfg->cfg.replicaNum;
    for (int i = 0; i < pSyncNode->pRaftCfg->cfg.replicaNum; ++i) {
      syncUtilnodeInfo2raftId(&pSyncNode->pRaftCfg->cfg.nodeInfo[i], pSyncNode->vgId, &pSyncNode->replicasId[i]);
    }

    // update quorum first
    pSyncNode->quorum = syncUtilQuorum(pSyncNode->pRaftCfg->cfg.replicaNum);

    syncIndexMgrUpdate(pSyncNode->pNextIndex, pSyncNode);
    syncIndexMgrUpdate(pSyncNode->pMatchIndex, pSyncNode);
    voteGrantedUpdate(pSyncNode->pVotesGranted, pSyncNode);
    votesRespondUpdate(pSyncNode->pVotesRespond, pSyncNode);

    // reset snapshot senders

    // clear new
    for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
      (pSyncNode->senders)[i] = NULL;
    }

    // reset new
    for (int i = 0; i < pSyncNode->replicaNum; ++i) {
      // reset sender
      bool reset = false;
      for (int j = 0; j < TSDB_MAX_REPLICA; ++j) {
        if (syncUtilSameId(&(pSyncNode->replicasId)[i], &oldReplicasId[j])) {
          char     host[128];
          uint16_t port;
          syncUtilU642Addr((pSyncNode->replicasId)[i].addr, host, sizeof(host), &port);

          do {
            char eventLog[256];
            snprintf(eventLog, sizeof(eventLog), "snapshot sender reset for: %" PRIu64 ", newIndex:%d, %s:%d, %p",
                     (pSyncNode->replicasId)[i].addr, i, host, port, oldSenders[j]);
            syncNodeEventLog(pSyncNode, eventLog);
          } while (0);

          (pSyncNode->senders)[i] = oldSenders[j];
          oldSenders[j] = NULL;
          reset = true;

          // reset replicaIndex
          int32_t oldreplicaIndex = (pSyncNode->senders)[i]->replicaIndex;
          (pSyncNode->senders)[i]->replicaIndex = i;

          do {
            char eventLog[256];
            snprintf(eventLog, sizeof(eventLog),
                     "snapshot sender udpate replicaIndex from %d to %d, %s:%d, %p, reset:%d", oldreplicaIndex, i, host,
                     port, (pSyncNode->senders)[i], reset);
            syncNodeEventLog(pSyncNode, eventLog);
          } while (0);
        }
      }
    }

    // create new
    for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
      if ((pSyncNode->senders)[i] == NULL) {
        (pSyncNode->senders)[i] = snapshotSenderCreate(pSyncNode, i);

        char* eventLog = snapshotSender2SimpleStr((pSyncNode->senders)[i], "snapshot sender create new");
        syncNodeEventLog(pSyncNode, eventLog);
        taosMemoryFree(eventLog);
      }
    }

    // free old
    for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
      if (oldSenders[i] != NULL) {
        snapshotSenderDestroy(oldSenders[i]);

        do {
          char eventLog[128];
          snprintf(eventLog, sizeof(eventLog), "snapshot sender delete old %p replica-index:%d", oldSenders[i], i);
          syncNodeEventLog(pSyncNode, eventLog);
        } while (0);

        oldSenders[i] = NULL;
      }
    }

    // persist cfg
    raftCfgPersist(pSyncNode->pRaftCfg);

    char  tmpbuf[512];
    char* oldStr = syncCfg2SimpleStr(&oldConfig);
    char* newStr = syncCfg2SimpleStr(pNewConfig);
    snprintf(tmpbuf, sizeof(tmpbuf), "config change from %d to %d, index:%" PRId64 ", %s  -->  %s",
             oldConfig.replicaNum, pNewConfig->replicaNum, lastConfigChangeIndex, oldStr, newStr);
    taosMemoryFree(oldStr);
    taosMemoryFree(newStr);

    // change isStandBy to normal (election timeout)
    if (pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
      syncNodeBecomeLeader(pSyncNode, tmpbuf);

      // Raft 3.6.2 Committing entries from previous terms
      syncNodeAppendNoop(pSyncNode);
      syncMaybeAdvanceCommitIndex(pSyncNode);

    } else {
      syncNodeBecomeFollower(pSyncNode, tmpbuf);
    }
  } else {
    // persist cfg
    raftCfgPersist(pSyncNode->pRaftCfg);

    char  tmpbuf[512];
    char* oldStr = syncCfg2SimpleStr(&oldConfig);
    char* newStr = syncCfg2SimpleStr(pNewConfig);
    snprintf(tmpbuf, sizeof(tmpbuf), "do not config change from %d to %d, index:%" PRId64 ", %s  -->  %s",
             oldConfig.replicaNum, pNewConfig->replicaNum, lastConfigChangeIndex, oldStr, newStr);
    taosMemoryFree(oldStr);
    taosMemoryFree(newStr);
    syncNodeEventLog(pSyncNode, tmpbuf);
  }

_END:

  // log end config change
  do {
    char  eventLog[256];
    char* pOldCfgStr = syncCfg2SimpleStr(&oldConfig);
    char* pNewCfgStr = syncCfg2SimpleStr(pNewConfig);
    snprintf(eventLog, sizeof(eventLog), "end do config change, from %s to %s", pOldCfgStr, pNewCfgStr);
    syncNodeEventLog(pSyncNode, eventLog);
    taosMemoryFree(pOldCfgStr);
    taosMemoryFree(pNewCfgStr);
  } while (0);
  return;
}

SSyncNode* syncNodeAcquire(int64_t rid) {
  SSyncNode* pNode = taosAcquireRef(tsNodeRefId, rid);
  if (pNode == NULL) {
    sTrace("failed to acquire node from refId:%" PRId64, rid);
  }

  return pNode;
}

void syncNodeRelease(SSyncNode* pNode) { taosReleaseRef(tsNodeRefId, pNode->rid); }

// raft state change --------------
void syncNodeUpdateTerm(SSyncNode* pSyncNode, SyncTerm term) {
  if (term > pSyncNode->pRaftStore->currentTerm) {
    raftStoreSetTerm(pSyncNode->pRaftStore, term);
    char tmpBuf[64];
    snprintf(tmpBuf, sizeof(tmpBuf), "update term to %" PRIu64, term);
    syncNodeBecomeFollower(pSyncNode, tmpBuf);
    raftStoreClearVote(pSyncNode->pRaftStore);
  }
}

void syncNodeUpdateTermWithoutStepDown(SSyncNode* pSyncNode, SyncTerm term) {
  if (term > pSyncNode->pRaftStore->currentTerm) {
    raftStoreSetTerm(pSyncNode->pRaftStore, term);
  }
}

void syncNodeStepDown(SSyncNode* pSyncNode, SyncTerm newTerm) {
  ASSERT(pSyncNode->pRaftStore->currentTerm <= newTerm);

  do {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "step down, new-term:%" PRIu64 ", current-term:%" PRIu64, newTerm,
             pSyncNode->pRaftStore->currentTerm);
    syncNodeEventLog(pSyncNode, logBuf);
  } while (0);

  if (pSyncNode->pRaftStore->currentTerm < newTerm) {
    raftStoreSetTerm(pSyncNode->pRaftStore, newTerm);
    char tmpBuf[64];
    snprintf(tmpBuf, sizeof(tmpBuf), "step down, update term to %" PRIu64, newTerm);
    syncNodeBecomeFollower(pSyncNode, tmpBuf);
    raftStoreClearVote(pSyncNode->pRaftStore);

  } else {
    if (pSyncNode->state != TAOS_SYNC_STATE_FOLLOWER) {
      syncNodeBecomeFollower(pSyncNode, "step down");
    }
  }
}

void syncNodeLeaderChangeRsp(SSyncNode* pSyncNode) { syncRespCleanRsp(pSyncNode->pSyncRespMgr); }

void syncNodeBecomeFollower(SSyncNode* pSyncNode, const char* debugStr) {
  // maybe clear leader cache
  if (pSyncNode->state == TAOS_SYNC_STATE_LEADER) {
    pSyncNode->leaderCache = EMPTY_RAFT_ID;
  }

  // state change
  pSyncNode->state = TAOS_SYNC_STATE_FOLLOWER;
  syncNodeStopHeartbeatTimer(pSyncNode);

  // reset elect timer
  syncNodeResetElectTimer(pSyncNode);

  // send rsp to client
  syncNodeLeaderChangeRsp(pSyncNode);

  // call back
  if (pSyncNode->pFsm != NULL && pSyncNode->pFsm->FpBecomeFollowerCb != NULL) {
    pSyncNode->pFsm->FpBecomeFollowerCb(pSyncNode->pFsm);
  }

  // min match index
  pSyncNode->minMatchIndex = SYNC_INDEX_INVALID;

  // trace log
  do {
    int32_t debugStrLen = strlen(debugStr);
    if (debugStrLen < 256) {
      char eventLog[256 + 64];
      snprintf(eventLog, sizeof(eventLog), "become follower %s", debugStr);
      syncNodeEventLog(pSyncNode, eventLog);
    } else {
      char* eventLog = taosMemoryMalloc(debugStrLen + 64);
      snprintf(eventLog, debugStrLen, "become follower %s", debugStr);
      syncNodeEventLog(pSyncNode, eventLog);
      taosMemoryFree(eventLog);
    }
  } while (0);
}

// TLA+ Spec
// \* Candidate i transitions to leader.
// BecomeLeader(i) ==
//     /\ state[i] = Candidate
//     /\ votesGranted[i] \in Quorum
//     /\ state'      = [state EXCEPT ![i] = Leader]
//     /\ nextIndex'  = [nextIndex EXCEPT ![i] =
//                          [j \in Server |-> Len(log[i]) + 1]]
//     /\ matchIndex' = [matchIndex EXCEPT ![i] =
//                          [j \in Server |-> 0]]
//     /\ elections'  = elections \cup
//                          {[eterm     |-> currentTerm[i],
//                            eleader   |-> i,
//                            elog      |-> log[i],
//                            evotes    |-> votesGranted[i],
//                            evoterLog |-> voterLog[i]]}
//     /\ UNCHANGED <<messages, currentTerm, votedFor, candidateVars, logVars>>
//
void syncNodeBecomeLeader(SSyncNode* pSyncNode, const char* debugStr) {
  pSyncNode->leaderTime = taosGetTimestampMs();

  // reset restoreFinish
  pSyncNode->restoreFinish = false;

  // state change
  pSyncNode->state = TAOS_SYNC_STATE_LEADER;

  // set leader cache
  pSyncNode->leaderCache = pSyncNode->myRaftId;

  for (int i = 0; i < pSyncNode->pNextIndex->replicaNum; ++i) {
    // maybe overwrite myself, no harm
    // just do it!

    // pSyncNode->pNextIndex->index[i] = pSyncNode->pLogStore->getLastIndex(pSyncNode->pLogStore) + 1;

    // maybe wal is deleted
    SyncIndex lastIndex;
    SyncTerm  lastTerm;
    int32_t   code = syncNodeGetLastIndexTerm(pSyncNode, &lastIndex, &lastTerm);
    ASSERT(code == 0);
    pSyncNode->pNextIndex->index[i] = lastIndex + 1;
  }

  for (int i = 0; i < pSyncNode->pMatchIndex->replicaNum; ++i) {
    // maybe overwrite myself, no harm
    // just do it!
    pSyncNode->pMatchIndex->index[i] = SYNC_INDEX_INVALID;
  }

  // init peer mgr
  syncNodePeerStateInit(pSyncNode);

  // update sender private term
  SSyncSnapshotSender* pMySender = syncNodeGetSnapshotSender(pSyncNode, &(pSyncNode->myRaftId));
  if (pMySender != NULL) {
    for (int i = 0; i < pSyncNode->pMatchIndex->replicaNum; ++i) {
      if ((pSyncNode->senders)[i]->privateTerm > pMySender->privateTerm) {
        pMySender->privateTerm = (pSyncNode->senders)[i]->privateTerm;
      }
    }
    (pMySender->privateTerm) += 100;
  }

  // close receiver
  if (snapshotReceiverIsStart(pSyncNode->pNewNodeReceiver)) {
    snapshotReceiverForceStop(pSyncNode->pNewNodeReceiver);
  }

  // stop elect timer
  syncNodeStopElectTimer(pSyncNode);

  // start heartbeat timer
  syncNodeStartHeartbeatTimer(pSyncNode);

  // send heartbeat right now
  syncNodeHeartbeatPeers(pSyncNode);

  // call back
  if (pSyncNode->pFsm != NULL && pSyncNode->pFsm->FpBecomeLeaderCb != NULL) {
    pSyncNode->pFsm->FpBecomeLeaderCb(pSyncNode->pFsm);
  }

  // min match index
  pSyncNode->minMatchIndex = SYNC_INDEX_INVALID;

  // trace log
  do {
    int32_t debugStrLen = strlen(debugStr);
    if (debugStrLen < 256) {
      char eventLog[256 + 64];
      snprintf(eventLog, sizeof(eventLog), "become leader %s", debugStr);
      syncNodeEventLog(pSyncNode, eventLog);
    } else {
      char* eventLog = taosMemoryMalloc(debugStrLen + 64);
      snprintf(eventLog, debugStrLen, "become leader %s", debugStr);
      syncNodeEventLog(pSyncNode, eventLog);
      taosMemoryFree(eventLog);
    }
  } while (0);
}

void syncNodeCandidate2Leader(SSyncNode* pSyncNode) {
  ASSERT(pSyncNode->state == TAOS_SYNC_STATE_CANDIDATE);
  ASSERT(voteGrantedMajority(pSyncNode->pVotesGranted));
  syncNodeBecomeLeader(pSyncNode, "candidate to leader");

  syncNodeLog2("==state change syncNodeCandidate2Leader==", pSyncNode);

  // Raft 3.6.2 Committing entries from previous terms
  syncNodeAppendNoop(pSyncNode);
  syncMaybeAdvanceCommitIndex(pSyncNode);

  if (pSyncNode->replicaNum > 1) {
    syncNodeReplicate(pSyncNode);
  }
}

bool syncNodeIsMnode(SSyncNode* pSyncNode) { return (pSyncNode->vgId == 1); }

int32_t syncNodePeerStateInit(SSyncNode* pSyncNode) {
  for (int i = 0; i < TSDB_MAX_REPLICA; ++i) {
    pSyncNode->peerStates[i].lastSendIndex = SYNC_INDEX_INVALID;
    pSyncNode->peerStates[i].lastSendTime = 0;
  }

  return 0;
}

void syncNodeFollower2Candidate(SSyncNode* pSyncNode) {
  ASSERT(pSyncNode->state == TAOS_SYNC_STATE_FOLLOWER);
  pSyncNode->state = TAOS_SYNC_STATE_CANDIDATE;

  syncNodeEventLog(pSyncNode, "follower to candidate");
}

void syncNodeLeader2Follower(SSyncNode* pSyncNode) {
  ASSERT(pSyncNode->state == TAOS_SYNC_STATE_LEADER);
  syncNodeBecomeFollower(pSyncNode, "leader to follower");

  syncNodeEventLog(pSyncNode, "leader to follower");
}

void syncNodeCandidate2Follower(SSyncNode* pSyncNode) {
  ASSERT(pSyncNode->state == TAOS_SYNC_STATE_CANDIDATE);
  syncNodeBecomeFollower(pSyncNode, "candidate to follower");

  syncNodeEventLog(pSyncNode, "candidate to follower");
}

// raft vote --------------

// just called by syncNodeVoteForSelf
// need assert
void syncNodeVoteForTerm(SSyncNode* pSyncNode, SyncTerm term, SRaftId* pRaftId) {
  ASSERT(term == pSyncNode->pRaftStore->currentTerm);
  ASSERT(!raftStoreHasVoted(pSyncNode->pRaftStore));

  raftStoreVote(pSyncNode->pRaftStore, pRaftId);
}

// simulate get vote from outside
void syncNodeVoteForSelf(SSyncNode* pSyncNode) {
  syncNodeVoteForTerm(pSyncNode, pSyncNode->pRaftStore->currentTerm, &(pSyncNode->myRaftId));

  SyncRequestVoteReply* pMsg = syncRequestVoteReplyBuild(pSyncNode->vgId);
  pMsg->srcId = pSyncNode->myRaftId;
  pMsg->destId = pSyncNode->myRaftId;
  pMsg->term = pSyncNode->pRaftStore->currentTerm;
  pMsg->voteGranted = true;

  voteGrantedVote(pSyncNode->pVotesGranted, pMsg);
  votesRespondAdd(pSyncNode->pVotesRespond, pMsg);
  syncRequestVoteReplyDestroy(pMsg);
}

// snapshot --------------

// return if has a snapshot
bool syncNodeHasSnapshot(SSyncNode* pSyncNode) {
  bool      ret = false;
  SSnapshot snapshot = {.data = NULL, .lastApplyIndex = -1, .lastApplyTerm = 0, .lastConfigIndex = -1};
  if (pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
    if (snapshot.lastApplyIndex >= SYNC_INDEX_BEGIN) {
      ret = true;
    }
  }
  return ret;
}

// return max(logLastIndex, snapshotLastIndex)
// if no snapshot and log, return -1
SyncIndex syncNodeGetLastIndex(const SSyncNode* pSyncNode) {
  SSnapshot snapshot = {.data = NULL, .lastApplyIndex = -1, .lastApplyTerm = 0, .lastConfigIndex = -1};
  if (pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
    pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
  }
  SyncIndex logLastIndex = pSyncNode->pLogStore->syncLogLastIndex(pSyncNode->pLogStore);

  SyncIndex lastIndex = logLastIndex > snapshot.lastApplyIndex ? logLastIndex : snapshot.lastApplyIndex;
  return lastIndex;
}

// return the last term of snapshot and log
// if error, return SYNC_TERM_INVALID (by syncLogLastTerm)
SyncTerm syncNodeGetLastTerm(SSyncNode* pSyncNode) {
  SyncTerm lastTerm = 0;
  if (syncNodeHasSnapshot(pSyncNode)) {
    // has snapshot
    SSnapshot snapshot = {.data = NULL, .lastApplyIndex = -1, .lastApplyTerm = 0, .lastConfigIndex = -1};
    if (pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
      pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
    }

    SyncIndex logLastIndex = pSyncNode->pLogStore->syncLogLastIndex(pSyncNode->pLogStore);
    if (logLastIndex > snapshot.lastApplyIndex) {
      lastTerm = pSyncNode->pLogStore->syncLogLastTerm(pSyncNode->pLogStore);
    } else {
      lastTerm = snapshot.lastApplyTerm;
    }

  } else {
    // no snapshot
    lastTerm = pSyncNode->pLogStore->syncLogLastTerm(pSyncNode->pLogStore);
  }

  return lastTerm;
}

// get last index and term along with snapshot
int32_t syncNodeGetLastIndexTerm(SSyncNode* pSyncNode, SyncIndex* pLastIndex, SyncTerm* pLastTerm) {
  *pLastIndex = syncNodeGetLastIndex(pSyncNode);
  *pLastTerm = syncNodeGetLastTerm(pSyncNode);
  return 0;
}

// return append-entries first try index
SyncIndex syncNodeSyncStartIndex(SSyncNode* pSyncNode) {
  SyncIndex syncStartIndex = syncNodeGetLastIndex(pSyncNode) + 1;
  return syncStartIndex;
}

// if index > 0, return index - 1
// else, return -1
SyncIndex syncNodeGetPreIndex(SSyncNode* pSyncNode, SyncIndex index) {
  SyncIndex preIndex = index - 1;
  if (preIndex < SYNC_INDEX_INVALID) {
    preIndex = SYNC_INDEX_INVALID;
  }

  return preIndex;
}

// if index < 0, return SYNC_TERM_INVALID
// if index == 0, return 0
// if index > 0, return preTerm
// if error, return SYNC_TERM_INVALID
SyncTerm syncNodeGetPreTerm(SSyncNode* pSyncNode, SyncIndex index) {
  if (index < SYNC_INDEX_BEGIN) {
    return SYNC_TERM_INVALID;
  }

  if (index == SYNC_INDEX_BEGIN) {
    return 0;
  }

  SyncTerm        preTerm = 0;
  SyncIndex       preIndex = index - 1;
  SSyncRaftEntry* pPreEntry = NULL;
  int32_t         code = pSyncNode->pLogStore->syncLogGetEntry(pSyncNode->pLogStore, preIndex, &pPreEntry);

  SSnapshot snapshot = {.data = NULL,
                        .lastApplyIndex = SYNC_INDEX_INVALID,
                        .lastApplyTerm = SYNC_TERM_INVALID,
                        .lastConfigIndex = SYNC_INDEX_INVALID};

  if (code == 0) {
    ASSERT(pPreEntry != NULL);
    preTerm = pPreEntry->term;
    taosMemoryFree(pPreEntry);
    return preTerm;
  } else {
    if (pSyncNode->pFsm->FpGetSnapshotInfo != NULL) {
      pSyncNode->pFsm->FpGetSnapshotInfo(pSyncNode->pFsm, &snapshot);
      if (snapshot.lastApplyIndex == preIndex) {
        return snapshot.lastApplyTerm;
      }
    }
  }

  do {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf),
             "sync node get pre term error, index:%" PRId64 ", snap-index:%" PRId64 ", snap-term:%" PRIu64, index,
             snapshot.lastApplyIndex, snapshot.lastApplyTerm);
    syncNodeErrorLog(pSyncNode, logBuf);
  } while (0);

  return SYNC_TERM_INVALID;
}

// get pre index and term of "index"
int32_t syncNodeGetPreIndexTerm(SSyncNode* pSyncNode, SyncIndex index, SyncIndex* pPreIndex, SyncTerm* pPreTerm) {
  *pPreIndex = syncNodeGetPreIndex(pSyncNode, index);
  *pPreTerm = syncNodeGetPreTerm(pSyncNode, index);
  return 0;
}

// for debug --------------
void syncNodePrint(SSyncNode* pObj) {
  char* serialized = syncNode2Str(pObj);
  printf("syncNodePrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncNodePrint2(char* s, SSyncNode* pObj) {
  char* serialized = syncNode2Str(pObj);
  printf("syncNodePrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncNodeLog(SSyncNode* pObj) {
  char* serialized = syncNode2Str(pObj);
  sTraceLong("syncNodeLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncNodeLog2(char* s, SSyncNode* pObj) {
  if (gRaftDetailLog) {
    char* serialized = syncNode2Str(pObj);
    sTraceLong("syncNodeLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}

void syncNodeLog3(char* s, SSyncNode* pObj) {
  char* serialized = syncNode2Str(pObj);
  sTraceLong("syncNodeLog3 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
  taosMemoryFree(serialized);
}

// ------ local funciton ---------
// enqueue message ----
static void syncNodeEqPingTimer(void* param, void* tmrId) {
  SSyncNode* pSyncNode = (SSyncNode*)param;
  if (atomic_load_64(&pSyncNode->pingTimerLogicClockUser) <= atomic_load_64(&pSyncNode->pingTimerLogicClock)) {
    SyncTimeout* pSyncMsg = syncTimeoutBuild2(SYNC_TIMEOUT_PING, atomic_load_64(&pSyncNode->pingTimerLogicClock),
                                              pSyncNode->pingTimerMS, pSyncNode->vgId, pSyncNode);
    SRpcMsg      rpcMsg;
    syncTimeout2RpcMsg(pSyncMsg, &rpcMsg);
    syncRpcMsgLog2((char*)"==syncNodeEqPingTimer==", &rpcMsg);
    if (pSyncNode->FpEqMsg != NULL) {
      int32_t code = pSyncNode->FpEqMsg(pSyncNode->msgcb, &rpcMsg);
      if (code != 0) {
        sError("vgId:%d, sync enqueue ping msg error, code:%d", pSyncNode->vgId, code);
        rpcFreeCont(rpcMsg.pCont);
        syncTimeoutDestroy(pSyncMsg);
        return;
      }
    } else {
      sTrace("syncNodeEqPingTimer pSyncNode->FpEqMsg is NULL");
    }
    syncTimeoutDestroy(pSyncMsg);

    if (syncEnvIsStart()) {
      taosTmrReset(syncNodeEqPingTimer, pSyncNode->pingTimerMS, pSyncNode, gSyncEnv->pTimerManager,
                   &pSyncNode->pPingTimer);
    } else {
      sError("sync env is stop, syncNodeEqPingTimer");
    }

  } else {
    sTrace("==syncNodeEqPingTimer== pingTimerLogicClock:%" PRIu64 ", pingTimerLogicClockUser:%" PRIu64,
           pSyncNode->pingTimerLogicClock, pSyncNode->pingTimerLogicClockUser);
  }
}

static void syncNodeEqElectTimer(void* param, void* tmrId) {
  SElectTimer* pElectTimer = (SElectTimer*)param;
  SSyncNode*   pSyncNode = pElectTimer->pSyncNode;

  SyncTimeout* pSyncMsg = syncTimeoutBuild2(SYNC_TIMEOUT_ELECTION, pElectTimer->logicClock, pSyncNode->electTimerMS,
                                            pSyncNode->vgId, pSyncNode);
  SRpcMsg      rpcMsg;
  syncTimeout2RpcMsg(pSyncMsg, &rpcMsg);
  if (pSyncNode->FpEqMsg != NULL) {
    int32_t code = pSyncNode->FpEqMsg(pSyncNode->msgcb, &rpcMsg);
    if (code != 0) {
      sError("vgId:%d, sync enqueue elect msg error, code:%d", pSyncNode->vgId, code);
      rpcFreeCont(rpcMsg.pCont);
      syncTimeoutDestroy(pSyncMsg);
      taosMemoryFree(pElectTimer);
      return;
    }

    do {
      char logBuf[128];
      snprintf(logBuf, sizeof(logBuf), "eq elect timer lc:%" PRIu64, pSyncMsg->logicClock);
      syncNodeEventLog(pSyncNode, logBuf);
    } while (0);

  } else {
    sTrace("syncNodeEqElectTimer FpEqMsg is NULL");
  }

  syncTimeoutDestroy(pSyncMsg);
  taosMemoryFree(pElectTimer);

#if 0
  // reset timer ms
  if (syncEnvIsStart() && pSyncNode->electBaseLine > 0) {
    pSyncNode->electTimerMS = syncUtilElectRandomMS(pSyncNode->electBaseLine, 2 * pSyncNode->electBaseLine);
    taosTmrReset(syncNodeEqElectTimer, pSyncNode->electTimerMS, pSyncNode, gSyncEnv->pTimerManager,
                 &pSyncNode->pElectTimer);
  } else {
    sError("sync env is stop, syncNodeEqElectTimer");
  }
#endif
}

static void syncNodeEqHeartbeatTimer(void* param, void* tmrId) {
  SSyncNode* pSyncNode = (SSyncNode*)param;

  syncNodeEventLog(pSyncNode, "eq hb timer");

  if (pSyncNode->replicaNum > 1) {
    if (atomic_load_64(&pSyncNode->heartbeatTimerLogicClockUser) <=
        atomic_load_64(&pSyncNode->heartbeatTimerLogicClock)) {
      SyncTimeout* pSyncMsg =
          syncTimeoutBuild2(SYNC_TIMEOUT_HEARTBEAT, atomic_load_64(&pSyncNode->heartbeatTimerLogicClock),
                            pSyncNode->heartbeatTimerMS, pSyncNode->vgId, pSyncNode);
      SRpcMsg rpcMsg;
      syncTimeout2RpcMsg(pSyncMsg, &rpcMsg);
      syncRpcMsgLog2((char*)"==syncNodeEqHeartbeatTimer==", &rpcMsg);
      if (pSyncNode->FpEqMsg != NULL) {
        int32_t code = pSyncNode->FpEqMsg(pSyncNode->msgcb, &rpcMsg);
        if (code != 0) {
          sError("vgId:%d, sync enqueue timer msg error, code:%d", pSyncNode->vgId, code);
          rpcFreeCont(rpcMsg.pCont);
          syncTimeoutDestroy(pSyncMsg);
          return;
        }
      } else {
        sError("vgId:%d, enqueue msg cb ptr (i.e. FpEqMsg) not set.", pSyncNode->vgId);
      }
      syncTimeoutDestroy(pSyncMsg);

      if (syncEnvIsStart()) {
        taosTmrReset(syncNodeEqHeartbeatTimer, pSyncNode->heartbeatTimerMS, pSyncNode, gSyncEnv->pTimerManager,
                     &pSyncNode->pHeartbeatTimer);
      } else {
        sError("sync env is stop, syncNodeEqHeartbeatTimer");
      }
    } else {
      sTrace("==syncNodeEqHeartbeatTimer== heartbeatTimerLogicClock:%" PRIu64 ", heartbeatTimerLogicClockUser:%" PRIu64
             "",
             pSyncNode->heartbeatTimerLogicClock, pSyncNode->heartbeatTimerLogicClockUser);
    }
  }
}

static void syncNodeEqPeerHeartbeatTimer(void* param, void* tmrId) {
  SSyncHbTimerData* pData = (SSyncHbTimerData*)param;
  SSyncNode*        pSyncNode = pData->pSyncNode;
  SSyncTimer*       pSyncTimer = pData->pTimer;

  if (pSyncNode->state != TAOS_SYNC_STATE_LEADER) {
    return;
  }

  // syncNodeEventLog(pSyncNode, "eq peer hb timer");

  int64_t timerLogicClock = atomic_load_64(&pSyncTimer->logicClock);
  int64_t msgLogicClock = atomic_load_64(&pData->logicClock);

  if (pSyncNode->replicaNum > 1) {
    if (timerLogicClock == msgLogicClock) {
      SyncHeartbeat* pSyncMsg = syncHeartbeatBuild(pSyncNode->vgId);
      pSyncMsg->srcId = pSyncNode->myRaftId;
      pSyncMsg->destId = pData->destId;
      pSyncMsg->term = pSyncNode->pRaftStore->currentTerm;
      pSyncMsg->commitIndex = pSyncNode->commitIndex;
      pSyncMsg->minMatchIndex = syncMinMatchIndex(pSyncNode);
      pSyncMsg->privateTerm = 0;

      SRpcMsg rpcMsg;
      syncHeartbeat2RpcMsg(pSyncMsg, &rpcMsg);

// eq msg
#if 0
      if (pSyncNode->FpEqCtrlMsg != NULL) {
        int32_t code = pSyncNode->FpEqCtrlMsg(pSyncNode->msgcb, &rpcMsg);
        if (code != 0) {
          sError("vgId:%d, sync ctrl enqueue timer msg error, code:%d", pSyncNode->vgId, code);
          rpcFreeCont(rpcMsg.pCont);
          syncHeartbeatDestroy(pSyncMsg);
          return;
        }
      } else {
        sError("vgId:%d, enqueue ctrl msg cb ptr (i.e. FpEqMsg) not set.", pSyncNode->vgId);
      }
#endif

      // send msg
      syncNodeSendHeartbeat(pSyncNode, &(pSyncMsg->destId), pSyncMsg);

      syncHeartbeatDestroy(pSyncMsg);

      if (syncEnvIsStart()) {
        taosTmrReset(syncNodeEqPeerHeartbeatTimer, pSyncTimer->timerMS, pData, gSyncEnv->pTimerManager,
                     &pSyncTimer->pTimer);
      } else {
        sError("sync env is stop, syncNodeEqHeartbeatTimer");
      }

    } else {
      sTrace("==syncNodeEqPeerHeartbeatTimer== timerLogicClock:%" PRIu64 ", msgLogicClock:%" PRIu64 "", timerLogicClock,
             msgLogicClock);
    }
  }
}

static int32_t syncNodeEqNoop(SSyncNode* ths) {
  int32_t ret = 0;
  ASSERT(ths->state == TAOS_SYNC_STATE_LEADER);

  SyncIndex       index = ths->pLogStore->syncLogWriteIndex(ths->pLogStore);
  SyncTerm        term = ths->pRaftStore->currentTerm;
  SSyncRaftEntry* pEntry = syncEntryBuildNoop(term, index, ths->vgId);
  ASSERT(pEntry != NULL);

  uint32_t           entryLen;
  char*              serialized = syncEntrySerialize(pEntry, &entryLen);
  SyncClientRequest* pSyncMsg = syncClientRequestBuild(entryLen);
  ASSERT(pSyncMsg->dataLen == entryLen);
  memcpy(pSyncMsg->data, serialized, entryLen);

  SRpcMsg rpcMsg = {0};
  syncClientRequest2RpcMsg(pSyncMsg, &rpcMsg);
  if (ths->FpEqMsg != NULL) {
    ths->FpEqMsg(ths->msgcb, &rpcMsg);
  } else {
    sTrace("syncNodeEqNoop pSyncNode->FpEqMsg is NULL");
  }

  syncEntryDestory(pEntry);
  taosMemoryFree(serialized);
  syncClientRequestDestroy(pSyncMsg);

  return ret;
}

static void deleteCacheEntry(const void* key, size_t keyLen, void* value) { taosMemoryFree(value); }

static int32_t syncCacheEntry(SSyncLogStore* pLogStore, SSyncRaftEntry* pEntry, LRUHandle** h) {
  int       code = 0;
  int       entryLen = sizeof(*pEntry) + pEntry->dataLen;
  LRUStatus status = taosLRUCacheInsert(pLogStore->pCache, &pEntry->index, sizeof(pEntry->index), pEntry, entryLen,
                                        deleteCacheEntry, h, TAOS_LRU_PRIORITY_LOW);
  if (status != TAOS_LRU_STATUS_OK) {
    code = -1;
  }

  return code;
}

static int32_t syncNodeAppendNoop(SSyncNode* ths) {
  int32_t ret = 0;

  SyncIndex       index = ths->pLogStore->syncLogWriteIndex(ths->pLogStore);
  SyncTerm        term = ths->pRaftStore->currentTerm;
  SSyncRaftEntry* pEntry = syncEntryBuildNoop(term, index, ths->vgId);
  ASSERT(pEntry != NULL);

  LRUHandle* h = NULL;
  syncCacheEntry(ths->pLogStore, pEntry, &h);

  if (ths->state == TAOS_SYNC_STATE_LEADER) {
    int32_t code = ths->pLogStore->syncLogAppendEntry(ths->pLogStore, pEntry);
    if (code != 0) {
      syncNodeErrorLog(ths, "append noop error");
      return -1;
    }
  }

  if (h) {
    taosLRUCacheRelease(ths->pLogStore->pCache, h, false);
  } else {
    syncEntryDestory(pEntry);
  }

  return ret;
}

// on message ----
int32_t syncNodeOnPing(SSyncNode* ths, SyncPing* pMsg) {
  sTrace("vgId:%d, recv sync-ping", ths->vgId);

  SyncPingReply* pMsgReply = syncPingReplyBuild3(&ths->myRaftId, &pMsg->srcId, ths->vgId);
  SRpcMsg        rpcMsg;
  syncPingReply2RpcMsg(pMsgReply, &rpcMsg);

  /*
    // htonl
    SMsgHead* pHead = rpcMsg.pCont;
    pHead->contLen = htonl(pHead->contLen);
    pHead->vgId = htonl(pHead->vgId);
  */

  syncNodeSendMsgById(&pMsgReply->destId, ths, &rpcMsg);
  syncPingReplyDestroy(pMsgReply);

  return 0;
}

int32_t syncNodeOnPingReply(SSyncNode* ths, SyncPingReply* pMsg) {
  int32_t ret = 0;
  sTrace("vgId:%d, recv sync-ping-reply", ths->vgId);
  return ret;
}

int32_t syncNodeOnHeartbeat(SSyncNode* ths, SyncHeartbeat* pMsg) {
  syncLogRecvHeartbeat(ths, pMsg, "");

  SyncHeartbeatReply* pMsgReply = syncHeartbeatReplyBuild(ths->vgId);
  pMsgReply->destId = pMsg->srcId;
  pMsgReply->srcId = ths->myRaftId;
  pMsgReply->term = ths->pRaftStore->currentTerm;
  pMsgReply->privateTerm = 8864;  // magic number

  SRpcMsg rpcMsg;
  syncHeartbeatReply2RpcMsg(pMsgReply, &rpcMsg);

#if 1
  if (pMsg->term >= ths->pRaftStore->currentTerm && ths->state != TAOS_SYNC_STATE_FOLLOWER) {
    syncNodeStepDown(ths, pMsg->term);
  }
#endif

  if (pMsg->term == ths->pRaftStore->currentTerm && ths->state != TAOS_SYNC_STATE_LEADER) {
    syncNodeResetElectTimer(ths);
    ths->minMatchIndex = pMsg->minMatchIndex;

#if 0
    if (ths->state == TAOS_SYNC_STATE_FOLLOWER) {
      syncNodeFollowerCommit(ths, pMsg->commitIndex);
    }
#endif
  }

  /*
    // htonl
    SMsgHead* pHead = rpcMsg.pCont;
    pHead->contLen = htonl(pHead->contLen);
    pHead->vgId = htonl(pHead->vgId);
  */

  // reply
  syncNodeSendMsgById(&pMsgReply->destId, ths, &rpcMsg);
  syncHeartbeatReplyDestroy(pMsgReply);

  return 0;
}

int32_t syncNodeOnHeartbeatReply(SSyncNode* ths, SyncHeartbeatReply* pMsg) {
  syncLogRecvHeartbeatReply(ths, pMsg, "");

  // update last reply time, make decision whether the other node is alive or not
  syncIndexMgrSetRecvTime(ths->pMatchIndex, &(pMsg->destId), pMsg->startTime);

  return 0;
}

// TLA+ Spec
// ClientRequest(i, v) ==
//     /\ state[i] = Leader
//     /\ LET entry == [term  |-> currentTerm[i],
//                      value |-> v]
//            newLog == Append(log[i], entry)
//        IN  log' = [log EXCEPT ![i] = newLog]
//     /\ UNCHANGED <<messages, serverVars, candidateVars,
//                    leaderVars, commitIndex>>
//

int32_t syncNodeOnClientRequest(SSyncNode* ths, SyncClientRequest* pMsg, SyncIndex* pRetIndex) {
  syncNodeEventLog(ths, "on client request");

  int32_t ret = 0;
  int32_t code = 0;

  SyncIndex       index = ths->pLogStore->syncLogWriteIndex(ths->pLogStore);
  SyncTerm        term = ths->pRaftStore->currentTerm;
  SSyncRaftEntry* pEntry = syncEntryBuild2((SyncClientRequest*)pMsg, term, index);
  ASSERT(pEntry != NULL);

  LRUHandle* h = NULL;
  syncCacheEntry(ths->pLogStore, pEntry, &h);

  if (ths->state == TAOS_SYNC_STATE_LEADER) {
    // append entry
    code = ths->pLogStore->syncLogAppendEntry(ths->pLogStore, pEntry);
    if (code != 0) {
      // del resp mgr, call FpCommitCb
      ASSERT(0);
      return -1;
    }

    // if mulit replica, start replicate right now
    if (ths->replicaNum > 1) {
      syncNodeReplicate(ths);
    }

    // if only myself, maybe commit right now
    if (ths->replicaNum == 1) {
      syncMaybeAdvanceCommitIndex(ths);
    }
  }

  if (pRetIndex != NULL) {
    if (ret == 0 && pEntry != NULL) {
      *pRetIndex = pEntry->index;
    } else {
      *pRetIndex = SYNC_INDEX_INVALID;
    }
  }

  if (h) {
    taosLRUCacheRelease(ths->pLogStore->pCache, h, false);
  } else {
    syncEntryDestory(pEntry);
  }

  return ret;
}

const char* syncStr(ESyncState state) {
  switch (state) {
    case TAOS_SYNC_STATE_FOLLOWER:
      return "follower";
    case TAOS_SYNC_STATE_CANDIDATE:
      return "candidate";
    case TAOS_SYNC_STATE_LEADER:
      return "leader";
    default:
      return "error";
  }
}

int32_t syncDoLeaderTransfer(SSyncNode* ths, SRpcMsg* pRpcMsg, SSyncRaftEntry* pEntry) {
  if (ths->state != TAOS_SYNC_STATE_FOLLOWER) {
    syncNodeEventLog(ths, "I am not follower, can not do leader transfer");
    return 0;
  }

  if (!ths->restoreFinish) {
    syncNodeEventLog(ths, "restore not finish, can not do leader transfer");
    return 0;
  }

  if (pEntry->term < ths->pRaftStore->currentTerm) {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "little term:%" PRIu64 ", can not do leader transfer", pEntry->term);
    syncNodeEventLog(ths, logBuf);
    return 0;
  }

  if (pEntry->index < syncNodeGetLastIndex(ths)) {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "little index:%" PRId64 ", can not do leader transfer", pEntry->index);
    syncNodeEventLog(ths, logBuf);
    return 0;
  }

  /*
    if (ths->vgId > 1) {
      syncNodeEventLog(ths, "I am vnode, can not do leader transfer");
      return 0;
    }
  */

  SyncLeaderTransfer* pSyncLeaderTransfer = syncLeaderTransferFromRpcMsg2(pRpcMsg);

  do {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf), "do leader transfer, index:%" PRId64, pEntry->index);
    syncNodeEventLog(ths, logBuf);
  } while (0);

  bool sameId = syncUtilSameId(&(pSyncLeaderTransfer->newLeaderId), &(ths->myRaftId));
  bool sameNodeInfo = strcmp(pSyncLeaderTransfer->newNodeInfo.nodeFqdn, ths->myNodeInfo.nodeFqdn) == 0 &&
                      pSyncLeaderTransfer->newNodeInfo.nodePort == ths->myNodeInfo.nodePort;

  bool same = sameId || sameNodeInfo;
  if (same) {
    // reset elect timer now!
    int32_t electMS = 1;
    int32_t ret = syncNodeRestartElectTimer(ths, electMS);
    ASSERT(ret == 0);

    char eventLog[256];
    snprintf(eventLog, sizeof(eventLog), "maybe leader transfer to %s:%d %" PRIu64,
             pSyncLeaderTransfer->newNodeInfo.nodeFqdn, pSyncLeaderTransfer->newNodeInfo.nodePort,
             pSyncLeaderTransfer->newLeaderId.addr);
    syncNodeEventLog(ths, eventLog);
  }

  if (ths->pFsm->FpLeaderTransferCb != NULL) {
    SFsmCbMeta cbMeta = {0};
    cbMeta.code = 0;
    cbMeta.currentTerm = ths->pRaftStore->currentTerm;
    cbMeta.flag = 0;
    cbMeta.index = pEntry->index;
    cbMeta.lastConfigIndex = syncNodeGetSnapshotConfigIndex(ths, cbMeta.index);
    cbMeta.isWeak = pEntry->isWeak;
    cbMeta.seqNum = pEntry->seqNum;
    cbMeta.state = ths->state;
    cbMeta.term = pEntry->term;
    ths->pFsm->FpLeaderTransferCb(ths->pFsm, pRpcMsg, cbMeta);
  }

  syncLeaderTransferDestroy(pSyncLeaderTransfer);
  return 0;
}

int32_t syncNodeUpdateNewConfigIndex(SSyncNode* ths, SSyncCfg* pNewCfg) {
  for (int i = 0; i < pNewCfg->replicaNum; ++i) {
    SRaftId raftId;
    raftId.addr = syncUtilAddr2U64((pNewCfg->nodeInfo)[i].nodeFqdn, (pNewCfg->nodeInfo)[i].nodePort);
    raftId.vgId = ths->vgId;

    if (syncUtilSameId(&(ths->myRaftId), &raftId)) {
      pNewCfg->myIndex = i;
      return 0;
    }
  }

  return -1;
}

static int32_t syncNodeConfigChangeFinish(SSyncNode* ths, SRpcMsg* pRpcMsg, SSyncRaftEntry* pEntry) {
  SyncReconfigFinish* pFinish = syncReconfigFinishFromRpcMsg2(pRpcMsg);
  ASSERT(pFinish);

  if (ths->pFsm->FpReConfigCb != NULL) {
    SReConfigCbMeta cbMeta = {0};
    cbMeta.code = 0;
    cbMeta.index = pEntry->index;
    cbMeta.term = pEntry->term;
    cbMeta.seqNum = pEntry->seqNum;
    cbMeta.lastConfigIndex = syncNodeGetSnapshotConfigIndex(ths, pEntry->index);
    cbMeta.state = ths->state;
    cbMeta.currentTerm = ths->pRaftStore->currentTerm;
    cbMeta.isWeak = pEntry->isWeak;
    cbMeta.flag = 0;

    cbMeta.oldCfg = pFinish->oldCfg;
    cbMeta.newCfg = pFinish->newCfg;
    cbMeta.newCfgIndex = pFinish->newCfgIndex;
    cbMeta.newCfgTerm = pFinish->newCfgTerm;
    cbMeta.newCfgSeqNum = pFinish->newCfgSeqNum;

    ths->pFsm->FpReConfigCb(ths->pFsm, pRpcMsg, &cbMeta);
  }

  // clear changing
  ths->changing = false;

  char  tmpbuf[512];
  char* oldStr = syncCfg2SimpleStr(&(pFinish->oldCfg));
  char* newStr = syncCfg2SimpleStr(&(pFinish->newCfg));
  snprintf(tmpbuf, sizeof(tmpbuf), "config change finish from %d to %d, index:%" PRId64 ", %s  -->  %s",
           pFinish->oldCfg.replicaNum, pFinish->newCfg.replicaNum, pFinish->newCfgIndex, oldStr, newStr);
  taosMemoryFree(oldStr);
  taosMemoryFree(newStr);
  syncNodeEventLog(ths, tmpbuf);

  syncReconfigFinishDestroy(pFinish);

  return 0;
}

static int32_t syncNodeConfigChange(SSyncNode* ths, SRpcMsg* pRpcMsg, SSyncRaftEntry* pEntry,
                                    SyncReconfigFinish* pFinish) {
  // set changing
  ths->changing = true;

  // old config
  SSyncCfg oldSyncCfg = ths->pRaftCfg->cfg;

  // new config
  SSyncCfg newSyncCfg;
  int32_t  ret = syncCfgFromStr(pRpcMsg->pCont, &newSyncCfg);
  ASSERT(ret == 0);

  // update new config myIndex
  syncNodeUpdateNewConfigIndex(ths, &newSyncCfg);

  // do config change
  syncNodeDoConfigChange(ths, &newSyncCfg, pEntry->index);

  // set pFinish
  pFinish->oldCfg = oldSyncCfg;
  pFinish->newCfg = newSyncCfg;
  pFinish->newCfgIndex = pEntry->index;
  pFinish->newCfgTerm = pEntry->term;
  pFinish->newCfgSeqNum = pEntry->seqNum;

  return 0;
}

static int32_t syncNodeProposeConfigChangeFinish(SSyncNode* ths, SyncReconfigFinish* pFinish) {
  SRpcMsg rpcMsg;
  syncReconfigFinish2RpcMsg(pFinish, &rpcMsg);

  int32_t code = syncNodePropose(ths, &rpcMsg, false);
  if (code != 0) {
    sError("syncNodeProposeConfigChangeFinish error");
    ths->changing = false;
  }
  return 0;
}

bool syncNodeIsOptimizedOneReplica(SSyncNode* ths, SRpcMsg* pMsg) {
  return (ths->replicaNum == 1 && syncUtilUserCommit(pMsg->msgType) && ths->vgId != 1);
}

int32_t syncNodeDoCommit(SSyncNode* ths, SyncIndex beginIndex, SyncIndex endIndex, uint64_t flag) {
  if (beginIndex > endIndex) {
    return 0;
  }

  if (ths == NULL) {
    return -1;
  }

  if (ths->pFsm != NULL && ths->pFsm->FpGetSnapshotInfo != NULL) {
    // advance commit index to sanpshot first
    SSnapshot snapshot = {0};
    ths->pFsm->FpGetSnapshotInfo(ths->pFsm, &snapshot);
    if (snapshot.lastApplyIndex >= 0 && snapshot.lastApplyIndex >= beginIndex) {
      char eventLog[128];
      snprintf(eventLog, sizeof(eventLog), "commit by snapshot from index:%" PRId64 " to index:%" PRId64, beginIndex,
               snapshot.lastApplyIndex);
      syncNodeEventLog(ths, eventLog);

      // update begin index
      beginIndex = snapshot.lastApplyIndex + 1;
    }
  }

  int32_t    code = 0;
  ESyncState state = flag;

  char eventLog[128];
  snprintf(eventLog, sizeof(eventLog), "commit by wal from index:%" PRId64 " to index:%" PRId64, beginIndex, endIndex);
  syncNodeEventLog(ths, eventLog);

  // execute fsm
  if (ths->pFsm != NULL) {
    for (SyncIndex i = beginIndex; i <= endIndex; ++i) {
      if (i != SYNC_INDEX_INVALID) {
        SSyncRaftEntry* pEntry;
        SLRUCache*      pCache = ths->pLogStore->pCache;
        LRUHandle*      h = taosLRUCacheLookup(pCache, &i, sizeof(i));
        if (h) {
          pEntry = (SSyncRaftEntry*)taosLRUCacheValue(pCache, h);
        } else {
          code = ths->pLogStore->syncLogGetEntry(ths->pLogStore, i, &pEntry);
          // ASSERT(code == 0);
          // ASSERT(pEntry != NULL);
          if (code != 0 || pEntry == NULL) {
			syncNodeErrorLog(ths, "get log entry error");
            continue;
          }
        }

        SRpcMsg rpcMsg;
        syncEntry2OriginalRpc(pEntry, &rpcMsg);

        // user commit
        if ((ths->pFsm->FpCommitCb != NULL) && syncUtilUserCommit(pEntry->originalRpcType)) {
          bool internalExecute = true;
          if ((ths->replicaNum == 1) && ths->restoreFinish && ths->vgId != 1) {
            internalExecute = false;
          }

          do {
            char logBuf[128];
            snprintf(logBuf, sizeof(logBuf), "commit index:%" PRId64 ", internal:%d", i, internalExecute);
            syncNodeEventLog(ths, logBuf);
          } while (0);

          // execute fsm in apply thread, or execute outside syncPropose
          if (internalExecute) {
            SFsmCbMeta cbMeta = {0};
            cbMeta.index = pEntry->index;
            cbMeta.lastConfigIndex = syncNodeGetSnapshotConfigIndex(ths, cbMeta.index);
            cbMeta.isWeak = pEntry->isWeak;
            cbMeta.code = 0;
            cbMeta.state = ths->state;
            cbMeta.seqNum = pEntry->seqNum;
            cbMeta.term = pEntry->term;
            cbMeta.currentTerm = ths->pRaftStore->currentTerm;
            cbMeta.flag = flag;

            ths->pFsm->FpCommitCb(ths->pFsm, &rpcMsg, cbMeta);
          }
        }

        // config change
        if (pEntry->originalRpcType == TDMT_SYNC_CONFIG_CHANGE) {
          SyncReconfigFinish* pFinish = syncReconfigFinishBuild(ths->vgId);
          ASSERT(pFinish != NULL);

          code = syncNodeConfigChange(ths, &rpcMsg, pEntry, pFinish);
          ASSERT(code == 0);

          if (ths->state == TAOS_SYNC_STATE_LEADER) {
            syncNodeProposeConfigChangeFinish(ths, pFinish);
          }
          syncReconfigFinishDestroy(pFinish);
        }

        // config change finish
        if (pEntry->originalRpcType == TDMT_SYNC_CONFIG_CHANGE_FINISH) {
          if (rpcMsg.pCont != NULL && rpcMsg.contLen > 0) {
            code = syncNodeConfigChangeFinish(ths, &rpcMsg, pEntry);
            ASSERT(code == 0);
          }
        }

#if 0
        // execute in pre-commit
        // leader transfer
        if (pEntry->originalRpcType == TDMT_SYNC_LEADER_TRANSFER) {
          code = syncDoLeaderTransfer(ths, &rpcMsg, pEntry);
          ASSERT(code == 0);
        }
#endif

        // restore finish
        // if only snapshot, a noop entry will be append, so syncLogLastIndex is always ok
        if (pEntry->index == ths->pLogStore->syncLogLastIndex(ths->pLogStore)) {
          if (ths->restoreFinish == false) {
            if (ths->pFsm->FpRestoreFinishCb != NULL) {
              ths->pFsm->FpRestoreFinishCb(ths->pFsm);
            }
            ths->restoreFinish = true;

            int64_t restoreDelay = taosGetTimestampMs() - ths->leaderTime;

            char eventLog[128];
            snprintf(eventLog, sizeof(eventLog), "restore finish, index:%" PRId64 ", elapsed:%" PRId64 " ms, ",
                     pEntry->index, restoreDelay);
            syncNodeEventLog(ths, eventLog);
          }
        }

        rpcFreeCont(rpcMsg.pCont);
        if (h) {
          taosLRUCacheRelease(pCache, h, false);
        } else {
          syncEntryDestory(pEntry);
        }
      }
    }
  }
  return 0;
}

bool syncNodeInRaftGroup(SSyncNode* ths, SRaftId* pRaftId) {
  for (int i = 0; i < ths->replicaNum; ++i) {
    if (syncUtilSameId(&((ths->replicasId)[i]), pRaftId)) {
      return true;
    }
  }
  return false;
}

SSyncSnapshotSender* syncNodeGetSnapshotSender(SSyncNode* ths, SRaftId* pDestId) {
  SSyncSnapshotSender* pSender = NULL;
  for (int i = 0; i < ths->replicaNum; ++i) {
    if (syncUtilSameId(pDestId, &((ths->replicasId)[i]))) {
      pSender = (ths->senders)[i];
    }
  }
  return pSender;
}

SSyncTimer* syncNodeGetHbTimer(SSyncNode* ths, SRaftId* pDestId) {
  SSyncTimer* pTimer = NULL;
  for (int i = 0; i < ths->replicaNum; ++i) {
    if (syncUtilSameId(pDestId, &((ths->replicasId)[i]))) {
      pTimer = &((ths->peerHeartbeatTimerArr)[i]);
    }
  }
  return pTimer;
}

SPeerState* syncNodeGetPeerState(SSyncNode* ths, const SRaftId* pDestId) {
  SPeerState* pState = NULL;
  for (int i = 0; i < ths->replicaNum; ++i) {
    if (syncUtilSameId(pDestId, &((ths->replicasId)[i]))) {
      pState = &((ths->peerStates)[i]);
    }
  }
  return pState;
}

bool syncNodeNeedSendAppendEntries(SSyncNode* ths, const SRaftId* pDestId, const SyncAppendEntries* pMsg) {
  SPeerState* pState = syncNodeGetPeerState(ths, pDestId);
  if (pState == NULL) {
    sError("vgId:%d, replica maybe dropped", ths->vgId);
    return false;
  }

  SyncIndex sendIndex = pMsg->prevLogIndex + 1;
  int64_t   tsNow = taosGetTimestampMs();

  if (pState->lastSendIndex == sendIndex && tsNow - pState->lastSendTime < SYNC_APPEND_ENTRIES_TIMEOUT_MS) {
    return false;
  }

  return true;
}

bool syncNodeCanChange(SSyncNode* pSyncNode) {
  if (pSyncNode->changing) {
    sError("sync cannot change");
    return false;
  }

  if ((pSyncNode->commitIndex >= SYNC_INDEX_BEGIN)) {
    SyncIndex lastIndex = syncNodeGetLastIndex(pSyncNode);
    if (pSyncNode->commitIndex != lastIndex) {
      sError("sync cannot change2");
      return false;
    }
  }

  for (int i = 0; i < pSyncNode->peersNum; ++i) {
    SSyncSnapshotSender* pSender = syncNodeGetSnapshotSender(pSyncNode, &(pSyncNode->peersId)[i]);
    if (pSender != NULL && pSender->start) {
      sError("sync cannot change3");
      return false;
    }
  }

  return true;
}

const char* syncTimerTypeStr(enum ESyncTimeoutType timerType) {
  if (timerType == SYNC_TIMEOUT_PING) {
    return "ping";
  } else if (timerType == SYNC_TIMEOUT_ELECTION) {
    return "elect";
  } else if (timerType == SYNC_TIMEOUT_HEARTBEAT) {
    return "heartbeat";
  } else {
    return "unknown";
  }
}

void syncLogRecvTimer(SSyncNode* pSyncNode, const SyncTimeout* pMsg, const char* s) {
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf), "recv sync-timer {type:%s, lc:%" PRIu64 ", ms:%d, data:%p}, %s",
           syncTimerTypeStr(pMsg->timeoutType), pMsg->logicClock, pMsg->timerMS, pMsg->data, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendRequestVote(SSyncNode* pSyncNode, const SyncRequestVote* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "send sync-request-vote to %s:%d {term:%" PRIu64 ", lindex:%" PRId64 ", lterm:%" PRIu64 "}, %s", host, port,
           pMsg->term, pMsg->lastLogIndex, pMsg->lastLogTerm, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvRequestVote(SSyncNode* pSyncNode, const SyncRequestVote* pMsg, const char* s) {
  char     logBuf[256];
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  snprintf(logBuf, sizeof(logBuf),
           "recv sync-request-vote from %s:%d, {term:%" PRIu64 ", lindex:%" PRId64 ", lterm:%" PRIu64 "}, %s", host,
           port, pMsg->term, pMsg->lastLogIndex, pMsg->lastLogTerm, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendRequestVoteReply(SSyncNode* pSyncNode, const SyncRequestVoteReply* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf), "send sync-request-vote-reply to %s:%d {term:%" PRIu64 ", grant:%d}, %s", host, port,
           pMsg->term, pMsg->voteGranted, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvRequestVoteReply(SSyncNode* pSyncNode, const SyncRequestVoteReply* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf), "recv sync-request-vote-reply from %s:%d {term:%" PRIu64 ", grant:%d}, %s", host,
           port, pMsg->term, pMsg->voteGranted, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendAppendEntries(SSyncNode* pSyncNode, const SyncAppendEntries* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "send sync-append-entries to %s:%d, {term:%" PRIu64 ", pre-index:%" PRId64 ", pre-term:%" PRIu64
           ", pterm:%" PRIu64 ", cmt:%" PRId64
           ", "
           "datalen:%d}, %s",
           host, port, pMsg->term, pMsg->prevLogIndex, pMsg->prevLogTerm, pMsg->privateTerm, pMsg->commitIndex,
           pMsg->dataLen, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvAppendEntries(SSyncNode* pSyncNode, const SyncAppendEntries* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "recv sync-append-entries from %s:%d {term:%" PRIu64 ", pre-index:%" PRIu64 ", pre-term:%" PRIu64
           ", cmt:%" PRIu64 ", pterm:%" PRIu64
           ", "
           "datalen:%d}, %s",
           host, port, pMsg->term, pMsg->prevLogIndex, pMsg->prevLogTerm, pMsg->commitIndex, pMsg->privateTerm,
           pMsg->dataLen, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendAppendEntriesBatch(SSyncNode* pSyncNode, const SyncAppendEntriesBatch* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "send sync-append-entries-batch to %s:%d, {term:%" PRIu64 ", pre-index:%" PRId64 ", pre-term:%" PRIu64
           ", pterm:%" PRIu64 ", cmt:%" PRId64 ", datalen:%d, count:%d}, %s",
           host, port, pMsg->term, pMsg->prevLogIndex, pMsg->prevLogTerm, pMsg->privateTerm, pMsg->commitIndex,
           pMsg->dataLen, pMsg->dataCount, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvAppendEntriesBatch(SSyncNode* pSyncNode, const SyncAppendEntriesBatch* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "recv sync-append-entries-batch from %s:%d, {term:%" PRIu64 ", pre-index:%" PRId64 ", pre-term:%" PRIu64
           ", pterm:%" PRIu64 ", cmt:%" PRId64 ", datalen:%d, count:%d}, %s",
           host, port, pMsg->term, pMsg->prevLogIndex, pMsg->prevLogTerm, pMsg->privateTerm, pMsg->commitIndex,
           pMsg->dataLen, pMsg->dataCount, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendAppendEntriesReply(SSyncNode* pSyncNode, const SyncAppendEntriesReply* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "send sync-append-entries-reply to %s:%d, {term:%" PRIu64 ", pterm:%" PRIu64 ", success:%d, match:%" PRId64
           "}, %s",
           host, port, pMsg->term, pMsg->privateTerm, pMsg->success, pMsg->matchIndex, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvAppendEntriesReply(SSyncNode* pSyncNode, const SyncAppendEntriesReply* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "recv sync-append-entries-reply from %s:%d {term:%" PRIu64 ", pterm:%" PRIu64 ", success:%d, match:%" PRId64
           "}, %s",
           host, port, pMsg->term, pMsg->privateTerm, pMsg->success, pMsg->matchIndex, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendHeartbeat(SSyncNode* pSyncNode, const SyncHeartbeat* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "send sync-heartbeat to %s:%d {term:%" PRIu64 ", cmt:%" PRId64 ", min-match:%" PRId64 ", pterm:%" PRIu64
           "}, %s",
           host, port, pMsg->term, pMsg->commitIndex, pMsg->minMatchIndex, pMsg->privateTerm, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvHeartbeat(SSyncNode* pSyncNode, const SyncHeartbeat* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf),
           "recv sync-heartbeat from %s:%d {term:%" PRIu64 ", cmt:%" PRId64 ", min-match:%" PRId64 ", pterm:%" PRIu64
           "}, %s",
           host, port, pMsg->term, pMsg->commitIndex, pMsg->minMatchIndex, pMsg->privateTerm, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogSendHeartbeatReply(SSyncNode* pSyncNode, const SyncHeartbeatReply* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->destId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf), "send sync-heartbeat-reply from %s:%d {term:%" PRIu64 ", pterm:%" PRIu64 "}, %s",
           host, port, pMsg->term, pMsg->privateTerm, s);
  syncNodeEventLog(pSyncNode, logBuf);
}

void syncLogRecvHeartbeatReply(SSyncNode* pSyncNode, const SyncHeartbeatReply* pMsg, const char* s) {
  char     host[64];
  uint16_t port;
  syncUtilU642Addr(pMsg->srcId.addr, host, sizeof(host), &port);
  char logBuf[256];
  snprintf(logBuf, sizeof(logBuf), "recv sync-heartbeat-reply from %s:%d {term:%" PRIu64 ", pterm:%" PRIu64 "}, %s",
           host, port, pMsg->term, pMsg->privateTerm, s);
  syncNodeEventLog(pSyncNode, logBuf);
}
