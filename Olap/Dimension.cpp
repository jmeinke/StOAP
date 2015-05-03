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
 * \author Christoffer Anselm, Jedox AG, Freiburg, Germany
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#include "Olap/Dimension.h"

#include <iostream>
#include <deque>
#include <string>
#include <vector>
#include <set>

#include "Collections/DeleteObject.h"
#include "Collections/StringBuffer.h"
#include "InputOutput/FileReader.h"
#include "InputOutput/FileWriter.h"
#include "Olap/Cube.h"
#include "Exceptions/FileFormatException.h"
#include "Exceptions/ParameterException.h"

ElementsWeightType Dimension::emptyChildren;
Dimension::ParentsType Dimension::emptyParents;

////////////////////////////////////////////////////////////////////////////////
// constructors and destructors
////////////////////////////////////////////////////////////////////////////////

Dimension::Dimension(IdentifierType identifier, const string& name)
    : parentToChildren(1000, Parent2ChildrenDesc()),
      childToParents(1000, Child2ParentsDesc()),
      nameToElement(1000, Name2ElementDesc()),
      positionToElement(1000, Position2ElementDesc()) {
  this->identifier = identifier;
  this->name = name;
  numElements = 0;
  isValidLevel = false;
  maxLevel = 0;
  maxIndent = 0;
  maxDepth = 0;
  isValidBaseElements = false;
  isValidSortedElements = false;
}

Dimension::~Dimension() {
  for_each(elements.begin(), elements.end(), DeleteObject());
  parentToChildren.clearAndDelete();
  childToParents.clearAndDelete();
}

////////////////////////////////////////////////////////////////////////////////
// functions to load a dimension
////////////////////////////////////////////////////////////////////////////////

void Dimension::loadDimension(FileReader* file) {
  DLOG(WARNING) << "Loading dimension from '" << name << "'. " ;

  // clear old dimension elements
  clearElements();

  // load dimension and elements from file
  uint32_t sizeElements = loadOverview(file);
  loadElements(file, sizeElements);

  isValidLevel = false;
  isValidBaseElements = false;
  isValidSortedElements = false;
}

uint32_t Dimension::loadOverview(FileReader* file) {
  const string section = "DIMENSION "
      + StringUtils::convertToString(identifier);
  DLOG(WARNING) << "Loading dimension info from '" << section << "'. "
                ;
  uint32_t sizeElements = 0;

  bool dimensionExists = false;
  while (!dimensionExists && !file->isEndOfFile()) {
    if (file->isSectionLine() && file->getSection() == section) {
      dimensionExists = true;
      file->nextLine();

      if (file->isDataLine()) {
        uint32_t level = file->getDataInteger(2);
        uint32_t indent = file->getDataInteger(3);
        uint32_t depth = file->getDataInteger(4);

        sizeElements = file->getDataInteger(5);
        elements.resize(sizeElements, 0);

        maxLevel = level;
        maxIndent = indent;
        maxDepth = depth;

        file->nextLine();
      } else {
        throw FileFormatException(
            "Could not read dimension information.",
            file);
      }
      break;
    }
    file->nextLine();
  }
  if (!dimensionExists) {
    throw FileFormatException(
        "Section '" + section + "' not found.",
        file);
  }

  return sizeElements;
}

void Dimension::loadElements(FileReader* file, uint32_t sizeElements) {
  const string section = "ELEMENTS DIMENSION "
      + StringUtils::convertToString(identifier);
  DLOG(WARNING) << "Load elements from '" << section << "'. " ;

  // construct all elements, unsused elements are cleared later
  for (IdentifierType i = 0; i < sizeElements; i++) {
    elements[i] = new Element(i);
  }

  // load elements
  size_t count = 0;

  if (file->isSectionLine() && file->getSection() == section) {
    file->nextLine();

    while (file->isDataLine()) {
      loadElement(file, sizeElements);

      count++;
      file->nextLine();
    }
  } else {
    throw FileFormatException("Section '" + section + "' not found.", file);
  }

  // clear unused elements
  for (IdentifierType i = 0; i < sizeElements; i++) {
    if (elements[i]->getElementType() == UNDEFINED) {
      delete elements[i];
      elements[i] = 0;
    }
  }

  numElements = count;

  // calculate the number of bits for bin packing
  IdentifierType maxId = getMaximalIdentifier();
  // and create a bitmask
  dimPos = 0;
  dimMask = 1;
  for (uint64_t i = 1; i <= maxId; i<<=1) {
    dimPos++;
    dimMask = dimMask | i;
  }
  // LOG(ERROR) << "maxId = " << maxId << " dimPos = " << dimPos << "dimMask = " << dimMask;
}

void Dimension::loadElement(FileReader* file, uint32_t sizeElements) {
  IdentifierType id = file->getDataInteger(0);
  string name = file->getDataString(1);
  PositionType pos = file->getDataInteger(2);
  long i = file->getDataInteger(3);
  ElementType type = UNDEFINED;

  switch (i) {
    case 1:
      type = NUMERIC;
      break;
    case 2:
      return;  // string elements are ignored
    case 4:
      type = CONSOLIDATED;
      break;

    default:
      LOG(ERROR) << "Element '" << name << "' has unknown type '" << i << "'";
      throw FileFormatException("element has wrong type", file);
  }

  DLOG(WARNING) << "Loading element '" << name << "' (id " << id
                << ", type " << type << ")." ;

  long level = file->getDataInteger(5);
  long indent = file->getDataInteger(6);
  long depth = file->getDataInteger(7);
  IdentifiersType parents = file->getDataIdentifiers(8);
  IdentifiersType children = file->getDataIdentifiers(9);
  vector<double> weights = file->getDataDoubles(10);

  if (id >= sizeElements) {
    LOG(ERROR) << "element identifier '" << id << "' of element '" << name
                  << "' is greater or equal than maximum (" << sizeElements
                  << ")";
    throw FileFormatException("Wrong element identifier found", file);
  }

  Element * element = elements[id];

  element->setName(name);
  element->setPosition(pos);
  element->setLevel(level);
  element->setIndent(indent);
  element->setDepth(depth);

  // update name and position
  nameToElement.addElement(name, element);
  positionToElement.addElement(pos, element);

  // children to parent
  if (!parents.empty()) {
    loadElementParents(file, element, &parents, sizeElements);
  }

  // parent to children
  if (children.size() != weights.size()) {
    LOG(ERROR) << "Size of children list and size of children weight list is not equal.";
    throw FileFormatException("children size != children weight size", file);
  }

  if (!children.empty()) {
    loadElementChildren(file, element, &children, &weights, sizeElements);
  }

  // check element type for consolidated elements
  if (!children.empty() && type != CONSOLIDATED) {
    type = CONSOLIDATED;
    DLOG(WARNING) << "Loading element '" << name << "', setting type to "
                  << type << ", because element has children." ;
  } else if (children.empty() && type == CONSOLIDATED) {
    type = NUMERIC;
    DLOG(WARNING) << "Loading element '" << name << "' setting type to "
                  << type << ", because element has no children." ;
  }

  element->setElementType(type);
}

void Dimension::loadElementParents(FileReader* file, Element* element,
                                   IdentifiersType* parents,
                                   uint32_t sizeElements) {
  ChildParentsPair * cpp = childToParents.findKey(element);

  if (cpp == 0) {
    cpp = new ChildParentsPair(element);
    childToParents.addElement(cpp);
  }

  ParentsType& pt = cpp->parents;
  pt.resize(parents->size());

  IdentifiersType::const_iterator parentsIter = parents->begin();

  ParentsType::iterator j = pt.begin();

  for (; parentsIter != parents->end(); parentsIter++, j++) {
    IdentifierType id = *parentsIter;

    if (id < sizeElements) {
      *j = elements[id];
    } else {
      LOG(ERROR) << "Parent element identifier '" << identifier
                    << "' of element '" << element->getName()
                    << "' is greater or equal than maximum (" << sizeElements
                    << ")" ;
      throw FileFormatException("Illegal identifier in parents list", file);
    }
  }
}

void Dimension::loadElementChildren(FileReader* file, Element* element,
                                    IdentifiersType* children,
                                    vector<double>* weights,
                                    uint32_t sizeElements) {
  ParentChildrenPair * pcp = parentToChildren.findKey(element);

  if (pcp == 0) {
    pcp = new ParentChildrenPair(element);
    parentToChildren.addElement(pcp);
  }

  ElementsWeightType& ew = pcp->children;
  ew.resize(children->size());

  IdentifiersType::const_iterator childrenIter = children->begin();
  vector<double>::const_iterator weightsIter = weights->begin();

  ElementsWeightType::iterator j = ew.begin();

  for (; childrenIter != children->end(); childrenIter++, weightsIter++, j++) {
    IdentifierType id = *childrenIter;

    if (id < sizeElements) {
      (*j).first = elements[id];
      (*j).second = *weightsIter;
    } else {
      LOG(ERROR) << "Child element identifier '" << identifier
                    << "' of element '" << element->getName()
                    << "' is greater or equal than maximum (" << sizeElements
                    << ")" ;
      throw FileFormatException("Illegal identifier in children list", file);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// getter and setter
////////////////////////////////////////////////////////////////////////////////

void Dimension::checkLevelIndentDepth() {
  if (!isValidLevel) {
    updateLevelIndentDepth();
  }

}

DepthType Dimension::getDepth() {
  if (!isValidLevel) {
    updateLevelIndentDepth();
  }

  return maxDepth;
}

////////////////////////////////////////////////////////////////////////////////
// element functions
////////////////////////////////////////////////////////////////////////////////

void Dimension::clearElements() {
  for_each(elements.begin(), elements.end(), DeleteObject());

  elements.clear();
  sortedElements.clear();
  numElements = 0;

  parentToChildren.clearAndDelete();
  childToParents.clearAndDelete();

  nameToElement.clear();
  positionToElement.clear();

  maxLevel = 0;
  maxIndent = 0;
  maxDepth = 0;

  isValidLevel = true;
  isValidBaseElements = true;
  isValidSortedElements = true;
}

vector<Element*> Dimension::getElements(IdentifierType level) {
  ElementsType result;
  IdentifierType totalElements = (IdentifierType) sizeElements();

  if (level == NO_IDENTIFIER ) {
    result.reserve(totalElements);
  }

  if (!isValidLevel) {
    updateLevelIndentDepth();
  }

  for (vector<Element*>::iterator i = elements.begin(); i != elements.end();
      i++) {
    Element * element = *i;
    if (element == 0) {
      continue;
    } else if (level != NO_IDENTIFIER && element->getLevel(this) != level) {
      continue;
    }
    result.push_back(element);
  }
  return result;
}

IdentifierType Dimension::getMaximalIdentifier() {
  IdentifierType maxId = 0;
  for (vector<Element *>::iterator i = elements.begin(); i != elements.end();
        i++) {
    if (*i == NULL) continue;
    IdentifierType id = (*i)->getIdentifier();
    // cout << id << " >>> " << maxId << endl;
    if (id > maxId) maxId = id;
  }
  return maxId;
}

vector<Element *> Dimension::getBaseElements() {
  if (!isValidLevel) {
    updateLevelIndentDepth();
  }
  vector<Element *> result;
  for (vector<Element *>::iterator i = elements.begin(); i != elements.end();
      i++) {
    Element *element = *i;
    if (!element || element->getElementType() == CONSOLIDATED) {
      continue;
    }
    result.push_back(element);
  }
  return result;
}

const IdentifiersWeightType Dimension::getBaseIWs(Element* parent) {
  IdentifiersWeightType baseElements;

  if (parent->getElementType() == CONSOLIDATED && parent->getLevel(this) > 1) {
    const IdentifiersWeightType children = getChildrenIds(parent);

    for (auto iter = children.begin(); iter != children.end(); iter++) {
      const IdentifiersWeightType subBase = getBaseIWs(
          lookupElement(iter->first));

      baseElements.insert(baseElements.end(), subBase.begin(), subBase.end());
    }
  } else if (parent->getElementType() == CONSOLIDATED
      && parent->getLevel(this) == 1) {
    const IdentifiersWeightType children = getChildrenIds(parent);
    baseElements.insert(baseElements.end(), children.begin(), children.end());

  } else if (parent->getElementType() == NUMERIC) {
    // parent is already a base element => add itself and return
    baseElements.push_back(
        pair<IdentifierType, double>(parent->getIdentifier(), 1.0));
  } else {
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "getBaseIWs contains a bad element");
  }

  return baseElements;
}

const WeightedSet* Dimension::getBaseElements(Element* parent) {
  IdentifiersWeightType baseElements = getBaseIWs(parent);
  WeightedSet* result = new WeightedSet();
  for (auto it = baseElements.begin(); it != baseElements.end(); ++it) {
    if (lookupElement((*it).first)->getElementType() != NUMERIC) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "Consolidated element in base element set!");
    }
    // warning: using pushSorted corrupts the set, which results in wrong aggregation values
    // result->pushSorted((*it).first, (*it).second);
    result->fastAdd((*it).first, (*it).second);
  }
  result->consolidate();

  return result;
}

void Dimension::updateTopologicalSortedElements() {
  // add parents first!

  if (!isValidSortedElements) {

    isValidSortedElements = true;

    set<Element*> knownElements;  // list of known elements
    sortedElements.clear();
    for (vector<Element*>::iterator i = elements.begin(); i != elements.end();
        i++) {
      Element * element = *i;

      set<Element*>::iterator find = knownElements.find(element);
      if (element != 0 && find == knownElements.end()) {
        // add parent
        addParentsToSortedList(element, &knownElements);

        // add element
        sortedElements.push_back(element);
        knownElements.insert(element);
      }
    }
  }
}

void Dimension::addParentsToSortedList(Element* child,
                                       set<Element*>* knownElements) {
  const ParentsType* pt = getParents(child);

  if (pt == 0) {
    return;
  }

  for (ParentsType::const_iterator pti = pt->begin(); pti != pt->end(); pti++) {
    Element* parent = *pti;

    set<Element*>::iterator find = knownElements->find(parent);
    if (parent != 0 && find == knownElements->end()) {
      // add parents
      addParentsToSortedList(parent, knownElements);

      // add elements
      sortedElements.push_back(parent);
      knownElements->insert(parent);
    }
  }
}

void Dimension::updateLevelIndentDepth() {
  if (!isValidLevel) {

    isValidLevel = true;

    // clear all info
    maxLevel = 0;
    maxIndent = 0;
    maxDepth = 0;

    // update the topological sorted list of elements
    updateTopologicalSortedElements();

    // update level
    updateLevel();

    // update depth and ident
    for (deque<Element*>::iterator i = sortedElements.begin();
        i != sortedElements.end(); i++) {
      Element * element = *i;

      DepthType depth = 0;
      IndentType indent = 1;

      const ParentsType * parents = getParents(element);
      if (parents != 0) {
        // depth
        for (ParentsType::const_iterator i = parents->begin();
            i != parents->end(); i++) {
          Element * parent = *i;

          DepthType d = parent->depth;  // firstParent->getDepth(this);
          if (depth <= d) {
            depth = d + 1;
          }
        }

        // ident
        if (parents->size() > 0) {
          Element * firstParent = (*parents)[0];
          indent = firstParent->indent + 1;  // firstParent->getIndent(this) + 1;
        }

      }

      if (maxDepth < depth) {
        maxDepth = depth;
      }

      if (maxIndent < indent) {
        maxIndent = indent;
      }

      element->setDepth(depth);
      element->setIndent(indent);
    }
  }
}

void Dimension::updateLevel() {
  //locked by the caller
  for (deque<Element*>::reverse_iterator i = sortedElements.rbegin();
      i != sortedElements.rend(); i++) {
    Element * element = *i;

    ParentChildrenPair * pcp = parentToChildren.findKey(element);
    LevelType level = 0;

    if (pcp != 0) {
      ElementsWeightType children = pcp->children;
      for (ElementsWeightType::iterator childPair = children.begin();
          childPair != children.end(); childPair++) {
        Element * child = childPair->first;
        LevelType l = child->level;  // child->getLevel(this);
        if (level <= l) {
          level = l + 1;
        }
      }
    }

    if (maxLevel < level) {
      maxLevel = level;
    }

    element->setLevel(level);
  }
}

size_t Dimension::getMemoryUsageStorage() {
  return sizeof(Element) * elements.size();
}

size_t Dimension::sizeElements(bool onlyBase) {
  if (!onlyBase)
    return numElements;

  if (!isValidLevel) {
    updateLevelIndentDepth();
  }

  size_t numBaseElements = 0;
  for (auto i = elements.begin(); i != elements.end(); i++) {
    Element *element = *i;
    if (!element || element->getElementType() == CONSOLIDATED) {
      continue;
    }
    numBaseElements++;
  }
  return numBaseElements;
}
