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

#include <NDBT.hpp>
#include <NDBT_Table.hpp>
#include <NDBT_Tables.hpp>
#include <NdbEnv.h>

/* ******************************************************* */
//    Define Ndb standard tables 
//
//  USE ONLY UPPERLETTERS IN TAB AND COLUMN NAMES
/* ******************************************************* */

static const NdbDictionary::Column::StorageType MM=
    NdbDictionary::Column::StorageTypeMemory;
static const NdbDictionary::Column::StorageType DD=
    NdbDictionary::Column::StorageTypeDisk;

/*
 * These are our "official" test tables
 *
 */
/* T1 */
static
const
NDBT_Attribute T1Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned),
};
static
const
NDBT_Table T1("T1", sizeof(T1Attribs)/sizeof(NDBT_Attribute), T1Attribs);

static
const
NDBT_Attribute T2Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Varbinary, 100, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL6", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL7", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table T2("T2", sizeof(T2Attribs)/sizeof(NDBT_Attribute), T2Attribs);

static
const
NDBT_Attribute T3Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Varbinary, 100, true), 
  NDBT_Attribute("KOL00", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL01", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL02", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL03", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL04", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL05", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL06", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL07", NdbDictionary::Column::Varbinary, 25),
  NDBT_Attribute("KOL08", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL09", NdbDictionary::Column::Varbinary, 25),
  NDBT_Attribute("KOL10", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL11", NdbDictionary::Column::Varbinary, 25),
  NDBT_Attribute("KOL12", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL13", NdbDictionary::Column::Varbinary, 25),
  NDBT_Attribute("KOL14", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL15", NdbDictionary::Column::Longvarbinary, 537),
  NDBT_Attribute("KOL16", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL17", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL18", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL19", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL20", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL21", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL22", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL23", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL24", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL25", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL26", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL27", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL28", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL29", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL30", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL31", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL32", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL33", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL34", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL35", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL36", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL37", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL38", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL39", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL40", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL41", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL42", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL43", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL44", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL45", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL46", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL47", NdbDictionary::Column::Varbinary, 100),
  NDBT_Attribute("KOL48", NdbDictionary::Column::Char, 100),
  NDBT_Attribute("KOL49", NdbDictionary::Column::Varbinary, 100),

  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table T3("T3", sizeof(T3Attribs)/sizeof(NDBT_Attribute), T3Attribs);

/* T4 */
static
const
NDBT_Attribute T4Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true,  false, 0, MM, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned, 1, false, false, 0, MM, true),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned, 1, false, false, 0, MM, true),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned, 1, false, true, 0, MM, true),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned, 1, false, false, 0, MM, true)
};

static
const
NDBT_Table T4("T4", sizeof(T4Attribs)/sizeof(NDBT_Attribute), T4Attribs);



/* T6 */
static
const
NDBT_Attribute T6Attribs[] = {
  NDBT_Attribute("PK1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("ATTR1", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR2", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR4", NdbDictionary::Column::Char, 
		 47, false, true),// Nullable
  NDBT_Attribute("ATTR5", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR6", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR7", NdbDictionary::Column::Char, 
		 48, false, true),// Nullable
  NDBT_Attribute("ATTR8", NdbDictionary::Column::Char, 
		 50, false, true), // Nullable
  NDBT_Attribute("ATTR9", NdbDictionary::Column::Int),
  NDBT_Attribute("ATTR10", NdbDictionary::Column::Float),
  NDBT_Attribute("ATTR11", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR12", NdbDictionary::Column::Char, 49),
  NDBT_Attribute("ATTR13", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR14", NdbDictionary::Column::Varbinary, 50),
  NDBT_Attribute("ATTR15", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR16", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR17", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR18", NdbDictionary::Column::Char, 257),
  NDBT_Attribute("ATTR19", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR20", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ATTR21", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("ATTR22", NdbDictionary::Column::Varbinary, 255, false, false, 0, MM, true)
};

static
const
NDBT_Table T6("T6", sizeof(T6Attribs)/sizeof(NDBT_Attribute), T6Attribs);

/* T13 - Long key table */
static
const
NDBT_Attribute T13Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Char, 257, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Char, 259, true),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Char, 113, true),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned, 1, true),
  NDBT_Attribute("KOL6", NdbDictionary::Column::Unsigned),
};
static
const
NDBT_Table T13("T13", sizeof(T13Attribs)/sizeof(NDBT_Attribute), T13Attribs);


/* T14 - 5 primary keys */
static
const
NDBT_Attribute T14Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL3", NdbDictionary::Column::Char, 4, true), 
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL20", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL30", NdbDictionary::Column::Int),
  NDBT_Attribute("KOL40", NdbDictionary::Column::Float),
  NDBT_Attribute("KOL50", NdbDictionary::Column::Char, 200, false, false, 0, MM, true)
};


static
const
NDBT_Table T14("T14", sizeof(T14Attribs)/sizeof(NDBT_Attribute), T14Attribs);

/*
  T15 - Dynamic attributes.
  Test many different combinations of attribute types, sizes, and NULLability.
  Also exersize >32bit dynattr bitmap.
*/
static
const
NDBT_Attribute T15Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true, false, 0, MM, true),
  NDBT_Attribute("KOL2", NdbDictionary::Column::Varbinary, 100, false, true, 0, MM, true),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned, 1, false, true, 0, MM, true),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Int, 1, false, false, 0, MM, true),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Float, 1, false, true, 0, MM, true),
  NDBT_Attribute("KOL6", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL7", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL8", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL9", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL10", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL11", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL12", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL13", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL14", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL15", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL16", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL17", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL18", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL19", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL20", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL21", NdbDictionary::Column::Varbinary, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL22", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL23", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL24", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL25", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL26", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL27", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL28", NdbDictionary::Column::Char, 4, false, false),
  NDBT_Attribute("KOL29", NdbDictionary::Column::Varbinary, 4, false, false),
  NDBT_Attribute("KOL30", NdbDictionary::Column::Char, 4, false, true, 0, DD),
  NDBT_Attribute("KOL31", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL32", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("BIT1", NdbDictionary::Column::Bit, 27, false, true, 0, MM, true),
  NDBT_Attribute("BIT2", NdbDictionary::Column::Bit, 1, false, false, 0, MM, true),
  NDBT_Attribute("BIT3", NdbDictionary::Column::Bit, 1, false, true, 0, MM, true),
  NDBT_Attribute("BIT4", NdbDictionary::Column::Bit, 8, false, false, 0, MM, true),
  NDBT_Attribute("KOL33", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL34", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL35", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL36", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL37", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL38", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL39", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL40", NdbDictionary::Column::Varbinary, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL41", NdbDictionary::Column::Char, 64, false, true, 0, MM, true),
  NDBT_Attribute("KOL42", NdbDictionary::Column::Char, 4, false, true, 0, MM, true),
  NDBT_Attribute("KOL43", NdbDictionary::Column::Char, 8, false, true, 0, MM, true),
  NDBT_Attribute("KOL44", NdbDictionary::Column::Char, 27, false, true, 0, MM, true),
  NDBT_Attribute("KOL45", NdbDictionary::Column::Char, 64, false, false, 0, MM, true),
  NDBT_Attribute("KOL46", NdbDictionary::Column::Char, 4, false, false, 0, MM, true),
  NDBT_Attribute("KOL47", NdbDictionary::Column::Char, 8, false, false, 0, MM, true),
  NDBT_Attribute("KOL48", NdbDictionary::Column::Char, 27, false, false, 0, MM, true),
  NDBT_Attribute("KOL49", NdbDictionary::Column::Varbinary, 255, false, false, 0, MM, true),
  /* This one is for update count, needed by hugoScanUpdate. */
  NDBT_Attribute("KOL99", NdbDictionary::Column::Unsigned, 1, false, false, 0, MM, true),
};

static
const
NDBT_Table T15("T15", sizeof(T15Attribs)/sizeof(NDBT_Attribute), T15Attribs);

/* Test dynamic bit types when no other varsize/dynamic. */
static
const
NDBT_Attribute T16Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true, false),
  NDBT_Attribute("Kol2", NdbDictionary::Column::Bit, 27, false, true, 0, MM, true),
  NDBT_Attribute("KOL99", NdbDictionary::Column::Unsigned, 1, false, false),
};

static
const
NDBT_Table T16("T16", sizeof(T16Attribs)/sizeof(NDBT_Attribute), T16Attribs);

/*
  C2 DHCP TABLES, MAYBE THESE SHOULD BE MOVED TO THE UTIL_TABLES?
*/
static 
const
NDBT_Attribute I1_Cols[] = {
  NDBT_Attribute("ID", NdbDictionary::Column::Unsigned, true),
  NDBT_Attribute("PORT", NdbDictionary::Column::Char, 16, true),
  NDBT_Attribute("ACCESSNODE", NdbDictionary::Column::Char, 16, true),
  NDBT_Attribute("POP", NdbDictionary::Column::Char, 64, true),
  NDBT_Attribute("VLAN", NdbDictionary::Column::Char, 16),
  NDBT_Attribute("COMMENT", NdbDictionary::Column::Char, 128),
  NDBT_Attribute("SNMPINDEX", NdbDictionary::Column::Int),
  NDBT_Attribute("PORTSTATE", NdbDictionary::Column::Int),
  NDBT_Attribute("UPDATES", NdbDictionary::Column::Unsigned)
};

static
const
char* I1_Indexes[] = {
  "UNIQUE", "ID", "PORT", "ACCESSNODE", "POP", "PORTSTATE", 0,
  0
};

static
NDBT_Table I1("I1", sizeof(I1_Cols)/sizeof(NDBT_Attribute), I1_Cols
	      );// ,I1_Indexes);

static 
const
NDBT_Attribute I2_Cols[] = {
  NDBT_Attribute("ID", NdbDictionary::Column::Unsigned, true),
  NDBT_Attribute("PORT", NdbDictionary::Column::Char, 16, true),
  NDBT_Attribute("ACCESSNODE", NdbDictionary::Column::Char, 16, true),
  NDBT_Attribute("POP", NdbDictionary::Column::Char, 64, true),
  NDBT_Attribute("ACCESSTYPE", NdbDictionary::Column::Int, true),
  NDBT_Attribute("CUSTOMER_ID", NdbDictionary::Column::Int),
  NDBT_Attribute("PROVIDER", NdbDictionary::Column::Int),
  NDBT_Attribute("TEXPIRE", NdbDictionary::Column::Int),
  NDBT_Attribute("NUM_IP", NdbDictionary::Column::Int),
  NDBT_Attribute("LEASED_NUM_IP", NdbDictionary::Column::Int),
  NDBT_Attribute("LOCKED_IP", NdbDictionary::Column::Int),
  NDBT_Attribute("STATIC_DNS", NdbDictionary::Column::Int),
  NDBT_Attribute("SUSPENDED_SERVICES", NdbDictionary::Column::Int),
  NDBT_Attribute("UPDATES", NdbDictionary::Column::Unsigned)
};

const
char* I2_Indexes[] = {
  "ORDERED", "CUSTOMER_ID", 0,
  "ORDERED", "NUM_IP", 0,
  0
};

static
NDBT_Table I2("I2", sizeof(I2_Cols)/sizeof(NDBT_Attribute), I2_Cols
	      );//, I2_Indexes);

static 
const
NDBT_Attribute I3_Cols[] = {
  NDBT_Attribute("ID", NdbDictionary::Column::Unsigned, true),
  NDBT_Attribute("PORT", NdbDictionary::Column::Char, 16), // SI2
  NDBT_Attribute("ACCESSNODE", NdbDictionary::Column::Char, 16), // SI2
  NDBT_Attribute("POP", NdbDictionary::Column::Char, 64), // SI2
  NDBT_Attribute("MAC", NdbDictionary::Column::Char, 12, true), 
  NDBT_Attribute("MAC_EXPIRE", NdbDictionary::Column::Int, 1),
  NDBT_Attribute("IIP", NdbDictionary::Column::Int), // SI1
  NDBT_Attribute("P_EXPIRE", NdbDictionary::Column::Int),
  NDBT_Attribute("HOSTNAME", NdbDictionary::Column::Char, 32),
  NDBT_Attribute("DETECTED", NdbDictionary::Column::Int),
  NDBT_Attribute("STATUS", NdbDictionary::Column::Int),
  NDBT_Attribute("NUM_REQUESTS", NdbDictionary::Column::Int),
  NDBT_Attribute("ACCESSTYPE", NdbDictionary::Column::Int),
  NDBT_Attribute("OS_TYPE", NdbDictionary::Column::Int),
  NDBT_Attribute("GW", NdbDictionary::Column::Int),
  NDBT_Attribute("UPDATES", NdbDictionary::Column::Unsigned)
};

const
char* I3_Indexes[] = {
  "UNIQUE", "ID", 0,
  "ORDERED", "MAC", 0,
  "ORDERED", "GW", 0,
  0
};

static
NDBT_Table I3("I3", sizeof(I3_Cols)/sizeof(NDBT_Attribute), I3_Cols
	      ); // ,I3_Indexes);

/* D1 */
static
const
NDBT_Attribute D1Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned, 1, false, false, 0, NdbDictionary::Column::StorageTypeDisk),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Char, 233, false, true, 0, NdbDictionary::Column::StorageTypeDisk),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL6", NdbDictionary::Column::Varbinary, 233, false, true, 0, NdbDictionary::Column::StorageTypeDisk),
};
static
const
NDBT_Table D1("D1", sizeof(D1Attribs)/sizeof(NDBT_Attribute), D1Attribs);

static
const char* BigVarDefault =
  "\x80\x1"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2"
  "KOL7 default in table D2";

static unsigned smallUintDefault = 77;

static
const
NDBT_Attribute D2Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Varbinary, 127, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned, 1, false, false, 0, NdbDictionary::Column::StorageTypeDisk, false,
                 &smallUintDefault, sizeof(unsigned)),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Varbinary, 133, false, true, 0, MM, true, 
                 "\x1E" "A default value for KOL4 in D2", 31),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Char, 199, false, true, 0, NdbDictionary::Column::StorageTypeDisk),
  NDBT_Attribute("KOL6", NdbDictionary::Column::Bit, 21, false, false, 0, NdbDictionary::Column::StorageTypeDisk),
  NDBT_Attribute("KOL7", NdbDictionary::Column::Longvarbinary, 384, false, true, 0, NdbDictionary::Column::StorageTypeDisk, false, 
                 BigVarDefault, 386),
  NDBT_Attribute("KOL8", NdbDictionary::Column::Varbinary, 88, false, true, 0, NdbDictionary::Column::StorageTypeDisk, false, 
                 "\x1E""A default value for KOL8 in D2", 31)

};
static
const
NDBT_Table D2("D2", sizeof(D2Attribs)/sizeof(NDBT_Attribute), D2Attribs);


// Define array with pointer to all tables 
static
const
NDBT_Table *test_tables[]=
{ 
  &T1,
  &T2,
  &T3,
  &T4,
  &T6,
  &T13,
  &T14,
  &T15,
  &T16,
  &I1,
  &I2,
  &I3,
  &D1, &D2
};

static
const
int numTestTables = sizeof(test_tables)/sizeof(NDBT_Table*);


/**
 * Define tables we should not be able to create
 */ 

/* F1 
 *
 * Error: PK and column with same name
 */
static
const
NDBT_Attribute F1Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table F1("F1", sizeof(F1Attribs)/sizeof(NDBT_Attribute), F1Attribs);

/* F2
 *
 * Error: Two columns with same name
 */
static
const
NDBT_Attribute F2Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table F2("F2", sizeof(F2Attribs)/sizeof(NDBT_Attribute), F2Attribs);

/* F3
 *
 * Error: Too many primary keys defined, 32 is max
 */
static
const
NDBT_Attribute F3Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL6", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL7", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL8", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL9", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL10", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL11", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL12", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL13", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL14", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL15", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL16", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL17", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL18", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL19", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL20", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL21", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL22", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL23", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL24", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL25", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL26", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL27", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL28", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL29", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL30", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL31", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL32", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL33", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("KOL40", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL50", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL60", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL70", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table F3("F3", sizeof(F3Attribs)/sizeof(NDBT_Attribute), F3Attribs);

/* F4
 *
 * Error: Too long key
 */
static
const
NDBT_Attribute F4Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL2", NdbDictionary::Column::Unsigned, 9999999, true), 
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table F4("F4", sizeof(F4Attribs)/sizeof(NDBT_Attribute), F4Attribs);

/* F5
 *
 * Error: Too long attr name
 */
static
const
NDBT_Attribute F5Attribs[] = {
  NDBT_Attribute("KOL1WITHVERRYLONGNAME_ISITTOLONG", NdbDictionary::Column::Unsigned, true),
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned)
};

static
const
NDBT_Table F5("F5", sizeof(F5Attribs)/sizeof(NDBT_Attribute), F5Attribs);

/* F6
 *
 * Error: Zero length of pk attribute
 */
static
const
NDBT_Attribute F6Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Char, 0, true, false), 
  NDBT_Attribute("KOL2", NdbDictionary::Column::Char, 256),
};

static
const
NDBT_Table F6("F6", sizeof(F6Attribs)/sizeof(NDBT_Attribute), F6Attribs);

/* F7
 *
 * Error: Table without primary key
 */
static
const
NDBT_Attribute F7Attribs[] = {
  NDBT_Attribute("KOL3", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Unsigned)
};

NDBT_Table F7("F7", sizeof(F7Attribs)/sizeof(NDBT_Attribute), F7Attribs);

/* F8
 *
 * Error: Table without nullable primary key
 */
static
const
NDBT_Attribute F8Attribs[] = {
  NDBT_Attribute("KOL3", NdbDictionary::Column::Int, 1, true, true),
  NDBT_Attribute("KOL4", NdbDictionary::Column::Int),
  NDBT_Attribute("KOL5", NdbDictionary::Column::Int)
};

NDBT_Table F8("F8", sizeof(F8Attribs)/sizeof(NDBT_Attribute), F8Attribs);


/* F15 - 2-node crash in v20x */
static
const
NDBT_Attribute F15Attribs[] = {
  NDBT_Attribute("KOL1", NdbDictionary::Column::Char, 40, true)
};
static
const
NDBT_Table F15("F15", sizeof(F15Attribs)/sizeof(NDBT_Attribute), F15Attribs);

// Define array with pointer to tables that we should not  be able to create
static
const
NDBT_Table *fail_tables[]=
{ 
  &F1,
  &F2,
  &F3,
  &F4,
  &F5,
  &F6,
  &F7,
  &F8,
  &F15
};

static
const
int numFailTables = sizeof(fail_tables)/sizeof(NDBT_Table*);


/**
 * Define util tables that we may create
 */ 


/* GL 
 * General ledger table for bank application
 */
static 
const
NDBT_Attribute GL_Attribs[] = {
  NDBT_Attribute("TIME", NdbDictionary::Column::Bigunsigned, 1, true), 
  NDBT_Attribute("ACCOUNT_TYPE", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("BALANCE", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("DEPOSIT_COUNT", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("DEPOSIT_SUM", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("WITHDRAWAL_COUNT", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("WITHDRAWAL_SUM", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("PURGED", NdbDictionary::Column::Unsigned)
};

static
NDBT_Table GL("GL", sizeof(GL_Attribs)/sizeof(NDBT_Attribute), GL_Attribs);

/* ACCOUNT
 * Account table for bank application
 */
static 
const
NDBT_Attribute ACCOUNT_Attribs[] = {
  NDBT_Attribute("ACCOUNT_ID", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("OWNER", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("BALANCE", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("ACCOUNT_TYPE", NdbDictionary::Column::Unsigned),
};

static
NDBT_Table ACCOUNT("ACCOUNT", sizeof(ACCOUNT_Attribs)/sizeof(NDBT_Attribute), ACCOUNT_Attribs);

/* TRANSACTION
 * Transaction table for bank application
 */
static 
const
NDBT_Attribute TRANSACTION_Attribs[] = {
  NDBT_Attribute("TRANSACTION_ID", NdbDictionary::Column::Bigunsigned, 1, true),
  NDBT_Attribute("ACCOUNT", NdbDictionary::Column::Unsigned, 1, true), 
  NDBT_Attribute("ACCOUNT_TYPE", NdbDictionary::Column::Unsigned), 
  NDBT_Attribute("OTHER_ACCOUNT", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("TRANSACTION_TYPE", NdbDictionary::Column::Unsigned),
  NDBT_Attribute("TIME", NdbDictionary::Column::Bigunsigned),
  NDBT_Attribute("AMOUNT", NdbDictionary::Column::Unsigned),
};

static
NDBT_Table TRANSACTION("TRANSACTION", sizeof(TRANSACTION_Attribs)/sizeof(NDBT_Attribute), TRANSACTION_Attribs);

/* SYSTEM_VALUES
 * System values table for bank application
 */
static 
const
NDBT_Attribute SYSTEM_VALUES_Attribs[] = {
  NDBT_Attribute("SYSTEM_VALUES_ID", NdbDictionary::Column::Unsigned, 1, true),
  NDBT_Attribute("VALUE", NdbDictionary::Column::Bigunsigned)
};

static
NDBT_Table SYSTEM_VALUES("SYSTEM_VALUES", sizeof(SYSTEM_VALUES_Attribs)/sizeof(NDBT_Attribute), SYSTEM_VALUES_Attribs);

/* ACCOUNT_TYPES
 * Account types table for bank application
 */
static 
const
NDBT_Attribute ACCOUNT_TYPES_Attribs[] = {
  NDBT_Attribute("ACCOUNT_TYPE_ID", NdbDictionary::Column::Unsigned, 1, true),
  NDBT_Attribute("DESCRIPTION", NdbDictionary::Column::Char, 64, false, false, &my_charset_latin1_bin)
};

static
NDBT_Table ACCOUNT_TYPES("ACCOUNT_TYPE", sizeof(ACCOUNT_TYPES_Attribs)/sizeof(NDBT_Attribute), ACCOUNT_TYPES_Attribs);


// Define array with pointer to util tables 
static
const
NDBT_Table *util_tables[]=
{ 
  &GL,
  &ACCOUNT,
  &TRANSACTION,
  &SYSTEM_VALUES,
  &ACCOUNT_TYPES
};

static
const
int numUtilTables = sizeof(util_tables)/sizeof(NDBT_Table*);


/**
 * Define other test tables that we may create
 */

/* WIDE_2COL
 * Single var length key going up to max size of key
 * Single var length attribute using remaining row space
 */
static 
const
NDBT_Attribute WIDE_2COL_ATTRIBS[] = {
  /* Note that we can't have any index on this table as it
   * has no space for the extra FRAGID the index requires!
   */
  NDBT_Attribute("KEY", NdbDictionary::Column::Longvarbinary, 
                 NDBT_Tables::MaxVarTypeKeyBytes, true), 
  NDBT_Attribute("ATTR", NdbDictionary::Column::Longvarbinary, 
                 NDBT_Tables::MaxKeyMaxVarTypeAttrBytes, false)
};


static
NDBT_Table WIDE_2COL("WIDE_2COL", sizeof(WIDE_2COL_ATTRIBS)/
                     sizeof(NDBT_Attribute), WIDE_2COL_ATTRIBS);

/* WIDE_2COL_IX
 * Single var length key going up to max size of key
 * Single var length attribute using most of remaining row space,
 * but not all as we need space in the index for FragId
 */
static 
const
NDBT_Attribute WIDE_2COL_IX_ATTRIBS[] = {
  NDBT_Attribute("KEY", NdbDictionary::Column::Longvarbinary,
                 NDBT_Tables::MaxVarTypeKeyBytes, true), 
  NDBT_Attribute("ATTR", NdbDictionary::Column::Longvarbinary,
                 NDBT_Tables::MaxKeyMaxVarTypeAttrBytesIndex , false)
};

static
NDBT_Table WIDE_2COL_IX("WIDE_2COL_IX", sizeof(WIDE_2COL_IX_ATTRIBS)/
                        sizeof(NDBT_Attribute), WIDE_2COL_IX_ATTRIBS);

static
const char* WIDE_2COL_IX_Indexes[] = {
  "UNIQUE", "ATTR", 0,
  0};

/* WIDE_MAXKEY_HUGO
 * Single var length key going up to max size of key
 * Var length attr going using up remaining space
 * Two unsigned int columns required by Hugo tools
 */
static 
const
NDBT_Attribute WIDE_MAXKEY_HUGO_ATTRIBS[] = {
  /* Note that we can't have any index on this table as it
   * has no space for the extra FRAGID the index requires!
   */
  NDBT_Attribute("KEY", NdbDictionary::Column::Longvarbinary,
                 NDBT_Tables::MaxVarTypeKeyBytes, true), 
  NDBT_Attribute("ATTR", NdbDictionary::Column::Longvarbinary,
                 NDBT_Tables::MaxKeyMaxVarTypeAttrBytes -
                 NDBT_Tables::HugoOverheadBytes, false),
  NDBT_Attribute("HUGOID", NdbDictionary::Column::Unsigned,
                 1, false),
  NDBT_Attribute("HUGOUPDATE", NdbDictionary::Column::Unsigned,
                 1, false)
};

static
NDBT_Table WIDE_MAXKEY_HUGO("WIDE_MAXKEY_HUGO", sizeof(WIDE_MAXKEY_HUGO_ATTRIBS)/
                          sizeof(NDBT_Attribute), WIDE_MAXKEY_HUGO_ATTRIBS);

/* WIDE_MAXATTR_HUGO
 * Single unsigned int key
 * Var length attr using up remaining space
 * Two unsigned int columns required by Hugo tools
 */
static 
const
NDBT_Attribute WIDE_MAXATTR_HUGO_ATTRIBS[] = {
  NDBT_Attribute("KEY", NdbDictionary::Column::Unsigned,
                 1, true), 
  NDBT_Attribute("ATTR", NdbDictionary::Column::Longvarbinary,
                 NDBT_Tables::MinKeyMaxVarTypeAttrBytes -
                 NDBT_Tables::HugoOverheadBytes, false),
  NDBT_Attribute("HUGOID", NdbDictionary::Column::Unsigned,
                 1, false),
  NDBT_Attribute("HUGOUPDATE", NdbDictionary::Column::Unsigned,
                 1, false)
};

static
NDBT_Table WIDE_MAXATTR_HUGO("WIDE_MAXATTR_HUGO", sizeof(WIDE_MAXATTR_HUGO_ATTRIBS)/
                             sizeof(NDBT_Attribute), WIDE_MAXATTR_HUGO_ATTRIBS);

typedef NDBT_Table* (*TableGenerator)(const char* name);

static NDBT_Table* WIDE_MAXKEYMAXCOLS_HUGO= NULL;

static
NDBT_Table* createMaxKeyMaxColsHugoTabDef(const char* name)
{
  if (WIDE_MAXKEYMAXCOLS_HUGO == NULL)
  {
    /* Create a wide table with the max num of keys
     * and the max num of attrs
     */
    NdbDictionary::Column* attrs[NDB_MAX_ATTRIBUTES_IN_TABLE];
    const int buffsize=100;
    char namebuff[buffsize];
    Uint32 attrNum=0;

    /* Keys */
    for (;attrNum < (NDB_MAX_ATTRIBUTES_IN_INDEX - 1); attrNum ++)
    {
      BaseString::snprintf(namebuff, buffsize, "K%d", attrNum);
      attrs[attrNum]= new NDBT_Attribute(namebuff,
                                         NdbDictionary::Column::Unsigned,
                                         1, true);
    }
    /* Last key uses remaining key space */
    BaseString::snprintf(namebuff, buffsize, "K%d", attrNum);
    attrs[attrNum]= new NDBT_Attribute(namebuff,
                                       NdbDictionary::Column::Char,
                                       (NDB_MAX_KEYSIZE_IN_WORDS -
                                        (NDB_MAX_ATTRIBUTES_IN_INDEX -1)) * 4,
                                       true);

    attrNum ++;

    /* Attributes */
    for (;attrNum < (NDB_MAX_ATTRIBUTES_IN_TABLE - 1); attrNum ++)
    {
      BaseString::snprintf(namebuff, buffsize, "A%d", attrNum);
      attrs[attrNum]= new NDBT_Attribute(namebuff,
                                         NdbDictionary::Column::Unsigned,
                                         1, false);
    }

    /* Last attr uses remaining attr space */
    BaseString::snprintf(namebuff, buffsize, "A%d", attrNum);
    Uint32 sz32 = NDB_MAX_TUPLE_SIZE_IN_WORDS;
    sz32 -= NDB_MAX_KEYSIZE_IN_WORDS;
    sz32 -= NDB_MAX_ATTRIBUTES_IN_INDEX;
    sz32 -= 2 * NDB_MAX_ATTRIBUTES_IN_TABLE;
    attrs[attrNum]= new NDBT_Attribute(namebuff,
                                       NdbDictionary::Column::Char, 4 * sz32,
                                       false);

    WIDE_MAXKEYMAXCOLS_HUGO= new NDBT_Table(name, NDB_MAX_ATTRIBUTES_IN_TABLE,
                                            attrs);

    /* Free attributes, table will remain until program exit */
    for (attrNum=0; attrNum < NDB_MAX_ATTRIBUTES_IN_TABLE; attrNum++)
      delete attrs[attrNum];
    
  }

  return WIDE_MAXKEYMAXCOLS_HUGO;
}

static NDBT_Table* WIDE_MINKEYMAXCOLS_HUGO= NULL;

static
NDBT_Table* createMinKeyMaxColsHugoTabDef(const char* name)
{
  if (WIDE_MINKEYMAXCOLS_HUGO == NULL)
  {
    /* Create a wide table with one key and the max number
     * of attributes
     */
    NdbDictionary::Column* attrs[NDB_MAX_ATTRIBUTES_IN_TABLE];
    const int buffsize=100;
    char namebuff[buffsize];
    Uint32 attrNum=0;
    attrs[attrNum]= new NDBT_Attribute("K1",
                                       NdbDictionary::Column::Unsigned,
                                       1, true);
    attrNum ++;

    /* Attributes */
    for (;attrNum < (NDB_MAX_ATTRIBUTES_IN_TABLE - 1); attrNum ++)
    {
      BaseString::snprintf(namebuff, buffsize, "A%d", attrNum);
      attrs[attrNum]= new NDBT_Attribute(namebuff,
                                         NdbDictionary::Column::Unsigned,
                                         1, false);
    }
    
    /* Last attr uses remaining attr space */
    BaseString::snprintf(namebuff, buffsize, "A%d", attrNum);
    attrs[attrNum]= new NDBT_Attribute(namebuff,
                                       NdbDictionary::Column::Char,
                                       (NDB_MAX_TUPLE_SIZE_IN_WORDS -
                                        (NDB_MAX_ATTRIBUTES_IN_TABLE - 1)) * 4,
                                       false);
    
    WIDE_MINKEYMAXCOLS_HUGO= new NDBT_Table(name, NDB_MAX_ATTRIBUTES_IN_TABLE,
                                            attrs);

    /* Free attributes, table will remain until program exit */
    for (attrNum=0; attrNum < NDB_MAX_ATTRIBUTES_IN_TABLE; attrNum++)
      delete attrs[attrNum];
    

  }

  return WIDE_MINKEYMAXCOLS_HUGO;
}

// Define array with pointer to other test tables

struct OtherTable
{
  const char* name;
  NDBT_Table* tab;
  TableGenerator tabGen;
};

static
const
OtherTable other_tables[]=
{ 
  {"WIDE_2COL", &WIDE_2COL, NULL},
  {"WIDE_2COL_IX", &WIDE_2COL_IX, NULL},
  {"WIDE_MAXKEY_HUGO", &WIDE_MAXKEY_HUGO, NULL},
  {"WIDE_MAXATTR_HUGO", &WIDE_MAXATTR_HUGO, NULL},
  {"WIDE_MAXKEYMAXCOLS_HUGO", NULL, createMaxKeyMaxColsHugoTabDef},
  {"WIDE_MINKEYMAXCOLS_HUGO", NULL, createMinKeyMaxColsHugoTabDef}
};

static
const
int numOtherTables = sizeof(other_tables)/sizeof(OtherTable);


/* Secondary indexes for our tables */
struct NDBT_IndexList {
  const char * m_table;
  const char ** m_indexes;
};

static
const
NDBT_IndexList indexes[] = {
  { "I1", I1_Indexes }, 
  { "I2", I2_Indexes }, 
  { "I3", I3_Indexes },
  { "WIDE_2COL_IX", WIDE_2COL_IX_Indexes },
  { 0, 0 }
};

const
NdbDictionary::Table*
NDBT_Tables::getTable(const char* _nam){
  // Search tables list to find a table
  NDBT_Table* tab = NULL;
  int i;
  for (i=0; i<numTestTables; i++){
    if (strcmp(test_tables[i]->getName(), _nam) == 0){
      return test_tables[i];
    }
  }
  for (i=0; i<numFailTables; i++){
    if (strcmp(fail_tables[i]->getName(), _nam) == 0){
      return fail_tables[i];
    }
  }
  for (i=0; i<numUtilTables; i++){
    if (strcmp(util_tables[i]->getName(), _nam) == 0){
      return util_tables[i];
    }
  }
  for(i=0; i<numOtherTables; i++){
    if (strcmp(other_tables[i].name, _nam) == 0){
      return (other_tables[i].tab != NULL)? 
        other_tables[i].tab :
        (*other_tables[i].tabGen)(other_tables[i].name);
    }
  }

  // TPK_no tables
  // Dynamcially create table vith primary key size
  // set to no
  // Useful for testing key sizes 1 - max
  int pkSizeOfTable;
  if(sscanf(_nam, "TPK_%d", &pkSizeOfTable) == 1){   
    return tableWithPkSize(_nam, pkSizeOfTable);
  }
  return tab;
}

const NdbDictionary::Table*
NDBT_Tables::tableWithPkSize(const char* _nam, Uint32 pkSize){
  NdbDictionary::Table* tab = new NdbDictionary::Table(_nam);

  // Add one PK of the desired length
  tab->addColumn(NDBT_Attribute("PK1",
				NdbDictionary::Column::Char,
				pkSize,
				true));
  
  // Add 4 attributes
  tab->addColumn(NDBT_Attribute("ATTR1",
				NdbDictionary::Column::Char,
				21));
  
  tab->addColumn(NDBT_Attribute("ATTR2",
				NdbDictionary::Column::Char,
				124));
  
  tab->addColumn(NDBT_Attribute("ATTR3",
				NdbDictionary::Column::Unsigned));
  
  tab->addColumn(NDBT_Attribute("ATTR4",
				NdbDictionary::Column::Unsigned));
  
  return tab;
}

const NdbDictionary::Table* 
NDBT_Tables::getTable(int _num){
  // Get table at pos _num
  assert(_num < numTestTables);
  return test_tables[_num];
}

int
NDBT_Tables::getNumTables(){
  return numTestTables;
}

const char**
NDBT_Tables::getIndexes(const char* table)
{
  Uint32 i = 0;
  for (i = 0; indexes[i].m_table != 0; i++) {
    if (strcmp(indexes[i].m_table, table) == 0)
      return indexes[i].m_indexes;
  }
  return 0;
}

int
NDBT_Tables::createAllTables(Ndb* pNdb, bool _temp, bool existsOk){
  
  for (int i=0; i < NDBT_Tables::getNumTables(); i++){
    pNdb->getDictionary()->dropTable(NDBT_Tables::getTable(i)->getName());
    int ret= createTable(pNdb, 
			 NDBT_Tables::getTable(i)->getName(), _temp, existsOk);
    if(ret){
      return ret;
    }
  }
  return NDBT_OK;
}

int
NDBT_Tables::createAllTables(Ndb* pNdb){
  return createAllTables(pNdb, false);
}

int
NDBT_Tables::create_default_tablespace(Ndb* pNdb)
{
  NdbDictionary::Dictionary* pDict = pNdb->getDictionary();

  int res;
  Uint32 mb = 8;
  {
    char buf[256];
    if (NdbEnv_GetEnv("UNDOBUFFER", buf, sizeof(buf)))
    {
      mb = atoi(buf);
      ndbout_c("Using %umb dd-undo-buffer", mb);
    }
  }

  NdbDictionary::LogfileGroup lg = pDict->getLogfileGroup("DEFAULT-LG");
  if (strcmp(lg.getName(), "DEFAULT-LG") != 0)
  {
    lg.setName("DEFAULT-LG");
    lg.setUndoBufferSize(mb*1024*1024);
    res = pDict->createLogfileGroup(lg);
    if(res != 0){
      g_err << "Failed to create logfilegroup:"
	    << endl << pDict->getNdbError() << endl;
      return NDBT_FAILED;
    }
  }

  mb = 96;
  Uint32 files = 13;

  {
    char buf[256];
    if (NdbEnv_GetEnv("UNDOSIZE", buf, sizeof(buf)))
    {
      mb = atoi(buf);
      ndbout_c("Using %umb dd-undo", mb);
    }
  }
  
  {
    char buf[256];
    if (NdbEnv_GetEnv("UNDOFILES", buf, sizeof(buf)))
    {
      files = atoi(buf);
      ndbout_c("Using max %u dd-undo files", files);
    }
  }
  
  Uint32 sz = 32;
  while (mb > files * sz)
    sz += 32;

  for (Uint32 i = 0; i * sz < mb; i++)
  {
    char tmp[256];
    BaseString::snprintf(tmp, sizeof(tmp), "undofile%u.dat", i);
    NdbDictionary::Undofile uf = pDict->getUndofile(0, tmp);
    if (strcmp(uf.getPath(), tmp) != 0)
    {
      uf.setPath(tmp);
      uf.setSize(Uint64(sz)*1024*1024);
      uf.setLogfileGroup("DEFAULT-LG");
      
      res = pDict->createUndofile(uf, true);
      if(res != 0){
	g_err << "Failed to create undofile:"
	      << endl << pDict->getNdbError() << endl;
	return NDBT_FAILED;
      }
    }
  }

  NdbDictionary::Tablespace ts = pDict->getTablespace("DEFAULT-TS");
  if (strcmp(ts.getName(), "DEFAULT-TS") != 0)
  {
    ts.setName("DEFAULT-TS");
    ts.setExtentSize(1024*1024);
    ts.setDefaultLogfileGroup("DEFAULT-LG");
    
    res = pDict->createTablespace(ts);
    if(res != 0){
      g_err << "Failed to create tablespace:"
	    << endl << pDict->getNdbError() << endl;
      return NDBT_FAILED;
    }
  }

  mb = 128;
  {
    char buf[256];
    if (NdbEnv_GetEnv("DATASIZE", buf, sizeof(buf)))
    {
      mb = atoi(buf);
      ndbout_c("Using %umb dd-data", mb);
    }
  }
  
  sz = 64;
  files = 13;
  {
    char buf[256];
    if (NdbEnv_GetEnv("DATAFILES", buf, sizeof(buf)))
    {
      files = atoi(buf);
      ndbout_c("Using max %u dd-data files", files);
    }
  }
  
  while (mb > files * sz)
    sz += 32;
  for (Uint32 i = 0; i * sz < mb; i++)
  {
    char tmp[256];
    BaseString::snprintf(tmp, sizeof(tmp), "datafile%u.dat", i);
    NdbDictionary::Datafile df = pDict->getDatafile(0, tmp);
    if (strcmp(df.getPath(), tmp) != 0)
    {
      df.setPath(tmp);
      df.setSize(Uint64(sz)*1024*1024);
      df.setTablespace("DEFAULT-TS");
      
      res = pDict->createDatafile(df, true);
      if(res != 0){
	g_err << "Failed to create datafile:"
	      << endl << pDict->getNdbError() << endl;
	return NDBT_FAILED;
      }
    }
  }
    
  return NDBT_OK;
}

int
NDBT_Tables::createTable(Ndb* pNdb, const char* _name, bool _temp, 
			 bool existsOk, NDBT_CreateTableHook f, void* arg)
{
  const NdbDictionary::Table* tab = NDBT_Tables::getTable(_name);
  if (tab == NULL){
    ndbout << "Could not create table " << _name 
	   << ", it doesn't exist in list of tables "\
      "that NDBT_Tables can create!" << endl;
    return NDBT_WRONGARGS;
  }

  Uint32 sum = 0;
  for (Uint32 i = 0; i<strlen(_name); i++)
    sum += 33 * sum + (Uint32)_name[i];
  
  bool forceVarPart = (sum & 1);
  
  int r = 0;
  do {
    NdbDictionary::Table tmpTab(* tab);
    tmpTab.setStoredTable(_temp ? 0 : 1);
    tmpTab.setForceVarPart(forceVarPart);

    {
      NdbError error;
      int ret = tmpTab.validate(error);
      assert(ret == 0);
    }
    if(f != 0 && f(pNdb, tmpTab, 0, arg))
    {
      ndbout << "Failed to create table" << endl;
      return NDBT_FAILED;
    }   
loop:   
    r = pNdb->getDictionary()->createTable(tmpTab);
    if(r == -1){
      if(pNdb->getDictionary()->getNdbError().code == 755)
      {
	ndbout << "Error: " << pNdb->getDictionary()->getNdbError() << endl;
	if (create_default_tablespace(pNdb) == 0)
	{
	  goto loop;
	}
      }
      if(!existsOk){
	ndbout << "Error0: " << pNdb->getDictionary()->getNdbError() << endl;
	
	break;
      }
      if(pNdb->getDictionary()->getNdbError().code != 721){
	ndbout << "Error: " << pNdb->getDictionary()->getNdbError() << endl;
	break;
      }
      
      r = 0;
    }
    
    Uint32 i = 0;
    for(i = 0; indexes[i].m_table != 0; i++){
      if(strcmp(indexes[i].m_table, _name) != 0)
	continue;
      Uint32 j = 0;
      while(indexes[i].m_indexes[j] != 0){
	NdbDictionary::Index tmpIndx;
	BaseString name;
	name.assfmt("%s$NDBT_IDX%d", _name, j);
	tmpIndx.setName(name.c_str());
	tmpIndx.setTable(_name);
	bool logging = !_temp;
	if(strcmp(indexes[i].m_indexes[j], "ORDERED") == 0){
	  logging = false;
	  tmpIndx.setType(NdbDictionary::Index::OrderedIndex);
	} else if(strcmp(indexes[i].m_indexes[j], "UNIQUE") == 0){
	  tmpIndx.setType(NdbDictionary::Index::UniqueHashIndex);
	} else {
	  ndbout << "Unknown index type";
	  abort();
	}
	tmpIndx.setLogging(logging);
	
	j++;
	while(indexes[i].m_indexes[j] != 0){
          tmpIndx.addIndexColumn(indexes[i].m_indexes[j]);
	  j++;
	}
	j++;
	if (tmpTab.getTemporary())
	{
	  tmpIndx.setTemporary(true);
	  tmpIndx.setLogging(false);
	}
	if(pNdb->getDictionary()->createIndex(tmpIndx) != 0){
	  ndbout << pNdb->getDictionary()->getNdbError() << endl;
	  return NDBT_FAILED;
	}
      }
    }
    if(f != 0 && f(pNdb, tmpTab, 1, arg))
    {
      ndbout << "Failed to create table" << endl;
      return NDBT_FAILED;
    }      
  } while(false);
  
  return r;
}

int
NDBT_Tables::dropAllTables(Ndb* pNdb){

  for (int i=0; i < NDBT_Tables::getNumTables(); i++){

    const NdbDictionary::Table* tab = NDBT_Tables::getTable(i);
    if (tab == NULL){
      return NDBT_ProgramExit(NDBT_FAILED);
    }
    
    if(pNdb->getDictionary()->dropTable(tab->getName()) == -1){
      return NDBT_FAILED;
    }
  }
  return NDBT_OK;
}


int
NDBT_Tables::print(const char * _name){
  
  const NDBT_Table * tab = (const NDBT_Table*)NDBT_Tables::getTable(_name);
  if (tab == NULL){
    ndbout << "Could not print table " << _name 
	   << ", it doesn't exist in list of tables "
	   << "that NDBT_Tables can create!" << endl;
    return NDBT_WRONGARGS;
  }
  ndbout << (* tab) << endl;
  return NDBT_OK;
}

int
NDBT_Tables::printAll(){

  for (int i=0; i < getNumTables(); i++){
    
    const NdbDictionary::Table* tab = getTable(i);
    if (tab == NULL){
      abort();
    }
    ndbout << (* (NDBT_Table*)tab) << endl;
  }
  
  return NDBT_OK;
}
