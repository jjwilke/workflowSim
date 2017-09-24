#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <PinTunnel.h>

unsigned int HIT_COUNT;
unsigned int MISS_COUNT;


//Pin call
bool exeProg(int argc, const char **argv, std::vector<trace_entry_t> &data);
bool pinCallFini(pid_t &pid, int &status, int &timeout, int argc, const char **argv);

//Input data
void printVector(std::vector<intptr_t> &src);
bool readInTrace(PinTunnel &tunnel, size_t &bufferIndex, std::vector<trace_entry_t> &data);

//Other
int readToResultCount(int readInDataSize);

//Cache sim
void initSimData(std::vector<trace_entry_t> &);
void generateCacheVector(intptr_t, std::vector<trace_entry_t> &, trace_entry_t);
void directCacheSim(std::vector<trace_entry_t>, std::vector<trace_entry_t>);
intptr_t findCacheIndex(intptr_t, intptr_t);
bool NOT_EQUAL(trace_entry_t, trace_entry_t);
void replaceLineData(intptr_t, std::vector<trace_entry_t> &, intptr_t);
void directSimResults(unsigned int);


//-----------------------
//Main
//-----------------------
int main (int argc, char** argv)
{

    //VARIABLES
    // To modify
    std::string call = "write/mmapWriteTunnel";

    // Pin call
    int my_argc = 1;
    const char *programCall[50] = {call.c_str(), nullptr};

    // Execute call and read in trace data
    std::vector<trace_entry_t> traceData;
    bool callSuccess = exeProg(my_argc, programCall, traceData);
    if (callSuccess)
    {
        bool notDone = true;
        while ( notDone )
        {
            /*
             * Cache Simulation
            */
            // Gather user data for cache
            std::vector<intptr_t> cache;
            initSimData(cache);
            unsigned int cache_size = cache.size();

            // Run simulation
            HIT_COUNT = 0;
            MISS_COUNT = 0;
            directCacheSim(traceData, cache);
            directSimResults(cache_size);

            printf("Enter '1' to continue and '0' to stop: ");
            std::cin >> notDone;
        }
    }

    return 0;
}


bool exeProg(int argc, const char **argv, std::vector<trace_entry_t> &data)
{
    pid_t pid;
    int status;
    int timeout;

    //Create a tunnel for trace addresses
    PinTunnel traceTunnel(NUM_OF_BUFFERS, WORKSPACE_LEN, 1);

    pid = fork();
    if (pid < 0)
    {
        perror("Error: Fork failed");
        exit(1);
    }
    if (pid == 0)
    {
        //-----------------------
        // Child Process
        //-----------------------
        printf("Before PIN call...\n");
        if ( execve(argv[0], (char **)argv, nullptr) )
        {
            perror("Error: Program failed to execute");
            return false;
        }

    }

    //-----------------------
    // Parent Process
    //-----------------------

    //Collect Trace
    size_t bufferIndex = 0;
    readInTrace(traceTunnel, bufferIndex, data);

    //TEST
    //printVector(data);

    //Wait for child and summarize fork call
    return pinCallFini(pid, status, timeout, argc, argv);

}


/*
 * Fork functions
 */
bool readInTrace(PinTunnel &tunnel, size_t &bufferIndex, std::vector<trace_entry_t> &result)
{
    //vector to hold pieces of the pin trace
    std::vector<trace_entry_t> readInData;

    while ( true )
    {
        //Collect trace segment, with size according to WORKSPACE_SIZE in macro.h
        //No need to clear readInData before passing it, as it is cleared in read()
        tunnel.readTraceSegment(bufferIndex, readInData);
        //Clear trace buffer
        tunnel.clearBuffer(bufferIndex);

        
        //Add readInData to result vector
        for (int i = 0; i < readInData.size(); i++)
        {
            //if at end of trace, stop reading and exit function
            if ( readInData[i] == END_OF_TRACE )
            {
                return true;
            }
            result.push_back( readInData[i] );
        }
    }
    return false;
}

void printVector(std::vector<intptr_t> &src)
{
    for (int i = 0; i < src.size(); i++)
    {
        std::cout << "[" << i << "]: " << src[i] << std::endl;
    }
}

bool pinCallFini(pid_t &pid, int &status, int &timeout, int argc, const char **argv)
{
    timeout = 1000;
    while (0 == waitpid(pid, &status, WNOHANG))
    {
        if (--timeout < 0)
        {
            perror("Timeout...");
            return false;
        }
        sleep(1);
    }

    std::cout << std::endl << "Fork() Summary:" << std::endl;
    for (int i = 0; i < argc; i++)
    {
        std::cout << "argv[" << i << "]= " << argv[i] << std::endl;
    }
    std::cout << "ExitStatus=" << WEXITSTATUS(status);
    std::cout << " WIFEXITED=" << WIFEXITED(status) << " Status=";
    std::cout << status << std::endl;

    if (0 != WEXITSTATUS(status) || 1 != WIFEXITED(status))
    {
        perror("Call failed");
        return false;
    }
    return true;
}

/*
 * Cache Sim Functions
 */
void initSimData(std::vector<intptr_t> &cache_vector)
{
        // Variables
        int words_per_line = 1;
        int line_size = 8 * words_per_line; //in bytes
        intptr_t cache_size;
        intptr_t num_of_lines;

        std::cout << "Input cache size (in bytes): ";
        std::cin >> cache_size;
        num_of_lines = cache_size / line_size;

        // Initialize cache vector elements with the third function input
        generateCacheVector(num_of_lines, cache_vector, 0);
}

void generateCacheVector(intptr_t numOfLines, std::vector<intptr_t> &src, intptr_t initial_value)
{
        for (int i = 0; i < numOfLines; i++)
        {
                src.push_back(initial_value);
        }
}

void directCacheSim(std::vector<intptr_t> data, std::vector<intptr_t> cache)
{
        intptr_t cacheSize = cache.size();
        for (int i = 0; i < data.size(); i++)
        {
                // Fine line to which the data address maps to
                intptr_t line_index = findCacheIndex(data[i], cacheSize);

                // Report hit or miss
                if ( NOT_EQUAL(data[i], cache[line_index]) )
                {
                        MISS_COUNT++;
                        replaceLineData(data[i], cache, line_index);
                }
                else
                {
                        HIT_COUNT++;
                }
        }

}

intptr_t findCacheIndex(intptr_t address, intptr_t cache_size)
{
        return address % cache_size;
}

bool NOT_EQUAL(intptr_t a, intptr_t b)
{
        if (a != b)
        {
                return true;
        }
        else
        {
                return false;
        }
}

void replaceLineData(intptr_t memory_address, std::vector<intptr_t> &cache, intptr_t line_index)
{
        cache[line_index] = memory_address;
}

void directSimResults(unsigned int cacheSize)
{
        // Calculations
        float ratio = (float)HIT_COUNT / (float)MISS_COUNT;
        unsigned int cacheSizeKiB = (cacheSize*8) / 1024;

        // Results
        std::cout << "Direct Mapping Simulation Results:" << std::endl;
        std::cout << "          Cache Size(KiB) = " << cacheSizeKiB << std::endl;
        std::cout << "                     Hits = " << HIT_COUNT << std::endl;
        std::cout << "                   Misses = " << MISS_COUNT << std::endl;
        std::cout << "           Hit/Miss Ratio = " << std::setprecision(5) << ratio << std::endl << std::endl;
}
