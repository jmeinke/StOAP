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
 * \author Alexander Haberstroh, Jedox AG, Freiburg, Germany
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */


#include "Engine/AggregationMap.h"

//#include "Engine/AggregationProcessor.h"
#include "Collections/WeightedSet.h"
#include "Exceptions/ErrorException.h"

typedef map<IdentifierType, const WeightedSet *> AMBase;

bool ISWeightNonDefault(const double w) {
  return w != 1.0;
}

uint32_t AggregationMap::storeDistributionSequence(
    const IdentifierType startId, const IdentifierType nextStartId,
    const IdentifiersType &targetIds, const vector<double> &targetWeights) {
  uint32_t distrMapIndex = uint32_t(-1);
  uint32_t distMapSequenceStart = uint32_t(-1);

  uint32_t newSize = (uint32_t) targetIds.size();
  uint32_t currentIndex = (uint32_t) distributionMap.size() - 1;
  // TODO: -jj- duplicities search in AggregationMap disabled because of performance problems
  for (vector<TargetSequence>::const_reverse_iterator dm = distributionMap
      .rbegin(); false && dm != distributionMap.rend(); ++dm, currentIndex--) {
    if (newSize == dm->length) {
      // compare content
      if (equal(targetIds.begin(), targetIds.end(),
                &targetIdBuffer[dm->startOffset])) {
        //uint32_t currentIndex = dm-distributionMap.rbegin();
        bool identicalWeights = false;
        if (targetWeights.empty() && weightBuffer.size() <= dm->startOffset) {
          // none of weights buffers exist - null == null
          identicalWeights = true;
        }
        if (weightBuffer.size()) {
          double *findEndIt = &weightBuffer[0]
              + min(dm->startOffset + dm->length,
                    (uint32_t) weightBuffer.size());
          if (!identicalWeights && targetWeights.empty()
              && std::find_if(&weightBuffer[dm->startOffset], findEndIt,
                              ISWeightNonDefault) == findEndIt) {
            // check if weightBuffer contains only 1.0 - null == default_weights
            identicalWeights = true;
          }
        }
        if (!identicalWeights && targetWeights.size()
            && weightBuffer.size() >= dm->startOffset + targetWeights.size()
            && equal(targetWeights.begin(), targetWeights.end(),
                     &weightBuffer[dm->startOffset])) {
          // both weight buffers exist - weights == weights
          identicalWeights = true;
        }
        if (identicalWeights) {
          distrMapIndex = currentIndex;
          //  LOG(INFO) << "duplicity saves: " << newSize*sizeof(targetIds[0]) << " B" << endl;
          break;
        }
      }
    }
  }

  //DLOG(INFO) << distrMapIndex << " " << distributionMap.size() << endl;
  if (distrMapIndex == uint32_t(-1)) {
    // copy sequence to buffers
    distMapSequenceStart = (uint32_t) targetIdBuffer.size();

    if (targetWeights.size()) {
      if (weightBuffer.size() != targetIdBuffer.size()) {
        weightBuffer.resize(targetIdBuffer.size(), 1.0);
      }
      copy(targetWeights.begin(), targetWeights.end(),
           std::back_insert_iterator<vector<double> >(weightBuffer));
    }
    copy(targetIds.begin(), targetIds.end(),
         std::back_insert_iterator<IdentifiersType>(targetIdBuffer));
    // write sequence start to distributionMap
    distrMapIndex = (uint32_t) (distributionMap.size());
    distributionMap.push_back(
        TargetSequence(distMapSequenceStart, (uint32_t) targetIds.size()));
  }
  if (source2TargetVector.empty()) {
    // update "map"
    if (source2TargetMap.empty()
        || source2TargetMap.back().distrMapIndex != distrMapIndex) {
      source2TargetMap.push_back(Source2TargetMapItem(startId, distrMapIndex));
      if (source2TargetMap.size() * sizeof(source2TargetMap[0])
          > (maxBaseId - minBaseId + 1) * sizeof(source2TargetVector[0])) {
        // convert "map" to vector
        source2TargetVector.resize(maxBaseId - minBaseId + 1);
        vector<Source2TargetMapItem>::const_iterator mapIt = source2TargetMap
            .begin();
        size_t seqBegin = mapIt->sourceRangeBegin - minBaseId;
        do {
          size_t seqEnd;
          uint32_t dmIndex = mapIt->distrMapIndex;
          if (++mapIt == source2TargetMap.end()) {
            seqEnd = maxBaseId - minBaseId + 1;
          } else {
            seqEnd = mapIt->sourceRangeBegin - minBaseId;
          }
          std::fill(&source2TargetVector[seqBegin],
                    &source2TargetVector[0] + seqEnd, dmIndex);
          seqBegin = seqEnd;
        } while (mapIt != source2TargetMap.end());
        source2TargetMap.clear();
      }
    }
  } else {
    // update vector
    size_t fillEnd = min(maxBaseId + 1, nextStartId);
    std::fill(&source2TargetVector[startId - minBaseId],
              &source2TargetVector[0] + fillEnd - minBaseId, distrMapIndex);
  }
  return distrMapIndex;
}

AggregationMap::TargetReader AggregationMap::getTargets(
    IdentifierType sourceId) const {
  uint32_t distrMapIndex;
  if (sourceId < minBaseId || sourceId > maxBaseId) {
    // sourceId out of range
    throw ErrorException(ErrorException::ERROR_INTERNAL,
                         "AggregationMap::getTargets(): sourceId out of range");
  }
  if (source2TargetVector.empty()) {
    // find in base2ParentMapNG
    vector<Source2TargetMapItem>::const_iterator mit = std::upper_bound(
        source2TargetMap.begin(), source2TargetMap.end(),
        Source2TargetMapItem(sourceId, 0), DistrMapCmp);
    if (mit != source2TargetMap.begin()) {
      --mit;
    }
    distrMapIndex = mit->distrMapIndex;
  } else {
    if (sourceId < minBaseId || sourceId > maxBaseId) {
      // sourceId out of range
      throw ErrorException(
          ErrorException::ERROR_INTERNAL,
          "AggregationMap::getTargets(): sourceId out of range");
    }
    distrMapIndex = source2TargetVector[sourceId - minBaseId];
  }
  const TargetSequence &distrMap = distributionMap[distrMapIndex];
  const double *weights =
      weightBuffer.size() <= distrMap.startOffset ?
          0 : &weightBuffer[distrMap.startOffset];
  AggregationMap::TargetReader reader(
      &targetIdBuffer[distrMap.startOffset],
      &targetIdBuffer[0] + distrMap.startOffset + distrMap.length, weights,
      sourceId);
  return reader;
}

bool AggregationMap::operator==(const AggregationMap &o) const {
  return targetIdBuffer == o.targetIdBuffer && weightBuffer == o.weightBuffer
      && distributionMap == o.distributionMap
      && source2TargetMap == o.source2TargetMap
      && source2TargetVector == o.source2TargetVector;
}

// key: first child in current range
// data: range iterator, end range iterator, target element
struct MapTuple {
  MapTuple(WeightedSet::range_iterator crange,
           WeightedSet::range_iterator erange, IdentifierType target)
      : crange(crange),
        erange(erange),
        target(target) {
  }
  WeightedSet::range_iterator crange;
  WeightedSet::range_iterator erange;
  IdentifierType target;
};

void AggregationMap::buildBaseToParentMap(IdentifierType parent,
                                          const WeightedSet *descElems) {
  for (WeightedSet::const_iterator it = descElems->begin();
      it != descElems->end(); ++it) {
    base2ParentMap[it.first()][parent] = it.second();
    minBaseId = min(minBaseId, it.first());
    maxBaseId = max(maxBaseId, it.first());
  }
}

void AggregationMap::compactSourceToTarget() {
  targetIdBuffer.clear();
  weightBuffer.clear();
  distributionMap.clear();
  source2TargetMap.clear();
  source2TargetVector.clear();

  for (SourceToTargetMapType::iterator stit = base2ParentMap.begin();
      stit != base2ParentMap.end(); ++stit) {
    IdentifiersType targetIds;
    vector<double> targetWeights;

    for (TargetMapType::const_iterator tmit = stit->second.begin();
        tmit != stit->second.end(); ++tmit) {
      targetIds.push_back(tmit->first);
      targetWeights.push_back(tmit->second);
    }
    SourceToTargetMapType::iterator nit = stit;
    ++nit;
    IdentifierType nextStartId =
        nit == base2ParentMap.end() ? stit->first + 1 : nit->first;
    storeDistributionSequence(stit->first, nextStartId, targetIds,
                              targetWeights);
  }
}

