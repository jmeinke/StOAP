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


#ifndef STOAP_OLAP_CUBE_H_
#define STOAP_OLAP_CUBE_H_ 1

#include <string>
#include <vector>

#include "Olap.h"
#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "InputOutput/FileUtils.h"
#include "Olap/CellPath.h"
#include "Olap/DoubleStorage.h"
#include "Olap/Dimension.h"
#include "Olap/Element.h"
#include "Olap/Area.h"
#include "Exceptions/ParameterException.h"
#include "Exceptions/FileFormatException.h"
#include "Exceptions/FileOpenException.h"


// This is the class defining an OLAP cube:
// It is responsible for storing the cube data
// and setting/getting the values at specific paths
// A cube path consists of the selected element
// for each of the cubes dimensions.

class Cube {

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Constructor creating an empty cube
  ////////////////////////////////////////////////////////////////////////////////

  Cube(const string& cubeName, const FileName& cubeFileName,
       vector<Dimension*>* dimensions);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Destructor
  ////////////////////////////////////////////////////////////////////////////////

  ~Cube();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Loads the cubes values from the given file
  ////////////////////////////////////////////////////////////////////////////////

  void loadCube();
  void loadCubeCells(FileReader* file);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Gets the cube dimension list
  ////////////////////////////////////////////////////////////////////////////////

  const vector<Dimension*>* getDimensions() const {
    return &_dimensions;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Returns the number of filled numeric cells
  ////////////////////////////////////////////////////////////////////////////////

  size_t sizeFilledCells();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief Returns the maximum number of all numeric cells
  ////////////////////////////////////////////////////////////////////////////////

  size_t sizeMaxCells();

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets NUMERIC cell value
  ////////////////////////////////////////////////////////////////////////////////

  DoubleStorage* getStorage() {
    return storage;
  }

  Area* getFilledArea() {
    return filledCellArea;
  }

  const string& getName() {
    return name;
  }

 private:
  size_t getFileLines(const char *file);
  size_t dataLines;

 protected:
  string name;  // user specified name of the cube
  FileName* fileName;  // file name of the cube
  DoubleStorage* storage;  // cell storage for NUMERIC values
  vector<Dimension*> _dimensions;  // list of dimensions used for the cube
  vector<size_t> dimensionsSize;  // list of dimension sizes
  Area* filledCellArea;
};

#endif  // STOAP_OLAP_CUBE_H_
