////////////////////////////////////////////////////////////////////////////////
/// @brief legacy argument variant
///
/// @file
///
/// Copyright (C) 2006-2008 Jedox AG
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU General Public License (Version 2) as published
/// by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along with
/// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
/// Place, Suite 330, Boston, MA 02111-1307 USA
/// 
/// You may obtain a copy of the License at
///
/// <a href="http://www.jedox.com/license_palo.txt">
///   http://www.jedox.com/license_palo.txt
/// </a>
///
/// If you are developing and distributing open source applications under the
/// GPL License, then you are free to use Palo under the GPL License.  For OEMs,
/// ISVs, and VARs who distribute Palo with their products, and do not license
/// and distribute their source code under the GPL, Jedox provides a flexible
/// OEM Commercial License.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef LEGACY_SERVER_LEGACY_ARGUMENT_H
#define LEGACY_SERVER_LEGACY_ARGUMENT_H 1

#include "palo.h"

#include <vector>

namespace palo {
  using namespace std;

  struct StringBuffer;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief legacy argument variant
    ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS LegacyArgument {
  public:
    static double ntod (double value);

    static LegacyArgument* unserializeArgument (const char * * buffer,
                                                const char * end);

    static vector<LegacyArgument*> unserializeArguments (const char * begin,
                                                         const char * end,
                                                         size_t numberArguments);


  public:
    static const uint32_t NET_ARG_TYPE_VOID    = 1;
    static const uint32_t NET_ARG_TYPE_STRING  = 2;
    static const uint32_t NET_ARG_TYPE_DOUBLE  = 3;
    static const uint32_t NET_ARG_TYPE_INT     = 4;
    static const uint32_t NET_ARG_TYPE_UINT    = 5;
    static const uint32_t NET_ARG_TYPE_MULTI   = 8;
    static const uint32_t NET_ARG_TYPE_DELAYED = 98;
    static const uint32_t NET_ARG_TYPE_ERROR   = 99;

  public:
    virtual ~LegacyArgument () {
    }

  public:
    virtual void serialize (StringBuffer& buffer) const = 0;
    virtual uint32_t getArgumentType () const = 0;
  };

}

#endif
