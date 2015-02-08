/*
   Copyright (C) 2004-2006 MySQL AB
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

#ifndef __CONFIG_VALUES_HPP
#define __CONFIG_VALUES_HPP

#include <ndb_types.h>
#include <UtilBuffer.hpp>

class ConfigValues {
  friend class ConfigValuesFactory;
  ConfigValues(Uint32 sz, Uint32 data);

public:
  ~ConfigValues();
  
  enum ValueType {
    InvalidType = 0,
    IntType     = 1,
    StringType  = 2,
    SectionType = 3,
    Int64Type   = 4
  };

  struct Entry {
    Uint32 m_key;
    ValueType m_type;
    union {
      Uint32 m_int;
      Uint64 m_int64;
      const char * m_string;
    };
  };
  
  class ConstIterator {
    friend class ConfigValuesFactory;
    const ConfigValues & m_cfg;
  public:
    Uint32 m_currentSection;
    ConstIterator(const ConfigValues&c) : m_cfg(c) { m_currentSection = 0;}
    
    bool openSection(Uint32 key, Uint32 no);
    bool closeSection();

    bool get(Uint32 key, Entry *) const;
    
    bool get(Uint32 key, Uint32 * value) const;
    bool get(Uint32 key, Uint64 * value) const;
    bool get(Uint32 key, const char ** value) const;
    bool getTypeOf(Uint32 key, ValueType * type) const;
    
    Uint32 get(Uint32 key, Uint32 notFound) const;
    Uint64 get64(Uint32 key, Uint64 notFound) const;
    const char * get(Uint32 key, const char * notFound) const;
    ValueType getTypeOf(Uint32 key) const;
  };

  class Iterator : public ConstIterator {
    ConfigValues & m_cfg;
  public:
    Iterator(ConfigValues&c) : ConstIterator(c), m_cfg(c) {}
    Iterator(ConfigValues&c, const ConstIterator& i):ConstIterator(c),m_cfg(c){
      m_currentSection = i.m_currentSection;
    }
    
    bool set(Uint32 key, Uint32 value);
    bool set(Uint32 key, Uint64 value);
    bool set(Uint32 key, const char * value);
  };

  Uint32 getPackedSize() const; // get size in bytes needed to pack
  Uint32 pack(UtilBuffer&) const;
  Uint32 pack(void * dst, Uint32 len) const;// pack into dst(of len %d);
  
private:
  friend class Iterator;
  friend class ConstIterator;

  bool getByPos(Uint32 pos, Entry *) const;
  Uint64 * get64(Uint32 index) const;
  char ** getString(Uint32 index) const;

  Uint32 m_size;
  Uint32 m_dataSize;
  Uint32 m_stringCount;
  Uint32 m_int64Count;

  Uint32 m_values[1];
  void * m_data[1];
};

class ConfigValuesFactory {
  Uint32 m_currentSection;
public:
  Uint32 m_sectionCounter;
  Uint32 m_freeKeys;
  Uint32 m_freeData;

public:
  ConfigValuesFactory(Uint32 keys = 50, Uint32 data = 10); // Initial
  ConfigValuesFactory(ConfigValues * m_cfg);        //
  ~ConfigValuesFactory();

  ConfigValues * m_cfg;
  ConfigValues * getConfigValues();

  bool openSection(Uint32 key, Uint32 no);
  bool put(const ConfigValues::Entry & );
  bool put(Uint32 key, Uint32 value);
  bool put64(Uint32 key, Uint64 value);
  bool put(Uint32 key, const char * value);
  bool closeSection();
  
  void expand(Uint32 freeKeys, Uint32 freeData);
  void shrink();

  bool unpack(const UtilBuffer&);
  bool unpack(const void * src, Uint32 len);

  static ConfigValues * extractCurrentSection(const ConfigValues::ConstIterator &);

private:
  static ConfigValues * create(Uint32 keys, Uint32 data);
  void put(const ConfigValues & src);
};

inline
bool
ConfigValues::ConstIterator::get(Uint32 key, Uint32 * value) const {
  Entry tmp;
  if(get(key, &tmp) && tmp.m_type == IntType){
    * value = tmp.m_int;
    return true;
  }
  return false;
}

inline
bool
ConfigValues::ConstIterator::get(Uint32 key, Uint64 * value) const {
  Entry tmp;
  if(get(key, &tmp) && tmp.m_type == Int64Type){
    * value = tmp.m_int64;
    return true;
  }
  return false;
}

inline
bool
ConfigValues::ConstIterator::get(Uint32 key, const char ** value) const {
  Entry tmp;
  if(get(key, &tmp) && tmp.m_type == StringType){
    * value = tmp.m_string;
    return true;
  }
  return false;
}

inline
bool 
ConfigValues::ConstIterator::getTypeOf(Uint32 key, ValueType * type) const{
  Entry tmp;
  if(get(key, &tmp)){
    * type = tmp.m_type;
    return true;
  }
  return false;
}

inline
Uint32
ConfigValues::ConstIterator::get(Uint32 key, Uint32 notFound) const {
  Entry tmp;
  if(get(key, &tmp) && tmp.m_type == IntType){
    return tmp.m_int;
  }
  return notFound;
}

inline
Uint64
ConfigValues::ConstIterator::get64(Uint32 key, Uint64 notFound) const {
  Entry tmp;
  if(get(key, &tmp) && tmp.m_type == Int64Type){
    return tmp.m_int64;
  }
  return notFound;
}

inline
const char *
ConfigValues::ConstIterator::get(Uint32 key, const char * notFound) const {
  Entry tmp;
  if(get(key, &tmp) && tmp.m_type == StringType){
    return tmp.m_string;
  }
  return notFound;
}

inline
ConfigValues::ValueType
ConfigValues::ConstIterator::getTypeOf(Uint32 key) const{
  Entry tmp;
  if(get(key, &tmp)){
    return tmp.m_type;
  }
  return ConfigValues::InvalidType;
}

inline
bool
ConfigValuesFactory::put(Uint32 key, Uint32 val){
  ConfigValues::Entry tmp;
  tmp.m_key = key;
  tmp.m_type = ConfigValues::IntType;
  tmp.m_int = val;
  return put(tmp);
}

inline
bool
ConfigValuesFactory::put64(Uint32 key, Uint64 val){
  ConfigValues::Entry tmp;
  tmp.m_key = key;
  tmp.m_type = ConfigValues::Int64Type;
  tmp.m_int64 = val;
  return put(tmp);
}

inline
bool
ConfigValuesFactory::put(Uint32 key, const char * val){
  ConfigValues::Entry tmp;
  tmp.m_key = key;
  tmp.m_type = ConfigValues::StringType;
  tmp.m_string = val;
  return put(tmp);
}

inline
Uint32
ConfigValues::pack(UtilBuffer& buf) const {
  Uint32 len = getPackedSize();
  void * tmp = buf.append(len);
  if(tmp == 0){
    return 0;
  }
  return pack(tmp, len);
}

inline
bool
ConfigValuesFactory::unpack(const UtilBuffer& buf){
  return unpack(buf.get_data(), buf.length());
}

#endif
