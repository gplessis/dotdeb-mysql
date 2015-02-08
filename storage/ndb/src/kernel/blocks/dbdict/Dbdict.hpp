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

#ifndef DBDICT_H
#define DBDICT_H

/**
 * Dict : Dictionary Block
 */
#include <ndb_limits.h>
#include <trigger_definitions.h>
#include <pc.hpp>
#include <DLHashTable.hpp>
#include <DLFifoList.hpp>
#include <CArray.hpp>
#include <KeyTable.hpp>
#include <KeyTable2.hpp>
#include <KeyTable2Ref.hpp>
#include <SimulatedBlock.hpp>
#include <SimpleProperties.hpp>
#include <SignalCounter.hpp>
#include <Bitmask.hpp>
#include <AttributeList.hpp>
#include <signaldata/GetTableId.hpp>
#include <signaldata/GetTabInfo.hpp>
#include <signaldata/DictTabInfo.hpp>
#include <signaldata/CreateTable.hpp>
#include <signaldata/CreateTab.hpp>
#include <signaldata/DropTable.hpp>
#include <signaldata/DropTab.hpp>
#include <signaldata/AlterTable.hpp>
#include <signaldata/AlterTab.hpp>
#include <signaldata/ListTables.hpp>
#include <signaldata/CreateIndx.hpp>
#include <signaldata/CreateIndxImpl.hpp>
#include <signaldata/DropIndx.hpp>
#include <signaldata/DropIndxImpl.hpp>
#include <signaldata/AlterIndx.hpp>
#include <signaldata/AlterIndxImpl.hpp>
#include <signaldata/BuildIndx.hpp>
#include <signaldata/BuildIndxImpl.hpp>
#include <signaldata/IndexStatSignal.hpp>
#include <signaldata/UtilPrepare.hpp>
#include <signaldata/CreateEvnt.hpp>
#include <signaldata/CreateTrig.hpp>
#include <signaldata/CreateTrigImpl.hpp>
#include <signaldata/DropTrig.hpp>
#include <signaldata/DropTrigImpl.hpp>
#include <signaldata/DictLock.hpp>
#include <signaldata/DictTakeover.hpp>
#include <signaldata/SumaImpl.hpp>
#include <signaldata/CreateHashMap.hpp>
#include <signaldata/HashMapImpl.hpp>
#include "SchemaFile.hpp"
#include <blocks/mutexes.hpp>
#include <SafeCounter.hpp>
#include <RequestTracker.hpp>
#include <Rope.hpp>
#include <signaldata/CreateFilegroupImpl.hpp>
#include <signaldata/DropFilegroupImpl.hpp>
#include <SLList.hpp>
#include <signaldata/DictSignal.hpp>
#include <signaldata/SchemaTransImpl.hpp>
#include <LockQueue.hpp>
#include <signaldata/CopyData.hpp>
#include <signaldata/NodeFailRep.hpp>
#include <signaldata/CreateNodegroup.hpp>
#include <signaldata/DropNodegroup.hpp>
#include <signaldata/CreateNodegroupImpl.hpp>
#include <signaldata/DropNodegroupImpl.hpp>


#ifdef DBDICT_C

/*--------------------------------------------------------------*/
// Constants for CONTINUEB
/*--------------------------------------------------------------*/
#define ZPACK_TABLE_INTO_PAGES 0
#define ZSEND_GET_TAB_RESPONSE 3
#define ZWAIT_SUBSTARTSTOP 4
#define ZDICT_TAKEOVER_REQ 5

#define ZCOMMIT_WAIT_GCI   6
#define ZINDEX_STAT_BG_PROCESS 7

/*--------------------------------------------------------------*/
// Other constants in alphabetical order
/*--------------------------------------------------------------*/
#define ZNOMOREPHASES 255

/*--------------------------------------------------------------*/
// Schema file defines
/*--------------------------------------------------------------*/
#define ZSCHEMA_WORDS 4

/*--------------------------------------------------------------*/
// Page constants
/*--------------------------------------------------------------*/
#define ZBAT_SCHEMA_FILE 0 //Variable number of page for NDBFS
#define ZBAT_TABLE_FILE 1 //Variable number of page for NDBFS
#define ZPAGE_HEADER_SIZE 32
#define ZPOS_PAGE_SIZE 16
#define ZPOS_CHECKSUM 17
#define ZPOS_VERSION 18
#define ZPOS_PAGE_HEADER_SIZE 19

/*--------------------------------------------------------------*/
// Size constants
/*--------------------------------------------------------------*/
#define ZFS_CONNECT_SIZE 4
#define ZSIZE_OF_PAGES_IN_WORDS 8192
#define ZLOG_SIZE_OF_PAGES_IN_WORDS 13
#define ZMAX_PAGES_OF_TABLE_DEFINITION \
  ((ZPAGE_HEADER_SIZE + MAX_WORDS_META_FILE + ZSIZE_OF_PAGES_IN_WORDS - 1) / \
   ZSIZE_OF_PAGES_IN_WORDS)

/**
 * - one for retreive
 * - one for read or write
 */
#define ZNUMBER_OF_PAGES (2 * ZMAX_PAGES_OF_TABLE_DEFINITION)

/*--------------------------------------------------------------*/
// Error codes
/*--------------------------------------------------------------*/
#define ZNODE_FAILURE_ERROR 704
#endif

/**
 * Systable NDB$EVENTS_0
 */
#define EVENT_SYSTEM_TABLE_LENGTH 9
#define EVENT_SYSTEM_TABLE_ATTRIBUTE_MASK2_ID 8

struct sysTab_NDBEVENTS_0 {
  char   NAME[MAX_TAB_NAME_SIZE];
  Uint32 EVENT_TYPE;
  Uint32 TABLEID;
  Uint32 TABLEVERSION;
  char   TABLE_NAME[MAX_TAB_NAME_SIZE];
  Uint32 ATTRIBUTE_MASK[MAXNROFATTRIBUTESINWORDS_OLD];
  Uint32 SUBID;
  Uint32 SUBKEY;

  // +1 to allow var size header to be received
  Uint32 ATTRIBUTE_MASK2[(MAX_ATTRIBUTES_IN_TABLE / 32) + 1];
};

/**
 *  DICT - This blocks handles all metadata
 */
class Dbdict: public SimulatedBlock {
public:
  /*
   *   2.3 RECORD AND FILESIZES
   */

  /**
   * Table attributes.  Permanent data.
   *
   * Indexes have an attribute list which duplicates primary table
   * attributes.  This is wrong but convenient.
   */
  struct AttributeRecord {
    AttributeRecord(){}

    /* attribute id */
    Uint16 attributeId;

    /* Attribute number within tuple key (counted from 1) */
    Uint16 tupleKey;

    /* Attribute name (unique within table) */
    RopeHandle attributeName;

    /* Attribute description (old-style packed descriptor) */
    Uint32 attributeDescriptor;

    /* Extended attributes */
    Uint32 extType;
    Uint32 extPrecision;
    Uint32 extScale;
    Uint32 extLength;

    /* Autoincrement flag, only for ODBC/SQL */
    bool autoIncrement;

    /* Default value as null-terminated string, only for ODBC/SQL */
    RopeHandle defaultValue;

    struct {
      Uint32 m_name_len;
      const char * m_name_ptr;
      RopePool * m_pool;
    } m_key;

    union {
      Uint32 nextPool;
      Uint32 nextList;
    };
    Uint32 prevList;
    Uint32 nextHash;
    Uint32 prevHash;
 
    Uint32 hashValue() const { return attributeName.hashValue();}
    bool equal(const AttributeRecord& obj) const { 
      if(obj.hashValue() == hashValue()){
	ConstRope r(* m_key.m_pool, obj.attributeName);
	return r.compare(m_key.m_name_ptr, m_key.m_name_len) == 0;
      }
      return false;
    }
  };
  typedef Ptr<AttributeRecord> AttributeRecordPtr;
  ArrayPool<AttributeRecord> c_attributeRecordPool;
  DLHashTable<AttributeRecord> c_attributeRecordHash;
  RSS_AP_SNAPSHOT(c_attributeRecordPool);

  /**
   * Shared table / index record.  Most of this is permanent data stored
   * on disk.  Index trigger ids are volatile.
   */
  struct TableRecord {
    TableRecord(){ m_upgrade_trigger_handling.m_upgrade = false;}
    Uint32 maxRowsLow;
    Uint32 maxRowsHigh;
    Uint32 minRowsLow;
    Uint32 minRowsHigh;
    /* Table id (array index in DICT and other blocks) */
    Uint32 tableId;
    Uint32 m_obj_ptr_i;

    /* Table version (incremented when tableId is re-used) */
    Uint32 tableVersion;

    /* */
    Uint32 hashMapObjectId;
    Uint32 hashMapVersion;

    /* Table name (may not be unique under "alter table") */
    RopeHandle tableName;

    /* Type of table or index */
    DictTabInfo::TableType tableType;

    /* Is table or index online (this flag is not used in DICT) */
    bool online;

    /* Primary table of index otherwise RNIL */
    Uint32 primaryTableId;

    /* Type of fragmentation (small/medium/large) */
    DictTabInfo::FragmentType fragmentType;

    /* Global checkpoint identity when table created */
    Uint32 gciTableCreated;

    /* Is the table logged (i.e. data survives system restart) */
    enum Bits
    {
      TR_Logged       = 0x1,
      TR_RowGCI       = 0x2,
      TR_RowChecksum  = 0x4,
      TR_Temporary    = 0x8,
      TR_ForceVarPart = 0x10
    };
    Uint8 m_extra_row_gci_bits;
    Uint8 m_extra_row_author_bits;
    Uint16 m_bits;

    /* Number of attibutes in table */
    Uint16 noOfAttributes;

    /* Number of null attributes in table (should be computed) */
    Uint16 noOfNullAttr;

    /* Number of primary key attributes (should be computed) */
    Uint16 noOfPrimkey;

    /* Length of primary key in words (should be computed) */
    /* For ordered index this is tree node size in words */
    Uint16 tupKeyLength;

    /** */
    Uint16 noOfCharsets;

    /* K value for LH**3 algorithm (only 6 allowed currently) */
    Uint8 kValue;

    /* Local key length in words (currently 1) */
    Uint8 localKeyLen;

    /*
     * Parameter for hash algorithm that specifies the load factor in
     * percentage of fill level in buckets. A high value means we are
     * splitting early and that buckets are only lightly used. A high
     * value means that we have fill the buckets more and get more
     * likelihood of overflow buckets.
     */
    Uint8 maxLoadFactor;

    /*
      Flag to indicate default number of partitions
    */
    bool defaultNoPartFlag;

    /*
      Flag to indicate using linear hash function
    */
    bool linearHashFlag;

    /*
     * Used when shrinking to decide when to merge buckets.  Hysteresis
     * is thus possible. Should be smaller but not much smaller than
     * maxLoadFactor
     */
    Uint8 minLoadFactor;

    /**
     * Table default storage method
     */
    Uint8 storageType; // NDB_STORAGETYPE_

    /* Convenience routines */
    bool isTable() const;
    bool isIndex() const;
    bool isUniqueIndex() const;
    bool isNonUniqueIndex() const;
    bool isHashIndex() const;
    bool isOrderedIndex() const;
    
    /****************************************************
     *    Support variables for table handling
     ****************************************************/

    /**    File pointer received from disk   */
    Uint32 filePtr[2];

    /**    Pointer to first attribute in table */
    DLFifoList<AttributeRecord>::Head m_attributes;

    Uint32 nextPool;

    Uint32 m_read_locked; // BACKUP_ONGOING

    /**    Number of words */
    Uint32 packedSize;

    /**   Index state (volatile data) TODO remove */
    enum IndexState {
      IS_UNDEFINED = 0,         // initial
      IS_OFFLINE = 1,           // index table created
      IS_BUILDING = 2,          // building (local state)
      IS_DROPPING = 3,          // dropping (local state)
      IS_ONLINE = 4,            // online
      IS_BROKEN = 9             // build or drop aborted
    };
    IndexState indexState;

    /**   Trigger ids of index (volatile data) */
    Uint32 triggerId;      // ordered index (1)
    Uint32 buildTriggerId; // temp during build

    struct UpgradeTriggerHandling
    {
      /**
       * In 6.3 (and prior) 3 triggers was created for each unique index
       *  in 6.4 these has been merged to 1
       *  but...we need to maintain these during an upgrade situation
       *  puh
       */
      bool m_upgrade;
      Uint32 insertTriggerId;
      Uint32 updateTriggerId;
      Uint32 deleteTriggerId;
    } m_upgrade_trigger_handling;
    
    Uint32 noOfNullBits;
    
    /**  frm data for this table */
    RopeHandle frmData;
    RopeHandle ngData;
    RopeHandle rangeData;

    Uint32 fragmentCount;
    Uint32 m_tablespace_id;

    /** List of indexes attached to table */
    DLFifoList<TableRecord>::Head m_indexes;
    Uint32 nextList, prevList;

    /*
     * Access rights to table during single user mode
     */
    Uint8 singleUserMode;

    /*
     * Fragment and node to use for index statistics.  Not part of
     * DICTTABINFO.  Computed locally by each DICT.  If the node is
     * down, no automatic stats update takes place.  This is not
     * critical and is not worth fixing.
     */
    Uint16 indexStatFragId;
    Uint16 indexStatNodeId;

    // pending background request (IndexStatRep::RequestType)
    Uint32 indexStatBgRequest;
  };

  typedef Ptr<TableRecord> TableRecordPtr;
  ArrayPool<TableRecord> c_tableRecordPool;
  RSS_AP_SNAPSHOT(c_tableRecordPool);

  /**  Node Group and Tablespace id+version + range or list data.
    *  This is only stored temporarily in DBDICT during an ongoing
    *  change.
    *  TODO RONM: Look into improvements of this
    */
  Uint32 c_fragDataLen;
  union {
    Uint16 c_fragData[MAX_NDB_PARTITIONS];
    Uint32 c_fragData_align32[1];
  };
  Uint32 c_tsIdData[2*MAX_NDB_PARTITIONS];

  /**
   * Triggers.  This is volatile data not saved on disk.  Setting a
   * trigger online creates the trigger in TC (if index) and LQH-TUP.
   */
  struct TriggerRecord {
    TriggerRecord() {}

    /** Trigger state */
    enum TriggerState { 
      TS_NOT_DEFINED = 0,
      TS_DEFINING = 1,
      TS_OFFLINE  = 2,   // created globally in DICT
      //TS_BUILDING = 3,
      //TS_DROPPING = 4,
      TS_ONLINE = 5,      // created in other blocks
      TS_FAKE_UPGRADE = 6
    };
    TriggerState triggerState;

    /** Trigger name, used by DICT to identify the trigger */ 
    RopeHandle triggerName;

    /** Trigger id, used by TRIX, TC, LQH, and TUP to identify the trigger */
    Uint32 triggerId;
    Uint32 m_obj_ptr_i;

    /** Table id, the table the trigger is defined on */
    Uint32 tableId;

    /** TriggerInfo (packed) */
    Uint32 triggerInfo;

    /**
     * Attribute mask, defines what attributes are to be monitored.
     * Can be seen as a compact representation of SQL column name list.
     */
    AttributeMask attributeMask;

    /** Receiver.  Not used from index triggers */
    BlockReference receiverRef;

    /** Index id, only used by secondary_index triggers */
    Uint32 indexId;

    /** Pointer to the next attribute used by ArrayPool */
    Uint32 nextPool;
  };
  
  Uint32 c_maxNoOfTriggers;
  typedef Ptr<TriggerRecord> TriggerRecordPtr;
  ArrayPool<TriggerRecord> c_triggerRecordPool;
  RSS_AP_SNAPSHOT(c_triggerRecordPool);

  /**
   * Information for each FS connection.
   ***************************************************************************/
  struct FsConnectRecord {
    enum FsState {
      IDLE = 0,
      OPEN_WRITE_SCHEMA = 1,
      WRITE_SCHEMA = 2,
      CLOSE_WRITE_SCHEMA = 3,
      OPEN_READ_SCHEMA1 = 4,
      OPEN_READ_SCHEMA2 = 5,
      READ_SCHEMA1 = 6,
      READ_SCHEMA2 = 7,
      CLOSE_READ_SCHEMA = 8,
      OPEN_READ_TAB_FILE1 = 9,
      OPEN_READ_TAB_FILE2 = 10,
      READ_TAB_FILE1 = 11,
      READ_TAB_FILE2 = 12,
      CLOSE_READ_TAB_FILE = 13,
      OPEN_WRITE_TAB_FILE = 14,
      WRITE_TAB_FILE = 15,
      CLOSE_WRITE_TAB_FILE = 16
    };
    /** File Pointer for this file system connection */
    Uint32 filePtr;

    /** Reference of owner record */
    Uint32 ownerPtr;

    /** State of file system connection */
    FsState fsState;

    /** Used by Array Pool for free list handling */
    Uint32 nextPool;
  };
  
  typedef Ptr<FsConnectRecord> FsConnectRecordPtr;
  ArrayPool<FsConnectRecord> c_fsConnectRecordPool;

  /**
   * This record stores all the information about a node and all its attributes
   ***************************************************************************/
  struct NodeRecord {
    enum NodeState {
      API_NODE = 0,
      NDB_NODE_ALIVE = 1,
      NDB_NODE_DEAD = 2,
      NDB_MASTER_TAKEOVER = 3
    };
    bool hotSpare;
    NodeState nodeState;

    // Schema transaction data
    enum RecoveryState {
      RS_NORMAL = 0,             // Node is up to date with master
      RS_PARTIAL_ROLLBACK = 1,   // Node has rolled back some operations
      RS_PARTIAL_ROLLFORWARD = 2 // Node has committed some operations
    };
    RecoveryState recoveryState;
    NodeFailRep nodeFailRep;
    NdbNodeBitmask m_nodes;      // Nodes sent DICT_TAKEOVER_REQ during takeover
    SafeCounterHandle m_counter; // Outstanding DICT_TAKEOVER_REQ's
    //TODO Accumulate in buffer when DictTakeoverConf becomes long signal
    DictTakeoverConf takeOverConf; // Accumulated replies

    // TODO: these should be moved to SchemaTransPtr
    // Starting operation for partial rollback/rollforward
    Uint32 start_op;
    // Starting state of operation for partial rollback/rollforward
    Uint32 start_op_state;
  };

  typedef Ptr<NodeRecord> NodeRecordPtr;
  CArray<NodeRecord> c_nodes;
  NdbNodeBitmask c_aliveNodes;
  
  struct PageRecord {
    Uint32 word[8192];
  };
  
  typedef Ptr<PageRecord> PageRecordPtr;
  CArray<PageRecord> c_pageRecordArray;

  struct SchemaPageRecord {
    Uint32 word[NDB_SF_PAGE_SIZE_IN_WORDS];
  };

  CArray<SchemaPageRecord> c_schemaPageRecordArray;

  unsigned g_trace;
  DictTabInfo::Table c_tableDesc;

  /**
   * A page for create index table signal.
   */
  PageRecord c_indexPage;

  struct File {
    File() {}
    
    Uint32 key;
    Uint32 m_magic;
    Uint32 m_version;
    Uint32 m_obj_ptr_i;
    Uint32 m_filegroup_id;
    Uint32 m_type;
    Uint64 m_file_size;
    Uint64 m_file_free;
    RopeHandle m_path;
    
    Uint32 nextList;
    union {
      Uint32 prevList;
      Uint32 nextPool;
    };
    Uint32 nextHash, prevHash;

    Uint32 hashValue() const { return key;}
    bool equal(const File& obj) const { return key == obj.key;}
  };
  typedef Ptr<File> FilePtr;
  typedef RecordPool<File, RWPool> File_pool;
  typedef DLListImpl<File_pool, File> File_list;
  typedef LocalDLListImpl<File_pool, File> Local_file_list;
  typedef KeyTableImpl<File_pool, File> File_hash;
  
  struct Filegroup {
    Filegroup(){}

    Uint32 key;
    Uint32 m_obj_ptr_i;
    Uint32 m_magic;
    
    Uint32 m_type;
    Uint32 m_version;
    RopeHandle m_name;

    union {
      struct {
	Uint32 m_extent_size;
	Uint32 m_default_logfile_group_id;
      } m_tablespace;
      
      struct {
	Uint32 m_undo_buffer_size;
	File_list::HeadPOD m_files;
      } m_logfilegroup;
    };
    
    union {
      Uint32 nextPool;
      Uint32 nextList;
      Uint32 nextHash;
    };
    Uint32 prevHash;

    Uint32 hashValue() const { return key;}
    bool equal(const Filegroup& obj) const { return key == obj.key;}
  };
  typedef Ptr<Filegroup> FilegroupPtr;
  typedef RecordPool<Filegroup, RWPool> Filegroup_pool;
  typedef KeyTableImpl<Filegroup_pool, Filegroup> Filegroup_hash;
  
  File_pool c_file_pool;
  Filegroup_pool c_filegroup_pool;
  File_hash c_file_hash;
  Filegroup_hash c_filegroup_hash;
  
  RopePool c_rope_pool;
  RSS_AP_SNAPSHOT(c_rope_pool);

  struct DictObject {
    DictObject() {
      m_trans_key = 0;
      m_op_ref_count = 0;
    };
    Uint32 m_id;
    Uint32 m_type;
    Uint32 m_ref_count;
    RopeHandle m_name;  
    union {
      struct {
	Uint32 m_name_len;
	const char * m_name_ptr;
	RopePool * m_pool;
      } m_key;
      Uint32 nextPool;
      Uint32 nextList;
    };
    Uint32 nextHash;
    Uint32 prevHash;
    
    Uint32 hashValue() const { return m_name.hashValue();}
    bool equal(const DictObject& obj) const { 
      if(obj.hashValue() == hashValue()){
	ConstRope r(* m_key.m_pool, obj.m_name);
	return r.compare(m_key.m_name_ptr, m_key.m_name_len) == 0;
      }
      return false;
    }

    // SchemaOp -> DictObject -> SchemaTrans
    Uint32 m_trans_key;
    Uint32 m_op_ref_count;
#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<DictObject> DictObjectPtr;
  
  DLHashTable<DictObject> c_obj_hash; // Name
  ArrayPool<DictObject> c_obj_pool;
  RSS_AP_SNAPSHOT(c_obj_pool);
  
  // 1
  DictObject * get_object(const char * name){
    return get_object(name, Uint32(strlen(name) + 1));
  }
  
  DictObject * get_object(const char * name, Uint32 len){
    return get_object(name, len, Rope::hash(name, len));
  }
  
  DictObject * get_object(const char * name, Uint32 len, Uint32 hash);

  //2
  bool get_object(DictObjectPtr& obj_ptr, const char * name){
    return get_object(obj_ptr, name, Uint32(strlen(name) + 1));
  }

  bool get_object(DictObjectPtr& obj_ptr, const char * name, Uint32 len){
    return get_object(obj_ptr, name, len, Rope::hash(name, len));
  }

  bool get_object(DictObjectPtr&, const char* name, Uint32 len, Uint32 hash);

  void release_object(Uint32 obj_ptr_i){
    release_object(obj_ptr_i, c_obj_pool.getPtr(obj_ptr_i));
  }
  
  void release_object(Uint32 obj_ptr_i, DictObject* obj_ptr_p);

  void increase_ref_count(Uint32 obj_ptr_i);
  void decrease_ref_count(Uint32 obj_ptr_i);

public:
  Dbdict(Block_context& ctx);
  virtual ~Dbdict();

private:
  BLOCK_DEFINES(Dbdict);

  // Signal receivers
  void execDICTSTARTREQ(Signal* signal);
  
  void execGET_TABINFOREQ(Signal* signal);
  void execGET_TABLEDID_REQ(Signal* signal);
  void execGET_TABINFO_REF(Signal* signal);
  void execGET_TABINFO_CONF(Signal* signal);
  void execCONTINUEB(Signal* signal);

  void execDUMP_STATE_ORD(Signal* signal);
  void execDBINFO_SCANREQ(Signal* signal);
  void execHOT_SPAREREP(Signal* signal);
  void execDIADDTABCONF(Signal* signal);
  void execDIADDTABREF(Signal* signal);
  void execTAB_COMMITCONF(Signal* signal);
  void execTAB_COMMITREF(Signal* signal);
  void execGET_SCHEMA_INFOREQ(Signal* signal);
  void execSCHEMA_INFO(Signal* signal);
  void execSCHEMA_INFOCONF(Signal* signal);
  void execREAD_NODESCONF(Signal* signal);
  void execFSCLOSECONF(Signal* signal);
  void execFSOPENCONF(Signal* signal);
  void execFSOPENREF(Signal* signal);
  void execFSREADCONF(Signal* signal);
  void execFSREADREF(Signal* signal);
  void execFSWRITECONF(Signal* signal);
  void execNDB_STTOR(Signal* signal);
  void execREAD_CONFIG_REQ(Signal* signal);
  void execSTTOR(Signal* signal);
  void execTC_SCHVERCONF(Signal* signal);
  void execNODE_FAILREP(Signal* signal);

  void send_nf_complete_rep(Signal* signal, const NodeFailRep*);

  void execINCL_NODEREQ(Signal* signal);
  void execAPI_FAILREQ(Signal* signal);

  void execWAIT_GCP_REF(Signal* signal);
  void execWAIT_GCP_CONF(Signal* signal);

  void execLIST_TABLES_REQ(Signal* signal);

  // Index signals
  void execCREATE_INDX_REQ(Signal* signal);
  void execCREATE_INDX_IMPL_CONF(Signal* signal);
  void execCREATE_INDX_IMPL_REF(Signal* signal);

  void execALTER_INDX_REQ(Signal* signal);
  void execALTER_INDX_CONF(Signal* signal);
  void execALTER_INDX_REF(Signal* signal);
  void execALTER_INDX_IMPL_CONF(Signal* signal);
  void execALTER_INDX_IMPL_REF(Signal* signal);

  void execCREATE_TABLE_CONF(Signal* signal);
  void execCREATE_TABLE_REF(Signal* signal);

  void execDROP_INDX_REQ(Signal* signal);
  void execDROP_INDX_IMPL_CONF(Signal* signal);
  void execDROP_INDX_IMPL_REF(Signal* signal);

  void execDROP_TABLE_CONF(Signal* signal);
  void execDROP_TABLE_REF(Signal* signal);

  void execBUILDINDXREQ(Signal* signal);
  void execBUILDINDXCONF(Signal* signal);
  void execBUILDINDXREF(Signal* signal);
  void execBUILD_INDX_IMPL_CONF(Signal* signal);
  void execBUILD_INDX_IMPL_REF(Signal* signal);

  void execBACKUP_LOCK_TAB_REQ(Signal*);

  // Util signals used by Event code
  void execUTIL_PREPARE_CONF(Signal* signal);
  void execUTIL_PREPARE_REF (Signal* signal);
  void execUTIL_EXECUTE_CONF(Signal* signal);
  void execUTIL_EXECUTE_REF (Signal* signal);
  void execUTIL_RELEASE_CONF(Signal* signal);
  void execUTIL_RELEASE_REF (Signal* signal);


  // Event signals from API
  void execCREATE_EVNT_REQ (Signal* signal);
  void execCREATE_EVNT_CONF(Signal* signal);
  void execCREATE_EVNT_REF (Signal* signal);

  void execDROP_EVNT_REQ (Signal* signal);

  void execSUB_START_REQ (Signal* signal);
  void execSUB_START_CONF (Signal* signal);
  void execSUB_START_REF (Signal* signal);

  void execSUB_STOP_REQ (Signal* signal);
  void execSUB_STOP_CONF (Signal* signal);
  void execSUB_STOP_REF (Signal* signal);

  // Event signals from SUMA

  void execCREATE_SUBID_CONF(Signal* signal);
  void execCREATE_SUBID_REF (Signal* signal);

  void execSUB_CREATE_CONF(Signal* signal);
  void execSUB_CREATE_REF (Signal* signal);

  void execSUB_REMOVE_REQ(Signal* signal);
  void execSUB_REMOVE_CONF(Signal* signal);
  void execSUB_REMOVE_REF(Signal* signal);

  // Trigger signals
  void execCREATE_TRIG_REQ(Signal* signal);
  void execCREATE_TRIG_CONF(Signal* signal);
  void execCREATE_TRIG_REF(Signal* signal);
  void execALTER_TRIG_REQ(Signal* signal);
  void execALTER_TRIG_CONF(Signal* signal);
  void execALTER_TRIG_REF(Signal* signal);
  void execDROP_TRIG_REQ(Signal* signal);
  void execDROP_TRIG_CONF(Signal* signal);
  void execDROP_TRIG_REF(Signal* signal);
  // from other blocks
  void execCREATE_TRIG_IMPL_CONF(Signal* signal);
  void execCREATE_TRIG_IMPL_REF(Signal* signal);
  void execDROP_TRIG_IMPL_CONF(Signal* signal);
  void execDROP_TRIG_IMPL_REF(Signal* signal);

  void execDROP_TABLE_REQ(Signal* signal);
  
  void execPREP_DROP_TAB_REQ(Signal* signal);
  void execPREP_DROP_TAB_REF(Signal* signal);  
  void execPREP_DROP_TAB_CONF(Signal* signal);

  void execDROP_TAB_REF(Signal* signal);  
  void execDROP_TAB_CONF(Signal* signal);

  void execCREATE_TABLE_REQ(Signal* signal);
  void execALTER_TABLE_REQ(Signal* signal);

  Uint32 get_fragmentation(Signal*, Uint32 tableId);
  Uint32 create_fragmentation(Signal* signal, TableRecordPtr,
                              const Uint16*, Uint32 cnt,
                              Uint32 flags = 0);
  void execCREATE_FRAGMENTATION_REQ(Signal*);
  void execCREATE_FRAGMENTATION_REF(Signal*);
  void execCREATE_FRAGMENTATION_CONF(Signal*);
  void execCREATE_TAB_REQ(Signal* signal);
  void execADD_FRAGREQ(Signal* signal);
  void execLQHFRAGREF(Signal* signal);
  void execLQHFRAGCONF(Signal* signal);
  void execLQHADDATTREF(Signal* signal);
  void execLQHADDATTCONF(Signal* signal);
  void execCREATE_TAB_REF(Signal* signal);
  void execCREATE_TAB_CONF(Signal* signal);  
  void execALTER_TAB_REF(Signal* signal);
  void execALTER_TAB_CONF(Signal* signal);
  void execALTER_TABLE_REF(Signal* signal);
  void execALTER_TABLE_CONF(Signal* signal);
  bool check_ndb_versions() const;
  int check_sender_version(const Signal* signal, Uint32 version) const;


  void execCREATE_FILE_REQ(Signal* signal);
  void execCREATE_FILEGROUP_REQ(Signal* signal);
  void execDROP_FILE_REQ(Signal* signal);
  void execDROP_FILEGROUP_REQ(Signal* signal);

  // Internal
  void execCREATE_FILE_IMPL_REF(Signal* signal);
  void execCREATE_FILE_IMPL_CONF(Signal* signal);
  void execCREATE_FILEGROUP_IMPL_REF(Signal* signal);
  void execCREATE_FILEGROUP_IMPL_CONF(Signal* signal);
  void execDROP_FILE_IMPL_REF(Signal* signal);
  void execDROP_FILE_IMPL_CONF(Signal* signal);
  void execDROP_FILEGROUP_IMPL_REF(Signal* signal);
  void execDROP_FILEGROUP_IMPL_CONF(Signal* signal);

  void execSCHEMA_TRANS_BEGIN_REQ(Signal* signal);
  void execSCHEMA_TRANS_BEGIN_CONF(Signal* signal);
  void execSCHEMA_TRANS_BEGIN_REF(Signal* signal);
  void execSCHEMA_TRANS_END_REQ(Signal* signal);
  void execSCHEMA_TRANS_END_CONF(Signal* signal);
  void execSCHEMA_TRANS_END_REF(Signal* signal);
  void execSCHEMA_TRANS_END_REP(Signal* signal);
  void execSCHEMA_TRANS_IMPL_REQ(Signal* signal);
  void execSCHEMA_TRANS_IMPL_CONF(Signal* signal);
  void execSCHEMA_TRANS_IMPL_REF(Signal* signal);

  void execDICT_LOCK_REQ(Signal* signal);
  void execDICT_UNLOCK_ORD(Signal* signal);

  void execDICT_TAKEOVER_REQ(Signal* signal);
  void execDICT_TAKEOVER_REF(Signal* signal);
  void execDICT_TAKEOVER_CONF(Signal* signal);

  // ordered index statistics
  void execINDEX_STAT_REQ(Signal* signal);
  void execINDEX_STAT_CONF(Signal* signal);
  void execINDEX_STAT_REF(Signal* signal);
  void execINDEX_STAT_IMPL_CONF(Signal* signal);
  void execINDEX_STAT_IMPL_REF(Signal* signal);
  void execINDEX_STAT_REP(Signal* signal);

  /*
   *  2.4 COMMON STORED VARIABLES
   */

  /**
   * This record stores all the state needed 
   * when the schema page is being sent to other nodes
   ***************************************************************************/
  struct SendSchemaRecord {
    /** Number of words of schema data */
    Uint32 noOfWords;
    /** Page Id of schema data */
    Uint32 pageId;

    Uint32 nodeId;
    SignalCounter m_SCHEMAINFO_Counter;
    
    Uint32 noOfWordsCurrentlySent;
    Uint32 noOfSignalsSentSinceDelay;

    bool inUse;
  };
  SendSchemaRecord c_sendSchemaRecord;

  /**
   * This record stores all the state needed 
   * when a table file is being read from disk
   ****************************************************************************/
  struct ReadTableRecord {
    /** Number of Pages */
    Uint32 no_of_words;
    /** Page Id*/
    Uint32 pageId;
    /** Table Id of read table */
    Uint32 tableId;
    
    bool inUse;
    Callback m_callback;
  };
  ReadTableRecord c_readTableRecord;

  /**
   * This record stores all the state needed 
   * when a table file is being written to disk
   ****************************************************************************/
  struct WriteTableRecord {
    /** Number of Pages */
    Uint32 no_of_words;
    /** Page Id*/
    Uint32 pageId;
    /** Table Files Handled, local state variable */
    Uint32 noOfTableFilesHandled;
    /** Table Id of written table */
    Uint32 tableId;
    /** State, indicates from where it was called */
    enum TableWriteState {
      IDLE = 0,
      WRITE_ADD_TABLE_MASTER = 1,
      WRITE_ADD_TABLE_SLAVE = 2,
      WRITE_RESTART_FROM_MASTER = 3,
      WRITE_RESTART_FROM_OWN = 4,
      TWR_CALLBACK = 5
    };
    TableWriteState tableWriteState;
    Callback m_callback;
  };
  WriteTableRecord c_writeTableRecord;

  /**
   * This record stores all the state needed 
   * when a schema file is being read from disk
   ****************************************************************************/
  struct ReadSchemaRecord {
    /** Page Id of schema page */
    Uint32 pageId;
    /** First page to read */
    Uint32 firstPage;
    /** Number of pages to read */
    Uint32 noOfPages;
    /** State, indicates from where it was called */
    enum SchemaReadState {
      IDLE = 0,
      INITIAL_READ_HEAD = 1,
      INITIAL_READ = 2
    };
    SchemaReadState schemaReadState;
  };
  ReadSchemaRecord c_readSchemaRecord;

  /**
   * This record stores all the state needed 
   * when a schema file is being written to disk
   ****************************************************************************/
  struct WriteSchemaRecord {
    /** Page Id of schema page */
    Uint32 pageId;
    /** Rewrite entire file */
    Uint32 newFile;
    /** First page to write */
    Uint32 firstPage;
    /** Number of pages to write */
    Uint32 noOfPages;
    /** Schema Files Handled, local state variable */
    Uint32 noOfSchemaFilesHandled;

    bool inUse;
    Callback m_callback;
  };
  WriteSchemaRecord c_writeSchemaRecord;

  /**
   * This record stores all the information needed 
   * when a file is being read from disk
   ****************************************************************************/
  struct RestartRecord {
    /**    Global check point identity       */
    Uint32 gciToRestart;

    /**    The active table at restart process */
    Uint32 activeTable;

    /**    The active table at restart process */
    BlockReference returnBlockRef;
    Uint32 m_senderData;

    Uint32 m_pass;     // 0 tablespaces/logfilegroups, 1 tables, 2 indexes
    Uint32 m_end_pass; //
    const char * m_start_banner;
    const char * m_end_banner;

    Uint32 m_tx_ptr_i;
    Uint32 m_op_cnt;
    SchemaFile::TableEntry m_entry;
  };
  RestartRecord c_restartRecord;

  /**
   * This record stores all the information needed 
   * when a file is being read from disk
   ****************************************************************************/
  struct RetrieveRecord {
    RetrieveRecord(){ noOfWaiters = 0;}
    
    /**    Only one retrieve table definition at a time       */
    bool busyState;
    
    /**
     * No of waiting in time queue
     */
    Uint32 noOfWaiters;
    
    /**    Block Reference of retriever       */
    BlockReference blockRef;

    /**    Id of retriever       */
    Uint32 m_senderData;

    /**    Table id of retrieved table       */
    Uint32 tableId;

    Uint32 m_table_type;

    /**    Starting page to retrieve data from   */
    Uint32 retrievePage;

    /**    Number of pages retrieved   */
    Uint32 retrievedNoOfPages;

    /**    Number of words retrieved   */
    Uint32 retrievedNoOfWords;

    /**    Number of words sent currently   */
    Uint32 currentSent;

    /**
     * Long signal stuff
     */
    bool m_useLongSig;

    Uint32 schemaTransId;
    Uint32 requestType;
  };
  RetrieveRecord c_retrieveRecord;

  /**
   * This record stores all the information needed 
   * when a file is being read from disk
   * 
   * This is the info stored in one entry of the schema
   * page. Each table has 4 words of info.
   * Word 1: Schema version (upper 16 bits)
   *         Table State (lower 16 bits)
   * Word 2: Number of pages of table description
   * Word 3: Global checkpoint id table was created
   * Word 4: Currently zero
   ****************************************************************************/
  struct SchemaRecord {
    enum
    {
      NEW_SCHEMA_FILE = 0, // Index in c_schemaFile
      OLD_SCHEMA_FILE = 1
    };

    /**    Schema file first page (0)   */
    Uint32 schemaPage;

    /**    Old Schema file first page (used at node restart)    */
    Uint32 oldSchemaPage;
    
    Callback m_callback;
  };
  SchemaRecord c_schemaRecord;

  /*
   * Schema file, list of schema pages.  Use an array until a pool
   * exists and NDBFS interface can use it.
   */
  struct XSchemaFile {
    SchemaFile* schemaPage;
    Uint32 noOfPages;
  };
  // 0-normal 1-old
  XSchemaFile c_schemaFile[2];

  void initSchemaFile(XSchemaFile *, Uint32 firstPage, Uint32 lastPage,
                      bool initEntries);
  void resizeSchemaFile(XSchemaFile * xsf, Uint32 noOfPages);
  void computeChecksum(XSchemaFile *, Uint32 pageNo);
  bool validateChecksum(const XSchemaFile *);
  SchemaFile::TableEntry * getTableEntry(Uint32 tableId);
  SchemaFile::TableEntry * getTableEntry(XSchemaFile *, Uint32 tableId);
  const SchemaFile::TableEntry * getTableEntry(const XSchemaFile*, Uint32);

  Uint32 computeChecksum(const Uint32 * src, Uint32 len);


  /* ----------------------------------------------------------------------- */
  // Node References
  /* ----------------------------------------------------------------------- */
  Uint16 c_masterNodeId;

  /* ----------------------------------------------------------------------- */
  // Various current system properties
  /* ----------------------------------------------------------------------- */
  Uint16 c_numberNode;
  Uint16 c_noHotSpareNodes;
  Uint16 c_noNodesFailed;
  Uint32 c_failureNr;

  /* ----------------------------------------------------------------------- */
  // State variables
  /* ----------------------------------------------------------------------- */

  struct PackTable {
    
    enum PackTableState {
      PTS_IDLE = 0,
      PTS_GET_TAB = 3
    } m_state;

  } c_packTable;

  Uint32 c_startPhase;
  Uint32 c_restartType;
  bool   c_initialStart;
  bool   c_systemRestart;
  bool   c_nodeRestart;
  bool   c_initialNodeRestart;
  Uint32 c_tabinfoReceived;

  /**
   * Temporary structure used when parsing table info
   */
  struct ParseDictTabInfoRecord {
    ParseDictTabInfoRecord() { tablePtr.setNull();}
    DictTabInfo::RequestType requestType;
    Uint32 errorCode;
    Uint32 errorLine;
    
    SimpleProperties::UnpackStatus status;
    Uint32 errorKey;
    TableRecordPtr tablePtr;
  };

  // Misc helpers

  template <Uint32 sz>
  inline bool
  copyRope(RopeHandle& rh_dst, const RopeHandle& rh_src)
  {
    char buf[sz];
    Rope r_dst(c_rope_pool, rh_dst);
    ConstRope r_src(c_rope_pool, rh_src);
    ndbrequire(r_src.size() <= sz);
    r_src.copy(buf);
    bool ok = r_dst.assign(buf, r_src.size());
    return ok;
  }

#ifdef VM_TRACE
  template <Uint32 sz>
  inline const char*
  copyRope(const RopeHandle& rh)
  {
    static char buf[2][sz];
    static int i = 0;
    ConstRope r(c_rope_pool, rh);
    char* str = buf[i++ % 2];
    r.copy(str);
    return str;
  }
#endif
 
  // Operation records

  /**
   * Common part of operation records.  Uses KeyTable2.  Note that each
   * seize/release invokes ctor/dtor automatically.
   */
  struct OpRecordCommon {
    OpRecordCommon() {}
    Uint32 key;         // key shared between master and slaves
    Uint32 nextHash;
    Uint32 prevHash;
    Uint32 hashValue() const {
      return key;
    }
    bool equal(const OpRecordCommon& rec) const {
      return key == rec.key;
    }
  };

  // MODULE: SchemaTrans

  struct SchemaOp;
  struct SchemaTrans;
  struct TxHandle;
  typedef Ptr<SchemaOp> SchemaOpPtr;
  typedef Ptr<SchemaTrans> SchemaTransPtr;
  typedef Ptr<TxHandle> TxHandlePtr;

  // ErrorInfo

  struct ErrorInfo {
    Uint32 errorCode;
    Uint32 errorLine;
    Uint32 errorNodeId;
    Uint32 errorCount;
    // for CreateTable
    Uint32 errorStatus;
    Uint32 errorKey;
    char errorObjectName[MAX_TAB_NAME_SIZE];
    ErrorInfo() {
      errorCode = 0;
      errorLine = 0;
      errorNodeId = 0;
      errorCount = 0;
      errorStatus = 0;
      errorKey = 0;
      errorObjectName[0] = 0;
    }
    void print(NdbOut&) const;
  private:
    ErrorInfo& operator=(const ErrorInfo&);
  };

  void setError(ErrorInfo&,
                Uint32 code,
                Uint32 line,
                Uint32 nodeId = 0,
                Uint32 status = 0,
                Uint32 key = 0,
                const char * name = 0);

  void setError(ErrorInfo&, 
                Uint32 code,
                Uint32 line,
                const char * name);

  void setError(ErrorInfo&, const ErrorInfo&);
  void setError(ErrorInfo&, const ParseDictTabInfoRecord&);

  void setError(SchemaOpPtr, Uint32 code, Uint32 line, Uint32 nodeId = 0);
  void setError(SchemaOpPtr, const ErrorInfo&);

  void setError(SchemaTransPtr, Uint32 code, Uint32 line, Uint32 nodeId = 0);
  void setError(SchemaTransPtr, const ErrorInfo&);

  void setError(TxHandlePtr, Uint32 code, Uint32 line, Uint32 nodeId = 0);
  void setError(TxHandlePtr, const ErrorInfo&);

  template <class Ref>
  inline void
  setError(ErrorInfo& e, const Ref* ref) {
    setError(e, ref->errorCode, ref->errorLine, ref->errorNodeId);
  }

  template <class Ref>
  inline void
  getError(const ErrorInfo& e, Ref* ref) {
    ref->errorCode = e.errorCode;
    ref->errorLine = e.errorLine;
    ref->errorNodeId = e.errorNodeId;
    ref->masterNodeId = c_masterNodeId;
  }

  bool hasError(const ErrorInfo&);

  void resetError(ErrorInfo&);
  void resetError(SchemaOpPtr);
  void resetError(SchemaTransPtr);
  void resetError(TxHandlePtr);

  // OpInfo

  struct OpInfo {
    const char m_opType[4]; // e.g. CTa for CreateTable
    Uint32 m_impl_req_gsn;
    Uint32 m_impl_req_length;

    // seize / release type-specific Data record
    bool (Dbdict::*m_seize)(SchemaOpPtr);
    void (Dbdict::*m_release)(SchemaOpPtr);

    // parse phase
    void (Dbdict::*m_parse)(Signal*, bool master,
                            SchemaOpPtr, SectionHandle&, ErrorInfo&);
    bool (Dbdict::*m_subOps)(Signal*, SchemaOpPtr);
    void (Dbdict::*m_reply)(Signal*, SchemaOpPtr, ErrorInfo);

    // run phases
    void (Dbdict::*m_prepare)(Signal*, SchemaOpPtr);
    void (Dbdict::*m_commit)(Signal*, SchemaOpPtr);
    void (Dbdict::*m_complete)(Signal*, SchemaOpPtr);
    // abort mode
    void (Dbdict::*m_abortParse)(Signal*, SchemaOpPtr);
    void (Dbdict::*m_abortPrepare)(Signal*, SchemaOpPtr);
  };

  // all OpInfo records
  static const OpInfo* g_opInfoList[];
  const OpInfo* findOpInfo(Uint32 gsn);

  // OpRec

  struct OpRec
  {
    char m_opType[4];

    Uint32 nextPool;

    // reference to the static member in subclass
    const OpInfo& m_opInfo;

    // pointer to internal signal in subclass instance
    Uint32* const m_impl_req_data;

    // DictObject operated on
    Uint32 m_obj_ptr_i;

    OpRec(const OpInfo& info, Uint32* impl_req_data) :
      m_opInfo(info),
      m_impl_req_data(impl_req_data) {
      m_obj_ptr_i = RNIL;
      memcpy(m_opType, m_opInfo.m_opType, 4);
    }
  };
  typedef Ptr<OpRec> OpRecPtr;

  /*
   * OpSection
   *
   * Signal sections are released in parse phase.  If necessary
   * they are first saved under schema op record.
   */

  enum { OpSectionSegmentSize = 127 };
  typedef
    LocalDataBuffer<OpSectionSegmentSize>
    OpSectionBuffer;
  typedef
    OpSectionBuffer::Head
    OpSectionBufferHead;
  typedef
    OpSectionBuffer::DataBufferPool
    OpSectionBufferPool;
  typedef
    DataBuffer<OpSectionSegmentSize>::ConstDataBufferIterator
    OpSectionBufferConstIterator;

  OpSectionBufferPool c_opSectionBufferPool;

  struct OpSection {
    OpSectionBufferHead m_head;
    Uint32 getSize() const {
      return m_head.getSize();
    }
  };

  bool copyIn(OpSection&, const SegmentedSectionPtr&);
  bool copyIn(OpSection&, const Uint32* src, Uint32 srcSize);
  bool copyOut(const OpSection&, SegmentedSectionPtr&);
  bool copyOut(const OpSection&, Uint32* dst, Uint32 dstSize);
  bool copyOut(OpSectionBuffer & buffer, OpSectionBufferConstIterator & iter,
               Uint32 * dst, Uint32 len);
  void release(OpSection&);

  // SchemaOp

  struct SchemaOp {
    Uint32 nextPool;

    enum OpState
    {
      OS_INITIAL          = 0,
      OS_PARSE_MASTER     = 1,
      OS_PARSING          = 2,
      OS_PARSED           = 3,
      OS_PREPARING        = 4,
      OS_PREPARED         = 5,
      OS_ABORTING_PREPARE = 6,
      OS_ABORTED_PREPARE  = 7,
      OS_ABORTING_PARSE   = 8,
      //OS_ABORTED_PARSE    = 9,  // Not used, op released
      OS_COMMITTING       = 10,
      OS_COMMITTED        = 11,
      OS_COMPLETING       = 12,
      OS_COMPLETED        = 13
    };

    Uint32 m_state;
    /*
      Return the "weight" of an operation state, used to determine
      the absolute order of operations.
     */
    static Uint32 weight(Uint32 state) {
      switch ((OpState) state) {
      case OS_INITIAL:
        return 0;
      case OS_PARSE_MASTER:
        return 1;
      case OS_PARSING:
        return 2;
      case OS_PARSED:
        return 5;
      case OS_PREPARING:
        return 6;
      case OS_PREPARED:
        return 9;
      case OS_ABORTING_PREPARE:
        return 8;
      case OS_ABORTED_PREPARE:
        return 7;
      case OS_ABORTING_PARSE:
        return 4;
      //case OS_ABORTED_PARSE    = 9,  // Not used, op released
        //return 3: 
      case OS_COMMITTING:
        return 10;
      case OS_COMMITTED:
        return 11;
      case OS_COMPLETING:
        return 12;
      case OS_COMPLETED:
        return 13;
      }
      assert(false);
      return -1;
    }
    Uint32 m_restart;
    Uint32 op_key;
    Uint32 m_base_op_ptr_i;
    Uint32 nextHash;
    Uint32 prevHash;
    Uint32 hashValue() const {
      return op_key;
    }
    bool equal(const SchemaOp& rec) const {
      return op_key == rec.op_key;
    }

    Uint32 nextList;
    Uint32 prevList;

    // tx client or DICT master for recursive ops
    Uint32 m_clientRef;
    Uint32 m_clientData;

    // requestExtra and requestFlags from REQ and trans level
    Uint32 m_requestInfo;

    // the op belongs to this trans
    SchemaTransPtr m_trans_ptr;

    // type specific record (the other half of schema op)
    OpRecPtr m_oprec_ptr;

    // saved signal sections or other variable data
    OpSection m_section[3];
    Uint32 m_sections;

    // callback for use with sub-operations
    Callback m_callback;

    // link to an extra "helper" op and link back from it
    SchemaOpPtr m_oplnk_ptr;
    SchemaOpPtr m_opbck_ptr;

    // error always propagates to trans level
    ErrorInfo m_error;

    // Copy of original and current schema file entry
    Uint32 m_orig_entry_id;
    SchemaFile::TableEntry m_orig_entry;

    // magic is on when record is seized
    enum { DICT_MAGIC = 0xd1c70001 };
    Uint32 m_magic;

    SchemaOp() {
      m_restart = 0;
      m_clientRef = 0;
      m_clientData = 0;
      m_requestInfo = 0;
      m_trans_ptr.setNull();
      m_oprec_ptr.setNull();
      m_sections = 0;
      m_callback.m_callbackFunction = 0;
      m_callback.m_callbackData = 0;
      m_oplnk_ptr.setNull();
      m_opbck_ptr.setNull();
      m_magic = 0;
      m_base_op_ptr_i = RNIL;

      m_orig_entry_id = RNIL;
      m_orig_entry.init();
    }

    SchemaOp(Uint32 the_op_key) {
      op_key = the_op_key;
    }

#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  ArrayPool<SchemaOp> c_schemaOpPool;
  DLHashTable<SchemaOp> c_schemaOpHash;

  const OpInfo& getOpInfo(SchemaOpPtr op_ptr);

  // set or get the type specific record cast to the specific type

  template <class T>
  inline void
  setOpRec(SchemaOpPtr op_ptr, const Ptr<T> t_ptr) {
    OpRecPtr& oprec_ptr = op_ptr.p->m_oprec_ptr;
    ndbrequire(!t_ptr.isNull());
    oprec_ptr.i = t_ptr.i;
    oprec_ptr.p = static_cast<OpRec*>(t_ptr.p);
    ndbrequire(memcmp(t_ptr.p->m_opType, T::g_opInfo.m_opType, 4) == 0);
  }

  template <class T>
  inline void
  getOpRec(SchemaOpPtr op_ptr, Ptr<T>& t_ptr) {
    OpRecPtr oprec_ptr = op_ptr.p->m_oprec_ptr;
    ndbrequire(!oprec_ptr.isNull());
    t_ptr.i = oprec_ptr.i;
    t_ptr.p = static_cast<T*>(oprec_ptr.p);
    ndbrequire(memcmp(t_ptr.p->m_opType, T::g_opInfo.m_opType, 4) == 0);
  }

  // OpInfo m_seize, m_release

  template <class T>
  inline bool
  seizeOpRec(SchemaOpPtr op_ptr) {
    OpRecPtr& oprec_ptr = op_ptr.p->m_oprec_ptr;
    ArrayPool<T>& pool = T::getPool(this);
    Ptr<T> t_ptr;
    if (pool.seize(t_ptr)) {
      new (t_ptr.p) T();
      setOpRec<T>(op_ptr, t_ptr);
      return true;
    }
    oprec_ptr.setNull();
    return false;
  }

  template <class T>
  inline void
  releaseOpRec(SchemaOpPtr op_ptr) {
    OpRecPtr& oprec_ptr = op_ptr.p->m_oprec_ptr;
    ArrayPool<T>& pool = T::getPool(this);
    Ptr<T> t_ptr;
    getOpRec<T>(op_ptr, t_ptr);
    pool.release(t_ptr);
    oprec_ptr.setNull();
  }

  // seize / find / release, atomic on op rec + data rec

  bool seizeSchemaOp(SchemaOpPtr& op_ptr, Uint32 op_key, const OpInfo& info);

  template <class T>
  inline bool
  seizeSchemaOp(SchemaOpPtr& op_ptr, Uint32 op_key) {
    return seizeSchemaOp(op_ptr, op_key, T::g_opInfo);
  }

  template <class T>
  inline bool
  seizeSchemaOp(SchemaOpPtr& op_ptr, Ptr<T>& t_ptr, Uint32 op_key) {
    if (seizeSchemaOp<T>(op_ptr, op_key)) {
      getOpRec<T>(op_ptr, t_ptr);
      return true;
    }
    return false;
  }

  template <class T>
  inline bool
  seizeSchemaOp(SchemaOpPtr& op_ptr) {
    /*
      Store node id in high 8 bits to make op_key globally unique
     */
    Uint32 op_key = 
      (getOwnNodeId() << 24) +
      ((c_opRecordSequence + 1) & 0x00FFFFFF);
    if (seizeSchemaOp<T>(op_ptr, op_key)) {
      c_opRecordSequence++;
      return true;
    }
    return false;
  }

  template <class T>
  inline bool
  seizeSchemaOp(SchemaOpPtr& op_ptr, Ptr<T>& t_ptr) {
    if (seizeSchemaOp<T>(op_ptr)) {
      getOpRec<T>(op_ptr, t_ptr);
      return true;
    }
    return false;
  }

  bool findSchemaOp(SchemaOpPtr& op_ptr, Uint32 op_key);

  template <class T>
  inline bool
  findSchemaOp(SchemaOpPtr& op_ptr, Ptr<T>& t_ptr, Uint32 op_key) {
    if (findSchemaOp(op_ptr, op_key)) {
      getOpRec(op_ptr, t_ptr);
      return true;
    }
    return false;
  }

  void releaseSchemaOp(SchemaOpPtr& op_ptr);

  // copy signal sections to schema op sections
  const OpSection& getOpSection(SchemaOpPtr, Uint32 ss_no);
  bool saveOpSection(SchemaOpPtr, SectionHandle&, Uint32 ss_no);
  bool saveOpSection(SchemaOpPtr, SegmentedSectionPtr ss_ptr, Uint32 ss_no);
  void releaseOpSection(SchemaOpPtr, Uint32 ss_no);

  // add operation to transaction OpList
  void addSchemaOp(SchemaTransPtr, SchemaOpPtr&);

  void updateSchemaOpStep(SchemaTransPtr, SchemaOpPtr);

  // the link between SdhemaOp and DictObject (1-way now)

  bool hasDictObject(SchemaOpPtr);
  void getDictObject(SchemaOpPtr, DictObjectPtr&);
  void linkDictObject(SchemaOpPtr op_ptr, DictObjectPtr obj_ptr);
  void unlinkDictObject(SchemaOpPtr op_ptr);
  void seizeDictObject(SchemaOpPtr, DictObjectPtr&, const RopeHandle& name);
  bool findDictObject(SchemaOpPtr, DictObjectPtr&, const char* name);
  bool findDictObject(SchemaOpPtr, DictObjectPtr&, Uint32 obj_ptr_i);
  void releaseDictObject(SchemaOpPtr);
  void findDictObjectOp(SchemaOpPtr&, DictObjectPtr);

  /*
   * Trans client is the API client (not us, for recursive ops).
   * Its state is shared by SchemaTrans / TxHandle (for takeover).
   */
  struct TransClient {
    enum State {
      StateUndef = 0,
      BeginReq = 1,   // begin trans received
      BeginReply = 2, // reply sent / waited for
      ParseReq = 3,
      ParseReply = 4,
      EndReq = 5,
      EndReply = 6
    };
    enum Flag {
      ApiFail = 1,
      Background = 2,
      TakeOver = 4,
      Commit = 8
    };
  };

  // SchemaTrans

  struct SchemaTrans {
    // ArrayPool
    Uint32 nextPool;

    enum TransState
    {
      TS_INITIAL          = 0,
      TS_STARTING         = 1, // Starting at participants
      TS_STARTED          = 2, // Started (potentially with parsed ops)
      TS_PARSING          = 3, // Parsing at participants
      TS_SUBOP            = 4, // Creating subop
      TS_ROLLBACK_SP      = 5, // Rolling back to SP (supported before prepare)
      TS_FLUSH_PREPARE    = 6,
      TS_PREPARING        = 7, // Preparing operations
      TS_ABORTING_PREPARE = 8, // Aborting prepared operations
      TS_ABORTING_PARSE   = 9, // Aborting parsed operations
      TS_FLUSH_COMMIT     = 10,
      TS_COMMITTING       = 11,// Committing
      TS_FLUSH_COMPLETE   = 12,// Committed
      TS_COMPLETING       = 13,// Completing
      TS_ENDING           = 14
    };

    Uint32 m_state;
    static Uint32 weight(Uint32 state) {
    /*
      Return the "weight" of a transaction state, used to determine
      the absolute order of beleived transaction states at master
      takeover.
     */
      switch ((TransState) state) {
      case TS_INITIAL:
        return 0;
      case TS_STARTING:
        return 1;
      case TS_STARTED:
        return 2;
      case TS_PARSING:
        return 3;
      case TS_SUBOP:
        return 6;
      case TS_ROLLBACK_SP:
        return 5;
      case TS_FLUSH_PREPARE:
        return 7;
      case TS_PREPARING:
        return 8;
      case TS_ABORTING_PREPARE:
        return 9;
      case TS_ABORTING_PARSE:
        return 4;
      case TS_FLUSH_COMMIT:
        return 10;
      case TS_COMMITTING:
        return 11;
      case TS_FLUSH_COMPLETE:
        return 12;
      case TS_COMPLETING:
        return 13;
      case TS_ENDING:
        return 14;
      }
      assert(false);
      return -1;
    }
    // DLHashTable
    Uint32 trans_key;
    Uint32 nextHash;
    Uint32 prevHash;
    Uint32 hashValue() const {
      return trans_key;
    }
    bool equal(const SchemaTrans& rec) const {
      return trans_key == rec.trans_key;
    }

    // DLFifoList where new ones are added at end
    Uint32 nextList;
    Uint32 prevList;

    bool m_isMaster;
    BlockReference m_masterRef;

    // requestFlags from begin/end trans
    Uint32 m_requestInfo;

    BlockReference m_clientRef;
    Uint32 m_obj_id;
    Uint32 m_transId;
    TransClient::State m_clientState;
    Uint32 m_clientFlags;
    Uint32 m_takeOverTxKey;

    NdbNodeBitmask m_nodes;      // Nodes part of transaction
    NdbNodeBitmask m_ref_nodes;  // Nodes replying REF to req
    SafeCounterHandle m_counter; // Outstanding REQ's

    Uint32 m_curr_op_ptr_i;
    DLFifoList<SchemaOp>::Head m_op_list;

    // Master takeover
    enum TakeoverRecoveryState
    {
      TRS_INITIAL     = 0,
      TRS_ROLLFORWARD = 1,
      TRS_ROLLBACK    = 2
    };
    Uint32 m_master_recovery_state;
    // These are common states all nodes must achieve
    // to be able to be involved in total rollforward/rollbackward
    Uint32 m_rollforward_op;
    Uint32 m_rollforward_op_state;
    Uint32 m_rollback_op;
    Uint32 m_rollback_op_state;
    Uint32 m_lowest_trans_state;
    Uint32 m_highest_trans_state;
    // Flag for signalling partial rollforward check during master takeover
    bool check_partial_rollforward;
    // Flag for signalling that already completed operation is recreated
    bool ressurected_op;

    // request for lock/unlock
    DictLockReq m_lockReq;

    // callback (not yet used)
    Callback m_callback;

    // error is reset after each req/reply
    ErrorInfo m_error;

    /**
     * Mutex handling
     */
    MutexHandle2<DIH_START_LCP_MUTEX> m_commit_mutex;

    bool m_flush_prepare;
    bool m_flush_commit;
    bool m_flush_complete;
    bool m_flush_end;
    bool m_wait_gcp_on_commit;

    // magic is on when record is seized
    enum { DICT_MAGIC = 0xd1c70002 };
    Uint32 m_magic;

    SchemaTrans() {
      m_state = TS_INITIAL;
      m_isMaster = false;
      m_masterRef = 0;
      m_requestInfo = 0;
      m_clientRef = 0;
      m_transId = 0;
      m_clientState = TransClient::StateUndef;
      m_clientFlags = 0;
      m_takeOverTxKey = 0;
      bzero(&m_lockReq, sizeof(m_lockReq));
      m_callback.m_callbackFunction = 0;
      m_callback.m_callbackData = 0;
      m_magic = 0;
      m_obj_id = RNIL;
      m_flush_prepare = false;
      m_flush_commit = false;
      m_flush_complete = false;
      m_flush_end = false;
      m_wait_gcp_on_commit = true;
    }

    SchemaTrans(Uint32 the_trans_key) {
      trans_key = the_trans_key;
    }

#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  Uint32 check_read_obj(Uint32 objId, Uint32 transId = 0);
  Uint32 check_read_obj(SchemaFile::TableEntry*, Uint32 transId = 0);
  Uint32 check_write_obj(Uint32 objId, Uint32 transId = 0);
  Uint32 check_write_obj(Uint32, Uint32, SchemaFile::EntryState, ErrorInfo&);

  ArrayPool<SchemaTrans> c_schemaTransPool;
  DLHashTable<SchemaTrans> c_schemaTransHash;
  DLFifoList<SchemaTrans> c_schemaTransList;
  Uint32 c_schemaTransCount;

  bool seizeSchemaTrans(SchemaTransPtr&, Uint32 trans_key);
  bool seizeSchemaTrans(SchemaTransPtr&);
  bool findSchemaTrans(SchemaTransPtr&, Uint32 trans_key);
  void releaseSchemaTrans(SchemaTransPtr&);

  // coordinator
  void createSubOps(Signal*, SchemaOpPtr, bool first = false);
  void abortSubOps(Signal*, SchemaOpPtr, ErrorInfo);

  void trans_recv_reply(Signal*, SchemaTransPtr);
  void trans_start_recv_reply(Signal*, SchemaTransPtr);
  void trans_parse_recv_reply(Signal*, SchemaTransPtr);

  void trans_prepare_start(Signal*, SchemaTransPtr);
  void trans_prepare_first(Signal*, SchemaTransPtr);
  void trans_prepare_recv_reply(Signal*, SchemaTransPtr);
  void trans_prepare_next(Signal*, SchemaTransPtr, SchemaOpPtr);
  void trans_prepare_done(Signal*, SchemaTransPtr);

  void trans_abort_prepare_start(Signal*, SchemaTransPtr);
  void trans_abort_prepare_recv_reply(Signal*, SchemaTransPtr);
  void trans_abort_prepare_next(Signal*, SchemaTransPtr, SchemaOpPtr);
  void trans_abort_prepare_done(Signal*, SchemaTransPtr);

  void trans_abort_parse_start(Signal*, SchemaTransPtr);
  void trans_abort_parse_recv_reply(Signal*, SchemaTransPtr);
  void trans_abort_parse_next(Signal*, SchemaTransPtr, SchemaOpPtr);
  void trans_abort_parse_done(Signal*, SchemaTransPtr);

  void trans_rollback_sp_start(Signal* signal, SchemaTransPtr);
  void trans_rollback_sp_recv_reply(Signal* signal, SchemaTransPtr);
  void trans_rollback_sp_next(Signal* signal, SchemaTransPtr, SchemaOpPtr);
  void trans_rollback_sp_done(Signal* signal, SchemaTransPtr, SchemaOpPtr);

  void trans_commit_start(Signal*, SchemaTransPtr);
  void trans_commit_wait_gci(Signal*);
  void trans_commit_mutex_locked(Signal*, Uint32, Uint32);
  void trans_commit_first(Signal*, SchemaTransPtr);
  void trans_commit_recv_reply(Signal*, SchemaTransPtr);
  void trans_commit_next(Signal*, SchemaTransPtr, SchemaOpPtr);
  void trans_commit_done(Signal* signal, SchemaTransPtr);
  void trans_commit_mutex_unlocked(Signal*, Uint32, Uint32);

  void trans_complete_start(Signal* signal, SchemaTransPtr);
  void trans_complete_first(Signal* signal, SchemaTransPtr);
  void trans_complete_next(Signal*, SchemaTransPtr, SchemaOpPtr);
  void trans_complete_recv_reply(Signal*, SchemaTransPtr);
  void trans_complete_done(Signal*, SchemaTransPtr);

  void trans_end_start(Signal* signal, SchemaTransPtr);
  void trans_end_recv_reply(Signal*, SchemaTransPtr);

  void trans_log(SchemaTransPtr);
  Uint32 trans_log_schema_op(SchemaOpPtr,
                             Uint32 objectId,
                             const SchemaFile::TableEntry*);

  void trans_log_schema_op_abort(SchemaOpPtr);
  void trans_log_schema_op_complete(SchemaOpPtr);

  void handle_master_takeover(Signal*);
  void check_takeover_replies(Signal*);
  void trans_recover(Signal*, SchemaTransPtr);
  void check_partial_trans_abort_prepare_next(SchemaTransPtr,
                                              NdbNodeBitmask &,
                                              SchemaOpPtr);
  void check_partial_trans_abort_parse_next(SchemaTransPtr,
                                            NdbNodeBitmask &,
                                            SchemaOpPtr);
  void check_partial_trans_complete_start(SchemaTransPtr, NdbNodeBitmask &);
  void check_partial_trans_commit_start(SchemaTransPtr, NdbNodeBitmask &);
  void check_partial_trans_commit_next(SchemaTransPtr,
                                       NdbNodeBitmask &,
                                       SchemaOpPtr);
  void check_partial_trans_end_recv_reply(SchemaTransPtr);

  // participant
  void recvTransReq(Signal*);
  void recvTransParseReq(Signal*, SchemaTransPtr,
                         Uint32 op_key, const OpInfo& info,
                         Uint32 requestInfo);
  void runTransSlave(Signal*, SchemaTransPtr);
  void update_op_state(SchemaOpPtr);
  void sendTransConf(Signal*, SchemaOpPtr);
  void sendTransConf(Signal*, SchemaTransPtr);
  void sendTransConfRelease(Signal*, SchemaTransPtr);
  void sendTransRef(Signal*, SchemaOpPtr);
  void sendTransRef(Signal*, SchemaTransPtr);

  void slave_run_start(Signal*, const SchemaTransImplReq*);
  void slave_run_parse(Signal*, SchemaTransPtr, const SchemaTransImplReq*);
  void slave_run_flush(Signal*, SchemaTransPtr, const SchemaTransImplReq*);
  void slave_writeSchema_conf(Signal*, Uint32, Uint32);
  void slave_commit_mutex_locked(Signal*, Uint32, Uint32);
  void slave_commit_mutex_unlocked(Signal*, Uint32, Uint32);

  // reply to trans client for begin/end trans
  void sendTransClientReply(Signal*, SchemaTransPtr);

  // on DB slave node failure exclude the node from transactions
  void handleTransSlaveFail(Signal*, Uint32 failedNode);

  // common code for different op types

  /*
   * Client REQ starts with find trans and add op record.
   * Sets request info in op and default request type in impl_req.
   */
  template <class T, class Req, class ImplReq>
  inline void
  startClientReq(SchemaOpPtr& op_ptr, Ptr<T>& t_ptr,
                 const Req* req, ImplReq*& impl_req, ErrorInfo& error)
  {
    SchemaTransPtr trans_ptr;

    const Uint32 requestInfo = req->requestInfo;
    const Uint32 requestType = DictSignal::getRequestType(requestInfo);
    const Uint32 requestExtra = DictSignal::getRequestExtra(requestInfo);
    const bool localTrans = (requestInfo & DictSignal::RF_LOCAL_TRANS);

    if (getOwnNodeId() != c_masterNodeId && !localTrans) {
      jam();
      setError(error, SchemaTransImplRef::NotMaster, __LINE__);
      return;
    }

    if (!findSchemaTrans(trans_ptr, req->transKey)) {
      jam();
      setError(error, SchemaTransImplRef::InvalidTransKey, __LINE__);
      return;
    }

    if (trans_ptr.p->m_transId != req->transId) {
      jam();
      setError(error, SchemaTransImplRef::InvalidTransId, __LINE__);
      return;
    }

    if (!seizeSchemaOp(op_ptr, t_ptr)) {
      jam();
      setError(error, SchemaTransImplRef::TooManySchemaOps, __LINE__);
      return;
    }

    trans_ptr.p->m_clientState = TransClient::ParseReq;

    DictSignal::setRequestExtra(op_ptr.p->m_requestInfo, requestExtra);
    DictSignal::addRequestFlags(op_ptr.p->m_requestInfo, requestInfo);

    // add op and global flags from trans level
    addSchemaOp(trans_ptr, op_ptr);

    // impl_req was passed via reference
    impl_req = &t_ptr.p->m_request;

    impl_req->senderRef = reference();
    impl_req->senderData = op_ptr.p->op_key;
    impl_req->requestType = requestType;

    // client of this REQ (trans client or us, recursively)
    op_ptr.p->m_clientRef = req->clientRef;
    op_ptr.p->m_clientData = req->clientData;
  }

  /*
   * The other half of client REQ processing.  On error starts
   * rollback of current client op and its sub-ops.
   */
  void handleClientReq(Signal*, SchemaOpPtr, SectionHandle&);

  // DICT receives recursive or internal CONF or REF

  template <class Conf>
  inline void
  handleDictConf(Signal* signal, const Conf* conf) {
    D("handleDictConf" << V(conf->senderData));
    ndbrequire(signal->getNoOfSections() == 0);

    Callback callback;
    bool ok = findCallback(callback, conf->senderData);
    ndbrequire(ok);
    execute(signal, callback, 0);
  }

  template <class Ref>
  inline void
  handleDictRef(Signal* signal, const Ref* ref) {
    D("handleDictRef" << V(ref->senderData) << V(ref->errorCode));
    ndbrequire(signal->getNoOfSections() == 0);

    Callback callback;
    bool ok = findCallback(callback, ref->senderData);
    ndbrequire(ok);
    ndbrequire(ref->errorCode != 0);
    execute(signal, callback, ref->errorCode);
  }

  /*
   * TxHandle
   *
   * DICT as schema trans client.  TxHandle is the client-side record.
   * It has same role as NdbDictInterface::Tx in NDB API.  It is used
   * for following:
   *
   * - create or drop table at NR/SR [not yet]
   * - build or activate indexes at NR/SR
   * - take over client trans if client requests this
   * - take over client trans when client API has failed
   */

  struct TxHandle {
    // ArrayPool
    Uint32 nextPool;

    // DLHashTable
    Uint32 tx_key;
    Uint32 nextHash;
    Uint32 prevHash;
    Uint32 hashValue() const {
      return tx_key;
    }
    bool equal(const TxHandle& rec) const {
      return tx_key == rec.tx_key;
    }

    Uint32 m_requestInfo; // global flags are passed to schema trans
    Uint32 m_transId;
    Uint32 m_transKey;
    Uint32 m_userData;

    // when take over for background or for failed API
    TransClient::State m_clientState;
    Uint32 m_clientFlags;
    BlockReference m_takeOverRef;
    Uint32 m_takeOverTransId;

    Callback m_callback;
    ErrorInfo m_error;

    // magic is on when record is seized
    enum { DICT_MAGIC = 0xd1c70003 };
    Uint32 m_magic;

    TxHandle() {
      m_requestInfo = 0;
      m_transId = 0;
      m_transKey = 0;
      m_userData = 0;
      m_clientState = TransClient::StateUndef;
      m_clientFlags = 0;
      m_takeOverRef = 0;
      m_takeOverTransId = 0;
      m_callback.m_callbackFunction = 0;
      m_callback.m_callbackData = 0;
      m_magic = 0;
    }

    TxHandle(Uint32 the_tx_key) {
      tx_key = the_tx_key;
    }
#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  ArrayPool<TxHandle> c_txHandlePool;
  DLHashTable<TxHandle> c_txHandleHash;

  bool seizeTxHandle(TxHandlePtr&);
  bool findTxHandle(TxHandlePtr&, Uint32 tx_key);
  void releaseTxHandle(TxHandlePtr&);

  void beginSchemaTrans(Signal*, TxHandlePtr);
  void endSchemaTrans(Signal*, TxHandlePtr, Uint32 flags = 0);

  void handleApiFail(Signal*, Uint32 failedApiNode);
  void takeOverTransClient(Signal*, SchemaTransPtr);
  void runTransClientTakeOver(Signal*, Uint32 tx_key, Uint32 ret);
  void finishApiFail(Signal*, TxHandlePtr tx_ptr);
  void apiFailBlockHandling(Signal*, Uint32 failedApiNode);

  /*
   * Callback key is for different record types in some cases.
   * For example a CONF can be for SchemaOp or for TxHandle.
   * This looks for match for one of op_key/trans_key/tx_key.
   */
  bool findCallback(Callback& callback, Uint32 any_key);

  // MODULE: CreateTable

  struct CreateTableRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateTableRec>&
    getPool(Dbdict* dict) {
      return dict->c_createTableRecPool;
    }

    CreateTabReq m_request;

    // wl3600_todo check mutex name and number later
    MutexHandle2<DIH_START_LCP_MUTEX> m_startLcpMutex;

    // long signal memory for temp use
    Uint32 m_tabInfoPtrI;
    Uint32 m_fragmentsPtrI;

    // connect pointers towards DIH and LQH
    Uint32 m_dihAddFragPtr;
    Uint32 m_lqhFragPtr;

    // who is using local create tab
    Callback m_callback;

    // flag if this op has been aborted in RT_PREPARE phase
    bool m_abortPrepareDone;

    CreateTableRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_tabInfoPtrI = RNIL;
      m_fragmentsPtrI = RNIL;
      m_dihAddFragPtr = RNIL;
      m_lqhFragPtr = RNIL;
      m_abortPrepareDone = false;
    }

#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<CreateTableRec> CreateTableRecPtr;
  ArrayPool<CreateTableRec> c_createTableRecPool;

  // OpInfo
  bool createTable_seize(SchemaOpPtr);
  void createTable_release(SchemaOpPtr);
  //
  void createTable_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool createTable_subOps(Signal*, SchemaOpPtr);
  void createTable_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createTable_prepare(Signal*, SchemaOpPtr);
  void createTable_commit(Signal*, SchemaOpPtr);
  void createTable_complete(Signal*, SchemaOpPtr);
  //
  void createTable_abortParse(Signal*, SchemaOpPtr);
  void createTable_abortPrepare(Signal*, SchemaOpPtr);

  // prepare
  void createTab_writeTableConf(Signal*, Uint32 op_key, Uint32 ret);
  void createTab_local(Signal*, SchemaOpPtr, OpSection fragSec, Callback*);
  void createTab_dih(Signal*, SchemaOpPtr);
  void createTab_localComplete(Signal*, Uint32 op_key, Uint32 ret);

  // commit
  void createTab_activate(Signal*, SchemaOpPtr, Callback*);
  void createTab_alterComplete(Signal*, Uint32 op_key, Uint32 ret);

  // abort prepare
  void createTable_abortLocalConf(Signal*, Uint32 aux_op_key, Uint32 ret);

  // MODULE: DropTable

  struct DropTableRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::DropTableRec>&
    getPool(Dbdict* dict) {
      return dict->c_dropTableRecPool;
    }

    DropTabReq m_request;

    // wl3600_todo check mutex name and number later
    MutexHandle2<BACKUP_DEFINE_MUTEX> m_define_backup_mutex;

    Uint32 m_block;
    enum { BlockCount = 5 };
    Uint32 m_blockNo[BlockCount];
    Callback m_callback;

    DropTableRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_block = 0;
    }

#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<DropTableRec> DropTableRecPtr;
  ArrayPool<DropTableRec> c_dropTableRecPool;

  // OpInfo
  bool dropTable_seize(SchemaOpPtr);
  void dropTable_release(SchemaOpPtr);
  //
  void dropTable_parse(Signal*, bool master,
                       SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool dropTable_subOps(Signal*, SchemaOpPtr);
  void dropTable_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void dropTable_prepare(Signal*, SchemaOpPtr);
  void dropTable_commit(Signal*, SchemaOpPtr);
  void dropTable_complete(Signal*, SchemaOpPtr);
  //
  void dropTable_abortParse(Signal*, SchemaOpPtr);
  void dropTable_abortPrepare(Signal*, SchemaOpPtr);

  // prepare
  void dropTable_backup_mutex_locked(Signal*, Uint32 op_key, Uint32 ret);

  // commit
  void dropTable_commit_nextStep(Signal*, SchemaOpPtr);
  void dropTable_commit_fromLocal(Signal*, Uint32 op_key, Uint32 errorCode);
  void dropTable_commit_done(Signal*, SchemaOpPtr);

  // complete
  void dropTable_complete_nextStep(Signal*, SchemaOpPtr);
  void dropTable_complete_fromLocal(Signal*, Uint32 op_key);
  void dropTable_complete_done(Signal*, Uint32 op_key, Uint32 ret);

  // MODULE: AlterTable

  struct AlterTableRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::AlterTableRec>&
    getPool(Dbdict* dict) {
      return dict->c_alterTableRecPool;
    }

    AlterTabReq m_request;

    // added attributes
    Uint32 m_newAttrData[2 * MAX_ATTRIBUTES_IN_TABLE];

    // wl3600_todo check mutex name and number later
    MutexHandle2<BACKUP_DEFINE_MUTEX> m_define_backup_mutex;

    // current and new temporary work table
    TableRecordPtr m_tablePtr;
    TableRecordPtr m_newTablePtr;
    Uint32 m_newTable_realObjectId;

    // before image
    RopeHandle m_oldTableName;
    RopeHandle m_oldFrmData;

    // connect ptr towards TUP, DIH, LQH
    Uint32 m_dihAddFragPtr;
    Uint32 m_lqhFragPtr;

    // local blocks to process
    enum { BlockCount = 3 };
    Uint32 m_blockNo[BlockCount];
    Uint32 m_blockIndex;

    // used for creating subops for add partitions, wrt ordered index
    bool m_sub_reorg_commit;
    bool m_sub_reorg_complete;
    bool m_sub_add_frag;
    Uint32 m_sub_add_frag_index_ptr;
    bool m_sub_trigger;
    bool m_sub_copy_data;
    bool m_sub_suma_enable;
    bool m_sub_suma_filter;

    AlterTableRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      memset(&m_newAttrData, 0, sizeof(m_newAttrData));
      m_tablePtr.setNull();
      m_newTablePtr.setNull();
      m_dihAddFragPtr = RNIL;
      m_lqhFragPtr = RNIL;
      m_blockNo[0] = DBLQH;
      m_blockNo[1] = DBDIH;
      m_blockNo[2] = DBTC;
      m_blockIndex = 0;
      m_sub_add_frag_index_ptr = RNIL;
      m_sub_add_frag = false;
      m_sub_reorg_commit = false;
      m_sub_reorg_complete = false;
      m_sub_trigger = false;
      m_sub_copy_data = false;
      m_sub_suma_enable = false;
      m_sub_suma_filter = false;
    }
#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<AlterTableRec> AlterTableRecPtr;
  ArrayPool<AlterTableRec> c_alterTableRecPool;

  // OpInfo
  bool alterTable_seize(SchemaOpPtr);
  void alterTable_release(SchemaOpPtr);
  //
  void alterTable_parse(Signal*, bool master,
                        SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool alterTable_subOps(Signal*, SchemaOpPtr);
  void alterTable_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void alterTable_prepare(Signal*, SchemaOpPtr);
  void alterTable_commit(Signal*, SchemaOpPtr);
  void alterTable_complete(Signal*, SchemaOpPtr);
  //
  void alterTable_abortParse(Signal*, SchemaOpPtr);
  void alterTable_abortPrepare(Signal*, SchemaOpPtr);

  void alterTable_toCopyData(Signal* signal, SchemaOpPtr op_ptr);
  void alterTable_fromCopyData(Signal*, Uint32 op_key, Uint32 ret);

  // prepare phase
  void alterTable_backup_mutex_locked(Signal*, Uint32 op_key, Uint32 ret);
  void alterTable_toLocal(Signal*, SchemaOpPtr);
  void alterTable_fromLocal(Signal*, Uint32 op_key, Uint32 ret);

  void alterTable_toAlterIndex(Signal*, SchemaOpPtr);
  void alterTable_fromAlterIndex(Signal*, Uint32 op_key, Uint32 ret);

  void alterTable_toReorgTable(Signal*, SchemaOpPtr, Uint32 step);
  void alterTable_fromReorgTable(Signal*, Uint32 op_key, Uint32 ret);

  void alterTable_toCreateTrigger(Signal* signal, SchemaOpPtr op_ptr);
  void alterTable_fromCreateTrigger(Signal*, Uint32 op_key, Uint32 ret);

  void alterTable_toSumaSync(Signal* signal, SchemaOpPtr op_ptr, Uint32);

  // commit phase
  void alterTable_toCommitComplete(Signal*, SchemaOpPtr, Uint32 = ~Uint32(0));
  void alterTable_fromCommitComplete(Signal*, Uint32 op_key, Uint32 ret);
  void alterTab_writeTableConf(Signal*, Uint32 op_key, Uint32 ret);

  // abort
  void alterTable_abortToLocal(Signal*, SchemaOpPtr);
  void alterTable_abortFromLocal(Signal*, Uint32 op_key, Uint32 ret);

  Uint32 check_supported_add_fragment(Uint16*, const Uint16*);
  Uint32 check_supported_reorg(Uint32, Uint32);

  // MODULE: CreateIndex

  typedef struct {
    Uint32 old_index;
    Uint32 attr_id;
    Uint32 attr_ptr_i;
  } AttributeMap[MAX_ATTRIBUTES_IN_INDEX];

  struct CreateIndexRec : public OpRec {
    CreateIndxImplReq m_request;
    char m_indexName[MAX_TAB_NAME_SIZE];
    IndexAttributeList m_attrList;
    AttributeMask m_attrMask;
    AttributeMap m_attrMap;
    Uint32 m_bits;
    Uint32 m_fragmentType;
    Uint32 m_indexKeyLength;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateIndexRec>&
    getPool(Dbdict* dict) {
      return dict->c_createIndexRecPool;
    }

    // sub-operation counters
    bool m_sub_create_table;
    bool m_sub_alter_index;

    CreateIndexRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      memset(m_indexName, 0, sizeof(m_indexName));
      memset(&m_attrList, 0, sizeof(m_attrList));
      m_attrMask.clear();
      memset(m_attrMap, 0, sizeof(m_attrMap));
      m_bits = 0;
      m_fragmentType = 0;
      m_indexKeyLength = 0;
      m_sub_create_table = false;
      m_sub_alter_index = false;
    }
#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<CreateIndexRec> CreateIndexRecPtr;
  ArrayPool<CreateIndexRec> c_createIndexRecPool;

  // OpInfo
  bool createIndex_seize(SchemaOpPtr);
  void createIndex_release(SchemaOpPtr);
  //
  void createIndex_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool createIndex_subOps(Signal*, SchemaOpPtr);
  void createIndex_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createIndex_prepare(Signal*, SchemaOpPtr);
  void createIndex_commit(Signal*, SchemaOpPtr);
  void createIndex_complete(Signal*, SchemaOpPtr);
  //
  void createIndex_abortParse(Signal*, SchemaOpPtr);
  void createIndex_abortPrepare(Signal*, SchemaOpPtr);

  // sub-ops
  void createIndex_toCreateTable(Signal*, SchemaOpPtr);
  void createIndex_fromCreateTable(Signal*, Uint32 op_key, Uint32 ret);
  void createIndex_toAlterIndex(Signal*, SchemaOpPtr);
  void createIndex_fromAlterIndex(Signal*, Uint32 op_key, Uint32 ret);

  // MODULE: DropIndex

  struct DropIndexRec : public OpRec {
    DropIndxImplReq m_request;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::DropIndexRec>&
    getPool(Dbdict* dict) {
      return dict->c_dropIndexRecPool;
    }

    // sub-operation counters
    bool m_sub_alter_index;
    bool m_sub_drop_table;

    DropIndexRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_sub_alter_index = false;
      m_sub_drop_table = false;
    }
#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<DropIndexRec> DropIndexRecPtr;
  ArrayPool<DropIndexRec> c_dropIndexRecPool;

  // OpInfo
  bool dropIndex_seize(SchemaOpPtr);
  void dropIndex_release(SchemaOpPtr);
  //
  void dropIndex_parse(Signal*, bool master,
                       SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool dropIndex_subOps(Signal*, SchemaOpPtr);
  void dropIndex_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void dropIndex_prepare(Signal*, SchemaOpPtr);
  void dropIndex_commit(Signal*, SchemaOpPtr);
  void dropIndex_complete(Signal*, SchemaOpPtr);
  //
  void dropIndex_abortParse(Signal*, SchemaOpPtr);
  void dropIndex_abortPrepare(Signal*, SchemaOpPtr);

  // sub-ops
  void dropIndex_toDropTable(Signal*, SchemaOpPtr);
  void dropIndex_fromDropTable(Signal*, Uint32 op_key, Uint32 ret);
  void dropIndex_toAlterIndex(Signal*, SchemaOpPtr);
  void dropIndex_fromAlterIndex(Signal*, Uint32 op_key, Uint32 ret);

  // MODULE: AlterIndex

  struct TriggerTmpl {
    const char* nameFormat; // contains one %u for index id
    const TriggerInfo triggerInfo;
  };

  static const TriggerTmpl g_hashIndexTriggerTmpl[1];
  static const TriggerTmpl g_orderedIndexTriggerTmpl[1];
  static const TriggerTmpl g_buildIndexConstraintTmpl[1];
  static const TriggerTmpl g_reorgTriggerTmpl[1];

  struct AlterIndexRec : public OpRec {
    AlterIndxImplReq m_request;
    IndexAttributeList m_attrList;
    AttributeMask m_attrMask;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::AlterIndexRec>&
    getPool(Dbdict* dict) {
      return dict->c_alterIndexRecPool;
    }

    // sub-operation counters (true = done or skip)
    const TriggerTmpl* m_triggerTmpl;
    bool m_sub_trigger;
    bool m_sub_build_index;
    bool m_sub_index_stat_dml;
    bool m_sub_index_stat_mon;

    // prepare phase
    bool m_tc_index_done;

    // connect pointers towards DIH and LQH
    Uint32 m_dihAddFragPtr;
    Uint32 m_lqhFragPtr;

    AlterIndexRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      memset(&m_attrList, 0, sizeof(m_attrList));
      m_attrMask.clear();
      m_triggerTmpl = 0;
      m_sub_trigger = false;
      m_sub_build_index = false;
      m_sub_index_stat_dml = false;
      m_sub_index_stat_mon = false;
      m_tc_index_done = false;
    }

#ifdef VM_TRACE
    void print(NdbOut&) const;
#endif
  };

  typedef Ptr<AlterIndexRec> AlterIndexRecPtr;
  ArrayPool<AlterIndexRec> c_alterIndexRecPool;

  // OpInfo
  bool alterIndex_seize(SchemaOpPtr);
  void alterIndex_release(SchemaOpPtr);
  //
  void alterIndex_parse(Signal*, bool master,
                        SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool alterIndex_subOps(Signal*, SchemaOpPtr);
  void alterIndex_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void alterIndex_prepare(Signal*, SchemaOpPtr);
  void alterIndex_commit(Signal*, SchemaOpPtr);
  void alterIndex_complete(Signal*, SchemaOpPtr);
  //
  void alterIndex_abortParse(Signal*, SchemaOpPtr);
  void alterIndex_abortPrepare(Signal*, SchemaOpPtr);

  // parse phase sub-routine
  void set_index_stat_frag(Signal*, TableRecordPtr indexPtr);

  // sub-ops
  void alterIndex_toCreateTrigger(Signal*, SchemaOpPtr);
  void alterIndex_atCreateTrigger(Signal*, SchemaOpPtr);
  void alterIndex_fromCreateTrigger(Signal*, Uint32 op_key, Uint32 ret);
  void alterIndex_toDropTrigger(Signal*, SchemaOpPtr);
  void alterIndex_atDropTrigger(Signal*, SchemaOpPtr);
  void alterIndex_fromDropTrigger(Signal*, Uint32 op_key, Uint32 ret);
  void alterIndex_toBuildIndex(Signal*, SchemaOpPtr);
  void alterIndex_fromBuildIndex(Signal*, Uint32 op_key, Uint32 ret);
  void alterIndex_toIndexStat(Signal*, SchemaOpPtr);
  void alterIndex_fromIndexStat(Signal*, Uint32 op_key, Uint32 ret);

  // prepare phase
  void alterIndex_toCreateLocal(Signal*, SchemaOpPtr);
  void alterIndex_toDropLocal(Signal*, SchemaOpPtr);
  void alterIndex_fromLocal(Signal*, Uint32 op_key, Uint32 ret);

  void alterIndex_toAddPartitions(Signal*, SchemaOpPtr);
  void alterIndex_fromAddPartitions(Signal*, Uint32 op_key, Uint32 ret);

  // abort
  void alterIndex_abortFromLocal(Signal*, Uint32 op_key, Uint32 ret);

  // MODULE: BuildIndex

  // this prepends 1 column used for FRAGMENT in hash index table key
  typedef Id_array<1 + MAX_ATTRIBUTES_IN_INDEX> FragAttributeList;

  struct BuildIndexRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::BuildIndexRec>&
    getPool(Dbdict* dict) {
      return dict->c_buildIndexRecPool;
    }

    BuildIndxImplReq m_request;

    IndexAttributeList m_indexKeyList;
    FragAttributeList m_tableKeyList;
    AttributeMask m_attrMask;

    // sub-operation counters (CTr BIn DTr)
    const TriggerTmpl* m_triggerTmpl;
    Uint32 m_subOpCount;    // 3 or 0
    Uint32 m_subOpIndex;

    // do the actual build (i.e. not done in a sub-op BIn)
    bool m_doBuild;

    BuildIndexRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      memset(&m_indexKeyList, 0, sizeof(m_indexKeyList));
      memset(&m_tableKeyList, 0, sizeof(m_tableKeyList));
      m_attrMask.clear();
      m_triggerTmpl = 0;
      m_subOpCount = 0;
      m_subOpIndex = 0;
      m_doBuild = false;
    }
  };

  typedef Ptr<BuildIndexRec> BuildIndexRecPtr;
  ArrayPool<BuildIndexRec> c_buildIndexRecPool;

  // OpInfo
  bool buildIndex_seize(SchemaOpPtr);
  void buildIndex_release(SchemaOpPtr);
  //
  void buildIndex_parse(Signal*, bool master,
                        SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool buildIndex_subOps(Signal*, SchemaOpPtr);
  void buildIndex_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void buildIndex_prepare(Signal*, SchemaOpPtr);
  void buildIndex_commit(Signal*, SchemaOpPtr);
  void buildIndex_complete(Signal*, SchemaOpPtr);
  //
  void buildIndex_abortParse(Signal*, SchemaOpPtr);
  void buildIndex_abortPrepare(Signal*, SchemaOpPtr);

  // parse phase
  void buildIndex_toCreateConstraint(Signal*, SchemaOpPtr);
  void buildIndex_atCreateConstraint(Signal*, SchemaOpPtr);
  void buildIndex_fromCreateConstraint(Signal*, Uint32 op_key, Uint32 ret);
  //
  void buildIndex_toBuildIndex(Signal*, SchemaOpPtr);
  void buildIndex_fromBuildIndex(Signal*, Uint32 op_key, Uint32 ret);
  //
  void buildIndex_toDropConstraint(Signal*, SchemaOpPtr);
  void buildIndex_atDropConstraint(Signal*, SchemaOpPtr);
  void buildIndex_fromDropConstraint(Signal*, Uint32 op_key, Uint32 ret);

  // prepare phase
  void buildIndex_toLocalBuild(Signal*, SchemaOpPtr);
  void buildIndex_fromLocalBuild(Signal*, Uint32 op_key, Uint32 ret);

  // commit phase
  void buildIndex_toLocalOnline(Signal*, SchemaOpPtr);
  void buildIndex_fromLocalOnline(Signal*, Uint32 op_key, Uint32 ret);

  // MODULE: IndexStat

  struct IndexStatRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::IndexStatRec>&
    getPool(Dbdict* dict) {
      return dict->c_indexStatRecPool;
    }

    IndexStatImplReq m_request;

    // sub-operation counters
    const TriggerTmpl* m_triggerTmpl;
    Uint32 m_subOpCount;
    Uint32 m_subOpIndex;

    IndexStatRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_subOpCount = 0;
      m_subOpIndex = 0;
    }
  };

  typedef Ptr<IndexStatRec> IndexStatRecPtr;
  ArrayPool<IndexStatRec> c_indexStatRecPool;

  Uint32 c_indexStatAutoCreate;
  Uint32 c_indexStatAutoUpdate;
  Uint32 c_indexStatBgId;

  // OpInfo
  bool indexStat_seize(SchemaOpPtr);
  void indexStat_release(SchemaOpPtr);
  //
  void indexStat_parse(Signal*, bool master,
                        SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool indexStat_subOps(Signal*, SchemaOpPtr);
  void indexStat_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void indexStat_prepare(Signal*, SchemaOpPtr);
  void indexStat_commit(Signal*, SchemaOpPtr);
  void indexStat_complete(Signal*, SchemaOpPtr);
  //
  void indexStat_abortParse(Signal*, SchemaOpPtr);
  void indexStat_abortPrepare(Signal*, SchemaOpPtr);

  // parse phase
  void indexStat_toIndexStat(Signal*, SchemaOpPtr, Uint32 requestType);
  void indexStat_fromIndexStat(Signal*, Uint32 op_key, Uint32 ret);

  // prepare phase
  void indexStat_toLocalStat(Signal*, SchemaOpPtr);
  void indexStat_fromLocalStat(Signal*, Uint32 op_key, Uint32 ret);

  // background processing of stat requests
  void indexStatBg_process(Signal*);
  void indexStatBg_fromBeginTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void indexStatBg_fromIndexStat(Signal*, Uint32 tx_key, Uint32 ret);
  void indexStatBg_fromEndTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void indexStatBg_sendContinueB(Signal*);

  // MODULE: CreateHashMap

  struct HashMapRecord {
    HashMapRecord(){}

    /* Table id (array index in DICT and other blocks) */
    union {
      Uint32 m_object_id;
      Uint32 key;
    };
    Uint32 m_obj_ptr_i;      // in HashMap_pool
    Uint32 m_object_version;

    RopeHandle m_name;

    /**
     * ptr.i, in g_hash_map
     */
    Uint32 m_map_ptr_i;
    union {
      Uint32 nextPool;
      Uint32 nextHash;
    };
    Uint32 prevHash;

    Uint32 hashValue() const { return key;}
    bool equal(const HashMapRecord& obj) const { return key == obj.key;}

  };
  typedef Ptr<HashMapRecord> HashMapPtr;
  typedef ArrayPool<HashMapRecord> HashMap_pool;
  typedef KeyTableImpl<HashMap_pool, HashMapRecord> HashMap_hash;

  HashMap_pool c_hash_map_pool;
  HashMap_hash c_hash_map_hash;
  RSS_AP_SNAPSHOT(c_hash_map_pool);
  RSS_AP_SNAPSHOT(g_hash_map);

  struct CreateHashMapRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateHashMapRec>&
    getPool(Dbdict* dict) {
      return dict->c_createHashMapRecPool;
    }

    CreateHashMapImplReq m_request;

    CreateHashMapRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
    }
  };

  typedef Ptr<CreateHashMapRec> CreateHashMapRecPtr;
  ArrayPool<CreateHashMapRec> c_createHashMapRecPool;
  void execCREATE_HASH_MAP_REQ(Signal* signal);

  // OpInfo
  bool createHashMap_seize(SchemaOpPtr);
  void createHashMap_release(SchemaOpPtr);
  //
  void createHashMap_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool createHashMap_subOps(Signal*, SchemaOpPtr);
  void createHashMap_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createHashMap_prepare(Signal*, SchemaOpPtr);
  void createHashMap_writeObjConf(Signal* signal, Uint32, Uint32);
  void createHashMap_commit(Signal*, SchemaOpPtr);
  void createHashMap_complete(Signal*, SchemaOpPtr);
  //
  void createHashMap_abortParse(Signal*, SchemaOpPtr);
  void createHashMap_abortPrepare(Signal*, SchemaOpPtr);

  void packHashMapIntoPages(SimpleProperties::Writer&, Ptr<HashMapRecord>);

  // MODULE: CopyData

  struct CopyDataRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CopyDataRec>&
    getPool(Dbdict* dict) {
      return dict->c_copyDataRecPool;
    }

    CopyDataImplReq m_request;

    CopyDataRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
    }
  };

  typedef Ptr<CopyDataRec> CopyDataRecPtr;
  ArrayPool<CopyDataRec> c_copyDataRecPool;
  void execCOPY_DATA_REQ(Signal* signal);
  void execCOPY_DATA_REF(Signal* signal);
  void execCOPY_DATA_CONF(Signal* signal);
  void execCOPY_DATA_IMPL_REF(Signal* signal);
  void execCOPY_DATA_IMPL_CONF(Signal* signal);

  // OpInfo
  bool copyData_seize(SchemaOpPtr);
  void copyData_release(SchemaOpPtr);
  //
  void copyData_parse(Signal*, bool master,
                      SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool copyData_subOps(Signal*, SchemaOpPtr);
  void copyData_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void copyData_prepare(Signal*, SchemaOpPtr);
  void copyData_fromLocal(Signal*, Uint32, Uint32);
  void copyData_commit(Signal*, SchemaOpPtr);
  void copyData_complete(Signal*, SchemaOpPtr);
  //
  void copyData_abortParse(Signal*, SchemaOpPtr);
  void copyData_abortPrepare(Signal*, SchemaOpPtr);

  /**
   * Operation record for Util Signals.
   */
  struct OpSignalUtil : OpRecordCommon{
    Callback m_callback;
    Uint32 m_userData;
  };
  typedef Ptr<OpSignalUtil> OpSignalUtilPtr;

  /**
   * Operation record for subscribe-start-stop
   */
  struct OpSubEvent : OpRecordCommon {
    Uint32 m_senderRef;
    Uint32 m_senderData;
    Uint32 m_errorCode;

    Uint32 m_gsn;
    Uint32 m_subscriptionId;
    Uint32 m_subscriptionKey;
    Uint32 m_subscriberRef;
    Uint32 m_subscriberData;
    Uint8 m_buckets_per_ng[256]; // For SUB_START_REQ
    union {
      SubStartConf m_sub_start_conf;
      SubStopConf m_sub_stop_conf;
    };
    RequestTracker m_reqTracker;
  };
  typedef Ptr<OpSubEvent> OpSubEventPtr;

  /**
   * Operation record for create event.
   */
  struct OpCreateEvent : OpRecordCommon {
    // original request (event id will be added)
    CreateEvntReq m_request;
    //AttributeMask m_attrListBitmask;
    //    AttributeList m_attrList;
    sysTab_NDBEVENTS_0 m_eventRec;
    //    char m_eventName[MAX_TAB_NAME_SIZE];
    //    char m_tableName[MAX_TAB_NAME_SIZE];

    // coordinator DICT
    RequestTracker m_reqTracker;
    // state info
    CreateEvntReq::RequestType m_requestType;
    // error info
    Uint32 m_errorCode;
    Uint32 m_errorLine;
    Uint32 m_errorNode; /* also used to store master node id
                           in case of NotMaster */
    // ctor
    OpCreateEvent() {
      memset(&m_request, 0, sizeof(m_request));
      m_requestType = CreateEvntReq::RT_UNDEFINED;
      m_errorCode = CreateEvntRef::NoError;
      m_errorLine = 0;
      m_errorNode = 0;
    }
    void init(const CreateEvntReq* req, Dbdict* dp) {
      m_request = *req;
      m_errorCode = CreateEvntRef::NoError;
      m_errorLine = 0;
      m_errorNode = 0;
      m_requestType = req->getRequestType();
    }
    bool hasError() {
      return m_errorCode != CreateEvntRef::NoError;
    }
    void setError(const CreateEvntRef* ref) {
      if (ref != 0 && ! hasError()) {
        m_errorCode = ref->getErrorCode();
        m_errorLine = ref->getErrorLine();
        m_errorNode = ref->getErrorNode();
      }
    }

  };
  typedef Ptr<OpCreateEvent> OpCreateEventPtr;

  /**
   * Operation record for drop event.
   */
  struct OpDropEvent : OpRecordCommon {
    // original request
    DropEvntReq m_request;
    //    char m_eventName[MAX_TAB_NAME_SIZE];
    sysTab_NDBEVENTS_0 m_eventRec;
    RequestTracker m_reqTracker;
    // error info
    Uint32 m_errorCode;
    Uint32 m_errorLine;
    Uint32 m_errorNode;
    // ctor
    OpDropEvent() {
      memset(&m_request, 0, sizeof(m_request));
      m_errorCode = 0;
      m_errorLine = 0;
      m_errorNode = 0;
    }
    void init(const DropEvntReq* req) {
      m_request = *req;
      m_errorCode = 0;
      m_errorLine = 0;
      m_errorNode = 0;
    }
    bool hasError() {
      return m_errorCode != 0;
    }
    void setError(const DropEvntRef* ref) {
      if (ref != 0 && ! hasError()) {
        m_errorCode = ref->getErrorCode();
        m_errorLine = ref->getErrorLine();
        m_errorNode = ref->getErrorNode();
      }
    }
  };
  typedef Ptr<OpDropEvent> OpDropEventPtr;

  // MODULE: CreateTrigger
  struct CreateTriggerRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateTriggerRec>&
    getPool(Dbdict* dict) {
      return dict->c_createTriggerRecPool;
    }

    CreateTrigImplReq m_request;

    char m_triggerName[MAX_TAB_NAME_SIZE];
    // sub-operation counters
    bool m_created;
    bool m_main_op;
    bool m_sub_dst; // Create trigger destination
    bool m_sub_src; // Create trigger source
    Uint32 m_block_list[1]; // Only 1 block...

    CreateTriggerRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      memset(m_triggerName, 0, sizeof(m_triggerName));
      m_main_op = true;
      m_sub_src = false;
      m_sub_dst = false;
      m_created = false;
    }
  };

  typedef Ptr<CreateTriggerRec> CreateTriggerRecPtr;
  ArrayPool<CreateTriggerRec> c_createTriggerRecPool;

  // OpInfo
  bool createTrigger_seize(SchemaOpPtr);
  void createTrigger_release(SchemaOpPtr);
  //
  void createTrigger_parse(Signal*, bool master,
                           SchemaOpPtr, SectionHandle&, ErrorInfo&);
  void createTrigger_parse_endpoint(Signal*, SchemaOpPtr op_ptr, ErrorInfo&);
  bool createTrigger_subOps(Signal*, SchemaOpPtr);
  void createTrigger_toCreateEndpoint(Signal*, SchemaOpPtr,
				      CreateTrigReq::EndpointFlag);
  void createTrigger_fromCreateEndpoint(Signal*, Uint32, Uint32);
  void createTrigger_create_drop_trigger_operation(Signal*,SchemaOpPtr,
                                                   ErrorInfo& error);

  void createTrigger_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createTrigger_prepare(Signal*, SchemaOpPtr);
  void createTrigger_prepare_fromLocal(Signal*, Uint32 op_key, Uint32 ret);
  void createTrigger_commit(Signal*, SchemaOpPtr);
  void createTrigger_commit_fromLocal(Signal*, Uint32 op_key, Uint32 ret);
  void createTrigger_complete(Signal*, SchemaOpPtr);
  //
  void createTrigger_abortParse(Signal*, SchemaOpPtr);
  void createTrigger_abortPrepare(Signal*, SchemaOpPtr);
  void createTrigger_abortPrepare_fromLocal(Signal*, Uint32, Uint32);
  void send_create_trig_req(Signal*, SchemaOpPtr);

  // MODULE: DropTrigger

  struct DropTriggerRec : public OpRec {
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::DropTriggerRec>&
    getPool(Dbdict* dict) {
      return dict->c_dropTriggerRecPool;
    }

    DropTrigImplReq m_request;

    char m_triggerName[MAX_TAB_NAME_SIZE];
    // sub-operation counters
    bool m_main_op;
    bool m_sub_dst; // Create trigger destination
    bool m_sub_src; // Create trigger source
    Uint32 m_block_list[1]; // Only 1 block...

    DropTriggerRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      memset(m_triggerName, 0, sizeof(m_triggerName));
      m_main_op = true;
      m_sub_src = false;
      m_sub_dst = false;
    }
  };

  typedef Ptr<DropTriggerRec> DropTriggerRecPtr;
  ArrayPool<DropTriggerRec> c_dropTriggerRecPool;

  // OpInfo
  bool dropTrigger_seize(SchemaOpPtr);
  void dropTrigger_release(SchemaOpPtr);
  //
  void dropTrigger_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  void dropTrigger_parse_endpoint(Signal*, SchemaOpPtr op_ptr, ErrorInfo&);
  bool dropTrigger_subOps(Signal*, SchemaOpPtr);
  void dropTrigger_toDropEndpoint(Signal*, SchemaOpPtr,
				  DropTrigReq::EndpointFlag);
  void dropTrigger_fromDropEndpoint(Signal*, Uint32, Uint32);
  void dropTrigger_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void dropTrigger_prepare(Signal*, SchemaOpPtr);
  void dropTrigger_commit(Signal*, SchemaOpPtr);
  void dropTrigger_commit_fromLocal(Signal*, Uint32, Uint32);
  void dropTrigger_complete(Signal*, SchemaOpPtr);
  //
  void dropTrigger_abortParse(Signal*, SchemaOpPtr);
  void dropTrigger_abortPrepare(Signal*, SchemaOpPtr);

  void send_drop_trig_req(Signal*, SchemaOpPtr);


  // MODULE: CreateFilegroup

  struct CreateFilegroupRec : public OpRec {
    bool m_parsed, m_prepared;
    CreateFilegroupImplReq m_request;
    Uint32 m_warningFlags;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateFilegroupRec>&
    getPool(Dbdict* dict) {
      return dict->c_createFilegroupRecPool;
    }

    CreateFilegroupRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_parsed = m_prepared = false;
      m_warningFlags = 0;
    }
  };

  typedef Ptr<CreateFilegroupRec> CreateFilegroupRecPtr;
  ArrayPool<CreateFilegroupRec> c_createFilegroupRecPool;

  // OpInfo
  bool createFilegroup_seize(SchemaOpPtr);
  void createFilegroup_release(SchemaOpPtr);
  //
  void createFilegroup_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool createFilegroup_subOps(Signal*, SchemaOpPtr);
  void createFilegroup_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createFilegroup_prepare(Signal*, SchemaOpPtr);
  void createFilegroup_commit(Signal*, SchemaOpPtr);
  void createFilegroup_complete(Signal*, SchemaOpPtr);
  //
  void createFilegroup_abortParse(Signal*, SchemaOpPtr);
  void createFilegroup_abortPrepare(Signal*, SchemaOpPtr);

  void createFilegroup_fromLocal(Signal*, Uint32, Uint32);
  void createFilegroup_fromWriteObjInfo(Signal*, Uint32, Uint32);

  // MODULE: CreateFile

  struct CreateFileRec : public OpRec {
    bool m_parsed, m_prepared;
    CreateFileImplReq m_request;
    Uint32 m_warningFlags;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateFileRec>&
    getPool(Dbdict* dict) {
      return dict->c_createFileRecPool;
    }

    CreateFileRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_parsed = m_prepared = false;
      m_warningFlags = 0;
    }
  };

  typedef Ptr<CreateFileRec> CreateFileRecPtr;
  ArrayPool<CreateFileRec> c_createFileRecPool;

  // OpInfo
  bool createFile_seize(SchemaOpPtr);
  void createFile_release(SchemaOpPtr);
  //
  void createFile_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool createFile_subOps(Signal*, SchemaOpPtr);
  void createFile_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createFile_prepare(Signal*, SchemaOpPtr);
  void createFile_commit(Signal*, SchemaOpPtr);
  void createFile_complete(Signal*, SchemaOpPtr);
  //
  void createFile_abortParse(Signal*, SchemaOpPtr);
  void createFile_abortPrepare(Signal*, SchemaOpPtr);

  void createFile_fromLocal(Signal*, Uint32, Uint32);
  void createFile_fromWriteObjInfo(Signal*, Uint32, Uint32);

  // MODULE: DropFilegroup

  struct DropFilegroupRec : public OpRec {
    bool m_parsed, m_prepared;
    DropFilegroupImplReq m_request;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::DropFilegroupRec>&
    getPool(Dbdict* dict) {
      return dict->c_dropFilegroupRecPool;
    }

    DropFilegroupRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_parsed = m_prepared = false;
    }
  };

  typedef Ptr<DropFilegroupRec> DropFilegroupRecPtr;
  ArrayPool<DropFilegroupRec> c_dropFilegroupRecPool;

  // OpInfo
  bool dropFilegroup_seize(SchemaOpPtr);
  void dropFilegroup_release(SchemaOpPtr);
  //
  void dropFilegroup_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool dropFilegroup_subOps(Signal*, SchemaOpPtr);
  void dropFilegroup_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void dropFilegroup_prepare(Signal*, SchemaOpPtr);
  void dropFilegroup_commit(Signal*, SchemaOpPtr);
  void dropFilegroup_complete(Signal*, SchemaOpPtr);
  //
  void dropFilegroup_abortParse(Signal*, SchemaOpPtr);
  void dropFilegroup_abortPrepare(Signal*, SchemaOpPtr);

  void dropFilegroup_fromLocal(Signal*, Uint32, Uint32);

  // MODULE: DropFile

  struct DropFileRec : public OpRec {
    bool m_parsed, m_prepared;
    DropFileImplReq m_request;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::DropFileRec>&
    getPool(Dbdict* dict) {
      return dict->c_dropFileRecPool;
    }

    DropFileRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_parsed = m_prepared = false;
    }
  };

  typedef Ptr<DropFileRec> DropFileRecPtr;
  ArrayPool<DropFileRec> c_dropFileRecPool;

  // OpInfo
  bool dropFile_seize(SchemaOpPtr);
  void dropFile_release(SchemaOpPtr);
  //
  void dropFile_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool dropFile_subOps(Signal*, SchemaOpPtr);
  void dropFile_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void dropFile_prepare(Signal*, SchemaOpPtr);
  void dropFile_commit(Signal*, SchemaOpPtr);
  void dropFile_complete(Signal*, SchemaOpPtr);
  //
  void dropFile_abortParse(Signal*, SchemaOpPtr);
  void dropFile_abortPrepare(Signal*, SchemaOpPtr);

  void dropFile_fromLocal(Signal*, Uint32, Uint32);

  // MODULE: CreateNodegroup

  struct CreateNodegroupRec : public OpRec {
    bool m_map_created;
    CreateNodegroupImplReq m_request;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::CreateNodegroupRec>&
    getPool(Dbdict* dict) {
      return dict->c_createNodegroupRecPool;
    }

    CreateNodegroupRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_map_created = false;
      m_blockIndex = RNIL;
      m_blockCnt = RNIL;
      m_cnt_waitGCP = RNIL;
      m_wait_gcp_type = RNIL;
      m_substartstop_blocked = false;
      m_gcp_blocked = false;
    }

    enum { BlockCount = 3 };
    Uint32 m_blockNo[BlockCount];
    Uint32 m_blockIndex;
    Uint32 m_blockCnt;
    Uint32 m_cnt_waitGCP;
    Uint32 m_wait_gcp_type;
    bool m_gcp_blocked;
    bool m_substartstop_blocked;
  };

  typedef Ptr<CreateNodegroupRec> CreateNodegroupRecPtr;
  ArrayPool<CreateNodegroupRec> c_createNodegroupRecPool;

  // OpInfo
  void execCREATE_NODEGROUP_REQ(Signal*);
  void execCREATE_NODEGROUP_IMPL_REF(Signal*);
  void execCREATE_NODEGROUP_IMPL_CONF(Signal*);

  bool createNodegroup_seize(SchemaOpPtr);
  void createNodegroup_release(SchemaOpPtr);
  //
  void createNodegroup_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool createNodegroup_subOps(Signal*, SchemaOpPtr);
  void createNodegroup_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void createNodegroup_prepare(Signal*, SchemaOpPtr);
  void createNodegroup_commit(Signal*, SchemaOpPtr);
  void createNodegroup_complete(Signal*, SchemaOpPtr);
  //
  void createNodegroup_abortParse(Signal*, SchemaOpPtr);
  void createNodegroup_abortPrepare(Signal*, SchemaOpPtr);

  void createNodegroup_toLocal(Signal*, SchemaOpPtr);
  void createNodegroup_fromLocal(Signal*, Uint32 op_key, Uint32 ret);
  void createNodegroup_fromCreateHashMap(Signal*, Uint32 op_key, Uint32 ret);
  void createNodegroup_fromWaitGCP(Signal*, Uint32 op_key, Uint32 ret);
  void createNodegroup_fromBlockSubStartStop(Signal*, Uint32 op_key, Uint32);

  void execCREATE_HASH_MAP_REF(Signal* signal);
  void execCREATE_HASH_MAP_CONF(Signal* signal);

  // MODULE: DropNodegroup

  struct DropNodegroupRec : public OpRec {
    DropNodegroupImplReq m_request;

    // reflection
    static const OpInfo g_opInfo;

    static ArrayPool<Dbdict::DropNodegroupRec>&
    getPool(Dbdict* dict) {
      return dict->c_dropNodegroupRecPool;
    }

    DropNodegroupRec() :
      OpRec(g_opInfo, (Uint32*)&m_request) {
      memset(&m_request, 0, sizeof(m_request));
      m_blockIndex = RNIL;
      m_blockCnt = RNIL;
      m_cnt_waitGCP = RNIL;
      m_wait_gcp_type = RNIL;
      m_gcp_blocked = false;
      m_substartstop_blocked = false;
    }

    enum { BlockCount = 3 };
    Uint32 m_blockNo[BlockCount];
    Uint32 m_blockIndex;
    Uint32 m_blockCnt;
    Uint32 m_cnt_waitGCP;
    Uint32 m_wait_gcp_type;
    bool m_gcp_blocked;
    bool m_substartstop_blocked;
  };

  typedef Ptr<DropNodegroupRec> DropNodegroupRecPtr;
  ArrayPool<DropNodegroupRec> c_dropNodegroupRecPool;

  // OpInfo
  void execDROP_NODEGROUP_REQ(Signal*);
  void execDROP_NODEGROUP_IMPL_REF(Signal*);
  void execDROP_NODEGROUP_IMPL_CONF(Signal*);

  bool dropNodegroup_seize(SchemaOpPtr);
  void dropNodegroup_release(SchemaOpPtr);
  //
  void dropNodegroup_parse(Signal*, bool master,
                         SchemaOpPtr, SectionHandle&, ErrorInfo&);
  bool dropNodegroup_subOps(Signal*, SchemaOpPtr);
  void dropNodegroup_reply(Signal*, SchemaOpPtr, ErrorInfo);
  //
  void dropNodegroup_prepare(Signal*, SchemaOpPtr);
  void dropNodegroup_commit(Signal*, SchemaOpPtr);
  void dropNodegroup_complete(Signal*, SchemaOpPtr);
  //
  void dropNodegroup_abortParse(Signal*, SchemaOpPtr);
  void dropNodegroup_abortPrepare(Signal*, SchemaOpPtr);

  void dropNodegroup_toLocal(Signal*, SchemaOpPtr);
  void dropNodegroup_fromLocal(Signal*, Uint32 op_key, Uint32 ret);
  void dropNodegroup_fromWaitGCP(Signal*, Uint32 op_key, Uint32 ret);
  void dropNodegroup_fromBlockSubStartStop(Signal*, Uint32 op_key, Uint32);

  /**
   * Only used at coordinator/master
   */
  // Common operation record pool
public:
  STATIC_CONST( opCreateEventSize = sizeof(OpCreateEvent) );
  STATIC_CONST( opSubEventSize = sizeof(OpSubEvent) );
  STATIC_CONST( opDropEventSize = sizeof(OpDropEvent) );
  STATIC_CONST( opSignalUtilSize = sizeof(OpSignalUtil) );
private:
#define PTR_ALIGN(n) ((((n)+sizeof(void*)-1)>>2)&~((sizeof(void*)-1)>>2))
  union OpRecordUnion {
    Uint32 u_opCreateEvent  [PTR_ALIGN(opCreateEventSize)];
    Uint32 u_opSubEvent     [PTR_ALIGN(opSubEventSize)];
    Uint32 u_opDropEvent    [PTR_ALIGN(opDropEventSize)];
    Uint32 u_opSignalUtil   [PTR_ALIGN(opSignalUtilSize)];
    Uint32 nextPool;
  };
  ArrayPool<OpRecordUnion> c_opRecordPool;
  
  // Operation records
  KeyTable2C<OpCreateEvent, OpRecordUnion> c_opCreateEvent;
  KeyTable2C<OpSubEvent, OpRecordUnion> c_opSubEvent;
  KeyTable2C<OpDropEvent, OpRecordUnion> c_opDropEvent;
  KeyTable2C<OpSignalUtil, OpRecordUnion> c_opSignalUtil;

  // Unique key for operation  XXX move to some system table
  Uint32 c_opRecordSequence;

  void handleNdbdFailureCallback(Signal* signal, 
                                 Uint32 failedNodeId,
                                 Uint32 ignoredRc);
  void handleApiFailureCallback(Signal* signal,
                                Uint32 failedNodeId,
                                Uint32 ignoredRc);
  // Statement blocks

  /* ------------------------------------------------------------ */
  // Start/Restart Handling
  /* ------------------------------------------------------------ */
  void sendSTTORRY(Signal* signal);
  void sendNDB_STTORRY(Signal* signal);
  void initSchemaFile(Signal* signal);
  
  /* ------------------------------------------------------------ */
  // Drop Table Handling
  /* ------------------------------------------------------------ */
  void releaseTableObject(Uint32 tableId, bool removeFromHash = true);
  
  /* ------------------------------------------------------------ */
  // General Stuff
  /* ------------------------------------------------------------ */
  Uint32 getFreeObjId(Uint32 minId, bool both = false);
  Uint32 getFreeTableRecord(Uint32 primaryTableId);
  Uint32 getFreeTriggerRecord();
  bool getNewAttributeRecord(TableRecordPtr tablePtr,
			     AttributeRecordPtr & attrPtr);
  void packTableIntoPages(Signal* signal);
  void packTableIntoPages(SimpleProperties::Writer &, TableRecordPtr, Signal* =0);
  void packFilegroupIntoPages(SimpleProperties::Writer &,
			      FilegroupPtr,
			      const Uint32 undo_free_hi,
			      const Uint32 undo_free_lo);
  void packFileIntoPages(SimpleProperties::Writer &, FilePtr, const Uint32);
  
  void sendGET_TABINFOREQ(Signal* signal,
                          Uint32 tableId);
  void sendTC_SCHVERREQ(Signal* signal,
                        Uint32 tableId,
                        BlockReference tcRef);
  
  /* ------------------------------------------------------------ */
  // System Restart Handling
  /* ------------------------------------------------------------ */
  void initSendSchemaData(Signal* signal);
  void sendSchemaData(Signal* signal);
  Uint32 sendSCHEMA_INFO(Signal* signal, Uint32 nodeId, Uint32* pagePointer);
  void sendDIHSTARTTAB_REQ(Signal* signal);
  
  /* ------------------------------------------------------------ */
  // Receive Table Handling
  /* ------------------------------------------------------------ */
  void handleTabInfoInit(Signal*, SchemaTransPtr&,
                         SimpleProperties::Reader &,
			 ParseDictTabInfoRecord *,
			 bool checkExist = true);
  void handleTabInfo(SimpleProperties::Reader & it, ParseDictTabInfoRecord *,
		     DictTabInfo::Table & tableDesc);
  
  void handleAddTableFailure(Signal* signal,
                             Uint32 failureLine,
                             Uint32 tableId);
  bool verifyTableCorrect(Signal* signal, Uint32 tableId);
  
  /* ------------------------------------------------------------ */
  // Add Fragment Handling
  /* ------------------------------------------------------------ */
  void sendLQHADDATTRREQ(Signal*, SchemaOpPtr, Uint32 attributePtrI);
  
  /* ------------------------------------------------------------ */
  // Read/Write Schema and Table files
  /* ------------------------------------------------------------ */
  void updateSchemaState(Signal* signal, Uint32 tableId, 
			 SchemaFile::TableEntry*, Callback*,
                         bool savetodisk = 1, bool dicttrans = 0);
  void startWriteSchemaFile(Signal* signal);
  void openSchemaFile(Signal* signal,
                      Uint32 fileNo,
                      Uint32 fsPtr,
                      bool writeFlag,
                      bool newFile);
  void writeSchemaFile(Signal* signal, Uint32 filePtr, Uint32 fsPtr);
  void writeSchemaConf(Signal* signal,
                               FsConnectRecordPtr fsPtr);
  void closeFile(Signal* signal, Uint32 filePtr, Uint32 fsPtr);
  void closeWriteSchemaConf(Signal* signal,
                               FsConnectRecordPtr fsPtr);
  void initSchemaFile_conf(Signal* signal, Uint32 i, Uint32 returnCode);
  
  void writeTableFile(Signal* signal, Uint32 tableId, 
		      SegmentedSectionPtr tabInfo, Callback*);
  void writeTableFile(Signal* signal, Uint32 tableId,
		      OpSection opSection, Callback*);
  void startWriteTableFile(Signal* signal, Uint32 tableId);
  void openTableFile(Signal* signal, 
                     Uint32 fileNo,
                     Uint32 fsPtr,
                     Uint32 tableId,
                     bool writeFlag);
  void writeTableFile(Signal* signal, Uint32 filePtr, Uint32 fsPtr);
  void writeTableConf(Signal* signal,
                      FsConnectRecordPtr fsPtr);
  void closeWriteTableConf(Signal* signal,
                           FsConnectRecordPtr fsPtr);

  void startReadTableFile(Signal* signal, Uint32 tableId);
  void openReadTableRef(Signal* signal,
                        FsConnectRecordPtr fsPtr);
  void readTableFile(Signal* signal, Uint32 filePtr, Uint32 fsPtr);
  void readTableConf(Signal* signal,
                     FsConnectRecordPtr fsPtr);
  void readTableRef(Signal* signal,
                    FsConnectRecordPtr fsPtr);
  void closeReadTableConf(Signal* signal,
                          FsConnectRecordPtr fsPtr);

  void startReadSchemaFile(Signal* signal);
  void openReadSchemaRef(Signal* signal,
                         FsConnectRecordPtr fsPtr);
  void readSchemaFile(Signal* signal, Uint32 filePtr, Uint32 fsPtr);
  void readSchemaConf(Signal* signal, FsConnectRecordPtr fsPtr);
  void readSchemaRef(Signal* signal, FsConnectRecordPtr fsPtr);
  void closeReadSchemaConf(Signal* signal,
                           FsConnectRecordPtr fsPtr);
  bool convertSchemaFileTo_5_0_6(XSchemaFile*);
  bool convertSchemaFileTo_6_4(XSchemaFile*);

  /* ------------------------------------------------------------ */
  // Get table definitions
  /* ------------------------------------------------------------ */
  void sendGET_TABINFOREF(Signal* signal, 
			  GetTabInfoReq*,
			  GetTabInfoRef::ErrorCode errorCode,
                          Uint32 errorLine);

  void sendGET_TABLEID_REF(Signal* signal, 
			   GetTableIdReq * req,
			   GetTableIdRef::ErrorCode errorCode);

  void sendGetTabResponse(Signal* signal);

  /* ------------------------------------------------------------ */
  // Indexes and triggers
  /* ------------------------------------------------------------ */

  // reactivate and rebuild indexes on start up
  void activateIndexes(Signal* signal, Uint32 i);
  void activateIndex_fromBeginTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void activateIndex_fromAlterIndex(Signal*, Uint32 tx_key, Uint32 ret);
  void activateIndex_fromEndTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void rebuildIndexes(Signal* signal, Uint32 i);
  void rebuildIndex_fromBeginTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void rebuildIndex_fromBuildIndex(Signal*, Uint32 tx_key, Uint32 ret);
  void rebuildIndex_fromEndTrans(Signal*, Uint32 tx_key, Uint32 ret);

  // Events
  void 
  createEventUTIL_PREPARE(Signal* signal,
			  Uint32 callbackData,
			  Uint32 returnCode);
  void 
  createEventUTIL_EXECUTE(Signal *signal, 
			  Uint32 callbackData,
			  Uint32 returnCode);
  void 
  dropEventUTIL_PREPARE_READ(Signal* signal,
			     Uint32 callbackData,
			     Uint32 returnCode);
  void 
  dropEventUTIL_EXECUTE_READ(Signal* signal,
			     Uint32 callbackData,
			     Uint32 returnCode);
  void
  dropEventUTIL_PREPARE_DELETE(Signal* signal,
			       Uint32 callbackData,
			       Uint32 returnCode);
  void 
  dropEventUTIL_EXECUTE_DELETE(Signal *signal, 
			       Uint32 callbackData,
			       Uint32 returnCode);
  void
  dropEventUtilPrepareRef(Signal* signal,
			  Uint32 callbackData,
			  Uint32 returnCode);
  void
  dropEventUtilExecuteRef(Signal* signal,
			  Uint32 callbackData,
			  Uint32 returnCode);
  int
  sendSignalUtilReq(Callback *c,
		    BlockReference ref, 
		    GlobalSignalNumber gsn, 
		    Signal* signal, 
		    Uint32 length, 
		    JobBufferLevel jbuf,
		    LinearSectionPtr ptr[3],
		    Uint32 noOfSections);
  int
  recvSignalUtilReq(Signal* signal, Uint32 returnCode);

  void completeSubStartReq(Signal* signal, Uint32 ptrI,	Uint32 returnCode);
  void completeSubStopReq(Signal* signal, Uint32 ptrI, Uint32 returnCode);
  void completeSubRemoveReq(Signal* signal, Uint32 ptrI, Uint32 returnCode);
  
  void dropEvent_sendReply(Signal* signal,
			   OpDropEventPtr evntRecPtr);

  void createEvent_RT_USER_CREATE(Signal* signal,
				  OpCreateEventPtr evntRecPtr,
				  SectionHandle& handle);
  void createEventComplete_RT_USER_CREATE(Signal* signal,
					  OpCreateEventPtr evntRecPtr);
  void createEvent_RT_USER_GET(Signal*, OpCreateEventPtr, SectionHandle&);
  void createEventComplete_RT_USER_GET(Signal* signal, OpCreateEventPtr evntRecPtr);

  void createEvent_RT_DICT_AFTER_GET(Signal* signal, OpCreateEventPtr evntRecPtr);

  void createEvent_nodeFailCallback(Signal* signal, Uint32 eventRecPtrI,
				    Uint32 returnCode);
  void createEvent_sendReply(Signal* signal, OpCreateEventPtr evntRecPtr,
			     LinearSectionPtr *ptr = NULL, int noLSP = 0);

  void prepareTransactionEventSysTable (Callback *c,
					Signal* signal,
					Uint32 senderData,
					UtilPrepareReq::OperationTypeValue prepReq);
  void prepareUtilTransaction(Callback *c,
			      Signal* signal,
			      Uint32 senderData,
			      Uint32 tableId,
			      const char *tableName,
			      UtilPrepareReq::OperationTypeValue prepReq,
			      Uint32 noAttr,
			      Uint32 attrIds[],
			      const char *attrNames[]);

  void executeTransEventSysTable(Callback *c,
				 Signal *signal,
				 const Uint32 ptrI,
				 sysTab_NDBEVENTS_0& m_eventRec,
				 const Uint32 prepareId,
				 UtilPrepareReq::OperationTypeValue prepReq);
  void executeTransaction(Callback *c,
			  Signal* signal, 
			  Uint32 senderData,
			  Uint32 prepareId,
			  Uint32 noAttr,
			  LinearSectionPtr headerPtr,
			  LinearSectionPtr dataPtr);

  void parseReadEventSys(Signal *signal, sysTab_NDBEVENTS_0& m_eventRec);
  bool upgrade_suma_NotStarted(Uint32 err, Uint32 ref) const;

  // support
  void getTableKeyList(TableRecordPtr, 
		       Id_array<MAX_ATTRIBUTES_IN_INDEX+1>& list);
  void getIndexAttr(TableRecordPtr indexPtr, Uint32 itAttr, Uint32* id);
  void getIndexAttrList(TableRecordPtr indexPtr, IndexAttributeList& list);
  void getIndexAttrMask(TableRecordPtr indexPtr, AttributeMask& mask);

  /* ------------------------------------------------------------ */
  // Initialisation
  /* ------------------------------------------------------------ */
  void initCommonData();
  void initRecords();
  void initConnectRecord();
  void initRetrieveRecord(Signal*, Uint32, Uint32 returnCode);
  void initSchemaRecord();
  void initRestartRecord(Uint32 sp = 0, Uint32 lp = 0,
                         const char * sb = 0, const char * eb = 0);
  void initSendSchemaRecord();
  void initReadTableRecord();
  void initWriteTableRecord();
  void initReadSchemaRecord();
  void initWriteSchemaRecord();

  void initNodeRecords();
  void initTableRecords();
  void initialiseTableRecord(TableRecordPtr tablePtr);
  void initTriggerRecords();
  void initialiseTriggerRecord(TriggerRecordPtr triggerPtr);
  void initPageRecords();

  Uint32 getFsConnRecord();

  bool getIsFailed(Uint32 nodeId) const;

  void printTables(); // For debugging only

  void startRestoreSchema(Signal*, Callback);
  void restartNextPass(Signal*);
  void restart_fromBeginTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void restart_fromEndTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void restartEndPass_fromEndTrans(Signal*, Uint32 tx_key, Uint32 ret);
  void restart_fromWriteSchemaFile(Signal*, Uint32, Uint32);
  void restart_nextOp(Signal*, bool commit = false);

  void checkSchemaStatus(Signal* signal);
  void checkPendingSchemaTrans(XSchemaFile* xsf);

  void restartCreateObj(Signal*, Uint32, const SchemaFile::TableEntry *, bool);
  void restartCreateObj_readConf(Signal*, Uint32, Uint32);
  void restartCreateObj_getTabInfoConf(Signal*);
  void restartCreateObj_parse(Signal*, SegmentedSectionPtr, bool);
  void restartDropObj(Signal*, Uint32, const SchemaFile::TableEntry *);

  void restart_checkSchemaStatusComplete(Signal*, Uint32 callback, Uint32);
  void masterRestart_checkSchemaStatusComplete(Signal*, Uint32, Uint32);

  void sendSchemaComplete(Signal*, Uint32 callbackData, Uint32);

public:
  void send_drop_file(Signal*, Uint32, Uint32, DropFileImplReq::RequestInfo);
  void send_drop_fg(Signal*, Uint32, Uint32, DropFilegroupImplReq::RequestInfo);
  
  int checkSingleUserMode(Uint32 senderRef);

  friend NdbOut& operator<<(NdbOut& out, const ErrorInfo&);
#ifdef VM_TRACE
  friend NdbOut& operator<<(NdbOut& out, const DictObject&);
  friend NdbOut& operator<<(NdbOut& out, const SchemaOp&);
  friend NdbOut& operator<<(NdbOut& out, const SchemaTrans&);
  friend NdbOut& operator<<(NdbOut& out, const TxHandle&);
  void check_consistency();
  void check_consistency_entry(TableRecordPtr tablePtr);
  void check_consistency_table(TableRecordPtr tablePtr);
  void check_consistency_index(TableRecordPtr indexPtr);
  void check_consistency_trigger(TriggerRecordPtr triggerPtr);
  void check_consistency_object(DictObjectPtr obj_ptr);
#endif

  /**
   * Dict lock queue does currently uniformly handle
   *
   * - starting node
   * - schema op
   *
   * The impl. is based on DbUtil lock's (LockQueue)
   *
   * It would be very nice to use this *fully*
   * But instead of introducing extra break in schema-op
   *   a lock queue in instantiated in Dict, for easy trylock-handling
   */
  struct DictLockType;
  friend struct DictLockType;
  
  struct DictLockType {
    DictLockReq::LockType lockType;
    const char* text;
  };
  static const DictLockType* getDictLockType(Uint32 lockType);
  void sendDictLockInfoEvent(Signal*, const UtilLockReq*, const char* text);
  void removeStaleDictLocks(Signal* signal, const Uint32* theFailedNodes);


  Uint32 dict_lock_trylock(const DictLockReq* req);
  Uint32 dict_lock_unlock(Signal* signal, const DictLockReq* req);
  
  LockQueue::Pool m_dict_lock_pool;
  LockQueue m_dict_lock;

  void sendOLD_LIST_TABLES_CONF(Signal *signal, ListTablesReq*);
  void sendLIST_TABLES_CONF(Signal *signal, ListTablesReq*);

  Uint32 c_outstanding_sub_startstop;
  NdbNodeBitmask c_sub_startstop_lock;

  Uint32 get_default_fragments(Signal*, Uint32 extra_nodegroups = 0);
  void wait_gcp(Signal* signal, SchemaOpPtr op_ptr, Uint32 flags);

  void block_substartstop(Signal* signal, SchemaOpPtr op_ptr);
  void unblock_substartstop();
  void wait_substartstop(Signal* signal, Uint32 opPtrI);

  void upgrade_seizeTrigger(Ptr<TableRecord> tabPtr, Uint32, Uint32, Uint32);

  void send_event(Signal*, SchemaTransPtr&,
                  Uint32 ev,
                  Uint32 id,
                  Uint32 version,
                  Uint32 type);

protected:
  virtual bool getParam(const char * param, Uint32 * retVal);
};

inline bool
Dbdict::TableRecord::isTable() const
{
  return DictTabInfo::isTable(tableType);
}

inline bool
Dbdict::TableRecord::isIndex() const
{
  return DictTabInfo::isIndex(tableType);
}

inline bool
Dbdict::TableRecord::isUniqueIndex() const
{
  return DictTabInfo::isUniqueIndex(tableType);
}

inline bool
Dbdict::TableRecord::isNonUniqueIndex() const
{
  return DictTabInfo::isNonUniqueIndex(tableType);
}

inline bool
Dbdict::TableRecord::isHashIndex() const
{
  return DictTabInfo::isHashIndex(tableType);
}

inline bool
Dbdict::TableRecord::isOrderedIndex() const
{
  return DictTabInfo::isOrderedIndex(tableType);
}

// quilt keeper
#endif
