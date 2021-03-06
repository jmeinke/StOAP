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

#include "InputOutput/FileWriter.h"

#include <stdio.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/file.h>

extern "C" {
#include <sys/stat.h>
}

#include <vector>
#include <iostream>

#include "Exceptions/FileOpenException.h"
#include "Collections/StringBuffer.h"
#include "InputOutput/FileUtils.h"
// #include "Olap/Context.h"
// #include "Engine/Area.h"

FileWriter::FileWriter(const FileName& fileName):
      outputFile(0) {
  setFirstValue(true);
}

FileWriter::~FileWriter() {
  closeFile();
}

FileWriter *FileWriter::getFileWriter(const FileName& fileName) {
  return new FileWriter(fileName);
}

void FileWriter::openFile(bool append) {
  if (append) {
    outputFile = FileUtils::newOfstreamAppend(fileName.fullPath());
  } else {
    outputFile = FileUtils::newOfstream(fileName.fullPath());
  }

  if (outputFile == 0) {
    LOG(WARNING) << "could not write to file '" << fileName.fullPath() << "'"
                    << " (" << strerror(errno) << ")" ;
    throw FileOpenException("could not open file for writing",
                            fileName.fullPath());
  }
}

void FileWriter::closeFile() {
  if (outputFile != 0) {
    nextLine();
    writeBuffer();
    FileUtils::paloofstream *o =
        dynamic_cast<FileUtils::paloofstream *>(outputFile);
    if (o) {
      o->close();
    }
    delete outputFile;
    outputFile = 0;
  }
}

void FileWriter::deleteFile(const FileName& fileName) {
  if (!FileUtils::remove(fileName)) {
    throw FileOpenException("could not delete file", fileName.fullPath());
  }
}

int32_t FileWriter::getFileSize(const FileName& fileName) {
  struct stat fileStat;
  int result = stat(fileName.fullPath().c_str(), &fileStat);

  if (result < 0) {
    return 0;
  }

  return (int32_t) fileStat.st_size;
}

void FileWriter::appendComment(const string& value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  if (!isFirstValue()) {
    nextLine();
  }

  // delete line feeds
  size_t pos = value.find_first_of("\r\n");

  if (pos != string::npos) {
    string copy = value;
    size_t pos = copy.find_first_of("\r\n");

    while (pos != string::npos) {
      copy[pos] = ' ';
      pos = copy.find_first_of("\r\n", pos);
    }

    *outputFile << "# " << copy;
  } else {
    *outputFile << "# " << value;
  }

  setFirstValue(false);

  nextLine();
}

void FileWriter::appendSection(const string& value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << "[" << value << "]";
  setFirstValue(false);
  nextLine();
}

void FileWriter::appendString(const string& value, char terminator) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << value;
  if (terminator) {
    *outputFile << terminator;
  }
  setFirstValue(false);
}

void FileWriter::appendEscapeString(const string& value, char terminator) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << escapeString(value);
  if (terminator) {
    *outputFile << terminator;
  }
  setFirstValue(false);
}

void FileWriter::appendEscapeStrings(const vector<string>* value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  bool first = true;

  for (vector<string>::const_iterator i = value->begin(); i != value->end();
      ++i) {
    if (first) {
      first = false;
    } else {
      *outputFile << ",";
    }
    *outputFile << escapeString(*i);
  }

  *outputFile << ";";

  setFirstValue(false);
}

void FileWriter::appendInteger(const int32_t value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << value << ";";
  setFirstValue(false);
}

void FileWriter::appendIntegers(const vector<int32_t>* value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  bool first = true;

  for (vector<int32_t>::const_iterator i = value->begin(); i != value->end();
      ++i) {
    if (first) {
      first = false;
    } else {
      *outputFile << ",";
    }
    *outputFile << *i;
  }

  *outputFile << ";";

  setFirstValue(false);
}

void FileWriter::appendIdentifier(const IdentifierType value,
                                     char terminator) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << value;
  if (terminator) {
    *outputFile << terminator;
  }

  setFirstValue(false);
}

void FileWriter::appendDouble(const double value, char terminator) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  outputFile->precision(15);

  *outputFile << value;
  if (terminator) {
    *outputFile << terminator;
  }
  setFirstValue(false);
}

void FileWriter::appendDoubles(const vector<double>* value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  bool first = true;

  for (vector<double>::const_iterator i = value->begin(); i != value->end();
      ++i) {
    if (first) {
      first = false;
    } else {
      *outputFile << ",";
    }
    *outputFile << *i;
  }

  *outputFile << ";";

  setFirstValue(false);
}

void FileWriter::appendBool(const bool value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  if (value) {
    *outputFile << "1;";
  } else {
    *outputFile << "0;";
  }

  setFirstValue(false);
}

void FileWriter::appendTimeStamp() {
  timeval tv;
  gettimeofday(&tv, 0);
  appendTimeStamp(tv);
}

void FileWriter::appendTimeStamp(timeval &tv) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << tv.tv_sec << "." << tv.tv_usec << ";";
  setFirstValue(false);
}

void FileWriter::nextLine() {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  if (!isFirstValue()) {
    *outputFile << endl;

    outputFile->flush();
  }

  setFirstValue(true);
}

void FileWriter::appendRaw(const string& value) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  *outputFile << value;
  outputFile->flush();
  setFirstValue(true);
}

void FileWriter::appendRaw(const char *p, streamsize size) {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  outputFile->write(p, size);
}

void FileWriter::writeBuffer() {
  if (outputFile == 0) {
    LOG(ERROR) << "file writer is closed" ;
    return;
  }

  outputFile->flush();
}

string FileWriter::escapeString(const string& text) {
  StringBuffer sb;
  size_t begin = 0;
  size_t end = text.find("\"");

  sb.appendText("\"");

  while (end != string::npos) {

    sb.appendText(text.substr(begin, end - begin));
    sb.appendText("\"\"");

    begin = end + 1;
    end = text.find("\"", begin);
  }

  sb.appendText(text.substr(begin, end - begin));
  sb.appendText("\"");
  string result = sb.c_str();
  return result;
}

bool FileWriter::isFirstValue() {
  return isFirst;
}

void FileWriter::setFirstValue(bool v) {
  isFirst = v;
}

void FileWriter::appendArray(bool bFirst, IdentifierType value,
                                char separator) {
  if (outputFile == 0) {
    return;
  }
  if (!bFirst) {
    *outputFile << separator;
  }
  *outputFile << value;
}

void FileWriter::appendArrayFinish(char terminator) {
  if (outputFile == 0) {
    return;
  }
  *outputFile << terminator;
  setFirstValue(false);
}


DirLock::DirLock(const string &dirName) {
  fileName = dirName + "/palo.lock";
  fd = open(fileName.c_str(), O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    LOG(ERROR) << "Directory is already used by another palo server process."
                  ;
    throw FileOpenException("couldn't lock directory", dirName);
  }
  if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
    LOG(ERROR) << "Directory is already used by another palo server process."
                  ;
    throw FileOpenException("couldn't lock directory", dirName);
  }
}

DirLock::~DirLock() {
  if (fd != -1) {
    close(fd);
  }
  remove(fileName.c_str());
}
