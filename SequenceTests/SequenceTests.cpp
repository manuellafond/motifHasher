// SequenceTests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <execution>

#include <sstream>
#include <string>
#include <fstream>


#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>

#include <boost/container/map.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/set.hpp>

/*
#define EIGEN_USE_BLAS 1
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <blaze/Blaze.h>
*/



#include "define.h"


#include <stdint.h>


#include "BitString.h"
#include "IntegerPairVector.h"

#include "hopscotch-map-master/include/tsl/hopscotch_map.h"


using namespace std;

#include <chrono>
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

//NOTE : fasta file to load is hard-coded in the main!


int seqlimit = 115000;


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

    for (UINT64_T i = 0; i < str.size(); ++i)
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
Counts the number of bits set to true in a UINT64_T.  Copy-pasted from Wikipedia I believe
*/
const UINT64_T __m1 = 0x5555555555555555; //binary: 0101...
const UINT64_T __m2 = 0x3333333333333333; //binary: 00110011..
const UINT64_T __m4 = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
const UINT64_T __m8 = 0x00ff00ff00ff00ff; //binary:  8 zeros,  8 ones ...
const UINT64_T __m16 = 0x0000ffff0000ffff; //binary: 16 zeros, 16 ones ...
const UINT64_T __m32 = 0x00000000ffffffff; //binary: 32 zeros, 32 ones
const UINT64_T __h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...
UINT64_T popcount64(UINT64_T x)
{
    x -= (x >> 1) & __m1;             //put count of each 2 bits into those 2 bits
    x = (x & __m2) + ((x >> 2) & __m2); //put count of each 4 bits into those 4 bits 
    x = (x + (x >> 4)) & __m4;        //put count of each 8 bits into those 8 bits 
    return (x * __h01) >> 56;  //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
}






/*
This is the f-scoring function from the paper!
*/
double getScore(UINT64_T motif, UINT64_T occ1, UINT64_T occ2)
{
    //size_t motifLength = popcount64(motif);
    UINT64_T motiflen_mask = ((static_cast<UINT64_T>(1) << LENGTH_NB_BITS) - 1) << (64 - LENGTH_NB_BITS);
    UINT64_T motifLength = (motif & motiflen_mask);

    return motifLength * min(occ1, occ2) / max(occ1, occ2);
}




UINT64_T getMotifKey(UINT64_T motif, UINT64_T len)
{
    UINT64_T key = motif;
    key |= (UINT64_T(len) << (64 - LENGTH_NB_BITS));
    return key;
}






/*
Calculates score of query versus all sequences in seqs.  Precomputed motifsTable is required.
NOTE : AS OF NOW, WILL ONLY WORK IF min = max.  SUPPORT FOR min != max will need to use getMotifKey to encode the length in the motif
*/
vector<float> computeScores(BitString& query, uint64_t minSeqIndex, uint64_t maxSeqIndex,
    UINT64_T min, UINT64_T max, boost::unordered_map<uint64_t, vector<uint32_t>> &motifs)
{
    uint64_t nbseqs = maxSeqIndex - minSeqIndex + 1;

    //fist build occurrence map for query
    unordered_map< UINT64_T, uint32_t > occurrences;

    for (UINT64_T len = min; len <= max; len += 2)
    {
        for (UINT64_T pos = 0; pos < query.size(); pos += 2)
        {
            UINT64_T substr = query.getChunk(pos, len);

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


    //then copute scores
    vector<float> scores(nbseqs);
    for (int i = 0; i < scores.size(); ++i)
    {
        scores[i] = 0;
    }
    
    //for each motif, add relevant scores
    for (auto it = occurrences.begin(); it != occurrences.end(); ++it)
    {
        UINT64_T substr = it->first;
        float occ1 = (float)it->second;
    
        if (motifs.find(substr) != motifs.end())
        {
            vector<uint32_t>& v = motifs[substr];
            for (uint32_t i = 0; i < v.size(); i += 2)
            {
                uint32_t seqindex = v[i + 1];
                float occ2 = (float)v[i];

                float minocc = (occ1 < occ2) ? occ1 : occ2;
                float maxocc = (occ1 >= occ2) ? occ1 : occ2;

                //TODO : hard-coded length
                scores[seqindex - minSeqIndex] += 20 * minocc / maxocc;
            }
            
        }
        
        
    }

    return scores;
}









/*
Calculates the motif occurrence hash map for each sequence in seqs.  min and max refer to the number of BITS!  Since each character is on two bits,
to evaluate motifs of length between 3 and 5 nucleotides, the user should pass min = 6 and max = 10.
We use a boost map because it is more space efficient than the std::map.
In the returned map, keys are the motifs represented as bitstrings, which must fit in 64 bits.
counts is the return value.  It is a vector with pairs.  First pair value = nb occurrences, second pair value = seqindex.
For instance, counts = [4 1 8 2 10 4]  means that seq 1 has 4 occurrences, seq 2 has 8 occurrences, seq 4 has 10 occurrences
*/
void buildMotifTable(vector<BitString>& seqs, uint64_t minSeqIndex, uint64_t maxSeqIndex, UINT64_T min, UINT64_T max, 
    boost::unordered_map<uint64_t, vector<uint32_t>> &counts)
{

    

    for (uint64_t seqindex = minSeqIndex; seqindex <= maxSeqIndex; ++seqindex)
    {
        BitString& str = seqs[seqindex];


        for (uint64_t len = min; len <= max; len += 2)
        {
            for (uint64_t pos = 0; pos < str.size() - len; pos += 2)
            {
                uint64_t substr = str.getChunk(pos, len);

                if (counts.find(substr) == counts.end())
                {
                    counts[substr].push_back(1);
                    counts[substr].push_back(seqindex);
                }
                else
                {
                    vector<uint32_t>& v = counts[substr];
                    if (v.back() != seqindex)
                    {
                        v.push_back(1);
                        v.push_back(seqindex);
                    }
                    else
                    {
                        v[v.size() - 2]++;
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
    //return motifs;

}







/*
void testEigenVectorXf()
{
    int siz = 5000;
    auto t1x = high_resolution_clock::now();
    Eigen::VectorXf v(siz);
    Eigen::VectorXf w(siz);
    for (int i = 0; i < v.size(); ++i)
    {
        v[i] = i;
        w[i] = v.size() - i;
    }

    Eigen::VectorXf z(siz);
    for (int j = 0; j < 1000000; ++j)
    {
        z = v.cwiseMin(500) + w.cwiseMax(500);
    }
    cout << z[0] << endl;
    auto t2x = high_resolution_clock::now();
    duration<double, std::milli> totaltimex = t2x - t1x;

    cout << "time = " << totaltimex.count() << endl;
    //return 0;

}



void testEigenVector()
{
    int siz = 5000;
    auto t1x = high_resolution_clock::now();
    Eigen::Array<float, Eigen::Dynamic, 1> v(siz);
    Eigen::Array<float, Eigen::Dynamic, 1> w(siz);
    for (int i = 0; i < v.size(); ++i)
    {
        v[i] = i;
        w[i] = v.size() - i;
    }

    Eigen::Array<float, Eigen::Dynamic, 1> z(siz);
    for (int j = 0; j < 1000000; ++j)
    {
        z = v.min(500) + w.max(500);
    }
    cout << z[0] << endl;
    auto t2x = high_resolution_clock::now();
    duration<double, std::milli> totaltimex = t2x - t1x;

    cout << "time = " << totaltimex.count() << endl;
    //return 0;

}



void testSparseVector()
{
    int siz = 5000;
    auto t1x = high_resolution_clock::now();
    Eigen::SparseMatrix<float> v(siz, 1);
    Eigen::SparseMatrix<float> w(siz, 1);

    vector<Eigen::Triplet<float>> vtrip;
    for (int i = 0; i < v.size(); i += 10)
    {
        
        vtrip.push_back( Eigen::Triplet<float>(i, 0, (float)i));
    }
    v.setFromTriplets(vtrip.begin(), vtrip.end());
    w.setFromTriplets(vtrip.begin(), vtrip.end());

    Eigen::Array<float, Eigen::Dynamic, 1> z(siz);
    for (int j = 0; j < 1000000; ++j)
    {
        
        for (int k = 0; k < v.outerSize(); ++k)
        {
            for (Eigen::SparseMatrix<float>::InnerIterator it(v, k); it; ++it)
            {
                z[0] = z[0] + it.value();
            }
        }
        //z = v.toDense().cwiseMin(500).cwiseQuotient(w.toDense().cwiseMax(500));
    }
    cout << z[0] << endl;
    auto t2x = high_resolution_clock::now();
    duration<double, std::milli> totaltimex = t2x - t1x;

    cout << "time = " << totaltimex.count() << endl;
    //return 0;

}



void testVector()
{
    int siz = 5000;
    auto t1x = high_resolution_clock::now();
    vector<float> v(siz);
    vector<float> w(siz);
    for (int i = 0; i < v.size(); ++i)
    {
        v[i] = i;
        w[i] = v.size() - i;
    }

    vector<float> z(siz);
    for (int j = 0; j < 1000000; ++j)
    {
        for (int i = 0; i < v.size(); i++)
        {
            z[i] = v[i] + w[i];
        }
        
    }
    cout << z[0] << endl;
    auto t2x = high_resolution_clock::now();
    duration<double, std::milli> totaltimex = t2x - t1x;

    cout << "time = " << totaltimex.count() << endl;
    //return 0;

}
*/



int main()
{

    

    //testSparseVector();
    //testEigenVectorXf();
    //testEigenVector();
    //testVector();

    

    /*BitStringTester test;
    test.testInit();

    return 0;*/

    //string infilename = "C:\\Users\\Manue\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\homo_sapiens.fasta";
    string infilename = "C:\\Users\\Manuel\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\homo_sapiens.fasta";
    //string infilename = "C:\\Users\\lafm2722\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\homo_sapiens.fasta";

    uint64_t partitionSize = 5000;
    uint64_t curSeqIndex = 0;
    uint64_t topToKeep = 0;

    UINT64_T min = 20;
    UINT64_T max = 20;

    //forget this, this is a test to see how many values Eigen could store
    /*Eigen::SparseMatrix<float, Eigen::RowMajor> motifs(pow(2, max), 22000);
    motifs.reserve(500000000);
    cout << "declared it" << endl;
    
    for (int i = 0; i < 1000; ++i)
    {
        for (int j = 0; j < 1000; ++j)
        {
            motifs.coeffRef(i * j, j) += 1;
        }
        if (i % 10 == 0)
        {
            cout << "i=" << i << endl;
        }
        
    }
    cout << "filled it" << endl;*/

    vector<BitString> seqs;

    cout << "Loading fasta" << endl;

    loadFasta(infilename, seqs);

    cout << "Done" << endl;

    int ttllen = 0;
    for (BitString b : seqs)
    {
        ttllen += b.size() / 2;
    }
    cout << "Total length = " << ttllen << endl;


    map<uint64_t, vector< pair<uint32_t, float> > >  allScores;

    while (curSeqIndex < seqs.size())
    {
        boost::unordered_map<uint64_t, vector<uint32_t>> motifs;

        auto t1 = high_resolution_clock::now();

        buildMotifTable(seqs, curSeqIndex, curSeqIndex + partitionSize - 1, min, max, motifs);
        
        auto t2 = high_resolution_clock::now();
        duration<double, std::milli> time_motifsTable = t2 - t1;
        cout << "Motifs table time = " << time_motifsTable.count() << endl;


        t1 = high_resolution_clock::now();

        //TODO : this parallelizes the queries, but each sub call does not know which query index it is executing.
        // Something needs to be done to store scores for each sequence
        int nbdone = 0;
        std::for_each(
            std::execution::par_unseq, 
            seqs.begin(),
            seqs.end(),
            [curSeqIndex, partitionSize, min, max, &motifs, &nbdone](auto&& item)
            {
                auto i_scores =
                    computeScores(item, curSeqIndex, curSeqIndex + partitionSize - 1, min, max, motifs);

                nbdone++;

                if (nbdone % 100 == 0)
                    cout << "nbdone = "<< nbdone << endl;
                //for (uint64_t s = 0; s < topToKeep; s++)
                //{
                //    allScores[i].push_back(make_pair(s + curSeqIndex, i_scores[s]));
                //}
            });
        

        t2 = high_resolution_clock::now();

        duration<double, std::milli> totaltime = t2 - t1;

        cout << "Query time = " << totaltime.count() << endl;


        cout << "nb seqs = " << seqs.size() << endl;
        
        //cout << "motifs count = " << occurrences.size() << endl;
        curSeqIndex += partitionSize;

        return 0;
    }

    

#if defined(USE_RSS_MEMORY)
    size_t peakSize = getPeakRSS();
    cout << "Peak size = " << peakSize << endl;
#endif
    

    return 0;
}
