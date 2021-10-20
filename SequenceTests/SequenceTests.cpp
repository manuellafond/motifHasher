// SequenceTests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <sstream>
#include <string>
#include <fstream>


#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>

#include <boost/container/map.hpp>
#include <boost/container/set.hpp>

#include <stdint.h>




#include "BitString.h"
#include "IntegerPairVector.h"



using namespace std;


//NOTE : fasta file to load is hard-coded in the main!



/*******************************************************************
* THIS IS COPY PASTED STUFF FROM STACK OVERFLOW TO CALCULATE MAX MEMORY USAGE
*******************************************************************/
#define USE_RSS_MEMORY 1

#if defined(USE_RSS_MEMORY)

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif





 /**
  * Returns the peak (maximum so far) resident set size (physical
  * memory use) measured in bytes, or zero if the value cannot be
  * determined on this OS.
  */
size_t getPeakRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
        return (size_t)0L;      /* Can't open? */
    if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo))
    {
        close(fd);
        return (size_t)0L;      /* Can't read? */
    }
    close(fd);
    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;          /* Unsupported. */
#endif
}





/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
        (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return (size_t)0L;      /* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
        return (size_t)0L;      /* Can't open? */
    if (fscanf(fp, "%*s%ld", &rss) != 1)
    {
        fclose(fp);
        return (size_t)0L;      /* Can't read? */
    }
    fclose(fp);
    return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L;          /* Unsupported. */
#endif
}

#endif

/*******************************************************************
* END OF COPY PASTED STUFF FROM STACK OVERFLOW TO CALCULATE MAX MEMORY USAGE
*******************************************************************/





/*
Converts an ACGT string to a bitstring where each character needs only 2 bits.
See code below for the encoding
*/
BitString convertDNAToBitString(string str)
{
    BitString retval(str.size() * 2);

    map<char, bool> bits1 = { {'A', true}, {'C', true}, {'G', false}, {'T', false} };
    map<char, bool> bits2 = { {'A', true}, {'C', false}, {'G', true}, {'T', false} };

    for (size_t i = 0; i < str.size(); ++i)
    {
        retval.setBit(2 * i, bits1[str[i]]);
        retval.setBit(2 * i + 1, bits2[str[i]]);
    }
    return retval;
}


void loadFasta(string filename, vector<BitString>& dest)
{
    std::ifstream infile(filename);
    string line;
    string curseqname = "";
    string curseq = "";

    
    int seqlimit = 100000;
    while (std::getline(infile, line) && dest.size() <= seqlimit)
    {
        
        if (line[0] == '>')
        {
            if (curseqname != "" && curseq != "")
            {
                dest.push_back(convertDNAToBitString(curseq));

                if (dest.size() % 5000 == 0)
                {
                    cout << dest.size() << " seqs loaded" << endl;
                }
            }
            curseq = "";
            curseqname = line;
        }
        else
        {
            curseq += line;
        }
        
        
    }

    //make sure we get that last one in buffer
    if (curseqname != "" && curseq != "")
    {
        dest.push_back(convertDNAToBitString(curseq));
    }

}



/*
Counts the number of bits set to true in a uint64_t.  Copy-pasted from Wikipedia I believe
*/
const uint64_t __m1 = 0x5555555555555555; //binary: 0101...
const uint64_t __m2 = 0x3333333333333333; //binary: 00110011..
const uint64_t __m4 = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
const uint64_t __m8 = 0x00ff00ff00ff00ff; //binary:  8 zeros,  8 ones ...
const uint64_t __m16 = 0x0000ffff0000ffff; //binary: 16 zeros, 16 ones ...
const uint64_t __m32 = 0x00000000ffffffff; //binary: 32 zeros, 32 ones
const uint64_t __h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...
size_t popcount64(uint64_t x)
{
    x -= (x >> 1) & __m1;             //put count of each 2 bits into those 2 bits
    x = (x & __m2) + ((x >> 2) & __m2); //put count of each 4 bits into those 4 bits 
    x = (x + (x >> 4)) & __m4;        //put count of each 8 bits into those 8 bits 
    return (x * __h01) >> 56;  //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
}






/*
This is the f-scoring function from the paper!
*/
double getScore(uint64_t motif, size_t occ1, size_t occ2)
{
    size_t motifLength = popcount64(motif);
    return motifLength * min(occ1, occ2) / max(occ1, occ2);
}








/*
Calculates score of query versus all sequences in seqs.  Precomputed motifsTable is required.
*/
void computeScores(BitString& query, vector<BitString>& seqs, boost::container::map<uint64_t, IntegerPairVector>& motifsTable, size_t min, size_t max)
{
    boost::container::map < uint64_t, uint64_t > occurrences;

    for (size_t len = min; len <= max; len += 2)
    {
        for (size_t pos = 0; pos < query.size(); pos += 2)
        {
            uint64_t substr = query.getChunk(pos, len);

            if (occurrences.find(substr) == occurrences.end())
            {
                occurrences[substr] = 1;
            }
            else
            {
                occurrences[substr] += 1;
            }
        }
    }


    vector< pair<size_t, double> > scores(seqs.size());  //first index = seqnum, second = score

    for (size_t i = 0; i < seqs.size(); ++i)
    {
        scores[i] = make_pair(i, 0);
    }

    for (auto it = occurrences.begin(); it != occurrences.end(); ++it)
    {
        uint64_t substr = it->first;
        uint64_t occ1 = it->second;

        IntegerPairVector vec = motifsTable[substr];

        for (size_t i = 0; i < vec.size(); ++i)
        {
            uint64_t seqnum = vec.getVal1(i);
            uint64_t occ2 = vec.getVal2(i);

            
            scores[seqnum].second += getScore(substr, occ1, occ2);
        }
    }



    std::sort(scores.begin(), scores.end(), [=](std::pair<size_t, double>& a, std::pair<size_t, double>& b)
        {
            return a.second < b.second;
        });

    
    for (size_t i = scores.size() - 1; i >= scores.size() - 6; --i)
    {
        auto sc = scores[i];
        cout << sc.first << ":" << sc.second << "   ";
    }
    cout << endl;
    
}







/*
Calculates the motif occurrence hash map for each sequence in seqs.  min and max refer to the number of BITS!  Since each character is on two bits,
to evaluate motifs of length between 3 and 5 nucleotides, the user should pass min = 6 and max = 10.
We use a boost map because it is more space efficient than the std::map.
In the returned map, keys are the motifs represented as bitstrings, which must fit in 64 bits.  
Values are IntegerPairVectors that stores pairs.  First pair value = sequence index, second pair value = nb occurrences.
*/
boost::container::map<uint64_t, IntegerPairVector> buildMotifTable(vector<BitString> &seqs, size_t min, size_t max) //, unordered_map<bitstring, map<int, int>> &dest)
{
    //boost::container::map<bitstring, NumberPairArray> dest;
     
    //ublas::mapped_matrix<unsigned char> matrix(1048576, seqs.size());

    boost::container::map<uint64_t, IntegerPairVector> dest;  //key = motif, value = strings that have it

    uint64_t nbbits1 = 18;
    uint64_t nbbits2 = 16;


    for (size_t seqindex = 0; seqindex < seqs.size(); ++seqindex)
    {
        
        BitString str = seqs[seqindex];
        size_t s = str.size();

        for (size_t len = min; len <= max; len += 2)
        {
            for (size_t pos = 0; pos < str.size(); pos += 2)
            {
                uint64_t substr = str.getChunk(pos, len);
                
                
                if (dest.find(substr) == dest.end())
                {
                    dest[substr] = IntegerPairVector(nbbits1, nbbits2);
                    dest[substr].add(seqindex, 1);
                }
                else
                {
                    //since sequences are handled in order, last entry must contain it
                    size_t lastIndex = dest[substr].size() - 1;
                    uint64_t lastval = dest[substr].getVal1(lastIndex);
                    if (lastval != seqindex)
                    {
                        dest[substr].add(seqindex, 1);
                    }
                    else
                    {
                        uint64_t newval = dest[substr].getVal2(lastIndex) + 1;

                        if (newval >= pow(2, nbbits2))
                        {
                            cout << "ERROR : occurrence number above " << pow(2, nbbits2) << endl;
                        }
                        dest[substr].setVal2(lastIndex, newval);
                    }
                }
            }
            
        }


        if (seqindex % 100 == 0)
        {
            //cout << i << " sequences done, dest.size() = " << dest.size() << " maxocc = " << maxocc <<endl;
            cout << seqindex << " sequences done, " << endl;
        }

       

    }
    
    //copy ellision will do its magic

    return dest;

}




int main()
{
    /*BitStringTester test;
    test.testInit();

    return 0;*/


    //string infilename = "C:\\Users\\lafm2722\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    //string infilename = "C:\\Users\\Manue\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    string infilename = "C:\\Users\\Manuel\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    
    vector<BitString> seqs;

    cout << "Loading fasta" << endl;

    loadFasta(infilename, seqs);

    cout << "Done" << endl;

    int totallength = 0;
    for (BitString str : seqs)
    {
        totallength += str.size() / 2;
    }

    //unordered_map<bitstring, map<int, int>> occurrences;
    //unordered_map<bitstring, map<int, int>> occurrences;

    size_t min = 6;
    size_t max = 16;

    auto motifsTable = buildMotifTable(seqs, min, max); // , occurrences);

    for (size_t i = 0; i < 100; ++i)
    {
        cout << "SEQ " << i << "   ";
        computeScores(seqs[i], seqs, motifsTable, min, max);
    }
    


    cout << "nb seqs = " << seqs.size() << endl;
    cout << "total length = " << totallength << endl;
    //cout << "motifs count = " << occurrences.size() << endl;

#if defined(USE_RSS_MEMORY)
    size_t peakSize = getPeakRSS();
    cout << "Peak size = " << peakSize << endl;
#endif
    

    return 0;
}
