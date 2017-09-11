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


bool exeProg(int argc, const char **argv, std::vector<trace_entry_t> &data)
{
    pid_t pid;
    int status;
    int timeout;

    //Make sure initial tunnel exists
    PinTunnel traceTunnel(NUM_OF_BUFFERS, WORKSPACE_LEN, 1);
    printf("PinTunnel Created!!!\n");

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
    std::vector<trace_entry_t> readInData;

    while ( true )
    {
        //Wait until there is something in the cir_buf_t to read
        printf("waiting to read...\n");

        //Collect trace segment, with size according to WORKSPACE_SIZE in macro.h
        //No need to clear readInData before passing it, as it is cleared in read()
        tunnel.readTraceSegment(bufferIndex, readInData);
        //Clear trace buffer
        tunnel.clearBuffer(bufferIndex);

        /*
        *  NEED TO CHANGE BELOW SECTION
        */
        //Add readInData to result vector
        int iterations = readToResultCount( readInData.size() );
        printf("readInData.size() = %lu\n", readInData.size());
        printf("iterations = %d\n", iterations);

        for (int i = 0; i < iterations; i++)
        {
            // printf("[%d]\n", i);
            if ( readInData[i] == END_OF_TRACE )
            {
                return true;
            }
            result.push_back( readInData[i] );
        }
    }
    return false;
}

int readToResultCount(int readInDataSize)
{
    if (readInDataSize > WORKSPACE_LEN)
    {
        return 0;
    }
    else if (readInDataSize < WORKSPACE_LEN)
    {
        return readInDataSize;
    }
    return WORKSPACE_LEN;
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
