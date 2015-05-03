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


#ifndef STOAP_OLAP_CELLPATH_H_
#define STOAP_OLAP_CELLPATH_H_ 1

#include <vector>
#include <string>

#include "Olap.h"
#include "Olap/Element.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief OLAP cell path
///
/// A cell path consists of a list of elements and a path type.
////////////////////////////////////////////////////////////////////////////////


class CellPath {
 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief creates cell path from list of identifiers
  ///
  /// @throws ParameterException on double or missing dimensions
  ///
  /// The object constructor verifies the elements and computes the path type.
  /// If the list of elements is not a suitable cell path for the given cube
  /// the constructor throws a ParameterException.
  ////////////////////////////////////////////////////////////////////////////////

  CellPath(const IdentifiersType* identifiers);

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief get type of path
  ////////////////////////////////////////////////////////////////////////////////

  /* ElementType getPathType() const {
    return pathType;
  } */

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief get path Elements
  ////////////////////////////////////////////////////////////////////////////////

  /* const PathType* getPathElements() const {
    return &pathElements;
  } */

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief get identifiers of path elements
  ////////////////////////////////////////////////////////////////////////////////

  const IdentifiersType* getPathIdentifier() const {
    return pathIdentifiers;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief is base path
  ////////////////////////////////////////////////////////////////////////////////

  bool isBase() const {
    return base;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief is base path
  ////////////////////////////////////////////////////////////////////////////////

  string toString();

 private:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief list of path elements
  ////////////////////////////////////////////////////////////////////////////////

  // PathType pathElements;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief list of identifiers (identifiers of the elements)
  ////////////////////////////////////////////////////////////////////////////////

  const IdentifiersType* pathIdentifiers;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief true if path contains only base elements
  ////////////////////////////////////////////////////////////////////////////////

  bool base;
};

#endif  // STOAP_OLAP_CELLPATH_H_
