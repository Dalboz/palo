/* 
 *
 * Copyright (C) 2006-2013 Jedox AG
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
 * 
 *
 */

#include "Logger/Logger.h"
#include "Engine/StorageCpu.h"
#include "Engine/CombinationProcessor.h"
#include "Engine/TransformationProcessor.h"

namespace palo {

class CellValueStorage : public StorageCpu
{
public:
	CellValueStorage(PPathTranslator pathTranslator) : StorageCpu(pathTranslator, true) {}
	virtual ~CellValueStorage() {}

	virtual double convertToDouble(const CellValue &value);
	virtual void convertToCellValue(CellValue &value, double d) const;
	virtual bool isNumeric() const {return false;}

	virtual PCellStream commitChanges(bool checkLocks, bool add, bool disjunctive) {return PCellStream();};
	bool merge(const CPCommitable &o, const PCommitable &p) {return false;}
	PCommitable copy() const {return PCommitable();}

private:
	vector<CellValue> internedValues;
	CellValue value;
};

class MixedStorage : public StorageBase {
public:
	MixedStorage(PPathTranslator pathTranslator) : StorageBase(pathTranslator), valueStorage(new CellValueStorage(pathTranslator)), numericStorage(new StorageCpu(pathTranslator, true)) {}
	virtual ~MixedStorage() {}
	virtual PProcessorBase getCellValues(CPArea area);
	virtual bool setCellValue(PCellStream stream);
	virtual size_t valuesCount() const {return valueStorage->valuesCount() + numericStorage->valuesCount();}
	virtual bool merge(const CPCommitable &o, const PCommitable &p) {return false;}
	virtual PCommitable copy() const {return PCommitable();}

	virtual void setCellValue(CPArea area, const CellValue &value, OperationType opType) {}
	virtual bool setCellValue(PPlanNode plan, PEngineBase engine) {return false;}
	virtual void setCellValue(CPPlanNode plan, PEngineBase engine, CPArea area, const CellValue &value, OperationType opType) {}
	virtual PCellStream commitChanges(bool checkLocks, bool add, bool disjunctive) {return PCellStream();};
private:
	PStorageCpu valueStorage;
	PStorageCpu numericStorage;
};

PProcessorBase MixedStorage::getCellValues(CPArea area)
{
	if (!valueStorage->valuesCount()) {
		return numericStorage->getCellValues(area);
	} else if (!numericStorage->valuesCount()) {
		return valueStorage->getCellValues(area);
	} else {
		vector<PProcessorBase> nodes;
		nodes.push_back(numericStorage->getCellValues(area));
		nodes.push_back(valueStorage->getCellValues(area));
		return PProcessorBase(new CombinationProcessor(PEngineBase(), nodes, CPPathTranslator()));
	}
}

bool MixedStorage::setCellValue(PCellStream stream)
{
	StorageCpu::Writer numW(*numericStorage, true);
	StorageCpu::Writer valW(*valueStorage, true);
	bool numOrdered = true;
	bool valOrdered = true;
	IdentifiersType lastNum;
	IdentifiersType lastVal;
	while (stream->next()) {
		const IdentifiersType &key = stream->getKey();
		const CellValue &value = stream->getValue();
		IdentifiersType &last = value.isNumeric() ? lastNum : lastVal;
		bool &ordered = value.isNumeric() ? numOrdered : valOrdered;
		StorageCpu::Writer &sw = value.isNumeric() ? numW : valW;
		if (ordered && last.size()) {
			if (CellValueStream::compare(last, key) >= 0) {
				ordered = false;
			}
		}
		last = key;
		if (ordered) {
			sw.push_back(key, sw.getStorage().convertToDouble(value));
		} else {
			sw.getStorage().setCellValue(key, value);
		}
	}
	if (!numOrdered) {
		numericStorage->commitChanges(false, false, false);
	}
	if (!valOrdered) {
		valueStorage->commitChanges(false, false, false);
	}
	return !numOrdered || !valOrdered;
}

double CellValueStorage::convertToDouble(const CellValue &value)
{
	internedValues.push_back(value);
	return (double)internedValues.size();
}

void CellValueStorage::convertToCellValue(CellValue &value, double d) const
{
	value = internedValues[size_t(d-1)];
}

class SERVER_CLASS RearrangeProcessor : public ProcessorBase {
public:
	RearrangeProcessor(PProcessorBase inputSP, const vector<uint32_t> &dimensionMapping, CPArea targetArea, CPArea sourceArea);
	virtual ~RearrangeProcessor() {}

	virtual bool next();
	virtual const CellValue &getValue();
	virtual double getDouble();
	virtual const IdentifiersType &getKey() const;
	virtual const GpuBinPath &getBinKey() const;
	virtual void reset();
	virtual bool move(const IdentifiersType &key, bool *found);
private:
	void cacheInput();
	PProcessorBase inputSP;
	vector<uint32_t> dimensionMapping;
	CPArea targetArea;
	CPArea sourceArea;
	IdentifiersType outKey;
	vector<uint32_t> target2Source;
	vector<uint32_t> source2Target;
	struct MisplacedDimension {
		MisplacedDimension(int targetOrdinal, int sourceOrdinal, const Set &set) : targetOrdinal(targetOrdinal), sourceOrdinal(sourceOrdinal), setCurrent(set.begin()), setBegin(set.begin()), setEnd(set.end()), set(&set) {}
		int targetOrdinal;
		int sourceOrdinal;
		Set::Iterator setCurrent;
		Set::Iterator setBegin;
		Set::Iterator setEnd;
		const Set *set;
	};
	typedef vector<MisplacedDimension> MPDV;
	MPDV misplacedDimensions;
	PStorageBase storage;
	PProcessorBase cachedInputSP;
	ProcessorBase *cachedInput;
};

RearrangeProcessor::RearrangeProcessor(PProcessorBase inputSP, const vector<uint32_t> &dimensionMapping, CPArea targetArea, CPArea sourceArea) :
		ProcessorBase(true, PEngineBase()), inputSP(inputSP), dimensionMapping(dimensionMapping),
		targetArea(targetArea), sourceArea(sourceArea), cachedInput(0)
{
	target2Source.resize(targetArea->dimCount());
	source2Target.resize(sourceArea->dimCount());
	vector<uint32_t>::const_iterator dmit = dimensionMapping.begin();
	while (dmit != dimensionMapping.end()) {
		uint32_t sourceOrdinal = *dmit++;
		if (dmit != dimensionMapping.end()) {
			uint32_t targetOrdinal = *dmit++;
			target2Source[targetOrdinal] = sourceOrdinal+1;
			source2Target[sourceOrdinal] = targetOrdinal+1;
		}
	}
	double iterationsCount = 1;
	for (vector<uint32_t>::const_iterator stit = source2Target.begin(); stit != source2Target.end(); ++stit) {
		if (*stit) {
			for (vector<uint32_t>::const_iterator stit2 = source2Target.begin(); stit2 != stit; ++stit2) {
				if (*stit2 && *stit < *stit2) {
					int sourceOrdinal = (int)(stit - source2Target.begin());
					int targetOrdinal = *stit - 1;
					MisplacedDimension mdr(targetOrdinal, sourceOrdinal, *sourceArea->getDim(sourceOrdinal));
					misplacedDimensions.push_back(mdr);
					iterationsCount *= sourceArea->getDim(sourceOrdinal)->size();
					break;
				}
			}
		}
	}
	if (iterationsCount && Logger::isTrace()) {
		Logger::trace << "RearrangeProcessor created with " << iterationsCount << " iterations in " << misplacedDimensions.size() << " dimensions" << endl;
	}
}

static ostream& operator<<(ostream& ostr, const IdentifiersType& v)
{
	bool first = true;
	ostr << dec;
	for (vector<IdentifierType>::const_iterator it = v.begin(); it != v.end(); ++it) {
		if (!first) {
			ostr << ",";
		}
		if (*it == NO_IDENTIFIER) {
			ostr << "*";
		} else {
			ostr << *it;
		}
		first = false;
	}
	return ostr;
}

bool RearrangeProcessor::next()
{
	if (outKey.empty()) {
		cacheInput();
	}
	for(;;) {
		if (!cachedInput) {
			PArea selectArea(new Area(*sourceArea));
			// create new reader for current batch
			for(MPDV::const_iterator mdit = misplacedDimensions.begin(); mdit != misplacedDimensions.end(); ++mdit) {
				PSet s(new Set());
				outKey[mdit->targetOrdinal] = *mdit->setCurrent;
				s->insert(*mdit->setCurrent);
				selectArea->insert(mdit->sourceOrdinal, s);
			}
			cachedInputSP = storage->getCellValues(selectArea);
			cachedInput = cachedInputSP.get();
		}
		if (cachedInput->next()) {
			int sourceOrdinal = 0;
			IdentifiersType inKey = cachedInput->getKey();
			for(vector<uint32_t>::const_iterator stit = source2Target.begin(); stit != source2Target.end(); ++stit, ++sourceOrdinal) {
				if (*stit) {
					outKey[*stit-1] = inKey[sourceOrdinal];
				}
			}
			if (Logger::isTrace()) {
				Logger::trace << "RearrangeProcessor::next() out:" << outKey << " in: " << inKey << endl; // << outKey
			}
			return true;
		}
		cachedInputSP.reset();
		cachedInput = 0;
		// iterate over misplacedDimensions
		MPDV::reverse_iterator mdit = misplacedDimensions.rbegin();
		while (mdit != misplacedDimensions.rend()) {
			++mdit->setCurrent;
			if (mdit->setCurrent != mdit->setEnd) {
				if (Logger::isTrace()) {
					Logger::trace << "change at " << (mdit - misplacedDimensions.rbegin()) << endl;
				}
				break;
			}
			mdit->setCurrent = mdit->setBegin;
			++mdit;
		}
		if (mdit == misplacedDimensions.rend()) {
			if (Logger::isTrace()) {
				Logger::trace << "no more combinations in RearrangeProcessor" << endl;
			}
			return false;
		}
	}
	return false;
}

bool RearrangeProcessor::move(const IdentifiersType &key, bool *found)
{
	if (outKey.empty()) {
		cacheInput();
	}
	// prepare key to move in source
	IdentifiersType moveToSourceKey(sourceArea->dimCount(), NO_IDENTIFIER);
	size_t targetDim = 0;
	for (vector<uint32_t>::const_iterator t2sit = target2Source.begin(); t2sit != target2Source.end(); ++t2sit, targetDim++) {
		if (*t2sit) {
			moveToSourceKey[*t2sit-1] = key[targetDim];
		}
	}
	bool bulkChanged = !cachedInput;

	// prepare source batch
	for (MPDV::iterator mdit = misplacedDimensions.begin(); mdit != misplacedDimensions.end(); ++mdit) {
		Set::Iterator newCurrent;
		if (key[mdit->targetOrdinal]) {
			newCurrent = mdit->set->find(key[mdit->targetOrdinal]);
			if (newCurrent == mdit->setEnd) {
				// problem
				throw ErrorException(ErrorException::ERROR_INTERNAL, "RearrangeProcessor::move(): key outside the area.");
			}
		} else {
			newCurrent = mdit->setBegin;
		}
		if (newCurrent != mdit->setCurrent) {
			mdit->setCurrent = newCurrent;
			bulkChanged = true;
		}
	}
	if (bulkChanged) {
		if (cachedInput) {
			cachedInputSP.reset();
			cachedInput = 0;
		}
		PArea selectArea(new Area(*sourceArea));
		// create new reader for current batch
		for(MPDV::const_iterator mdit = misplacedDimensions.begin(); mdit != misplacedDimensions.end(); ++mdit) {
			PSet s(new Set());
			outKey[mdit->targetOrdinal] = *mdit->setCurrent;
			s->insert(*mdit->setCurrent);
			selectArea->insert(mdit->sourceOrdinal, s);
		}
		cachedInputSP = storage->getCellValues(selectArea);
		cachedInput = cachedInputSP.get();
	}
	if (!cachedInput->move(moveToSourceKey, found)) {
		bool result = next();
		// nothing found in this bulk
		if (Logger::isTrace()) {
			Logger::trace << "RearrangeProcessor::move() out:" << (result ? outKey : EMPTY_KEY) << " move to key: " << key << endl; // << outKey
		}
		return result;
	}
	int sourceOrdinal = 0;
	IdentifiersType inKey = cachedInput->getKey();
	for(vector<uint32_t>::const_iterator stit = source2Target.begin(); stit != source2Target.end(); ++stit, ++sourceOrdinal) {
		if (*stit) {
			outKey[*stit-1] = inKey[sourceOrdinal];
		}
	}
	if (Logger::isTrace()) {
		Logger::trace << "RearrangeProcessor::move() out:" << outKey << " move to key: " << key << endl; // << outKey
	}
	return true;
}

const CellValue &RearrangeProcessor::getValue()
{
	return cachedInput->getValue();
}

double RearrangeProcessor::getDouble()
{
	return cachedInput->getDouble();
}

const IdentifiersType &RearrangeProcessor::getKey() const
{
	if (cachedInput) {
		return outKey;
	} else {
		return EMPTY_KEY;
	}
}

const GpuBinPath &RearrangeProcessor::getBinKey() const
{
	throw ErrorException(ErrorException::ERROR_INTERNAL, "Unsupported method RearrangeProcessor::getBinKey()");
}

void RearrangeProcessor::reset()
{
	for(MPDV::iterator mdit = misplacedDimensions.begin(); mdit != misplacedDimensions.end(); ++mdit) {
		mdit->setCurrent = mdit->setBegin;
	}
	cachedInputSP.reset();
	cachedInput = 0;
}

void RearrangeProcessor::cacheInput()
{
	outKey = *targetArea->pathBegin();
	PCellStream inStream(boost::dynamic_pointer_cast<CellValueStream, ProcessorBase>(inputSP));
	storage = PStorageBase(new MixedStorage(PPathTranslator()));
	storage->setCellValue(inStream);
	if (Logger::isTrace()) {
		Logger::trace << "RearrangeProcessor cached " << storage->valuesCount() << " values" << endl;
//		PCellStream cachedVals = storage->getCellValues(PArea());
//		while (cachedVals->next()) {
//			Logger::trace << cachedVals->getKey() << " " << cachedVals->getDouble() << endl;
//		}
	}
}

bool TransformationProcessor::move(const IdentifiersType &key, bool *found)
{
//	Logger::debug << this << " M " << key << " from " << getKey() << endl;
	// if the key is already reached
	size_t firstOutputChange = key.size();
	if (nextResult) {
		const IdentifiersType &currentKey = getKey();
		int diff = 0;
		size_t size = key.size();
		for (size_t i = 0; i < size; i++) {
			if (currentKey[i] < key[i]) {
				diff = -1;
				firstOutputChange = i;
				break;
			} else if (currentKey[i] > key[i]) {
				firstOutputChange = i;
				diff = 1;
				break;
			}
		}
		if (diff >= 0) {
			if (found) {
				*found = diff == 0;
			}
			if (Logger::isTrace()) {
				Logger::trace << "TransformationProcessor::move() out:" << currentKey << " move to key: " << key << endl; // << outKey
			}
			return true;
		}
	}

	// generate new start for input
	IdentifiersType nextMoveToInKey(moveToInKey);
	size_t firstSrcDim = key.size();
	// transform the key here
	for (DimsMappingType::const_iterator transIt = dimMapping.begin(); transIt != dimMapping.end(); ++transIt) {
		nextMoveToInKey[transIt->second] = key[transIt->first];
		firstSrcDim = min(transIt->first, firstSrcDim);
	}
	int relativeToInput = lastInKey.empty() ? 1 : CellValueStream::compare(nextMoveToInKey, lastInKey);

//	return CellValueStream::move(key, found);

	bool nextInput = false;
	bool inputFound = false;
	if (relativeToInput < 0) {
		// move back
		childSP->reset();
	} else {
		// move forward
	}
	if (!child) {
		if (!childSP) {
			childSP = createProcessor(transformationPlanNode->getChildren()[0], true);
			if (transformationPlanNode->getDimMapping().size()) {
				childSP = PProcessorBase(new RearrangeProcessor(childSP, transformationPlanNode->getDimMapping(), transformationPlanNode->getArea(), transformationPlanNode->getChildren()[0]->getArea()));
			}
		}
		child = childSP.get();
	}

	nextInput = childSP->move(nextMoveToInKey, &inputFound);
	// if input value found
	if (nextInput) {
		const IdentifiersType &foundInputKey = child->getKey();

		ExpansionRangesType::iterator eRIt;
		for (eRIt = expansionRanges.begin(); eRIt != expansionRanges.end(); ++eRIt) {
			if (firstOutputChange <= eRIt->first.first) {
				moveToInKey = eRIt->second;
				while (++eRIt != expansionRanges.end()) {
					eRIt->second = moveToInKey;
				}
				break;
			}
		}

		lastInKey = foundInputKey;
		// compare found input value with requested and reset part of the generated output key iterators
		size_t firstOutputChange = key.size();
		for (DimsMappingType::const_iterator transIt = dimMapping.begin(); transIt != dimMapping.end(); ++transIt) {
			if (foundInputKey[transIt->second] != key[transIt->first]) {
				firstOutputChange = min(transIt->first, firstOutputChange);
			}
			outKey[transIt->first] = foundInputKey[transIt->second];
		}

		for (size_t targetDim = 0; targetDim < key.size(); targetDim++) {
			if (expansions[targetDim].first) {
				if (targetDim <= firstOutputChange) {
					// find the requested element in the set
					expansions[targetDim].second = expansions[targetDim].first->find(key[targetDim]);
					if (expansions[targetDim].second == expansions[targetDim].first->end()) {
						// key outside the selection?
						throw ErrorException(ErrorException::ERROR_INTERNAL, "TransformationProcessor::move(): key outside the area.");
					}
					outKey[targetDim] = key[targetDim];
				} else {
					// get beginning of the set
					expansions[targetDim].second = expansions[targetDim].first->begin();
					outKey[targetDim] = *expansions[targetDim].second;
				}
			}
		}

		if (found) {
			*found = CellValueStream::compare(outKey, key) == 0;
		}
		if (Logger::isTrace()) {
			Logger::trace << "TransformationProcessor::move() out:" << outKey << " move to key: " << key << endl; // << outKey
		}
		nextResult = true;
		return true;
	} else {
		// no input found
		if (found) {
			*found = false;
		}
		if (expansionRanges.size() && expansionRanges[0].first.first < firstSrcDim) {
			size_t dim;
			for (dim = expansionRanges[0].first.first; dim <= expansionRanges[0].first.second; dim++) {
				if (expansions[dim].first) {
					expansions[dim].second = expansions[dim].first->find(key[dim]);
					if (expansions[dim].second == expansions[dim].first->end()) {
						throw ErrorException(ErrorException::ERROR_INTERNAL, "TransformationProcessor::move(): key outside the area.");
					}
					outKey[dim] = *expansions[dim].second;
				}
			}
			for (; dim < outKey.size(); dim++) {
				if (expansions[dim].first) {
					expansions[dim].second = expansions[dim].first->begin();
					outKey[dim] = *expansions[dim].second;
				}
			}
			for (int expandDim = expansionRanges[0].first.second; expandDim >= 0; expandDim--) {
				if (expansions[expandDim].first) {
					++expansions[expandDim].second;
					if (expansions[expandDim].second == expansions[expandDim].first->end()) {
						expansions[expandDim].second = expansions[expandDim].first->begin();
						outKey[expandDim] = *expansions[expandDim].second;
						if (expandDim == 0) {
							break;
						}
						// continue to next dimension
					} else {
						ExpansionRangesType::iterator eRIt = expansionRanges.begin();
						moveToInKey = eRIt->second;
						while (++eRIt != expansionRanges.end()) {
							eRIt->second = moveToInKey;
						}
						outKey[expandDim] = *expansions[expandDim].second;
						childSP->reset();
						child = 0;
						bool res = next();
						if (Logger::isTrace()) {
							Logger::trace << "TransformationProcessor::move() out:" << (res ? outKey : EMPTY_KEY) << " move to key: " << key << endl; // << outKey
						}
						return res;
					}
				}
			}
		}
		nextResult = false;
		if (Logger::isTrace()) {
			Logger::trace << "TransformationProcessor::move() out:" << EMPTY_KEY << " move to key: " << key << endl; // << outKey
		}
		return false;
	}
}

TransformationProcessor::TransformationProcessor(PEngineBase engine, CPPlanNode node) :
		ProcessorBase(true, engine), node(node), transformationPlanNode(dynamic_cast<const TransformationPlanNode *>(node.get())),
		pathTranslator(node->getArea()->getPathTranslator()), nextResult(false)
{
	CPArea targetArea = transformationPlanNode->getArea();
	vector<uint32_t> target2Source;

	CPArea sourceArea = transformationPlanNode->getChildren()[0]->getArea();
	size_t targetDimOrd = 0;

	const vector<uint32_t> &dimensionMapping = transformationPlanNode->getDimMapping();
	if (dimensionMapping.size()) {
		moveToInKey.resize(targetArea->dimCount());

		target2Source.resize(targetArea->dimCount());
		vector<uint32_t>::const_iterator dmit = dimensionMapping.begin();
		while (dmit != dimensionMapping.end()) {
			uint32_t sourceOrdinal = *dmit++;
			if (dmit != dimensionMapping.end()) {
				uint32_t targetOrdinal = *dmit++;
				target2Source[targetOrdinal] = sourceOrdinal+1;
			}
		}
	} else {
		moveToInKey = *sourceArea->pathBegin();
	}

	outKey.resize(targetArea->dimCount());
	size_t lastExpand = 0;
	size_t dimOrdinal;
	for (dimOrdinal = 0; targetDimOrd < targetArea->dimCount(); targetDimOrd++, dimOrdinal++) {
//		if (*targetSet != *sourceSet && **targetSet != **sourceSet) {
//			if ((*sourceSet)->size() == 1 && (*targetSet)->size() > 1) {
//				// expansion
//				Logger::trace << "Expansion in dim: " << dimOrdinal << endl;
//			} else if ((*sourceSet)->size() == 1 && (*targetSet)->size() == 1) {
//				// mapping 1:1
//				Logger::trace << "Mapping in dim: " << dimOrdinal << endl;
//			} else {
//				// mapping N:M, 1:M
//				Logger::error << "Undefined in dim: " << dimOrdinal << endl;
//			}
//		} else {
//			// identical subset
////			Logger::trace << "Identical dim: " << dimOrdinal << endl;
//		}
		CPSet targetSet = targetArea->getDim(targetDimOrd);
		CPSet sourceSet;

		if (target2Source.size()) {
			if (target2Source[targetDimOrd]) {
				sourceSet = sourceArea->getDim(target2Source[targetDimOrd]-1);
			}
		} else if (targetArea->dimCount() == sourceArea->dimCount()) {
			// only if dimensionality is identical compare  source and target sets
			sourceSet = sourceArea->getDim(targetDimOrd);
		}
		if (targetSet->size() == 1) {
			// target restricted to 1 element
			outKey[dimOrdinal] = *targetSet->begin();
			expansions.push_back(pair<const Set *, Set::Iterator>((const Set *)0, Set::Iterator()));
		} else if (sourceSet && (targetSet == sourceSet || *targetSet == *sourceSet)) {
			// target dimension is source dimension
			dimMapping.push_back(DimMapping(dimOrdinal, dimOrdinal));
			expansions.push_back(pair<const Set *, Set::Iterator>((const Set *)0, Set::Iterator()));
			lastExpand = 0;
		} else if ((!sourceSet || sourceSet->size() == 1) && targetSet->size() > 1) {
			// expansion
			if (!lastExpand) {
				expansionRanges.push_back(make_pair(make_pair(dimOrdinal, dimOrdinal), moveToInKey));
			} else {
				expansionRanges.back().first.second = dimOrdinal;
			}
			Set::Iterator setBegin = targetSet->begin();
			expansions.push_back(pair<const Set *, Set::Iterator>(targetSet.get(), setBegin));
			outKey[dimOrdinal] = *setBegin;
			lastExpand = dimOrdinal;
		} else {
			const SetMultimaps *setMultiMaps = transformationPlanNode->getSetMultiMaps();
			if (!setMultiMaps || setMultiMaps->empty() || !setMultiMaps->at(dimOrdinal)) {
				// unsupported transformation
				Logger::error << "Unsupported transformation type in TransformationProcessor!" << endl;
				throw ErrorException(ErrorException::ERROR_INTERNAL, "Unsupported transformation type in TransformationProcessor!");
			} else {
				// multimapping in this dimension
				dimMapping.push_back(DimMapping(dimOrdinal, dimOrdinal));
				lastExpand = 0;
			}
		}
	}
	if (lastExpand) {
		// extend last expansionRange to the end
		expansionRanges.back().first.second = dimOrdinal-1;
	}
	factor = transformationPlanNode->getFactor();
	childSP.reset();
	child = 0;
}

bool TransformationProcessor::next()
{
	bool hasNext = true;
	bool newStart = false;

	if (!child) {
		if (!childSP) {
			childSP = createProcessor(transformationPlanNode->getChildren()[0], true);
			if (transformationPlanNode->getDimMapping().size()) {
				childSP = PProcessorBase(new RearrangeProcessor(childSP, transformationPlanNode->getDimMapping(), transformationPlanNode->getArea(), transformationPlanNode->getChildren()[0]->getArea()));
			}
		}
		child = childSP.get();
		bool found;
		hasNext = child->move(moveToInKey, &found);
		if (hasNext && !found) {
			moveToInKey = child->getKey();
		}
		lastInKey.clear();
		newStart = true;
	} else {
		// TODO: -jj- expand tail
		hasNext = child->next();
	}

	size_t nextMoveToFirstDim = outKey.size();
	const IdentifiersType *inKey = 0;

	if (hasNext || !newStart) {
		if (hasNext) {
			inKey = &child->getKey();
			const IdentifiersType compareToKey = lastInKey.empty() ? moveToInKey : lastInKey;
			// find first changed dimension of inKey
			for (size_t dim = 0; dim < inKey->size(); dim++) {
				if ((*inKey)[dim] != compareToKey[dim]) {
					nextMoveToFirstDim = dim;
					break;
				}
			}
		} else {
			nextMoveToFirstDim = 0;
		}

		// compare nextMoveToFirstDim to expansionRanges
		ExpansionRangesType::iterator eRFirstIt;
		for (eRFirstIt = expansionRanges.begin(); eRFirstIt != expansionRanges.end(); ++eRFirstIt) {
			if (nextMoveToFirstDim <= eRFirstIt->first.first) {
				break;
			}
		}
		if (eRFirstIt != expansionRanges.end()) {
			ExpansionRangesType::iterator eRIt = --expansionRanges.end();
			for (;;--eRIt) {
				// expand ranges from eRFirstIt to the end of expansionRanges in reverse order
				for (size_t expandDim = eRIt->first.second; expandDim >= eRIt->first.first; expandDim--) {
					if (expansions[expandDim].first) {
						++expansions[expandDim].second;
						if (expansions[expandDim].second == expansions[expandDim].first->end()) {
							expansions[expandDim].second = expansions[expandDim].first->begin();
							outKey[expandDim] = *expansions[expandDim].second;
							if (expandDim == 0) {
								nextResult = false;
								return false;
							}
							// continue to next dimension
						} else {
							moveToInKey = eRIt->second;
							while (++eRIt != expansionRanges.end()) {
								eRIt->second = moveToInKey;
							}
							outKey[expandDim] = *expansions[expandDim].second;
//							if (inKey != moveToInKey) {
								// restart
								childSP->reset();
								child = 0;
//							} else {
								// TODO: -jj- optimize - do not reset child processor - just use the last key and value
//							}
							return next();
						}
					}
				}
				if (eRIt == eRFirstIt) {
					break;
				}
			}
			if (!hasNext) {
				nextResult = false;
				return false;
			}
			if (eRIt == eRFirstIt) {
				// dimensions expanded completely - continue with current value
				moveToInKey = *inKey;
				eRIt->second = moveToInKey;
				while (++eRIt != expansionRanges.end()) {
					eRIt->second = moveToInKey;
				}
			}
		}

		if (!hasNext) {
			nextResult = false;
			return false;
		}
		// transform the key here
		for (DimsMappingType::const_iterator transIt = dimMapping.begin(); transIt != dimMapping.end(); ++transIt) {
			outKey[transIt->first] = (*inKey)[transIt->second];
		}
		lastInKey = *inKey;
		// if the sub-area needs to be replicated
//		if (printOut) {
//			Logger::info << "out " << getKey() << " " << getDouble() << " " << moveToInKey << endl;
//		}
		nextResult = true;
		return true;
	}
	nextResult = false;
	return false;
}

const CellValue &TransformationProcessor::getValue()
{
	if (factor == 1.0) {
		return child->getValue();
	}
	value = child->getValue();
	if (value.isNumeric() && !value.isEmpty()) {
		value = value.getNumeric() * factor;
	}
	return value;
}

double TransformationProcessor::getDouble()
{
	return getValue().getNumeric();
}

void TransformationProcessor::setValue(const CellValue &value)
{
	throw ErrorException(ErrorException::ERROR_INTERNAL, "TransformationProcessor::setValue not supported");
}

const IdentifiersType &TransformationProcessor::getKey() const
{
	if (child && nextResult) {
		return outKey;
	} else {
		return EMPTY_KEY;
	}
}

const GpuBinPath &TransformationProcessor::getBinKey() const
{
	pathTranslator->pathToBinPath(getKey(), const_cast<GpuBinPath &>(binPath));
	return binPath;
}

void TransformationProcessor::reset()
{
	if (child) {
		child->reset();
	}
//	lastInKey.clear();
	nextResult = false;
}

TransformationMapProcessor::TransformationMapProcessor(PEngineBase engine, CPPlanNode node) : TransformationProcessor(engine, node), multiMaps(0), endOfMultiMapping(true), node(node)
{
	const TransformationPlanNode *transformationPlanNode = dynamic_cast<const TransformationPlanNode *>(node.get());
	multiMaps = transformationPlanNode->getSetMultiMaps();
}

bool TransformationMapProcessor::next()
{
	bool result = true;
	if (mapOperations.empty()) {
		result = TransformationProcessor::next();
		mapOperations.clear();
		if (result) {
			size_t dimOrdinal = 0;
			for (SetMultimaps::const_iterator mm = multiMaps->begin(); mm != multiMaps->end(); ++mm, dimOrdinal++) {
				if (!*mm) {
					continue;
				}
				mapOperation mop;
				mop.end = (*mm)->end();
				mop.sourceId = getKey().at(dimOrdinal);
				mop.current = mop.begin = (*mm)->find(mop.sourceId);
				if (mop.begin == mop.end) {
					continue;
				}
				mop.dimOrdinal = dimOrdinal;
				outKey[dimOrdinal] = mop.current->second;
				++mop.current;
				if (mop.current == mop.end || mop.current->first != mop.sourceId) {
					// single mapping
					continue;
				}
				mop.current = mop.begin;
				mop.multiMap = mm->get();
				mapOperations.push_front(mop);
				endOfMultiMapping = false;
			}
		}
	}
	if (result) {
		// change the key
		for (MapOperations::const_iterator mop = mapOperations.begin(); mop != mapOperations.end(); ++mop) {
			outKey[mop->dimOrdinal] = mop->current->second;
		}
		if (endOfMultiMapping) {
			mapOperations.clear();
		} else {
			// next iteration - set multiMapping when last iteration reached
			MapOperations::iterator mop;
			for (mop = mapOperations.begin(); mop != mapOperations.end(); ++mop) {
				++(mop->current);
				if (mop->current == mop->end || mop->current->first != mop->sourceId) {
					mop->current = mop->begin;
				} else {
					break;
				}
			}
			if (mop == mapOperations.end()) {
				endOfMultiMapping = true;
			}
		}
	}
	return result;
}

}
