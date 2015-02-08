/*
   Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.

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

#include "Ndbinfo.hpp"


#define DECLARE_NDBINFO_TABLE(var, num)  \
static const struct  {                   \
  Ndbinfo::Table::Members m;             \
  Ndbinfo::Column col[num];              \
} ndbinfo_##var 


DECLARE_NDBINFO_TABLE(TABLES,3) =
{ { "tables", 3, 0, "metadata for tables available through ndbinfo" },
  {
    {"table_id",  Ndbinfo::Number, ""},

    {"table_name",Ndbinfo::String, ""},
    {"comment",   Ndbinfo::String, ""}
  }
};

DECLARE_NDBINFO_TABLE(COLUMNS,5) =
{ { "columns", 5, 0, "metadata for columns available through ndbinfo " },
  {
    {"table_id",    Ndbinfo::Number, ""},
    {"column_id",   Ndbinfo::Number, ""},

    {"column_name", Ndbinfo::String, ""},
    {"column_type", Ndbinfo::Number, ""},
    {"comment",     Ndbinfo::String, ""}
  }
};

DECLARE_NDBINFO_TABLE(TEST,5) =
{ { "test", 5, 0, "for testing" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"block_number",       Ndbinfo::Number, ""},
    {"block_instance",     Ndbinfo::Number, ""},

    {"counter",            Ndbinfo::Number, ""},
    {"counter2",           Ndbinfo::Number64, ""}
  }
};

DECLARE_NDBINFO_TABLE(POOLS,12) =
{ { "pools", 12, 0, "pool usage" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"block_number",       Ndbinfo::Number, ""},
    {"block_instance",     Ndbinfo::Number, ""},
    {"pool_name",          Ndbinfo::String, ""},

    {"used",               Ndbinfo::Number64, "currently in use"},
    {"total",              Ndbinfo::Number64, "total allocated"},
    {"high",               Ndbinfo::Number64, "in use high water mark"},
    {"entry_size",         Ndbinfo::Number64, "size in bytes of each object"},
    {"config_param1",      Ndbinfo::Number, "config param 1 affecting pool"},
    {"config_param2",      Ndbinfo::Number, "config param 2 affecting pool"},
    {"config_param3",      Ndbinfo::Number, "config param 3 affecting pool"},
    {"config_param4",      Ndbinfo::Number, "config param 4 affecting pool"}
  }
};

DECLARE_NDBINFO_TABLE(TRANSPORTERS, 3) =
{ { "transporters", 3, 0, "transporter status" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"remote_node_id",     Ndbinfo::Number, ""},

    {"connection_status",  Ndbinfo::Number, ""}
  }
};

DECLARE_NDBINFO_TABLE(LOGSPACES, 7) =
{ { "logspaces", 7, 0, "logspace usage" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"log_type",           Ndbinfo::Number, "0 = REDO, 1 = DD-UNDO"},
    {"log_id",             Ndbinfo::Number, ""},
    {"log_part",           Ndbinfo::Number, ""},

    {"total",              Ndbinfo::Number64, "total allocated"},
    {"used",               Ndbinfo::Number64, "currently in use"},
    {"high",               Ndbinfo::Number64, "in use high water mark"}
  }
};

DECLARE_NDBINFO_TABLE(LOGBUFFERS, 7) =
{ { "logbuffers", 7, 0, "logbuffer usage" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"log_type",           Ndbinfo::Number, "0 = REDO, 1 = DD-UNDO"},
    {"log_id",             Ndbinfo::Number, ""},
    {"log_part",           Ndbinfo::Number, ""},

    {"total",              Ndbinfo::Number64, "total allocated"},
    {"used",               Ndbinfo::Number64, "currently in use"},
    {"high",               Ndbinfo::Number64, "in use high water mark"}
  }
};

DECLARE_NDBINFO_TABLE(RESOURCES,6) =
{ { "resources", 6, 0, "resources usage (a.k.a superpool)" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"resource_id",        Ndbinfo::Number, ""},

    {"reserved",           Ndbinfo::Number, "reserved for this resource"},
    {"used",               Ndbinfo::Number, "currently in use"},
    {"max",                Ndbinfo::Number, "max available"},
    {"high",               Ndbinfo::Number, "in use high water mark"}
  }
};

DECLARE_NDBINFO_TABLE(COUNTERS,5) =
{ { "counters", 5, 0, "monotonic counters" },
  {
    {"node_id",            Ndbinfo::Number, ""},
    {"block_number",       Ndbinfo::Number, ""},
    {"block_instance",     Ndbinfo::Number, ""},
    {"counter_id",         Ndbinfo::Number, ""},

    {"val",                Ndbinfo::Number64, "monotonically increasing since process start"}
  }
};

DECLARE_NDBINFO_TABLE(NODES,5) =
{ { "nodes", 5, 0, "node status" },
  {
    {"node_id",            Ndbinfo::Number, ""},

    {"uptime",             Ndbinfo::Number64, "time in seconds that node has been running"},
    {"status",             Ndbinfo::Number, "starting/started/stopped etc."},
    {"start_phase",        Ndbinfo::Number, "start phase if node is starting"},
    {"config_generation",  Ndbinfo::Number, "configuration generation number"}
  }
};

DECLARE_NDBINFO_TABLE(DISKPAGEBUFFER, 9) =
{ { "diskpagebuffer", 9, 0, "disk page buffer info" },
  {
    {"node_id",                     Ndbinfo::Number, ""},
    {"block_instance",              Ndbinfo::Number, ""},

    {"pages_written",               Ndbinfo::Number64, "Pages written to disk"},
    {"pages_written_lcp",           Ndbinfo::Number64, "Pages written by local checkpoint"},
    {"pages_read",                  Ndbinfo::Number64, "Pages read from disk"},
    {"log_waits",                   Ndbinfo::Number64, "Page writes waiting for log to be written to disk"},
    {"page_requests_direct_return", Ndbinfo::Number64, "Page in buffer and no requests waiting for it"},
    {"page_requests_wait_queue",    Ndbinfo::Number64, "Page in buffer, but some requests are already waiting for it"},
    {"page_requests_wait_io",       Ndbinfo::Number64, "Page not in buffer, waiting to be read from disk"},
  }
};

#define DBINFOTBL(x) { Ndbinfo::x##_TABLEID, (Ndbinfo::Table*)&ndbinfo_##x }

static
struct ndbinfo_table_list_entry {
  Ndbinfo::TableId id;
  Ndbinfo::Table * table;
} ndbinfo_tables[] = {
  // NOTE! the tables must be added to the list in the same order
  // as they are in "enum TableId"
  DBINFOTBL(TABLES),
  DBINFOTBL(COLUMNS),
  DBINFOTBL(TEST),
  DBINFOTBL(POOLS),
  DBINFOTBL(TRANSPORTERS),
  DBINFOTBL(LOGSPACES),
  DBINFOTBL(LOGBUFFERS),
  DBINFOTBL(RESOURCES),
  DBINFOTBL(COUNTERS),
  DBINFOTBL(NODES),
  DBINFOTBL(DISKPAGEBUFFER)
};

static int no_ndbinfo_tables =
  sizeof(ndbinfo_tables) / sizeof(ndbinfo_tables[0]);


int Ndbinfo::getNumTables(){
  return no_ndbinfo_tables;
}

const Ndbinfo::Table& Ndbinfo::getTable(int i)
{
  assert(i >= 0 && i < no_ndbinfo_tables);
  ndbinfo_table_list_entry& entry = ndbinfo_tables[i];
  assert(entry.id == i);
  return *entry.table;
}

const Ndbinfo::Table& Ndbinfo::getTable(Uint32 i)
{
  return getTable((int)i);
}
