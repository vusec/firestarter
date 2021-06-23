/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef KEY_INFO_HPP
#define KEY_INFO_HPP

#include "SignalData.hpp"

class KeyInfo {
  /**
   * Sender(s)
   */
  friend class DbUtil;
  friend class NdbOperation;
  friend class NdbScanOperation;
  friend class NdbIndexScanOperation;
  friend class Restore;

  /**
   * Reciver(s)
   */
  friend class Dbtc;
  
public:
  STATIC_CONST( HeaderLength = 3 );
  STATIC_CONST( DataLength = 20 );
  STATIC_CONST( MaxSignalLength = HeaderLength + DataLength );

private:
  Uint32 connectPtr;
  Uint32 transId[2];
  Uint32 keyData[DataLength];
};

#endif
