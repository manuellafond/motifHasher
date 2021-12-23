#pragma once

#include "BitString.h"

#include <boost/container/vector.hpp>



#define IPV_VECTOR 1

#ifdef IPV_VECTOR


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
	UINT64_T nbPairs;
	UINT64_T nbbits1;
	UINT64_T nbbits2;
	
	boost::container::vector<uint32_t> vec;

	//BitString bstr;

public:


	IntegerPairVector() : IntegerPairVector(32, 32)
	{

	}


	IntegerPairVector(UINT64_T nbits1, UINT64_T nbits2)
	{

		nbbits1 = nbits1;
		nbbits2 = nbits2;

		nbPairs = 0;
		//bstr.setNbBits((nbbits1 + nbbits2) * 50);
	}


	/*
	Add a pair of values.  Only the rightmost nbbits1 of val1 are kept, and only the rightmost nbbits2 of val2 are kept.
	nbPairs is increased by 1 after the pair is added.
	*/
	void add(UINT64_T val1, UINT64_T val2)
	{
		vec.push_back(val1);  vec.push_back(val2);
		nbPairs++;
		/*UINT64_T bitsNeeded = (nbbits1 + nbbits2) * (nbPairs + 1);
		if (bstr.size() < bitsNeeded)
		{
			bstr.setNbBits(bstr.size() * 2);
		}
		bstr.setChunk(val1, (nbbits1 + nbbits2) * nbPairs, nbbits1);
		bstr.setChunk(val2, (nbbits1 + nbbits2) * nbPairs + nbbits1, nbbits2);

		nbPairs++;*/
	}


	/*
	Returns the position of the pair that contains val1 as its first value, or -1 if not present.
	Watch out, the return value is a UINT64_T, and -1 is 11111...111.
	*/
	UINT64_T indexOfV1(UINT64_T val1)
	{
		for (UINT64_T i = 0; i < nbPairs; ++i)
		{
			//UINT64_T v = bstr.getChunk(i * (nbbits1 + nbbits2), nbbits1);
			if (vec[2*i] == val1)

			//if (v == val1)
				return i;
		}
		return -1;
	}


	/*
	Get the first value of the index-th pair.
	*/
	UINT64_T getVal1(UINT64_T index)
	{
		return vec[2 * index];
		//UINT64_T pos = (nbbits1 + nbbits2) * index;
		//return bstr.getChunk(pos, nbbits1);
	}

	/*
	Get the second value of the index-th pair.
	*/
	UINT64_T getVal2(UINT64_T index)
	{
		return vec[2 * index + 1];
		//UINT64_T pos = (nbbits1 + nbbits2) * index + nbbits1;
		//return bstr.getChunk(pos, nbbits2);
	}


	/*
	Set the second value of the index-th pair.
	*/
	void setVal2(UINT64_T index, UINT64_T val2)
	{
		vec[2 * index + 1] = val2;
		//UINT64_T pos = (nbbits1 + nbbits2) * index + nbbits1;
		//bstr.setChunk(val2, pos, nbbits2);
	}


	/*
	Returns the number of pairs stored
	*/
	UINT64_T size()
	{
		return nbPairs;
	}

};

#else 



#pragma once

#include "BitString.h"

#include <boost/container/vector.hpp>

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
	UINT64_T nbPairs;
	UINT64_T nbbits1;
	UINT64_T nbbits2;

	

	BitString bstr;

public:


	IntegerPairVector() : IntegerPairVector(32, 32)
	{

	}


	IntegerPairVector(UINT64_T nbits1, UINT64_T nbits2)
	{

		nbbits1 = nbits1;
		nbbits2 = nbits2;

		nbPairs = 0;
		bstr.setNbBits((nbbits1 + nbbits2) * 50);
	}


	/*
	Add a pair of values.  Only the rightmost nbbits1 of val1 are kept, and only the rightmost nbbits2 of val2 are kept.
	nbPairs is increased by 1 after the pair is added.
	*/
	void add(UINT64_T val1, UINT64_T val2)
	{
		
		UINT64_T bitsNeeded = (nbbits1 + nbbits2) * (nbPairs + 1);
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
	Watch out, the return value is a UINT64_T, and -1 is 11111...111.
	*/
	UINT64_T indexOfV1(UINT64_T val1)
	{
		for (UINT64_T i = 0; i < nbPairs; ++i)
		{
			UINT64_T v = bstr.getChunk(i * (nbbits1 + nbbits2), nbbits1);

			if (v == val1)
				return i;
		}
		return -1;
	}


	/*
	Get the first value of the index-th pair.
	*/
	UINT64_T getVal1(UINT64_T index)
	{
		UINT64_T pos = (nbbits1 + nbbits2) * index;
		return bstr.getChunk(pos, nbbits1);
	}

	/*
	Get the second value of the index-th pair.
	*/
	UINT64_T getVal2(UINT64_T index)
	{
		UINT64_T pos = (nbbits1 + nbbits2) * index + nbbits1;
		return bstr.getChunk(pos, nbbits2);
	}


	/*
	Set the second value of the index-th pair.
	*/
	void setVal2(UINT64_T index, UINT64_T val2)
	{
		UINT64_T pos = (nbbits1 + nbbits2) * index + nbbits1;
		bstr.setChunk(val2, pos, nbbits2);
	}


	/*
	Returns the number of pairs stored
	*/
	UINT64_T size()
	{
		return nbPairs;
	}

};








#endif