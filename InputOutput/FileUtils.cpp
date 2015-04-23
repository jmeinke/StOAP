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
 * \author Martin Jakl, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Dolezal, qBicon s.r.o., Hradec Kralove, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#include "InputOutput/FileUtils.h"

#include <unistd.h>
#include <utime.h>
#include <vector>
#include <string>

#include "Exceptions/ErrorException.h"
#include "Exceptions/ParameterException.h"


FileUtils::paloifstream *FileUtils::newIfstream(const string& filename) {
  paloifstream *s = new paloifstream(filename.c_str());
  if (s && !*s) {
    delete s;
    return 0;
  }
  return s;
}

FileUtils::paloofstream *FileUtils::newOfstream(const string& filename) {
  paloofstream *s = new paloofstream(filename.c_str());
  if (s && !*s) {
    delete s;
    return 0;
  }
  return s;
}

FileUtils::paloofstream *FileUtils::newOfstreamAppend(const string& filename) {
  paloofstream *s = new paloofstream(filename.c_str(), ios::app);
  if (s && !*s) {
    delete s;
    return 0;
  }
  return s;
}

bool FileUtils::isReadable(const FileName& fileName) {
  FILE* file = fopen(fileName.fullPath().c_str(), "r");

  if (file == 0) {
    return false;
  } else {
    fclose(file);
    return true;
  }
}

bool FileUtils::remove(const FileName& fileName) {
  int result = std::remove(fileName.fullPath().c_str());
  return (result != 0) ? false : true;
}

bool FileUtils::rename(const FileName& oldName, const FileName& newName) {
  int result = std::rename(oldName.fullPath().c_str(),
                           newName.fullPath().c_str());
  return (result != 0) ? false : true;
}

bool FileUtils::renameDirectory(const FileName& oldName,
                                const FileName& newName) {
  int result = std::rename(oldName.path.c_str(), newName.path.c_str());
  return (result != 0) ? false : true;
}

bool FileUtils::removeDirectory(const FileName& name) {
  int result = rmdir(name.path.c_str());
  return (result != 0) ? false : true;
}

bool FileUtils::createDirectory(const string& name) {
  int result = mkdir(name.c_str(), 0777);

  if (result != 0 && errno == EEXIST && isDirectory(name)) {
    result = 0;
  }

  return (result != 0) ? false : true;
}

bool FileUtils::copyFile(const std::string& fromFile,
                         const std::string &toFile) {
  throw ErrorException(ErrorException::ERROR_COPY_FAILED,
                       "file copy not implemented");
  // return true;
}

bool FileUtils::copyDirectory(std::string fromDir, std::string toDir) {
  if (fromDir.empty()) {
    throw ErrorException(ErrorException::ERROR_COPY_FAILED,
                         "source folder not specified");
  }
  if (toDir.empty()) {
    throw ErrorException(ErrorException::ERROR_COPY_FAILED,
                         "target folder not specified");
  }
  if (fromDir.at(fromDir.size() - 1) != '/') {
    fromDir += "/";
  }
  if (toDir.at(toDir.size() - 1) != '/') {
    toDir += "/";
  }
  if (isDirectory(toDir)) {
    throw ErrorException(ErrorException::ERROR_COPY_FAILED,
                         "target folder already exists");
  }
  if (!createDirectory(toDir)) {
    throw ErrorException(ErrorException::ERROR_COPY_FAILED,
                         "target folder can't be created");
  }
  vector<string> files = listFiles(fromDir);
  if (files.empty()) {
    throw ErrorException(ErrorException::ERROR_COPY_FAILED,
                         "can't read file list to backup");
  }
  for (vector<string>::iterator it = files.begin(); it != files.end(); ++it) {
    if (!copyFile(fromDir + *it, toDir + *it)) {
      throw ParameterException(ErrorException::ERROR_COPY_FAILED,
                               "database folder can't be copied", "filename",
                               *it);
    }
  }

  return true;
}

vector<string> FileUtils::listFiles(const string& directory) {
  vector<string> result;

  DIR * d = opendir(directory.c_str());

  if (d == 0) {
    return result;
  }

  struct dirent * de = readdir(d);

  while (de != 0) {
    if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
      result.push_back(de->d_name);
    }

    de = readdir(d);
  }

  closedir(d);

  return result;
}

bool FileUtils::isDirectory(const string& path) {
  struct stat stbuf;
  stat(path.c_str(), &stbuf);

  return (stbuf.st_mode & S_IFMT) == S_IFDIR;
}

bool FileUtils::isRegularFile(const string& path) {
  struct stat stbuf;
  stat(path.c_str(), &stbuf);

  return (stbuf.st_mode & S_IFMT) == S_IFREG;
}
bool FileUtils::isRegularFile(const string& path, const string& name) {
  string fileName = path + "/" + name;

  return isRegularFile(fileName);
}

bool FileUtils::createFile(const string &dirName, const string &name) {
  string fileName;
  int fd;
  fileName = dirName + "/" + name;

  // remove(fileName);

  fd = open(fileName.c_str(), O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    return false;
  }
  close(fd);

  return true;
}
