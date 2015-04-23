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
 * \author Zurab Khadikov, Jedox AG, Freiburg, Germany
 * \author Tobias Lauer, Jedox AG, Freiburg, Germany
 * \author Christoffer Anselm, Jedox AG, Freiburg, Germany
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */


#ifndef STOAP_OLAP_ELEMENT_H_
#define STOAP_OLAP_ELEMENT_H_ 1

#include <string>
#include <map>

#include "Olap.h"
#include "Collections/WeightedSet.h"

class Dimension;

////////////////////////////////////////////////////////////////////////////////
/// @brief palo element
////////////////////////////////////////////////////////////////////////////////

class Element {
 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief creates new element
  ////////////////////////////////////////////////////////////////////////////////

  Element()
      : identifier(0),
        position(0),
        type(UNDEFINED),
        level(0),
        depth(0),
        numBaseElements(0) {
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief creates new element with a identifier
  ////////////////////////////////////////////////////////////////////////////////

  explicit Element(IdentifierType identifier)
      : identifier(identifier),
        position(0),
        type(UNDEFINED),
        level(0),
        depth(0) {
  }

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets identifier of element
  ////////////////////////////////////////////////////////////////////////////////

  void setIdentifier(IdentifierType identifier) {
    this->identifier = identifier;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets identifier of element
  ////////////////////////////////////////////////////////////////////////////////

  IdentifierType getIdentifier() const {
    return identifier;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets or rename element
  ////////////////////////////////////////////////////////////////////////////////

  void setName(const string& name) {
    this->name = name;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets element name
  ////////////////////////////////////////////////////////////////////////////////

  const string& getName() const {
    return name;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets element type
  ////////////////////////////////////////////////////////////////////////////////

  void setElementType(ElementType type) {
    this->type = type;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets element type
  ////////////////////////////////////////////////////////////////////////////////

  ElementType getElementType() const {
    return (ElementType) type;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets position
  ////////////////////////////////////////////////////////////////////////////////

  void setPosition(PositionType position) {
    this->position = position;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets position
  ////////////////////////////////////////////////////////////////////////////////

  PositionType getPosition() const {
    return position;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets level
  ////////////////////////////////////////////////////////////////////////////////

  void setLevel(LevelType level) {
    this->level = level;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets level
  ////////////////////////////////////////////////////////////////////////////////

  LevelType getLevel(Dimension* dimension);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets indent
  ////////////////////////////////////////////////////////////////////////////////

  void setIndent(IndentType indent) {
    this->indent = indent;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief gets indent
  ////////////////////////////////////////////////////////////////////////////////

  IndentType getIndent(Dimension* dimension);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief sets depth
  ////////////////////////////////////////////////////////////////////////////////

  void setDepth(DepthType depth) {
    this->depth = depth;
  }


 private:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the identifier of the element
  ////////////////////////////////////////////////////////////////////////////////

  IdentifierType identifier;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the name of the element
  ////////////////////////////////////////////////////////////////////////////////

  string name;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the position of the element in the list of elements
  ////////////////////////////////////////////////////////////////////////////////

  PositionType position;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the type of the element
  ////////////////////////////////////////////////////////////////////////////////

  unsigned type :3;

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the level of the element
  ////////////////////////////////////////////////////////////////////////////////

  unsigned level :9;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the depth of the element
  ////////////////////////////////////////////////////////////////////////////////

  unsigned depth :9;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief the indent of the element
  ////////////////////////////////////////////////////////////////////////////////

  unsigned indent :9;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief all base elements
  /// all base elements and their weight
  ////////////////////////////////////////////////////////////////////////////////

  map<IdentifierType, double> baseElements;

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief number (weight != 0.0) of base elements
  ////////////////////////////////////////////////////////////////////////////////

  size_t numBaseElements;
};

#endif  // STOAP_OLAP_ELEMENT_H_
