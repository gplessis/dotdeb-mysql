/*
   Copyright (C) 2003-2008 MySQL AB
    All rights reserved. Use is subject to license terms.

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

#include <NDBT_Table.hpp>
#include <NdbTimer.hpp>
#include <NDBT.hpp>

class NdbOut& 
operator <<(class NdbOut& ndbout, const NDBT_Table & tab)
{
  ndbout << "-- " << tab.getName() << " --" << endl;
  
  ndbout << "Version: " <<  tab.getObjectVersion() << endl; 
  ndbout << "Fragment type: " <<  (unsigned) tab.getFragmentType() << endl; 
  ndbout << "K Value: " <<  tab.getKValue()<< endl; 
  ndbout << "Min load factor: " <<  tab.getMinLoadFactor()<< endl;
  ndbout << "Max load factor: " <<  tab.getMaxLoadFactor()<< endl; 
  ndbout << "Temporary table: " <<  (tab.getStoredTable() ? "no" : "yes") << endl;
  ndbout << "Number of attributes: " <<  tab.getNoOfColumns() << endl;
  ndbout << "Number of primary keys: " <<  tab.getNoOfPrimaryKeys() << endl;
  ndbout << "Length of frm data: " << tab.getFrmLength() << endl;
  ndbout << "Row Checksum: " << tab.getRowChecksumIndicator() << endl;
  ndbout << "Row GCI: " << tab.getRowGCIIndicator() << endl;
  ndbout << "SingleUserMode: " << (Uint32) tab.getSingleUserMode() << endl;
  ndbout << "ForceVarPart: " << tab.getForceVarPart() << endl;
  ndbout << "FragmentCount: " << tab.getFragmentCount() << endl;
  ndbout << "ExtraRowGciBits: " << tab.getExtraRowGciBits() << endl;
  ndbout << "ExtraRowAuthorBits: " << tab.getExtraRowAuthorBits() << endl;

  //<< ((tab.getTupleKey() == TupleId) ? " tupleid" : "") <<endl;
  ndbout << "TableStatus: ";
  switch(tab.getObjectStatus()){
  case NdbDictionary::Object::New:
    ndbout << "New" << endl;
    break;
  case NdbDictionary::Object::Changed:
    ndbout << "Changed" << endl;
    break;
  case NdbDictionary::Object::Retrieved:
    ndbout << "Retrieved" << endl;
    break;
  default:
    ndbout << "Unknown(" << (unsigned) tab.getObjectStatus() << ")" << endl;
  }
  
  ndbout << "-- Attributes -- " << endl;
  int noOfAttributes = tab.getNoOfColumns();
  for(int i = 0; i<noOfAttributes; i++){
    ndbout << (* (const NDBT_Attribute*)tab.getColumn(i)) << endl;
  }
  
  return ndbout;
}

class NdbOut& operator <<(class NdbOut&, const NdbDictionary::Index & idx)
{
  ndbout << idx.getName();
  ndbout << "(";
  for (unsigned i=0; i < idx.getNoOfColumns(); i++)
  {
    const NdbDictionary::Column *col = idx.getColumn(i);
    ndbout << col->getName();
    if (i < idx.getNoOfColumns()-1)
      ndbout << ", ";
  }
  ndbout << ")";
  
  ndbout << " - ";
  switch (idx.getType()) {
  case NdbDictionary::Object::UniqueHashIndex:
    ndbout << "UniqueHashIndex";
    break;
  case NdbDictionary::Object::OrderedIndex:
    ndbout << "OrderedIndex";
    break;
  default:
    ndbout << "Type " << (unsigned) idx.getType();
    break;
  }
  return ndbout;
}

