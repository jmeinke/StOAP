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
 * \author Jiri Junek, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Jakl, qBicon s.r.o., Prague, Czech Republic
 * 
 *
 */

#ifndef STOAP_INPUTOUTPUT_FILEWRITER_H_
#define STOAP_INPUTOUTPUT_FILEWRITER_H_ 1

#include <string>
#include <vector>
#include "Olap.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief file a data writer
///
/// The file write generates CSV files to store the OLAP server, database, dimension
/// and cube data.
////////////////////////////////////////////////////////////////////////////////

class FileWriter {
 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief constructs a new file writer object for a given filename, path and file extension
  ////////////////////////////////////////////////////////////////////////////////

  explicit FileWriter(const FileName& fileName);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief deletes a file writer object
  ////////////////////////////////////////////////////////////////////////////////

  ~FileWriter();

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief opens the file for writing
  ////////////////////////////////////////////////////////////////////////////////

  void openFile(bool append = false);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief close the file for writing
  ////////////////////////////////////////////////////////////////////////////////

  void closeFile();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends a comment line
  ////////////////////////////////////////////////////////////////////////////////

  void appendComment(const string& value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends a section line
  ////////////////////////////////////////////////////////////////////////////////

  void appendSection(const string& value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends a string to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendString(const string& value, char terminator = ';');

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief escapes a string and appends the result to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendEscapeString(const string& value, char terminator = ';');

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief escapes a list of strings and appends the result to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendEscapeStrings(const vector<string>* value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends an integer to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendInteger(const int32_t value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends list of integers to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendIntegers(const vector<int32_t>* value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends an identifier to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendIdentifier(const IdentifierType value, char terminator =
                                    ';');

  template<typename Iter> void appendIdentifiers(Iter begin, Iter end,
                                                 char separator = ',',
                                                 char terminator = ';') {
    bool first = true;
    for (; begin != end; ++begin) {
      appendArray(first, (IdentifierType) *begin, separator);
      first = false;
    }
    appendArrayFinish(terminator);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends a double to the actual line
  ////////////////////////////////////////////////////////////////////////////////
  void appendDouble(const double value, char terminator = ';');

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends list of doubles to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendDoubles(const vector<double>* value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends a boolean to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendBool(const bool value);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief appends a timestamp to the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void appendTimeStamp();
  void appendTimeStamp(timeval &tv);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief finalize the actual line
  ////////////////////////////////////////////////////////////////////////////////

  void nextLine();

  void appendRaw(const string& value);

  void appendRaw(const char *p, streamsize size);

  static void deleteFile(const FileName& fileName);
  static int32_t getFileSize(const FileName& fileName);
  static FileWriter *getFileWriter(const FileName& fileName);

 private:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief escape a string and return the escaped string
  ////////////////////////////////////////////////////////////////////////////////

  string escapeString(const string& text);

 protected:
  void appendArray(bool bFirst, IdentifierType value, char separator);
  void appendArrayFinish(char terminator);
  bool isFirstValue();
  void setFirstValue(bool v);
  void writeBuffer();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the output stream
  ////////////////////////////////////////////////////////////////////////////////

  ostream* outputFile;
  FileName fileName;

 private:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief true if the buffer should be flushed after a new line
  ////////////////////////////////////////////////////////////////////////////////

  bool isFirst;
};

class DirLock {
 public:
  explicit DirLock(const string &dirName);
  ~DirLock();
 private:
  int fd;
  string fileName;
};


#endif  // STOAP_INPUTOUTPUT_FILEWRITER_H_
