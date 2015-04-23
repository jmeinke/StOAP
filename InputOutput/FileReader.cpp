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
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */


#include "InputOutput/FileReader.h"

#include <iostream>
#include <vector>
#include <string>

#include "InputOutput/FileUtils.h"
#include "Collections/StringBuffer.h"
#include "Exceptions/FileOpenException.h"
// #include "Engine/Area.h"


FileReader::FileReader(const FileName& fileName)
    : fileName(fileName),
      data(false),
      section(false),
      endOfFile(false),
      sectionName() {
  inputFile = 0;
  lineNumber = 0;
}

FileReader::~FileReader() {
  if (inputFile != 0) {
    dynamic_cast<FileUtils::paloifstream *>(inputFile)->close();
    delete inputFile;
  }
}

FileReader *FileReader::getFileReader(const FileName& fileName) {
  return new FileReader(fileName);
}

bool FileReader::openFile(bool throwError, bool skipMessage) {
  inputFile = FileUtils::newIfstream(fileName.fullPath());

  if (inputFile == 0) {
    if (throwError) {
      LOG(WARNING) << "could not read from file '" << fileName.fullPath()
                      << "'" << " (" << strerror(errno) << ")" ;
      throw FileOpenException("could not open file for reading",
                              fileName.fullPath());
    } else {
      if (!skipMessage) {
        DLOG(INFO) << "file not found: '" << fileName.fullPath() << "'"
                      << " (" << strerror(errno) << ")" ;
      }
      return false;
    }
  }

  lineNumber = 0;

  // read first line
  nextLine();

  return true;
}

void FileReader::nextLine(bool strip) {
  data = false;
  section = false;

  string line = "";

  while (line.empty() || line[0] == '#') {
    if (inputFile->bad() || inputFile->eof()) {
      // IO error or end of file
      endOfFile = true;
      return;
    }

    lineNumber++;

    getline(*inputFile, line);

    string::size_type invalidCharPos = 0;  // CTRL+Z removing
    while ((invalidCharPos = line.find(static_cast<char>(26), invalidCharPos)) != line.npos) {
      line.erase(invalidCharPos, 1);
    }

    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }

    if (strip) {
      while (!line.empty()
          && (line[line.size() - 1] == ' ' || line[line.size() - 1] == '\t')) {
        line.erase(line.size() - 1);
      }
    }
  }

  if (line[0] == '[') {
    processSection(line);
  } else {
    processDataLine(line);
  }
}

void FileReader::getRaw(char *p, streamsize size) {
  inputFile->read(p, size);
  if (inputFile->gcount() != size) {
    throw FileOpenException("error during reading", fileName.fullPath());
  }
}

void FileReader::processSection(const string& line) {
  size_t end = line.find("]", 1);
  if (end != string::npos) {
    section = true;
    sectionName = line.substr(1, end - 1);
  }
}

void FileReader::processDataLine(string& line) {
  dataLine.clear();
  splitString(line, &dataLine, ';', true);
  data = true;
}

void FileReader::splitString(string& line, vector<string>* elements,
                                char seperator, bool readNextLine) {
  if (line.empty()) {
    return;
  }

  string s = "";
  bool first = true;
  bool escaped = false;
  bool endFound = false;

  while (!endFound) {
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

    if (!first && readNextLine) {
      // error or string has a new line
      getline(*inputFile, line);

      string::size_type invalidCharPos = 0;  // CTRL+Z removing
      while ((invalidCharPos = line.find(static_cast<char>(26), invalidCharPos)) != line.npos) {
        line.erase(invalidCharPos, 1);
      }

      lineNumber++;

      if (!line.empty() && line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1);
      }

      if (inputFile->eof()) {
        // error
        elements->push_back(s);
        endFound = true;
      } else {
        s += '\n';
      }
    } else {
      elements->push_back(s);
      endFound = true;
    }
  }
}

const string& FileReader::getDataString(int num) const {
  static const string empty;

  if (num >= 0 && (size_t) num < dataLine.size()) {
    return dataLine[num];
  }

  return empty;
}

int32_t FileReader::getDataInteger(int num, int defaultValue) const {
  string x = getDataString(num);

  if (x == "") {
    return defaultValue;
  }

  char *p;
  int32_t result = strtol(x.c_str(), &p, 10);

  if (*p != '\0') {
    return defaultValue;
  }

  return result;
}

double FileReader::getDataDouble(int num) const {
  string x = getDataString(num);
  char *p;
  double result = strtod(x.c_str(), &p);

  if (*p != '\0' || isnanLocal(result)) {
    return 0.0;
  }

  return result;
}

bool FileReader::getDataBool(int num, bool defaultValue) const {
  string x = getDataString(num);
  if (x == "1") {
    return true;
  } else if (x == "") {
    return defaultValue;
  }
  return false;
}

const vector<string> FileReader::getDataStrings(int num, char separator) {
  if ((IdentifierType)num == NO_IDENTIFIER) {
    return dataLine;
  }

  string x = getDataString(num);
  vector<string> result;
  splitString(x, &result, separator);

  return result;
}

const vector<int> FileReader::getDataIntegers(int num) {
  const vector<string> stringList = getDataStrings(num, ',');
  vector<int> result;
  char *p;

  for (vector<string>::const_iterator i = stringList.begin();
      i != stringList.end(); ++i) {
    int li = strtol((*i).c_str(), &p, 10);

    if (*p != '\0') {
      result.push_back(0);
    } else {
      result.push_back(li);
    }
  }

  return result;
}

const IdentifiersType FileReader::getDataIdentifiers(int num) {
  const vector<string> stringList = getDataStrings(num, ',');
  IdentifiersType result;
  char *p;

  for (vector<string>::const_iterator i = stringList.begin();
      i != stringList.end(); ++i) {
    int li = strtol((*i).c_str(), &p, 10);

    if (*p != '\0') {
      result.push_back(0);
    } else {
      result.push_back(li);
    }
  }

  return result;
}

const vector<double> FileReader::getDataDoubles(int num) {
  const vector<string> stringList = getDataStrings(num, ',');
  vector<double> result;
  char *p;

  for (vector<string>::const_iterator i = stringList.begin();
      i != stringList.end(); ++i) {
    double d = strtod((*i).c_str(), &p);

    if (*p != '\0') {
      result.push_back(0);
    } else {
      result.push_back(d);
    }
  }

  return result;
}

