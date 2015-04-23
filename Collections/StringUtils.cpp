/* 
 *
 * Copyright (C) 2006-2015 Jedox AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as published
 * by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * If you are developing and distributing open source applications under the
 * GPL License, then you are free to use Palo under the GPL License.  For OEMs,
 * ISVs, and VARs who distribute Palo with their products, and do not license
 * and distribute their source code under the GPL, Jedox provides a flexible
 * OEM Commercial License.
 *
 * \author Frank Celler, triagens GmbH, Cologne, Germany
 * \author Achim Brandt, triagens GmbH, Cologne, Germany
 * \author Jerome Meinke, University of Freiburg, Germany
 * 
 *
 */


#include "Collections/StringUtils.h"

#include <vector>
#include <string>
#include <iostream>

#include "Collections/StringBuffer.h"
#include "Exceptions/ParameterException.h"


StringUtils StringUtils::initializer;

// /////////////////////////////////////////////////////////////////////////////
// static variables
// /////////////////////////////////////////////////////////////////////////////

const uint32_t StringUtils::ccitt32[256] = { 0x00000000L, 0x77073096L,
    0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L,
    0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
    0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L, 0x1db71064L,
    0x6ab020f2L, 0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL,
    0xf4d4b551L, 0x83d385c7L, 0x136c9856L, 0x646ba8c0L, 0xfd62f97aL,
    0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
    0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L,
    0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL, 0x35b5a8faL, 0x42b2986cL,
    0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL,
    0xabd13d59L, 0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
    0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL, 0x2802b89eL,
    0x5f058808L, 0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L,
    0xc1611dabL, 0xb6662d3dL, 0x76dc4190L, 0x01db7106L, 0x98d220bcL,
    0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
    0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL,
    0x086d3d2dL, 0x91646c97L, 0xe6635c01L, 0x6b6b51f4L, 0x1c6c6162L,
    0x856530d8L, 0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L,
    0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
    0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L, 0x4db26158L,
    0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L,
    0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL, 0x346ed9fcL, 0xad678846L,
    0xda60b8d0L, 0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
    0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L,
    0x206f85b3L, 0xb966d409L, 0xce61e49fL, 0x5edef90eL, 0x29d9c998L,
    0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL,
    0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
    0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L, 0xe3630b12L,
    0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL,
    0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L, 0x8708a3d2L, 0x1e01f268L,
    0x6906c2feL, 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
    0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL,
    0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L, 0xd6d6a3e8L, 0xa1d1937eL,
    0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL,
    0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
    0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L, 0xcb61b38cL,
    0xbc66831aL, 0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L,
    0x220216b9L, 0x5505262fL, 0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L,
    0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
    0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L,
    0xeb0e363fL, 0x72076785L, 0x05005713L, 0x95bf4a82L, 0xe2b87a14L,
    0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L,
    0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
    0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L, 0x88085ae6L,
    0xff0f6a70L, 0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L,
    0x616bffd3L, 0x166ccf45L, 0xa00ae278L, 0xd70dd2eeL, 0x4e048354L,
    0x3903b3c2L, 0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
    0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L,
    0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L, 0xbdbdf21cL, 0xcabac28aL,
    0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L,
    0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
    0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL };

uint32_t StringUtils::lower32[256];

// /////////////////////////////////////////////////////////////////////////////
// constructor
// /////////////////////////////////////////////////////////////////////////////

StringUtils::StringUtils() {
  for (int i = 0; i < 256; i++) {
    lower32[i] = ::tolower(static_cast<char>(i));
  }
}

// /////////////////////////////////////////////////////////////////////////////
// utility functions
// /////////////////////////////////////////////////////////////////////////////

static bool simpleMatch(const char* ptr, const char* end, const char* qtr,
                        const char* qnd) {
  for (; qtr < qnd; qtr++) {
    if (*qtr == '?') {
      if (ptr >= end) {
        return false;
      }

      ptr++;
    } else if (*qtr == '*') {
      for (; ptr < end; ptr++) {
        if (simpleMatch(ptr, end, qtr + 1, qnd)) {
          return true;
        }
      }

      return false;
    } else if (*qtr == '~') {
      qtr++;

      if (qtr == qnd) {
        return true;
      }

      if (ptr >= end || *ptr != *qtr) {
        return false;
      }

      ptr++;
    } else {
      if (ptr >= end || *ptr != *qtr) {
        return false;
      }

      ptr++;
    }
  }

  return true;
}

// /////////////////////////////////////////////////////////////////////////////
// public methods
// /////////////////////////////////////////////////////////////////////////////

bool StringUtils::simpleSearch(string text, string search) {
  text = tolower(text);
  const char * ptr = text.c_str();
  const char * end = ptr + text.size();

  search = tolower(search);
  const char * qtr = search.c_str();
  const char * qnd = qtr + search.size();

  for (; ptr < end; ptr++) {
    if (simpleMatch(ptr, end, qtr, qnd)) {
      return true;
    }
  }

  return false;
}

int64_t StringUtils::stringToInteger(const string& str) {
  if (str.empty()) {
    throw ParameterException(
        ErrorException::ERROR_CONVERSION_FAILED,
        "error converting a string to a number, string is empty", "str", str);
  }

  char *p;

  int64_t i = strtol(str.c_str(), &p, 10);

  if (*p != '\0') {
    throw ParameterException(
        ErrorException::ERROR_CONVERSION_FAILED,
        "error converting a string to a number, string has illegal characters",
        "str", str);
  }

  return i;
}

uint64_t StringUtils::stringToUnsignedInteger(const string& str) {
  if (str.empty()) {
    throw ParameterException(
        ErrorException::ERROR_CONVERSION_FAILED,
        "error converting a string to a number, string is empty", "str", str);
  }

  char *p;

  uint64_t i = strtoul(str.c_str(), &p, 10);

  if (*p != '\0') {
    throw ParameterException(
        ErrorException::ERROR_CONVERSION_FAILED,
        "error converting a string to a number, string has illegal characters",
        "str", str);
  }

  return i;
}

double StringUtils::stringToDouble(const string& str) {
  if (str.empty()) {
    throw ParameterException(
        ErrorException::ERROR_CONVERSION_FAILED,
        "error converting a string to a number, string is empty", "str", str);
  }

  char *p;

  double d = strtod(str.c_str(), &p);

  if (*p != '\0') {
    throw ParameterException(
        ErrorException::ERROR_CONVERSION_FAILED,
        "error converting a string to a number, string has illegal characters",
        "str", str);
  }

  return d;
}

static uint8_t hex2int(uint8_t ch) {
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  } else if ('A' <= ch && ch <= 'F') {
    return ch - 'A' + 10;
  } else if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 10;
  }

  throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                           "error decoding a hex value", "ch", ch);

  return 0;
}

string StringUtils::encodeToHex(const string& str) {
  static const char *hexChars = "0123456789abcdef";
  string result;

  StringBuffer s;

  for (size_t i = 0; i < str.length(); i++) {
    char x = str[i];

    s.appendChar(hexChars[x >> 4]);
    s.appendChar(hexChars[x & 0x0F]);
  }

  result = s.c_str();

  return result;
}

string StringUtils::decodeHexToString(const string& str) {
  string result;

  StringBuffer s;

  size_t l = str.length();

  if ((l % 2) == 1) {
    throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                             "error decoding a hex value", "str", str);
  }

  for (size_t i = 0; i < l; i = i + 2) {
    char x = str[i];
    char y = str[i + 1];
    char ch = hex2int(x) << 4 | hex2int(y);

    s.appendChar(ch);
  }

  result = s.c_str();

  return result;
}

string StringUtils::encodeBase64(const uint8_t * str, size_t length) {
  static string base64_chars = "0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "!$";

  if (length == 0) {
    return "";
  }

  StringBuffer s;

  uint8_t buffer[3];
  size_t j = 0;

  for (size_t i = 0; i < length; i++) {
    buffer[j++] = str[i];

    if (j == 3) {
      uint8_t a = (uint8_t) (buffer[0] >> 2);
      uint8_t b = (uint8_t) (((buffer[0] & 0x03) << 4) | (buffer[1] >> 4));
      uint8_t c = (uint8_t) (((buffer[1] & 0x0f) << 2) | (buffer[2] >> 6));
      uint8_t d = (uint8_t) (buffer[2] & 0x3f);

      s.appendChar(base64_chars[a]);
      s.appendChar(base64_chars[b]);
      s.appendChar(base64_chars[c]);
      s.appendChar(base64_chars[d]);

      j = 0;
    }
  }

  if (j != 0) {
    for (size_t i = j; i < 3; i++) {
      buffer[i] = 0;
    }

    s.appendChar(base64_chars[buffer[0] >> 2]);
    s.appendChar(base64_chars[((buffer[0] & 0x03) << 4) | (buffer[1] >> 4)]);

    if (j == 2) {
      s.appendChar(base64_chars[(buffer[1] & 0x0f) << 2]);
      s.appendChar('=');
    } else {
      s.appendText("==");
    }
  }

  string result = s.c_str();

  return result;
}

static uint8_t base64ToInt(const uint8_t c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  } else if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 36;
  } else if (c == '!') {
    return 62;
  } else if (c == '$') {
    return 63;
  } else if (c == '=') {
    return 0;
  }

  throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                           "error decoding a base 64 value", "c", c);
}

void StringUtils::decodeBase64(const string& str, uint8_t * dest,
                               size_t& length) {
  if (str.empty()) {
    length = 0;
    return;
  }

  size_t l = str.length();

  if ((l % 4) != 0) {
    throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                             "error decoding a base 64 value", "str", str);
  }

  char buffer[4];
  size_t j = 0;
  size_t p = 0;

  for (size_t i = 0; i < l; i++) {
    buffer[j++] = str[i];

    if (j == 4) {
      dest[p++] = (base64ToInt(buffer[0]) << 2) + (base64ToInt(buffer[1]) >> 4);

      if (p > length) {
        throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                 "base 64 buffer overflow", "str", str);
      }

      if (buffer[1] != '=' && buffer[2] != '=') {
        dest[p++] = ((base64ToInt(buffer[1]) & 0x0f) << 4)
            + (base64ToInt(buffer[2]) >> 2);

        if (p > length) {
          throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                   "base 64 buffer overflow", "str", str);
        }
      }

      if (buffer[2] != '=' && buffer[3] != '=') {
        dest[p++] = ((base64ToInt(buffer[2]) & 0x03) << 6)
            + (base64ToInt(buffer[3]));

        if (p > length) {
          throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                   "base 64 buffer overflow", "str", str);
        }
      }

      j = 0;
    }
  }

  length = p;
}

void StringUtils::decodeBase64(const char* str, const char* end, uint8_t * dest,
                               size_t& length) {
  if (str == end) {
    length = 0;
    return;
  }

  size_t l = (end - str);

  if ((l % 4) != 0) {
    throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                             "error decoding a base 64 value", "str", str);
  }

  char buffer[4];
  size_t j = 0;
  size_t p = 0;

  for (size_t i = 0; i < l; i++) {
    buffer[j++] = str[i];

    if (j == 4) {
      dest[p++] = (base64ToInt(buffer[0]) << 2) + (base64ToInt(buffer[1]) >> 4);

      if (p > length) {
        throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                 "base 64 buffer overflow", "str", str);
      }

      if (buffer[1] != '=' && buffer[2] != '=') {
        dest[p++] = ((base64ToInt(buffer[1]) & 0x0f) << 4)
            + (base64ToInt(buffer[2]) >> 2);

        if (p > length) {
          throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                   "base 64 buffer overflow", "str", str);
        }
      }

      if (buffer[2] != '=' && buffer[3] != '=') {
        dest[p++] = ((base64ToInt(buffer[2]) & 0x03) << 6)
            + (base64ToInt(buffer[3]));

        if (p > length) {
          throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                   "base 64 buffer overflow", "str", str);
        }
      }

      j = 0;
    }
  }

  length = p;
}

string StringUtils::escapeHtml(const string& str) {
  size_t begin = 0;
  size_t end = str.find_first_of("<>&\n\"");

  if (end == std::string::npos) {
    return str;
  }

  string result = "";
  while (end != std::string::npos) {
    result += str.substr(begin, end - begin);

    switch (str[end]) {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      case '\n':
        result += "<br>";
        break;
      case '"':
        result += "&quot;";
        break;
    }
    begin = end + 1;
    end = str.find_first_of("<>&\n\"", begin);
  }
  result += str.substr(begin);

  return result;
}

string StringUtils::escapeXml(const string& str) {
  size_t begin = 0;
  size_t end = str.find_first_of("<>&'\"");

  if (end == std::string::npos) {
    return str;
  }

  string result = "";
  while (end != std::string::npos) {
    result += str.substr(begin, end - begin);

    switch (str[end]) {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      case '\'':
        result += "&apos;";
        break;
      case '"':
        result += "&quot;";
        break;
    }
    begin = end + 1;
    end = str.find_first_of("<>&'\"", begin);
  }
  result += str.substr(begin);

  return result;
}

void StringUtils::splitString(const string& line, vector<string>* elements,
                              char seperator) {
  if (line.empty()) {
    return;
  }

  string s = "";
  bool first = true;
  bool escaped = false;

  size_t len = line.length();
  size_t pos = 0;

  while (pos < len) {
    char c = line[pos];

    if (first) {
      if (line[pos] == '"') {
        escaped = true;
        first = false;
      } else if (line[pos] == seperator) {  // empty value found
        elements->push_back("");
      } else {
        s += c;
        first = false;
      }
    } else {
      if (escaped) {
        if (line[pos] == '"') {
          if (pos + 1 < len) {
            pos++;
            if (line[pos] == seperator) {
              elements->push_back(s);
              s = "";
              first = true;
              escaped = false;
            } else {
              s += c;
            }
          } else {
            elements->push_back(s);
          }
        } else {
          s += c;
        }
      } else {
        if (line[pos] == seperator) {
          elements->push_back(s);
          s = "";
          first = true;
        } else {
          s += c;
        }
      }
    }
    pos++;
  }
  if (!first) {
    elements->push_back(s);
  }
}

string StringUtils::escapeString(const string& text) {
  StringBuffer sb;

  string buffer = text;
  size_t begin = 0;
  size_t end = buffer.find("\"");

  sb.appendText("\"");

  while (end != string::npos) {
    string result = text.substr(begin, end - begin);

    sb.appendText(result);
    sb.appendText("\"\"");

    begin = end + 1;
    end = buffer.find("\"", begin);
  }

  sb.appendText(text.substr(begin));
  sb.appendText("\"");

  string result = sb.c_str();

  return result;
}

string StringUtils::escapeStringSingle(const string& text) {
  StringBuffer sb;

  string buffer = text;
  size_t begin = 0;
  size_t end = buffer.find("'");

  sb.appendText("'");

  while (end != string::npos) {
    string result = text.substr(begin, end - begin);

    sb.appendText(result);
    sb.appendText("''");

    begin = end + 1;
    end = buffer.find("'", begin);
  }

  sb.appendText(text.substr(begin));
  sb.appendText("'");

  string result = sb.c_str();

  return result;
}

string StringUtils::getNextElement(const string& buffer, size_t& pos,
                                   char seperator, bool quote) {
  if (pos >= buffer.size()) {
    return "";
  }

  string result;

  if (quote && buffer[pos] == '"') {
    const char * p = buffer.c_str() + (pos + 1);
    const char * l = p;
    const char * q = buffer.c_str() + (buffer.size());

    char * b = new char[buffer.size()];
    char * k = b;

    while (p < q) {
      if (p[0] == '"' && p[1] == '"') {
        *k++ = '"';
        p += 2;
      } else if (p[0] == '"' && p[1] == seperator) {
        p += 2;
        break;
      } else if (p[0] == '"' && p + 1 == q) {
        p += 1;
        break;
      } else {
        *k++ = *p++;
      }
    }

    *k = '\0';

    result = b;
    pos += (p - l) + 1;

    delete[] b;

    return result;
  } else {
    size_t end = buffer.find_first_of(seperator, pos);

    if (end != string::npos) {
      result = buffer.substr(pos, end - pos);
      pos = end + 1;
    } else {
      result = buffer.substr(pos);
      pos = buffer.size() + 1;
    }
  }

  return result;
}

void StringUtils::splitString2(char* ss, char* se,
                               vector<vector<string> >* elements,
                               char separator1, char separator2, bool quote) {
  std::stringstream* sss = new std::stringstream();

  bool newline = true;
  char* lq = 0;
  int q = 0;

  for (; ss <= se; ++ss) {
    if ((q % 2 == 0) && (ss == se || *ss == separator1 || *ss == separator2)) {
      if (newline)
        elements->push_back(vector<string>());
      elements->back().push_back(sss->str());
      delete sss;
      newline = (ss == se) || *ss == separator1;
      if (ss == se)
        return;
      lq = 0;
      q = 0;
      sss = new std::stringstream();
    } else if (quote && *ss == '"') {
      ++q;
      if (q % 2 == 1 && lq + 1 == ss)
        (*sss) << '"';
      lq = ss;
    } else {
      (*sss) << *ss;
    }
  }
  if (sss)
    delete sss;
}

