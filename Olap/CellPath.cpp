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

#include "Olap/CellPath.h"

#include <string>
#include <vector>

#include "Stoap/AggregationEnvironment.h"
#include "Olap/Cube.h"
#include "Olap/Dimension.h"
#include "Collections/StringBuffer.h"
#include "Exceptions/ErrorException.h"

// The CellPath constructor is called quite often, so it has to be as fast as possible.
CellPath::CellPath(const IdentifiersType* identifiers)
    : pathIdentifiers(identifiers),
      base(true) {

  const vector<Dimension*>* dimensions = AggrEnv::instance().getCube()->getDimensions();

  // the number of dimensions has to match the number of identifiers in the given path
  if (identifiers->size() != dimensions->size()) {
    std::ostringstream stringStream;
    stringStream << "number of dimensions does not match number of identifiers: " << this->toString();
    throw ErrorException(
        ErrorException::ERROR_INVALID_COORDINATES,
        stringStream.str());
  }

  // convert identifiers into elements
  IdentifiersType::const_iterator identifiersIter = identifiers->begin();
  vector<Dimension*>::const_iterator dimensionIter = dimensions->begin();
  // PathType::iterator pathIter = pathElements.begin();

  for (; identifiersIter != identifiers->end();
      ++identifiersIter, ++dimensionIter) {
    // Dimension* dimension = *dimensionIter;

    // find element from identifier
    Element* element = (*dimensionIter)->lookupElement(*identifiersIter);
    if (!element) {
      std::ostringstream stringStream;
      stringStream << "Element " << (*identifiersIter) << " was not found in dimension " << (*dimensionIter)->getName();
      throw ErrorException(
          ErrorException::ERROR_INVALID_COORDINATES,
          stringStream.str());
    }

    // copy values to member variables
    // *pathIter = element;

    // compute path type
    if (element->getElementType() == CONSOLIDATED) {
      base = false;
      // break;
    }
  }
}

string CellPath::toString() {
  StringBuffer sb;

  for (size_t i = 0; i < pathIdentifiers->size(); i++) {
    if (i > 0)
      sb.appendChar(',');
    sb.appendInteger(pathIdentifiers->at(i));
  }

  string result = sb.c_str();
  return result;
}

