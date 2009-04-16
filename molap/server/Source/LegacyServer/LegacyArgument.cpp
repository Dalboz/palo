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

#include "LegacyServer/LegacyArgument.h"

#include <algorithm>

#include "Exceptions/CommunicationException.h"

#include "Collections/DeleteObject.h"

#include "InputOutput/Logger.h"

#include "LegacyServer/ArgumentDouble.h"
#include "LegacyServer/ArgumentInt32.h"
#include "LegacyServer/ArgumentMulti.h"
#include "LegacyServer/ArgumentString.h"
#include "LegacyServer/ArgumentUInt32.h"
#include "LegacyServer/ArgumentVoid.h"

namespace palo {
  double LegacyArgument::ntod (double value) {
#ifdef WORDS_BIGENDIAN
    return value;
#else
    union {
      double  x;
      uint32_t y[2];
    } conv;

    conv.x = value;
    uint32_t tmp = ntohl(conv.y[0]);
    conv.y[0] = ntohl(conv.y[1]);
    conv.y[1] = tmp;

    return conv.x;
#endif
  }



  LegacyArgument * LegacyArgument::unserializeArgument (const char * * buffer,
                                                        const char * end) {
    if (*buffer + 2 * sizeof(uint32_t) >= end) {
      throw CommunicationException(ErrorException::ERROR_NET_ARG, "client command too short");
    }

    uint32_t type   = ntohl(reinterpret_cast<const uint32_t*>(*buffer)[0]);
    uint32_t length = ntohl(reinterpret_cast<const uint32_t*>(*buffer)[1]);

    *buffer += 2 * sizeof(uint32_t);

    if (*buffer + length > end) {
      throw CommunicationException(ErrorException::ERROR_NET_ARG, "client command too short");
    }

    LegacyArgument * argument = 0;
    
    size_t num;
    
    switch (type) {
      case NET_ARG_TYPE_VOID:
        if (length != 0) {
          throw CommunicationException(ErrorException::ERROR_NET_ARG, "invalid length for void");
        }

        argument = new ArgumentVoid();
        break;

      case NET_ARG_TYPE_STRING:
        if (length < 1) {
          throw CommunicationException(ErrorException::ERROR_NET_ARG, "invalid length for string");
        }

        argument = new ArgumentString(*buffer, length - 1);
        *buffer += length;

        break;

      case NET_ARG_TYPE_DOUBLE:
        if (length != sizeof(double)) {
          throw CommunicationException(ErrorException::ERROR_NET_ARG, "invalid length for double");
        }

        argument = new ArgumentDouble(ntod(reinterpret_cast<const double*>(*buffer)[0]));
        *buffer += sizeof(double);

        break;
        

      case NET_ARG_TYPE_INT:
        if (length != sizeof(int32_t)) {
          throw CommunicationException(ErrorException::ERROR_NET_ARG, "invalid length for signed integer");
        }

        argument = new ArgumentInt32(ntohl(reinterpret_cast<const int32_t*>(*buffer)[0]));
        *buffer += sizeof(int32_t);
        
        break;

      case NET_ARG_TYPE_UINT:
        if (length != sizeof(uint32_t)) {
          throw CommunicationException(ErrorException::ERROR_NET_ARG, "invalid length for signed integer");
        }

        argument = new ArgumentUInt32(ntohl(reinterpret_cast<const uint32_t*>(*buffer)[0]));
        *buffer += sizeof(uint32_t);

        break;

      case NET_ARG_TYPE_MULTI:
        if (length != sizeof(uint32_t)) {
          throw CommunicationException(ErrorException::ERROR_NET_ARG, "invalid length for list");
        }

        num = ntohl(reinterpret_cast<const uint32_t*>(*buffer)[0]);
        *buffer   += sizeof(uint32_t);

        {
          ArgumentMulti * multi = new ArgumentMulti();

          try {
            for (size_t i = 0;  i < num;  i++) {
              multi->append(LegacyArgument::unserializeArgument(buffer, end));
            }
          }
          catch (...) {
            delete multi;

            throw;
          }

          argument = multi;
        }

        break;


      default:
        Logger::warning << "got unknown argument type " << type << " in legacy mode" << endl;
        throw CommunicationException(ErrorException::ERROR_INV_ARG_TYPE, "unknown type");
    }

    return argument;
  }



  vector<LegacyArgument*> LegacyArgument::unserializeArguments (const char * begin,
                                                                const char * end,
                                                                size_t numberArguments) {
    vector<LegacyArgument*> arguments;
    
    try {
      for (size_t i = 0;  i < numberArguments;  i++) {
        LegacyArgument * argument = unserializeArgument(&begin, end);

        arguments.push_back(argument);
      }
    }
    catch (...) {
      for_each(arguments.begin(), arguments.end(), DeleteObject());
      throw;
    }

    return arguments;
  }
}
