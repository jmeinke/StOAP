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
 * \author Jiri Junek, qBicon s.r.o., Prague, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#include "Stoap/AggregationProcessor.h"

#include "Olap/DoubleStorage.h"
#include "Olap/Area.h"

/**
 * @brief Constructor
 * @param cArea Area to be calculated
 * @param cType Aggregation type
 */
AggregationProcessor::AggregationProcessor(CubeArea* cArea,
                                           AggregationType cType) {
  // assign the area to be calculated
  calcArea = cArea;

  // compute the source cell area and the corresponding aggregation maps
  srcArea = calcArea->expandBase(&parentMaps);

  // assign the aggregation type
  calcType = cType;

  // calculate the size of the result
  resultSize.clear();
  for (size_t d = 0; d < calcArea->dimCount(); ++d) {
    resultSize.push_back(calcArea->elemCount(d));
  }

  // create a temporary storage with enough place for holding the area
  resultStorage = new DoubleStorage();
  resultStorage->m.resize(calcArea->getSize());

  lastTargets.resize(calcArea->dimCount());
  currentTarget.resize(calcArea->dimCount());
  parentKey.resize(calcArea->dimCount());
  multiDims.resize(calcArea->dimCount());
  prevSourceKey = IdentifiersType(calcArea->dimCount(), NO_IDENTIFIER );
  lastKeyParent = prevSourceKey;
}

/**
 * @brief this is the place where the aggregation is actually computed
 */
void AggregationProcessor::aggregate() {
  cpu_timer t;
  LOG(WARNING)<< "Aggregation started for " << calcArea->toString();

  // process all source cells
  if (calcArea->getSize() == 1 && calcArea->isBase(calcArea->pathBegin())) {
    // we have requested an aggregation of a single base cell
    LOG(WARNING)<< "Skipping aggregation of a single base cell." << endl;
    return;
  }

  LOG(WARNING)<< "Source area is: " << srcArea->toString();
  LOG(WARNING)<< "Size: " << srcArea->getSize();
  LOG(WARNING)<< "Target area is: " << calcArea->toString();
  LOG(WARNING)<< "Size: " << calcArea->getSize();

  DoubleStorage* storage = calcArea->getCube()->getStorage();
  // Area* filledCellArea = calcArea->getCube()->getFilledArea();

  // iteration through all stored entries only makes sense if the source area is bigger than the stored values
  // otherswise look up the cells which are contained in the source area
  if (calcArea->getSize() <= 60 &&  // this is the same condition as used by the Jedox OLAP server
      srcArea->getSize() <= storage->m.size()) {
    LOG(INFO)<< "Using target-based aggregation!";
    // this is the target-based aggregation
    // iterate through the entries in the source area
    // problem: source area can be very large and there can be a huge amount of non-existent cells
    // therefore better only use it when we can intersect srcArea and filledCells
    for (auto srcIt = srcArea->pathBegin(); srcIt != srcArea->pathEnd(); ++srcIt) {
      try {
        double* value = storage->getValue(&(*srcIt));
        if (value == NULL) continue;

        aggregateCell(*srcIt, *value);
      } catch (const ErrorException& e) {
        LOG(ERROR) << "Error in AggregationProcessor::aggregate: " << e.getMessage();
        return;
      }
    }
  } else {
    LOG(INFO) << "Using source-based aggregation!";
    // this is the source-based aggregation
    // iterate through all the stored entries in the storage map

    // TODO(jmeinke): improve intersection performance of filled cells and relevant cells
    /* Area* relevantCells = filledCellArea->intersect(*srcArea);
     for (auto cellIt = relevantCells->pathBegin(); cellIt != relevantCells->pathEnd(); ++cellIt) {
     try {
     aggregateCell(*cellIt, *(storage->getValue(&(*cellIt))));
     } catch (const ErrorException& e) {
     LOG(ERROR) << "Error in AggregationProcessor::aggregate: " << e.getMessage();
     return;
     }
     } */

    for (auto srcIt = storage->m.begin(); srcIt != storage->m.end(); ++srcIt) {
      try {
        // TODO(jmeinke): this is probably not the fastest way to check
        // and might be the cause why this is so much slower than the OLAP server
        // Idea: maintain an area object for the source cells,
        // then compute the intersection area first, and iterate over it
        if (srcArea->find(srcIt->first) == srcArea->pathEnd()) continue;
        //Area testCell(srcIt->first, true);
        //if (srcArea->intersection(testCell))
        
        aggregateCell(srcIt->first, srcIt->second);
      } catch (const ErrorException& e) {
        LOG(ERROR) << "Error in AggregationProcessor::aggregate: " << e.getMessage();
        break;
      }
    }
  }
  LOG(INFO)<< "Aggregation time: " << t.format();
}

void AggregationProcessor::aggregateCell(const IdentifiersType &key,
                                         const double value) {
  double fixedWeight;
  size_t multiDimCount;

  initParentKey(key, multiDimCount, &fixedWeight);

  // for each parent cell
  size_t changeMultiDim;
  do {
    CellPath source(&key);
    CellPath target(&parentKey);
    /* DLOG(WARNING) << "Aggregating source " << source.toString()
     << " into parent " << target.toString(); */

    // get old value
    double oldVal = 0.0;
    double* oldValPointer = resultStorage->getValue(&parentKey);
    if (oldValPointer != NULL) {
      oldVal = *oldValPointer;
    }

    double weight = fixedWeight;
    // add value * weight to parent
    for (size_t multiDim = 0; multiDim < multiDimCount; multiDim++) {
      weight *= currentTarget[multiDims[multiDim]].getWeight();
    }

    if (weight != 1) {
      // DLOG(WARNING) << "Weight: " << weight;
      double destVal = oldVal + weight * value;
      resultStorage->setValue(&parentKey, destVal);
    } else {
      double destVal = oldVal + value;
      resultStorage->setValue(&parentKey, destVal);
    }
    nextParentKey(multiDimCount, changeMultiDim);
  } while (changeMultiDim < multiDimCount);
}

void AggregationProcessor::initParentKey(const IdentifiersType &key,
                                         size_t &multiDimCount,
                                         double *fixedWeight) {
  if (fixedWeight) {
    *fixedWeight = 1;
  }
  // generate all combinations of parents
  multiDimCount = 0;
  vector<AggregationMap::TargetReader>::iterator lastTarget =
      lastTargets.begin();
  IdentifiersType::iterator prevSourceKeyIt = prevSourceKey.begin();
  IdentifiersType::const_iterator elemId = key.begin();
  for (size_t dim = 0; dim < calcArea->dimCount();
      dim++, ++lastTarget, ++prevSourceKeyIt, ++elemId) {
    AggregationMap::TargetReader targets;
    IdentifierType lastTargetId = NO_IDENTIFIER;

    if (*elemId != *prevSourceKeyIt) {
      *prevSourceKeyIt = *elemId;
      targets = parentMaps[dim].getTargets(*elemId);
      *lastTarget = targets;
      if (targets.size() == 1) {
        lastTargetId = *targets;
      }
      lastKeyParent[dim] = lastTargetId;
    } else {
      //dimParents = *lastParent;
      targets = *lastTarget;
      lastTargetId = lastKeyParent[dim];
    }
    if (lastTargetId != NO_IDENTIFIER ) {
      // exactly one parent with default weight
      parentKey[dim] = lastTargetId;
    }

    parentKey[dim] = *targets;
    currentTarget[dim] = targets;
    if (targets.size() > 1) {
      // multiple targets for this source
      multiDims[multiDimCount++] = (IdentifierType) dim;
    } else {
      // single target
      if (fixedWeight) {
        *fixedWeight *= targets.getWeight();
      }
    }
  }
}

void AggregationProcessor::nextParentKey(size_t multiDimCount,
                                         size_t &changeMultiDim) {
  changeMultiDim = multiDimCount - 1;
  bool endOfIteration = false;
  while (!endOfIteration && changeMultiDim < multiDimCount) {
    size_t currentDim = multiDims[changeMultiDim];
    AggregationMap::TargetReader &target = currentTarget[currentDim];

    ++target;
    if (target.end()) {
      target.reset();
      changeMultiDim--;
    } else {
      endOfIteration = true;
    }
    parentKey[currentDim] = *target;
  }
}

void AggregationProcessor::print() {
  cout << setprecision(numeric_limits<double>::digits10);
  cout << "Type:\tPath:\tValue:" << endl;

  DoubleStorage* storage = calcArea->getCube()->getStorage();

  for (auto pathIt = calcArea->pathBegin(); pathIt != calcArea->pathEnd();
      ++pathIt) {

    CellPath myPath(&(*pathIt));
    if (myPath.getPathType() == CONSOLIDATED) {
      double* value = resultStorage->getValue(myPath.getPathIdentifier());

      cout << "Cons.\t" << myPath.toString() << ":\t";
      if ((value) == NULL) {
        cout << "error (resultStorage empty)" << endl;
      } else {
        cout << *value << endl;
      }
    } else {
      cout << "Base\t" << myPath.toString() << ":\t";
      double* value = storage->getValue(myPath.getPathIdentifier());
      if ((value) == NULL) {
        cout << "error (cubeStorage empty)" << endl;
      } else {
        cout << *value << endl;
      }
    }
  }
}

string AggregationProcessor::result(const vector<IdentifiersType>& req,
                                    bool addPath, bool addZero) {
  DoubleStorage* storage = calcArea->getCube()->getStorage();

  stringstream ss;
  ss << setprecision(numeric_limits<double>::digits10);

  if (!req.empty()) {
    for (auto pathIt = req.begin(); pathIt != req.end(); ++pathIt) {
      CellPath myPath(&(*pathIt));

      ss << "1;";  // cell type (numeric)
      double* value;
      if (myPath.getPathType() == CONSOLIDATED) {
        value = resultStorage->getValue(&(*pathIt));
      } else {
        value = storage->getValue(&(*pathIt));
      }
      if ((value) == NULL) {
        ss << "0;;";  // not found
      } else {
        ss << "1;" << *value << ";";  // found + value
      }

      if (addPath) ss << myPath.toString() << ";";  // path
      if (addZero) ss  << ";0;";  // zero
      ss << endl;
    }
  } else {
    for (auto pathIt = calcArea->pathBegin(); pathIt != calcArea->pathEnd();
        ++pathIt) {
      CellPath myPath(&(*pathIt));

      ss << "1;";  // cell type (numeric)
      double* value;
      if (myPath.getPathType() == CONSOLIDATED) {
        value = resultStorage->getValue(&(*pathIt));
      } else {
        value = storage->getValue(&(*pathIt));
      }
      if ((value) == NULL) {
        ss << "0;;";  // not found
      } else {
        ss << "1;" << *value << ";";  // found + value
      }

      if (addPath) ss << myPath.toString() << ";";  // path
      if (addZero) ss << ";0;";  // zero
      ss << endl;
    }
  }
  return ss.str();
}

/**
 * @brief Destructor
 */
AggregationProcessor::~AggregationProcessor() {
  // free memory
}
