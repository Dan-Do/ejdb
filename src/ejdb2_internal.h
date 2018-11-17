#pragma once
#ifndef EJDB2_INTERNAL_H
#define EJDB2_INTERNAL_H

#include "ejdb2.h"
#include "jql.h"
#include "jql_internal.h"
#include "jbl_internal.h"
#include <iowow/iwkv.h>
#include <iowow/iwxstr.h>
#include <iowow/iwexfile.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "khash.h"
#include "ejdb2cfg.h"

static_assert(JBNUMBUF_SIZE >= IWFTOA_BUFSIZE, "JBNUMBUF_SIZE >= IWFTOA_BUFSIZE");

#define METADB_ID 1
#define KEY_PREFIX_COLLMETA   "c." // Full key format: c.<coldbid>
#define KEY_PREFIX_IDXMETA    "i." // Full key format: i.<coldbid>.<idxdbid>

#define IWDB_DUP_FLAGS (IWDB_DUP_UINT32_VALS | IWDB_DUP_UINT64_VALS)

#define ENSURE_OPEN(db_) \
  if (!(db_) || !((db_)->open)) return IW_ERROR_INVALID_STATE;

#define API_RLOCK(db_, rci_) \
  ENSURE_OPEN(db_);  \
  rci_ = pthread_rwlock_rdlock(&(db_)->rwl); \
  if (rci_) return iwrc_set_errno(IW_ERROR_THREADING_ERRNO, rci_)

#define API_WLOCK(db_, rci_) \
  ENSURE_OPEN(db_);  \
  rci_ = pthread_rwlock_wrlock(&(db_)->rwl); \
  if (rci_) return iwrc_set_errno(IW_ERROR_THREADING_ERRNO, rci_)

#define API_UNLOCK(db_, rci_, rc_)  \
  rci_ = pthread_rwlock_unlock(&(db_)->rwl); \
  if (rci_) IWRC(iwrc_set_errno(IW_ERROR_THREADING_ERRNO, rci_), rc_)

#define API_COLL_UNLOCK(jbc_, rci_, rc_)                                     \
  do {                                                                    \
    rci_ = pthread_rwlock_unlock(&(jbc_)->rwl);                            \
    if (rci_) IWRC(iwrc_set_errno(IW_ERROR_THREADING_ERRNO, rci_), rc_);  \
    API_UNLOCK((jbc_)->db, rci_, rc_);                                   \
  } while(0)

struct _JBIDX;
typedef struct _JBIDX *JBIDX;

/** Database collection */
typedef struct _JBCOLL {
  uint32_t dbid;            /**< IWKV collection database ID */
  const char *name;         /**< Collection name */
  IWDB cdb;                 /**< IWKV collection database */
  EJDB db;                  /**< Main database reference */
  JBL meta;                 /**< Collection meta object */
  JBIDX idx;                /**< First index in chain */
  pthread_rwlock_t rwl;
  uint64_t id_seq;
} *JBCOLL;

/** Database collection index */
struct _JBIDX {
  ejdb_idx_mode_t mode;     /**< Index mode/type mask */
  iwdb_flags_t idbf;        /**< Index database flags */
  JBCOLL jbc;               /**< Owner document collection */
  JBL_PTR ptr;              /**< Indexed JSON path poiner 0*/
  IWDB idb;                 /**< KV database for this index */
  IWDB auxdb;               /**< Auxiliary database, used by `EJDB_IDX_ARR` indexes */
  uint32_t dbid;            /**< IWKV collection database ID */
  uint32_t auxdbid;         /**< Auxiliary database ID, used by `EJDB_IDX_ARR` indexes */
  struct _JBIDX *next;      /**< Next index in chain */
};

KHASH_MAP_INIT_STR(JBCOLLM, JBCOLL)

struct _EJDB {
  IWKV iwkv;
  IWDB metadb;
  khash_t(JBCOLLM) *mcolls;
  volatile bool open;
  iwkv_openflags oflags;
  pthread_rwlock_t rwl;       /**< Main RWL */
  struct _EJDB_OPTS opts;
};

struct _JBPHCTX {
  uint64_t id;
  JBCOLL jbc;
  const JBL jbl;
};

struct _JBEXEC;

typedef iwrc(*JB_SCAN_CONSUMER)(struct _JBEXEC *ctx, IWKV_cursor cur, uint64_t id, int64_t *step);

/**
 * @brief Index can sorter consumer context
 */
struct _JBSSC {
  uint32_t *refs;         /**< Document references array */
  uint32_t refs_asz;      /**< Document references array allocated size */
  uint32_t refs_num;      /**< Document references array elements count */
  struct _JBL *docs;      /**< Documents array */
  uint32_t docs_asz;      /**< Documents array allocated size */
  uint32_t docs_npos;     /**< Next document offset */
  IWFS_EXT sof;           /**< Sort overflow file */
  bool sof_active;       /**< Is sort overflow enabled */
};

typedef struct _JBEXEC {
  EJDB_EXEC ux;           /**< User defined context */
  JBCOLL jbc;             /**< Collection */
  JBIDX idx;              /**< Selected index for query (optional) */
  JQP_EXPR *iexpr;        /**< Expression used to match selected index (oprional) */
  uint32_t cnt;           /**< Current result row count */
  bool sorting;           /**< Resultset sorting needed */
  iwrc(*scanner)(struct _JBEXEC *ctx, JB_SCAN_CONSUMER consumer);
  void *jblbuf;           /**< Buffer used to keep currently processed document */
  size_t jblbufsz;        /**< Size of jblbuf allocated memory */
  struct _JBSSC ssc;
} JBEXEC;


iwrc jb_scan_consumer(struct _JBEXEC *ctx, IWKV_cursor cur, uint64_t id, int64_t *step);
iwrc jb_scan_sorter_consumer(struct _JBEXEC *ctx, IWKV_cursor cur, uint64_t id, int64_t *step);


#endif
