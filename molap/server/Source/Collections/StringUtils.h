////////////////////////////////////////////////////////////////////////////////
/// @brief string utilities
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

#ifndef COLLECTIONS_STRING_UTILS_H
#define COLLECTIONS_STRING_UTILS_H 1

#include "palo.h"

#include <sstream>

#include "Exceptions/ErrorException.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief string utilities
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS StringUtils {
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks for a suffix
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static bool isSuffix (const string& str, const string& postfix) {
      if (postfix.length() > str.length()) {
        return false;
      }
      else if (postfix.length() == str.length()) {
        return str == postfix;
      }
      else {
        return str.compare(str.size() - postfix.length(), postfix.length(), postfix) == 0;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief search text
    ///
    /// Simple text search with wild cards.
    ////////////////////////////////////////////////////////////////////////////////

    static bool simpleSearch (string text, string search);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief computes the hash value
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static uint32_t hashValue (const uint8_t * key, size_t length) {
      uint32_t crc = 0x12345678L;
      uint32_t oldI;
      uint32_t newI;

      for (;  0 < length;  length--, key++) {
        oldI = (crc >> 8) & 0x00FFFFFFL;
        newI = ccitt32[ ( (uint32_t)( crc ^ *key ) ) & 0xff ];
        crc  = oldI ^ newI;
      }

      return crc;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief computes the hash value
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static uint32_t hashValue (const char * str, size_t length) {
      return hashValue((const uint8_t*) str, length);
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief computes the hash value as lowercase
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static uint32_t hashValueLower (const char * key, size_t length) {
      uint32_t crc = 0x12345678L;
      uint32_t oldI;
      uint32_t newI;

      for (;  0 < length;  length--, key++) {
        oldI = (crc >> 8) & 0x00FFFFFFL;
        newI = ccitt32[ ( (uint32_t)( crc ^ ((uint8_t) ::tolower(*key)) ) ) & 0xff ];
        crc  = oldI ^ newI;
      }

      return crc;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief computes the hash value
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static uint32_t hashValue32 (const uint32_t * key, size_t length) {
      uint32_t crc = 0x12345678L;
      uint32_t oldI;
      uint32_t newI;

      for (;  0 < length;  length--, key++) {
        uint32_t value = *key;

        oldI = (crc >> 8) & 0x00FFFFFFL;
        newI = ccitt32[ ( (uint32_t)( crc ^ (value & 0xFF) ) ) & 0xff ];
        crc  = oldI ^ newI;

        value = value >> 8;

        oldI = (crc >> 8) & 0x00FFFFFFL;
        newI = ccitt32[ ( (uint32_t)( crc ^ (value & 0xFF) ) ) & 0xff ];
        crc  = oldI ^ newI;

        value = value >> 8;

        oldI = (crc >> 8) & 0x00FFFFFFL;
        newI = ccitt32[ ( (uint32_t)( crc ^ (value & 0xFF) ) ) & 0xff ];
        crc  = oldI ^ newI;

        value = value >> 8;

        oldI = (crc >> 8) & 0x00FFFFFFL;
        newI = ccitt32[ ( (uint32_t)( crc ^ (value & 0xFF) ) ) & 0xff ];
        crc  = oldI ^ newI;
      }

      return crc;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts double to string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string convertToString (double i) {
      stringstream str;

      str << i;
    
      return str.str();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts integer to string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string convertToString (int64_t i) {
      stringstream str;

      str << i;
    
      return str.str();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts integer to string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string convertToString (int32_t i) {
      stringstream str;

      str << i;
    
      return str.str();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts unsigned integer to string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string convertToString (uint64_t i) {
      stringstream str;

      str << i;
    
      return str.str();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts unsigned integer to string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string convertToString (uint32_t i) {
      stringstream str;

      str << i;
    
      return str.str();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts size_t to string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

#ifdef OVERLOAD_FUNCS_SIZE_T_LONG

    static string convertToString (size_t i) {
      stringstream str;

      str << i;
    
      return str.str();
    }

#endif

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts string to lower case
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string tolower (const string& str) {
      size_t len = str.length();

      if (len == 0) {
        return "";
      }

      char * buffer = new char[len];
      char * qtr = buffer;
      const char * ptr = str.c_str();

      for (; 0 < len;  len--, ptr++, qtr++) {
        *qtr = (char) ::tolower(*ptr);
      }

      string result(buffer, str.size());
      delete [] buffer;

      return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts string to upper case
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string toupper (const string& str) {
      size_t len = str.length();

      if (len == 0) {
        return "";
      }

      char * buffer = new char[len];
      char * qtr = buffer;
      const char * ptr = str.c_str();

      for (; 0 < len;  len--, ptr++, qtr++) {
        *qtr = (char) ::toupper(*ptr);
      }

      string result(buffer, str.size());
      delete [] buffer;

      return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief capitalization of a string
    ///
    /// This method is implemented here in order to allow inlining.
    ////////////////////////////////////////////////////////////////////////////////

    static string capitalization (const string& str) {
      size_t len = str.length();              

      if (len == 0) {
        return "";
      }

      char * buffer = new char[len];
      char * qtr = buffer;
      const char * ptr = str.c_str();

      bool letter = false;

      for (; 0 < len;  len--, ptr++, qtr++) {
        if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')) {

          // character is a letter
          if (!letter) {
            *qtr = (char) ::toupper(*ptr);
            letter = true;
          }
          else {
            *qtr = (char) ::tolower(*ptr);
          }
        }
        else {
          *qtr = (char) ::tolower(*ptr);
          letter = false;
        }
      }

      string result(buffer, str.size());
      delete [] buffer;

      return result;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief escapes html special characters
    ////////////////////////////////////////////////////////////////////////////////

    static string escapeHtml (const string& str);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief escapes xml special characters
    ////////////////////////////////////////////////////////////////////////////////

    static string escapeXml (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief escapes special characters by "
    ////////////////////////////////////////////////////////////////////////////////

    static string escapeString (const string& text);    

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief escapes special characters by '
    ////////////////////////////////////////////////////////////////////////////////

    static string escapeStringSingle (const string& text);    

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts string to integer
    ////////////////////////////////////////////////////////////////////////////////

    static long stringToInteger (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts string to unsigned integer
    ////////////////////////////////////////////////////////////////////////////////

    static unsigned long stringToUnsignedInteger (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief converts string to double
    ////////////////////////////////////////////////////////////////////////////////

    static double stringToDouble (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief encodes a string to hex
    ////////////////////////////////////////////////////////////////////////////////

    static string encodeToHex (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief decodes hex to a string
    ////////////////////////////////////////////////////////////////////////////////

    static string decodeHexToString (const string& str);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief encodes a string to base64
    ////////////////////////////////////////////////////////////////////////////////

    static string encodeBase64 (const uint8_t * str, size_t length);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief decodes base64 to a string
    ////////////////////////////////////////////////////////////////////////////////

    static void decodeBase64 (const string& str, uint8_t * dest, size_t& length);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief splits string along seperator
    ////////////////////////////////////////////////////////////////////////////////

    static void splitString (const string& line, vector<string>* elements, char seperator);

	////////////////////////////////////////////////////////////////////////////////
    /// @brief splits string using 2 seperators and respecting quotes
    ////////////////////////////////////////////////////////////////////////////////

	static bool splitString2Sep (const string& line, vector<vector<string> >* elements, char separator1, char separator2, bool quote);

    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief extracts next element from string
    ////////////////////////////////////////////////////////////////////////////////

    static string getNextElement(string& buffer, size_t& pos, const char seperator, bool quote = false);

  private:
    static const uint32_t ccitt32[ 256 ];
  };
}

#endif
