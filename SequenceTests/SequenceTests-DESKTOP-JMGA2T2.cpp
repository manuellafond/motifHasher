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


#include <boost/numeric/ublas/matrix_sparse.hpp>

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


void loadFasta(string filename, vector<bitstring>& dest)
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
                bitstring b = convertStringToBitset(curseq);
                dest.push_back(convertStringToBitset(curseq));

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
        bitstring b = convertStringToBitset(curseq);
        dest.push_back(convertStringToBitset(curseq));
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
    for (int i = 0; i < str.size(); ++i)
    {
        if (str[i])
            theint += curpower;

        curpower *= 2;
    }

    return theint;
}




void buildMotifTable(vector<bitstring> &seqs, int min, int max) //, unordered_map<bitstring, map<int, int>> &dest)
{
    //boost::container::map<bitstring, boost::container::set<int>> dest;
    ublas::compressed_matrix<int> matrix(1048576, seqs.size());

    int maxocc = 0;
    for (int i = 0; i < seqs.size(); ++i)
    {
        for (int len = min; len <= max; len += 2)
        {
            for (int pos = 0; pos < seqs[i].size() - len; ++pos)
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

                /*if (dest.find(substr) == dest.end())
                {
                    dest[substr] = vector<char>(0, 0);
                }
                dest[substr][i]++;
                */

                //MATRIX VERSION
                int row = bitstringToInt(substr);
                int col = i;
                matrix(row, col) += 1;
                

                
            }
            
        }

        if (i % 1 == 0)
        {
            //cout << i << " sequences done, dest.size() = " << dest.size() << " maxocc = " << maxocc <<endl;
            cout << i << " sequences done, " << " maxocc = " << maxocc <<endl;
        }
        if (i == 5000)
            return;

    }
    

}




int main()
{
    string infilename = "C:\\Users\\lafm2722\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    //string infilename = "C:\\Users\\Manue\\OneDrive - USherbrooke\\Bureau\\tmp\\sequences\\GRCh38_latest_rna.fna";
    
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
