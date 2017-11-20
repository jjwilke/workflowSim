#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
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
std::ofstream fout;


//Pin call
bool exeProg(int argc, const char **argv, std::vector<trace_entry_t> &data);
bool pinCallComplete(pid_t &pid, int &status, int &timeout, int argc, const char **argv);

//Input data
void printVector(std::vector<intptr_t> &src);
bool readInTrace(PinTunnel &tunnel, size_t &bufferIndex, std::vector<trace_entry_t> &data);

//Cache sim
void initSimData(std::vector<trace_entry_t> &);
void modified_initSimData(std::vector<intptr_t> &cache_vector, intptr_t cache_size);
void generateCacheVector(intptr_t, std::vector<trace_entry_t> &, trace_entry_t);
void directCacheSim(std::vector<trace_entry_t>, std::vector<trace_entry_t>);
intptr_t findCacheIndex(intptr_t, intptr_t);
bool NOT_EQUAL(trace_entry_t, trace_entry_t);
void replaceLineData(intptr_t, std::vector<trace_entry_t> &, intptr_t);
void directSimResults(unsigned int);
void direct_sim_results_to_file(unsigned int cacheSize);


//-----------------------
//Main
//-----------------------
int main (int argc, char** argv)
{

	// CALL VARIABLES
	// To modify
	std::string home = "/Users/gvanmou"; //would use $HOME, but execve does not read
    std::string pinCall = home + "/install/pin-3.2-81205-clang-mac/pin";
    std::string pintool = "traceTool.dylib";
    std::string progToTrace = "forkTest";

    // ADD debugging. Does not work properly.
    // std::string debug1 = "-appdebug";
    // std::string debug2 = "-appdebug_lldb_options";
	std::string pinOptions = "-t";
	std::string pintoolCall = "bin/traceTool.dylib";
    std::string progCall = "bin/forkTest";

	// Pin call
    int my_argc = 5;
    const char *programCall[100] = {pinCall.c_str(), pinOptions.c_str(),
					pintoolCall.c_str(), "--", progCall.c_str(), 
                    nullptr};

	// Execute call and read in trace data
    std::vector<trace_entry_t> traceData;

    // Call child and collect address trace in traceData
	bool callSuccess = exeProg(my_argc, programCall, traceData);

    // If the child call was successful run the cache simulation
	if (callSuccess)
	{
        // MANUAL INPUT APPROACH
        //
        // bool notDone = true;
        // while ( notDone )
        // {
        //     /*
        //      * Cache Simulation
        //     */
        //     // Gather user data for cache
        //     std::vector<intptr_t> cache;
        //     initSimData(cache);
        //     unsigned int cache_size = cache.size();

        //     // Run simulation
        //     HIT_COUNT = 0;
        //     MISS_COUNT = 0;
        //     directCacheSim(traceData, cache);
        //     directSimResults(cache_size);

        //     printf("Enter '1' to continue and '0' to stop: ");
        //     std::cin >> notDone;
        // }


        // AUTOMATED APPROACH
        // 
        std::vector<double> cache_sizes;
        int numberOfSizes = 19;
        for (int i = 1; i < numberOfSizes; i++)
        {
            cache_sizes.push_back( 1024 * pow(2,i) );
        }

        std::string fname = "size_vs_ratio.data";
        fout.open("data/" + fname);
        if ( fout.is_open() )
        {
            printf("Running %d cache simulations...please wait...\n", numberOfSizes);
            for (int i = 0; i < cache_sizes.size(); i++)
            {
                /*
                 * Cache Simulation
                */
                // Gather user data for cache
                std::vector<intptr_t> cache;
                modified_initSimData( cache, (intptr_t)cache_sizes[i] );
                unsigned int cache_length = cache.size();

                // Run simulation
                HIT_COUNT = 0;
                MISS_COUNT = 0;
                directCacheSim(traceData, cache);
                direct_sim_results_to_file(cache_length);
            }
            fout.close();
            printf("Simulations complete...\n");
            printf("Data recorded in 'data/%s'\n\n", fname.c_str());
        }
        else
        {
            perror("ERROR unable to open file");
        }	
	}
	
	return 0;
}


bool exeProg(int argc, const char **argv, std::vector<trace_entry_t> &data)
{
	pid_t pid;
	int status;
	int timeout;

    // Parent traceTunnel
    // NUM_OF_BUFFERS and WORKSPACE_LEN set in macros.h
    PinTunnel traceTunnel(NUM_OF_BUFFERS, WORKSPACE_LEN, 1);
    printf("Parent PinTunnel Parameters:\n");
    printf("NUM_OF_BUFFERS = %d\n", NUM_OF_BUFFERS);
    printf("WORKSPACE_LEN = %d\n", WORKSPACE_LEN);

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
		if ( execve(argv[0], (char **)argv, nullptr) )
		{
			perror("Error: Program failed to execute");
            printf("Check path in 'home' variable {main()}\n");
			return false;
		}

	}

	//-----------------------
	// Parent Process
	//-----------------------

    //Collect Trace
    size_t bufferIndex = 0;
    readInTrace(traceTunnel, bufferIndex, data);
	
	//Wait for child and summarize fork call
	return pinCallComplete(pid, status, timeout, argc, argv);	
		
}


/*
 * Fork functions
 */
bool readInTrace(PinTunnel &tunnel, size_t &bufferIndex, std::vector<trace_entry_t> &result)
{
    printf("\nreading in pintool data...\n");

    //need a temp, as readTraceSegment() clears the passed vector
    std::vector<trace_entry_t> readInData;
    int index = 0;
	
	while ( true )
	{
        // Collect trace segment, with size according to WORKSPACE_SIZE in macro.h
        // No need to clear readInData before passing it, as it is cleared in read()
        tunnel.readTraceSegment(bufferIndex, readInData);
        // Clear trace buffer
        tunnel.clearBuffer(bufferIndex);

		//Add readInData to result vector
		for (int i = 0; i < readInData.size(); i++)
		{
			if ( readInData[i] == END_OF_TRACE )
			{
				return true;
			}
			result.push_back( readInData[i] );
			index++;
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

bool pinCallComplete(pid_t &pid, int &status, int &timeout, int argc, const char **argv)
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
    std::cout << status << std::endl << std::endl << std::endl;

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

        printf("Cache Simulation:\n");
        std::cout << "Input cache size (in bytes): ";
        std::cin >> cache_size;
        num_of_lines = cache_size / line_size;

        // Initialize cache vector elements with the third function input
        generateCacheVector(num_of_lines, cache_vector, 0);
}

void modified_initSimData(std::vector<intptr_t> &cache_vector, intptr_t cache_size)
{
        // Variables
        int words_per_line = 1;
        int line_size = 8 * words_per_line; //in bytes
        intptr_t num_of_lines;
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

void direct_sim_results_to_file(unsigned int cacheSize)
{
        unsigned int cacheSizeKiB = (cacheSize*8) / 1024;
        float ratio = (float)HIT_COUNT / (float)MISS_COUNT;

        // Print to file
        fout << cacheSizeKiB << " ";
        fout << std::setprecision(6) << ratio << std::endl;
}
