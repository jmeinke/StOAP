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
 */

#include "Olap/Cube.h"

#include <algorithm>
#include <string>
#include <vector>
#include <utility>

Cube::Cube(const string& cubeName, const FileName& cubeFileName,
           vector<Dimension*>* dimensions) {
  name = cubeName;
  fileName = new FileName(cubeFileName);
  _dimensions = *dimensions;
  if (_dimensions.empty()) {
    throw ParameterException(ErrorException::ERROR_CUBE_EMPTY,
                             "missing dimensions", "dimensions", "");
  }

  dataLines = 0;
  storage = new DoubleStorage();
  filledCellArea = new Area(0);
}

Cube::~Cube() {
  delete storage;

  if (fileName != 0) {
    delete fileName;
  }
}

void Cube::loadCube() {
  if (fileName == 0) {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "cube file name not set");
  }

  if (!FileUtils::isReadable(*fileName)
      && FileUtils::isReadable(FileName(*fileName, "tmp"))) {
    LOG(WARNING)<< "using temp file for cube ";

    // rename temp file
    FileUtils::rename(FileName(*fileName, "tmp"), *fileName);
  }

  FileName fn(*fileName, "csv");
  dataLines = getFileLines(fn.fullPath().c_str());
  LOG(INFO) << "There are approximately " << dataLines << " values to load.";

  // prepare the storage container
  // storage->m.clear();
  // storage->m.min_load_factor(0.9);
  // storage->m.max_load_factor(1.0);
  storage->m.resize(dataLines);

  FileReader* file(FileReader::getFileReader(fn));
  file->openFile(true, false);

  // load overview
  vector<int> sizes;

  if (file->isSectionLine() && file->getSection() == "CUBE") {
    file->nextLine();

    while (file->isDataLine()) {
      sizes = file->getDataIntegers(1);

      file->nextLine();
    }
  } else {
    throw FileFormatException("Section 'CUBE' not found.", file);
  }

  dimensionsSize.clear();
  dimensionsSize.assign(sizes.begin(), sizes.end());

  if (dimensionsSize.size() != _dimensions.size()) {
    LOG(ERROR)<< "size of dimension list and size of size list is not equal";
    throw FileFormatException("list of dimension is corrupted", file);
  }

  // and cell values
  if (file->isSectionLine()) {
    loadCubeCells(file);
  } else {
    LOG(WARNING)<< "section line not found for cube '" << name << "'";
  }

  delete file;
}

size_t Cube::getFileLines(const char *file) {
  FILE *fp = fopen(file, "r");
  size_t lineCount = 0;

  if (fp == NULL) {
    fclose(fp);
    return 0;
  }
  int c;
  while ((c = fgetc(fp)) != EOF) {
    if (c == '\n')
      lineCount++;
  }

  fclose(fp);
  return lineCount;
}

void Cube::loadCubeCells(FileReader* file) {
  size_t size = _dimensions.size();

  // vector<IdentifiersType> filledSources;

  if (file->isSectionLine() && file->getSection() == "NUMERIC") {
    file->nextLine();

    while (file->isDataLine()) {
      IdentifiersType ids = file->getDataIdentifiers(0);
      double d = file->getDataDouble(1);

      if (size != ids.size()) {
        LOG(ERROR)<< "error in numeric cell path of cube '" << name << "'"
        ;
        throw FileFormatException("error in numeric cell path", file);
      }

      bool failed = false;
      for (size_t i = 0; i < size; i++) {
        Element *elem = _dimensions[i]->lookupElement(ids[i]);
        if (!elem || elem->getElementType() == CONSOLIDATED) {
          if (!elem) {
            DLOG(INFO)<< "error in numeric cell path of cube '" << name
            << "', skipping entry " << ids[i] << " in dimension '"
            << _dimensions[i]->getName() << "'.";
          } else {
            DLOG(INFO) << "consolidation in numeric cell path of cube '"
            << name << "', skipping entry " << ids[i]
            << " in dimension '" << _dimensions[i]->getName()
            << "'.";
          }

          failed = true;
          break;
        }
      }

      if (!failed) {
        // filledSources.push_back(ids);
        storage->setValue(&ids, d);
      }

      file->nextLine();
    }
  } else {
    throw FileFormatException("section 'NUMERIC' not found", file);
  }

  // assign filledSourceArea with the ids
  // filledCellArea = new Area(filledSources);

  // attempt to free some memory
  storage->m.resize(0);

  // Don't attempt to load the string section, if it exists.
  /*
   if (file->isSectionLine() && file->getSection() == "STRING") {
   file->nextLine();

   // TODO(jmeinke): Test loading a cube which holds some string values
   while (file->isDataLine()) {
   file->nextLine();
   }
   } else {
   throw FileFormatException("section 'STRING' not found", file);
   }
   */
}

size_t Cube::sizeFilledCells() {
  return storage->m.size();
}

// TODO(jmeinke): maybe take into account the number of consolidated cells
size_t Cube::sizeMaxCells() {
  size_t cells = 1;
  for (auto i = dimensionsSize.begin(); i != dimensionsSize.end(); i++) {
    cells *= *i;
  }
  return cells;
}

/*
 *
void Cube::getCellValue(CellPath* cellPath, CellValueType& cellValue,
                        bool* found) {
  // cellValue.isBase = cellPath->isBase();
  // look up a numeric path in the double storage
  if (cellPath->isBase()) {
    double* value = storage->getValue(cellPath->getPathIdentifier());
    if (value == NULL) {
      cellValue.doubleValue = 0.0;
      (*found) = false;
      return;
    } else {
      cellValue.doubleValue = *value;
      (*found) = true;
      return;
    }
  } else {
    // look up a consolidation in the double storage
    // do not compute values for an emtpy cube
    throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                             "consolidated element (NYI)", "isBaseCell",
                             static_cast<int>(isBase));
  }

  // (*found) = false;
}

*/
