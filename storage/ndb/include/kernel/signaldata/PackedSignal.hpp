/*
   Copyright (C) 2003-2006 MySQL AB
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

#ifndef PACKED_SIGNAL_HPP
#define PACKED_SIGNAL_HPP

#include "SignalData.hpp"

// -------- CODES FOR COMPRESSED SIGNAL (PACKED_SIGNAL) -------
#define ZCOMMIT 0
#define ZCOMPLETE 1
#define ZCOMMITTED 2
#define ZCOMPLETED 3
#define ZLQHKEYCONF 4
#define ZREMOVE_MARKER 5
#define ZFIRE_TRIG_REQ 6
#define ZFIRE_TRIG_CONF 7

class PackedSignal {

  static Uint32 getSignalType(Uint32 data);

  /**
   * For printing
   */
  friend bool printPACKED_SIGNAL(FILE * output, const Uint32 * theData, Uint32 len, Uint16 receiverBlockNo);
};

inline
Uint32 PackedSignal::getSignalType(Uint32 data) { return data >> 28; }

#endif
