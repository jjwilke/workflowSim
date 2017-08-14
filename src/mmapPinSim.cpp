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
#include <semaphore.h>
#include <sstmutex.h>
#include <macros.h>
#include <myCircularBuffer.h>

using namespace SST::Core::Interprocess;

unsigned int HIT_COUNT;
unsigned int MISS_COUNT;
sem_t *empty = NULL;
sem_t *full = NULL;

//-----------------------
//Declarations
//-----------------------
//Pin call
bool exeProg(int argc, const char **argv, std::vector<TRACE_TYPE> &data);
bool pinCallFini(pid_t &pid, int &status, int &timeout, int argc, const char **argv);

//Input data
void printVector(std::vector<intptr_t> &src);
bool mmapInit(size_t size, BUFFER* &map, int &fd);
bool mmapClose(size_t size, BUFFER* &map, int &fd);
bool readInTrace(BUFFER* &pinMap, std::vector<TRACE_TYPE> &data);

//Cache sim
void initSimData(std::vector<TRACE_TYPE> &);
void generateCacheVector(intptr_t, std::vector<TRACE_TYPE> &, TRACE_TYPE);
void directCacheSim(std::vector<TRACE_TYPE>, std::vector<TRACE_TYPE>);
intptr_t findCacheIndex(intptr_t, intptr_t);
bool NOT_EQUAL(TRACE_TYPE, TRACE_TYPE);
void replaceLineData(intptr_t, std::vector<TRACE_TYPE> &, intptr_t);
void directSimResults(unsigned int);


//-----------------------
//Main
//-----------------------
int main (int argc, char** argv)
{
    typo
	//VARIABLES
	// To modify
	std::string home = "/Users/gvanmou"; //would use $HOME, but execve does not read
	std::string pinRoot = home + "/install/pin-3.2-81205-clang-mac/";
	std::string pintool = "traceTest.dylib";
	std::string progToTrace = "forkTest";

	std::string pinCall = pinRoot + "pin";
	std::string pinOptions = "-t";
	std::string pintoolCall = "bin/" + pintool;
	std::string progCall = "bin/" + progToTrace;
		
	// Semaphores
	empty = sem_open(EMPTY, O_CREAT, 0664, 1);
	full = sem_open(FULL, O_CREAT, 0664, 0);
	if ( empty == NULL || full == NULL)
	{
		perror("Semaphores were not initialized properly");
		return -1;
	}


	// Pin call
	int my_argc = 5;
	const char *programCall[100] = {pinCall.c_str(), pinOptions.c_str(), 
					pintoolCall.c_str(), "--", progCall.c_str(), 
					nullptr};

	// Execute call and read in trace data
	std::vector<TRACE_TYPE> traceData;
	bool callSuccess = exeProg(my_argc, programCall, traceData);		
	if (callSuccess)
	{


/*		//-----------------------
		//cache sim
		//-----------------------
		// Gather user data for cache
        std::vector<intptr_t> cache;
        initSimData(cache);
        unsigned int cache_size = cache.size();

        // Run simulation
        HIT_COUNT = 0;
        MISS_COUNT = 0;
        directCacheSim(traceData, cache);
        directSimResults(cache_size);
*/				
	}
	
	return 0;
}


bool exeProg(int argc, const char **argv, std::vector<TRACE_TYPE> &data)
{
	pid_t pid;
	int status;
	int timeout;
	
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
			return false;
		}

	}

	//-----------------------
	// Parent Process
	//-----------------------

	//-----------------------
	//collect trace from mmap to vector
	//-----------------------	
	int pinfd;
	BUFFER* pinMap;
	size_t totalSize = (size_t)(BUFFER_SIZE + WORKSPACE_SIZE);

	mmapInit(totalSize, pinMap, pinfd);	
	readInTrace(pinMap, data);
	mmapClose(totalSize, pinMap, pinfd);
	sem_unlink(EMPTY);
	sem_unlink(FULL);

	//-----------------------
	//print collected trace data
	//-----------------------
	//printVector(traceData);
	
	//Wait for child and summarize fork call
	return pinCallFini(pid, status, timeout, argc, argv);	
		
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

void printVector(std::vector<intptr_t> &src)
{
	for (int i = 0; i < src.size(); i++)
	{
		std::cout << "[" << i << "]: " << src[i] << std::endl;
	}
}

bool mmapInit(size_t size, BUFFER* &map, int &fd)
{     
    printf("File: %s\n", PATH);
    fd = open(PATH, O_RDONLY, (mode_t)0600);

    if (fd == -1)
    {
            perror("File did not *open* properly");
            return false;
    }
	
	//struct stat file = {0};
	//if ( fstat(fd, &file) == -1)
	//{
	//	perror("Error collecting the file size.");
	//	return false;
	//}
	//fileSize = file.st_size - 1;

    printf("File size is...%lu bytes\n", size);
    map = (BUFFER *) mmap( NULL, size, PROT_READ, MAP_SHARED, fd, 0 );
    if (map == MAP_FAILED)
    {       
            perror("mmap failed to open");
            return false;
    }
    printf("[Parent] Map pointer atOPEN = %p\n", (void*)map);

    return true;
}

bool mmapClose(size_t size, BUFFER* &map, int &fd)
{
        printf("[Parent] Map pointer atCLOSE = %p\n", (void*)map);
        if (munmap(map, size) == -1)
        {
                perror("munmap failed");
                return false;
        }
        if (close(fd) == -1)
        {
                perror("file did not close");
                return false;
        }

        return true;
}

bool readInTrace(BUFFER* &pinMap, std::vector<TRACE_TYPE> &result)
{
	std::vector<TRACE_TYPE> temp;
	BUFFER pinBuf(WORKSPACE_LEN);
	
	while ( true )
	{
		//Wait until there is something in the buffer to read
		printf("waiting to read...\n");
		//-----------------------
        	//Critical Region
        	//-----------------------	
		sem_wait(full);
		pinBuf = pinMap[0];
		if ( pinBuf.read(temp) ) 
		{
			pinBuf.clear();
			pinMap[0] = pinBuf;
		}
		sem_post(empty);
        	//-----------------------	
		
		printf("Adding vector to result of %lu elements\n", temp.size());

		//Add temp to result vector
		int index = 0;
		for (int i = 0; i < temp.size(); i++)
		{
			if ( temp[i] == END_OF_TRACE )
			{
				return true;
			}
			result[index] = temp[i];
			index++;
		}
	}
	return false;
}

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
