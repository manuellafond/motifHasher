// SequenceTests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <sstream>
#include <string>
#include <fstream>


#include <vector>
#include <unordered_map>
#include <map>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>

#include <stdint.h>


#include <boost/numeric/ublas/matrix_sparse.hpp>


#include "BitString.h"

namespace ublas = boost::numeric::ublas;



#define bitstring boost::dynamic_bitset<>

using namespace std;




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






bitstring convertStringToBitset(string str)
{
    bitstring retval(str.size() * 2);

    map<char, bool> bits1 = { {'A', true}, {'C', true}, {'G', false}, {'T', false} };
    map<char, bool> bits2 = { {'A', true}, {'C', false}, {'G', true}, {'T', false} };

    for (int i = 0; i < str.size(); ++i)
    {
        retval[2 * i] = bits1[str[i]];
        retval[2 * i + 1] = bits2[str[i]];
    }
    return retval;

}


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

    
    int seqlimit = 20000;
    while (std::getline(infile, line) && dest.size() <= seqlimit)
    {
        
        if (line[0] == '>')
        {
            //cout << "new seq" << endl;
            if (curseqname != "" && curseq != "")
            {
                //bitstring b = convertStringToBitset(curseq);
                //dest.push_back(convertStringToBitset(curseq));
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



bitstring getSubBitstring(bitstring str, int pos, int length)
{
    bitstring b(length);

    for (int i = pos; i <= pos + length - 1; ++i)
    {
        b[i - pos] = str[i];
    }
    

    return b;
}



int bitstringToInt(bitstring str)
{
    //TODO : not the most efficient, I'm sure of it
    int theint = 0;
    int curpower = 1;
    for (int i = str.size() - 1; i >= 0; --i)
    {
        if (str[i])
            theint += curpower;

        curpower *= 2;
    }

    return theint;
}




bitstring intToBitstring(int number, size_t nbbits)
{
    //TODO : not the most efficient, I'm sure of it
    bitstring bstr(nbbits);
    int curpower = 1;
    for (size_t i = 0; i < nbbits; ++i)
    {
        if (number & curpower)
        {
            bstr[nbbits - 1 - i] = true;
        }
        else
        {
            bstr[nbbits - 1 - i] = false;
        }

        curpower *= 2;
    }

    return bstr;
}






class NumberPairArray
{
public:
    int bitsN1;
    int bitsN2;

    int size;

    bitstring arr;

    NumberPairArray() : NumberPairArray(32, 32)
    {

    }

    NumberPairArray(int nbbits1, int nbbits2)
    {
        bitsN1 = nbbits1;
        bitsN2 = nbbits2;

        size = 0;
    }

    void add(bitstring val1, bitstring val2)
    {
        for (int i = 0; i < val1.size(); ++i)
        {
            arr.push_back(val1[i]);
        }
        for (int i = 0; i < val2.size(); ++i)
        {
            arr.push_back(val2[i]);
        }
        size++;
    }

    int indexOf(bitstring val1)
    {
        int curpos = 0;
        bool done = false;
        int i = 0;

        while (curpos < arr.size())
        {
            bitstring substr = getSubBitstring(arr, curpos, bitsN1);

            if (substr == val1)
                return i;

            curpos += (bitsN1 + bitsN2);
            i++;
        }

        return -1;
    }


};















void buildMotifTable(vector<BitString> &seqs, int min, int max) //, unordered_map<bitstring, map<int, int>> &dest)
{
    //boost::container::map<bitstring, NumberPairArray> dest;
     
    //ublas::mapped_matrix<unsigned char> matrix(1048576, seqs.size());

    boost::container::map<uint64_t, vector<int>> dest;  //key = motif, value = strings that have it


    int nbrepeats = 0;
    int maxocc = 0;

    for (int i = 0; i < seqs.size(); ++i)
    {
        boost::container::set<bitstring> placesAdded;

        BitString str = seqs[i];

        for (int len = min; len <= max; len += 2)
        {
            for (int pos = 0; pos < str.size() - len; pos += 2)
            {
                //if (pos % 1000 == 0)
                //    cout << "pos=" << pos << " size=" << seqs[i].size() << endl;
                bitstring substr = getSubBitstring(seqs[i], pos, len);

                /*if (dest[substr].find(i) == dest[substr].end())
                {
                    dest[substr][i] = 0;
                }
                dest[substr][i]++;

                if (dest[substr][i] > maxocc)
                    maxocc = dest[substr][i];
                    */


                //SET VERSION
                /*if (dest[substr].find(i) == dest[substr].end())
                {
                    dest[substr].insert(i);
                }
                if (dest[substr].size() > maxocc)
                    maxocc = dest[substr].size();
                */


                //VECTOR
                if (dest.find(substr) == dest.end())
                {
                    dest[substr] = vector<int>();
                }
                /*bool found = false;
                for (int v = 0; v < dest[substr].size(); ++v)
                {
                    if (dest[substr][v] == i)
                    {
                        found = true;
                        break;
                    }
                }*/
                
                if (placesAdded.find(substr) == placesAdded.end())
                {
                    dest[substr].push_back(i);
                    placesAdded.insert(substr);
                }
                //dest[substr][i]++;
                
                /*if (dest.find(substr) == dest.end())
                {
                    dest[substr] = NumberPairArray(20, 10);
                }
                
                bitstring istr = intToBitstring(i, 20);
                int ipos = dest[substr].indexOf(istr);
                if (ipos == -1)
                {
                    dest[substr].add(istr, intToBitstring(1, 10));
                }*/




                //MATRIX VERSION
                /*int row = bitstringToInt(substr);
                int col = i;
                //cout << "row = " << row << " col = " << col << " curval = " << matrix(row, col) << endl;

                if (matrix(row, col) > 0)
                    nbrepeats += 1;

                matrix(row, col) += 1;
                //cout << "row = " << row << " col = " << col << " curval = " << matrix(row, col) << endl;
                if (matrix(row, col) > maxocc)
                    maxocc = matrix(row, col);*/

                
            }
            
        }

        placesAdded.clear();

        if (i % 100 == 0)
        {
            //cout << i << " sequences done, dest.size() = " << dest.size() << " maxocc = " << maxocc <<endl;
            cout << i << " sequences done, " << " maxocc = " << maxocc << " nbrepeats = " << nbrepeats << endl;
        }
        if (i == 20000)
            return;

    }
    

}




int main()
{
    //string infilename = "C:\\Users\\lafm2722\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    string infilename = "C:\\Users\\Manue\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    
    vector<bitstring> seqs;

    cout << "Loading fasta" << endl;

    loadFasta(infilename, seqs);

    cout << "Done" << endl;

    int totallength = 0;
    for (bitstring str : seqs)
    {
        totallength += str.size() / 2;
    }

    //unordered_map<bitstring, map<int, int>> occurrences;
    //unordered_map<bitstring, map<int, int>> occurrences;
    buildMotifTable(seqs, 20, 20); // , occurrences);

    cout << "nb seqs = " << seqs.size() << endl;
    cout << "total length = " << totallength << endl;
    //cout << "motifs count = " << occurrences.size() << endl;

#if defined(USE_RSS_MEMORY)
    size_t peakSize = getPeakRSS();
    cout << "Peak size = " << peakSize << endl;
#endif
    

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
