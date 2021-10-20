#pragma once

#include "BitString.h"


/*
This class stores pairs of integers efficiently in memory.  This class does not store key/value pairs, just pairs.  That is, 
two pairs may be identical in their first or second value, or both.

The user may specify a number nbbits1 of bits for the first value of each pair, 
and a number nbbits2 for the second value, and the space required for each pair will be (nbbits1 + nbbits2) for each bit 
The class uses a BitString to store the pairs, and no padding space is lost to fit this number into a multiple of 64 
or anything of the sorts (except maybe at trailing 64 bits at the end).  
In other words, storing n elements requires at most n * (nbbits + nbbits) + 64 bits of memory.
*/
class IntegerPairVector
{
private:
	size_t nbPairs;
	size_t nbbits1;
	size_t nbbits2;
	

	BitString bstr;

public:


	IntegerPairVector() : IntegerPairVector(32, 32)
	{

	}


	IntegerPairVector(size_t nbits1, size_t nbits2)
	{
		nbbits1 = nbits1;
		nbbits2 = nbits2;

		nbPairs = 0;
		bstr.setNbBits((nbbits1 + nbbits2) * 10);
	}


	/*
	Add a pair of values.  Only the rightmost nbbits1 of val1 are kept, and only the rightmost nbbits2 of val2 are kept.
	nbPairs is increased by 1 after the pair is added.
	*/
	void add(uint64_t val1, uint64_t val2)
	{
		size_t bitsNeeded = (nbbits1 + nbbits2) * (nbPairs + 1);
		if (bstr.size() < bitsNeeded)
		{
			bstr.setNbBits(bstr.size() * 2);
		}
		bstr.setChunk(val1, (nbbits1 + nbbits2) * nbPairs, nbbits1);
		bstr.setChunk(val2, (nbbits1 + nbbits2) * nbPairs + nbbits1, nbbits2);

		nbPairs++;
	}


	/*
	Returns the position of the pair that contains val1 as its first value, or -1 if not present.
	Watch out, the return value is a size_t, and -1 is 11111...111.
	*/
	size_t indexOfV1(uint64_t val1)
	{
		for (size_t i = 0; i < nbPairs; ++i)
		{
			uint64_t v = bstr.getChunk(i * (nbbits1 + nbbits2), nbbits1);
			if (v == val1)
				return i;
		}
		return -1;
	}


	/*
	Get the first value of the index-th pair.
	*/
	uint64_t getVal1(size_t index)
	{
		size_t pos = (nbbits1 + nbbits2) * index;
		return bstr.getChunk(pos, nbbits1);
	}

	/*
	Get the second value of the index-th pair.
	*/
	uint64_t getVal2(size_t index)
	{
		size_t pos = (nbbits1 + nbbits2) * index + nbbits1;
		return bstr.getChunk(pos, nbbits2);
	}


	/*
	Set the second value of the index-th pair.
	*/
	void setVal2(size_t index, uint64_t val2)
	{
		size_t pos = (nbbits1 + nbbits2) * index + nbbits1;
		bstr.setChunk(val2, pos, nbbits2);
	}


	/*
	Returns the number of pairs stored
	*/
	size_t size()
	{
		return nbPairs;
	}

};

