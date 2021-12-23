#pragma once

#include <stdint.h>
#include <cstddef>

#include <iostream>
#include <fstream>
#include <string>

#include "define.h"

using namespace std;


/*
Represents a sequence of bits.  They are stored in an UINT64_T* array, and each UINT64_T is called a block.
Each block can be accessed, and any range of bits of length 64 or less can also be accessed - even if they span 2 blocks.
*/
class BitString
{
private:

    UINT64_T* blocks;
    UINT64_T m_nbbits;

public:

    /*
    If default constructor is used, the function setNbBits must be called.  Otherwise, no memory is reserved.
    */
    BitString()
    {
        blocks = nullptr;
        m_nbbits = 0;
    }

    BitString(const BitString& src)
    {
        (*this) = src;
    }

    BitString& operator=(const BitString& src)
    {
        m_nbbits = src.m_nbbits;

        uint64_t nb = src.getNbBlocks();
        blocks = new UINT64_T[nb];
        for (UINT64_T i = 0; i < nb; ++i)
        {
            uint64_t bi = src.blocks[i];
            blocks[i] = bi;
        }

        return *this;  // Return a reference to myself.
    }



    BitString(UINT64_T nbbits) : BitString()
    {
        setNbBits(nbbits);
    }

    ~BitString()
    {
        if (blocks) {
            delete[] blocks;
            blocks = nullptr;
        }
    }

    UINT64_T size()
    {
        return m_nbbits;
    }


    UINT64_T getNbBlocks() const
    {
        if (m_nbbits == 0)
            return 0;
        else
            return m_nbbits / 64 + 1;
    }


    /*
    Sets the number of bits.  A new UINT64_T* array is created and the previous blocks, if any, are copied into that new array.
    */
    void setNbBits(UINT64_T nbbits)
    {
        
        UINT64_T* prevblocks = blocks;
        UINT64_T prevNbBlocks = getNbBlocks();

        

        m_nbbits = nbbits;

        uint64_t nb = getNbBlocks();
        blocks = new UINT64_T[ nb ];

        if (!prevblocks)
        {
            for (UINT64_T i = 0; i < getNbBlocks(); ++i)
            {
                blocks[i] = (UINT64_T)0;
            }
        }
        else
        {
            //copy previous blocks
            for (UINT64_T i = 0; i < min(nb, prevNbBlocks); ++i)
            {
                blocks[i] = prevblocks[i];
            }
            for (UINT64_T i = prevNbBlocks; i < nb; ++i)
            {
                blocks[i] = 0;
            }

            delete[] prevblocks;
        }

    }


    /*
    Set an individual bit.  bitpos is from the left
    */
    void setBit(UINT64_T bitpos, bool value)
    {
        UINT64_T block = bitpos / 64;
        UINT64_T offset = bitpos - block * 64;

        UINT64_T bit = static_cast<UINT64_T>(1) << (63 - offset);

        if (value)
        {
            blocks[block] |= bit;
        }
        else
        {
            blocks[block] &= (~bit);
        }
    }


    /*
    Returns the bits at positions [pos .. pos + len - 1]
    In the return value, these bits are put into the rightmost len bits
    */
    UINT64_T getChunk(UINT64_T pos, UINT64_T len)
    {
        UINT64_T block = pos / 64;
        UINT64_T offset = pos - block * 64;

        if (offset + len - 1 < 64)
        {
            UINT64_T bits = getBitsInRange(blocks[block], offset, offset + len - 1);

            UINT64_T t = (63 - offset - len + 1);
            
            //if (t > 63)
            //{
            //    cout << "ERROR : t is way too large, t = " << t << endl;
            //}
            bits = bits >> t;
            return bits;
            //return (bits >> (63 - offset - len + 1)); //had to sit down and calculate that, should work
        }
        else
        {
            UINT64_T bits1 = getBitsInRange(blocks[block], offset, 63);
            //get remainder
            UINT64_T remaining_len = len - (63 - offset + 1);
            UINT64_T bits2 = getBitsInRange(blocks[block + 1], 0, remaining_len);

            return (bits1 << remaining_len) | (bits2 >> (63 - remaining_len + 1));

        }
    }


    /*
    * Set the bits in the range [pos .. pos + len - 1]
    The rightmost len bits from the chunk argument will be used and applied to the chunk location.  
    The leftmost 64 - len bits of chunk should be 0, otherwise the behavior is undefined.
    */
    void setChunk(UINT64_T chunk, UINT64_T pos, UINT64_T len)
    {
        UINT64_T block = pos / 64;
        UINT64_T offset = pos - block * 64;

        if (offset + len - 1 < 64)
        {
            blocks[block] = setBitsInRange(blocks[block], chunk, offset, offset + len - 1);
        }
        else
        {
            //chunk will span 2 blocks --> split it
            UINT64_T len1 = 63 - offset + 1;
            UINT64_T bits1 = getBitsInRange(chunk, 63 - len + 1, 63 - len + len1) >> (len - len1);
            blocks[block] = setBitsInRange(blocks[block], bits1, offset, offset + len1 - 1);

            UINT64_T len2 = len - len1;
            UINT64_T bits2 = getBitsInRange(chunk, 63 - len2 + 1, 63);
            blocks[block + 1] = setBitsInRange(blocks[block + 1], bits2, 0, len2 - 1);
        }
    }


    /*
    Return the bit value at position pos
    */
    bool getBit(UINT64_T pos)
    {
        UINT64_T b = getChunk(pos, 1);
        
        return ((getChunk(pos, 1) & static_cast<UINT64_T>(1)) > 0);
    }


    /*
    * Returns an int in which every bit between start and end is set to 1
    */
    UINT64_T getMask(UINT64_T start, UINT64_T end)
    {
        UINT64_T v1, v2;

        //puts 1's from start to last bit.  The if is needed because we cannot do a 64 bit shift
        if (start == 0)
            v1 = ~0;    //puts 1's everywhere
        else
            v1 = ((static_cast<UINT64_T>(1) << (63 - start + 1)) - 1);

        v2 = ((static_cast<UINT64_T>(1) << (63 - end)) - 1);

        return v1 ^ v2;
    }

    UINT64_T getBitsInRange(UINT64_T theint, UINT64_T start, UINT64_T end)
    {
        return theint & getMask(start, end);
    }


    /*
    Takes the rightmost end - start + 1 bits of thebits, and puts them in the start-end range of theint, and returns the result 
    */
    UINT64_T setBitsInRange(UINT64_T theint, UINT64_T thebits, UINT64_T start, UINT64_T end)
    {
        UINT64_T antimask = ~getMask(start, end);   //zeros in range
        
        UINT64_T len = end - start + 1;

        return (theint & antimask) | (thebits << (63 - len - start + 1));
    }




    string ToString()
    {
        string str = "";
        
        /*for (UINT64_T i = 0; i < getNbBlocks(); ++i)
        {
            str += std::to_string(blocks[i]) + " ";
        }

        str += "\n";*/

        for (UINT64_T i = 0; i < size(); ++i)
        {
            if (i > 0 && i % 64 == 0)
            {
                str += " ";
            }
            bool b = getBit(i);
            if (b)
                str += "1";
            else
                str += "0";
        }

        return str;
    }

    UINT64_T getBlock(UINT64_T block)
    {
        return blocks[block];
    }

};



class BitStringTester
{
public:
    void testInit()
    {


        BitString bstr(100);

        //cout << bstr.ToString() << endl;

        //TEST 1 : Init
        bool ok = true;
        for (UINT64_T i = 0; i < bstr.getNbBlocks(); ++i)
        {
            UINT64_T b = bstr.getBlock(i);
            if (b > 0)
            {
                cout << "Error : block not init at 0" << endl << bstr.ToString() << endl;
                ok = false;
            }
        }

        cout << "Test 1 Init : " << ok << endl;

        //TEST 2 : Set a single bit
        bstr.setBit(63, true);

        ok = true;
        if (bstr.getBlock(0) != 1 || bstr.getBlock(1) != 0)
        {
            cout << "Error : block 0 = " << bstr.getBlock(0) << " block 1 = "<< bstr.getBlock(1) << endl;
            ok = false;
        }

        cout << "Test 2 setBit : " << ok << endl;


        //TEST 3 : getBit
        ok = true;
        for (UINT64_T i = 0; i < bstr.size(); ++i)
        {
            bool b = bstr.getBit(i);
            if ((i != 63 && b) || (i == 63 && !b))
            {
                cout << "Error : bit " << i << " not set correctly" << endl;
                ok = false;
            }
        }
        cout << "Test 3 getBit : " << ok << endl;



        //TEST 4 : Set all bits to 1
        ok = true;
        for (UINT64_T i = 0; i < bstr.size(); ++i)
        {
            bstr.setBit(i, true);
        }
        



        UINT64_T testval = 0;
        for (UINT64_T i = 63; i >= 100 - 64; --i)
        {
            testval += (static_cast<UINT64_T>(1) << i);
        }
        if (!(bstr.getBlock(0) == 18446744073709551615) && !bstr.getBlock(1) == testval)
        {
            cout << "Error : block 0 = " << bstr.getBlock(0) << " vs " << 18446744073709551615 << endl
                << " block 1 = " << bstr.getBlock(1) << " vs " << testval << endl;
            ok = false;
        }

        cout << "Test 4 setAllBits : " << ok << endl;



        //TEST 5 : getAllBits
        ok = true;
        for (UINT64_T i = 0; i < bstr.size(); ++i)
        {
            bool b = bstr.getBit(i);
            if (!b)
            {
                cout << "Error : bit " << i << " not set correctly" << endl;
                ok = false;
            }
        }
        cout << "Test 5 getAllBits : " << ok << endl;


        //TEST 6 : getChunk
        ok = true;
        UINT64_T chunk = bstr.getChunk(0, 64);
        if (chunk != 18446744073709551615)
        {
            ok = false;
            cout << "Error : chunk 1 not set to 18446744073709551615" << "   instead is " << chunk <<endl;
        }

        chunk = bstr.getChunk(67, 4);
        if (chunk != 15)
        {
            ok = false;
            cout << "Error : chunk 1.1 not set to " << 15 << "   instead is " << chunk << endl;
        }



        chunk = bstr.getChunk(64, 100 - 64);
        if (chunk != UINT64_T(pow(2, 100 - 64) - 1))
        {
            ok = false;
            cout << "Error : chunk 2 not set to " << UINT64_T(pow(2, 100 - 64) - 1) << "   instead is " << chunk << endl;
        }

        chunk = bstr.getChunk(47, 32);
        
        if (chunk != 4294967295)
        {
            ok = false;
            cout << "Error : chunk 3 not set to " << 4294967295 << "   instead is " << chunk << endl;
        }

        cout << "Test 6 getChunk : " << ok << endl;




        //TEST 7 : set Chunk - inserts 0100 at position 10
        ok = true;
        UINT64_T toinsert = static_cast<UINT64_T>(1) << 2;  //yeah that's 4, I know
        bstr.setChunk(toinsert, 10, 4);
        

        if (bstr.getBit(10) || !bstr.getBit(11) || bstr.getBit(12) || bstr.getBit(13))
        {
            cout << "Error : 0100 was not inserted correctly.  bstr=" << endl << bstr.ToString() << endl;
            ok = false;
        }
        if (bstr.getChunk(10, 4) != toinsert)
        {
            cout << "Error : getChunk != toinsert, got " << bstr.getChunk(10, 4) << " vs " << toinsert << endl;
            ok = false;
        }
        cout << "Test 7 getChunk : " << ok << endl;



        //TEST 8 : set Chunk - inserts 0010010 at position 61
        ok = true;
        toinsert = UINT64_T(18);
        bstr.setChunk(toinsert, 61, 7);

        
        if (bstr.getBit(61) || bstr.getBit(62) || !bstr.getBit(63) || bstr.getBit(64) || bstr.getBit(65) || !bstr.getBit(66) || bstr.getBit(67))
        {
            cout << "Error : 0010010 was not inserted correctly.  bstr=" << endl << bstr.ToString() << endl;
            ok = false;
        }
        if (bstr.getChunk(61, 7) != toinsert)
        {
            cout << "Error : getChunk != toinsert, got " << bstr.getChunk(61, 7) << " vs " << toinsert << endl;
            ok = false;
        }
        cout << "Test 8 getChunk crossing : " << ok << endl;



        //TEST 9 : resize
        ok = true;
        
        UINT64_T prevblock0 = bstr.getBlock(0);
        UINT64_T prevblock1 = bstr.getBlock(1);

        bstr.setNbBits(200);

        if (bstr.getBlock(0) != prevblock0)
        {
            cout << "Error : block 0 is " << bstr.getBlock(0) << ", expected " << prevblock0 << endl;
            ok = false;
        }
        if (bstr.getBlock(1) != prevblock1)
        {
            cout << "Error : block 1 is " << bstr.getBlock(1) << ", expected " << prevblock1 << endl;
            ok = false;
        }
        if (bstr.getBlock(2) != 0 || bstr.getBlock(3) != 0)
        {
            cout << "Error : new block is not 0 " << endl << bstr.ToString() << endl;
            ok = false;
        }
        cout << "Test 9 getChunk crossing : " << ok << endl;
        


        //TEST 10 : affect whole new block
        ok = true;
        toinsert = ~0;

        bstr.setChunk(toinsert, 128, 64);

        cout << bstr.ToString() << endl;

        if (bstr.getBlock(0) != prevblock0 || bstr.getBlock(1) != prevblock1 || bstr.getBlock(3) != 0)
        {
            cout << "Error : a side block was affected " << endl << bstr.ToString();
            ok = false;
        }
        if (bstr.getBlock(2) != ~0)
        {
            cout << "Error : block 2 is bad " << bstr.getBlock(2) << ", expected " << ~0 << endl;
            ok = false;
        }
        cout << "Test 10 getChunk crossing : " << ok << endl;



    }

};