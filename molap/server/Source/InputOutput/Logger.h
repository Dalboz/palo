////////////////////////////////////////////////////////////////////////////////
/// @brief logger
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

#ifndef INPUT_OUTPUT_LOGGER_H
#define INPUT_OUTPUT_LOGGER_H 1

#include "palo.h"

#include <iostream>
#include <sstream>

extern "C" {
#include <time.h>
}

namespace palo {
  using namespace std;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief logger
  ///
  /// This class provides various static members which can be used as logging
  /// streams. Output to the logging stream is appended by using the operator <<,
  /// as soon as a line is completed endl should be used to flush the stream.
  /// Each line of output is prefixed by some informational data.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Logger {
  private:
    class LoggerStream {
    public:
      LoggerStream (ostream& output)
        : on(true), output(&output) {
      }

    public:
      template<typename T>
      LoggerStream& operator<< (const T& value) {
        if (on) { (*output) << value; }
        return *this;
      }

#if defined(_MSC_VER)
      template<>
      LoggerStream& operator<< (const socket_t& value) {
         if (on) { (*output) << (unsigned int) value; }
         return *this;
      }
#endif

      LoggerStream& operator<< (ostream& (*fptr)(ostream&)) {
        if (on) { fptr((*output)); }
        return *this;
      }

      void activate() {
        on = true;
      }

      void deactivate() {
        on = false;
      }

      void setStream (ostream& os) {
        output = &os;
      }

    private:
      bool on;
      ostream* output;
    };

    class LoggerStreamHeader {
    public:
      LoggerStreamHeader (const string& prefix, ostream&);

    public:
      template<typename T>
      LoggerStream& operator<< (const T& value) {

        if (on) {
          time_t tt = time(0);
          struct tm* t = localtime(&tt);
          int max = 32;
          char * s = new char[max];    
          strftime(s,  max, "%Y-%m-%d %H:%M:%S ", t);    
          stream << s;
          delete[] s;

          stream << prefix << ": ";
          stream << value;
        }
  
        return stream;
      }

      void activate() {
        on = true;
        stream.activate();
      }
  
      void deactivate() {
        on = false;
        stream.deactivate();
      }

      void setStream (ostream& os) {
        stream.setStream(os);
      }
  
    private:
      bool on;
      const string prefix;
      LoggerStream stream;
    };



  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief changes log level
    ////////////////////////////////////////////////////////////////////////////////

    static void setLogLevel (const string& level);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief changes the log file
    ///
    /// "-" means stdout, "+" means stderr
    ////////////////////////////////////////////////////////////////////////////////

    static void setLogFile (const string& file);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief logger for error messages
    ////////////////////////////////////////////////////////////////////////////////

    static LoggerStreamHeader error;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief logger for warning messages
    ////////////////////////////////////////////////////////////////////////////////

    static LoggerStreamHeader warning;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief logger for info messages
    ////////////////////////////////////////////////////////////////////////////////

    static LoggerStreamHeader info;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief logger for trace messages
    ////////////////////////////////////////////////////////////////////////////////

    static LoggerStreamHeader trace;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief logger for debug messages
    ////////////////////////////////////////////////////////////////////////////////

    static LoggerStreamHeader debug;

  private:
    Logger ();

  private:
    static Logger logger;
  };

}

#endif
