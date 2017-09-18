#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <queue>
#include <sys/types.h>

#include <PinTunnel.h>

const std::string TRACE_FILE = "write/trace.out";

//Declarations
void traceFileToQueue(std::queue<intptr_t> &src, int fileLen);
int findFileLength();


//simulates the pintool
int main (int argc, char** argv)
{
	//-------------------
	//Init queue
	//-------------------
    std::queue<trace_entry_t> fileData;
    std::queue<trace_entry_t> toWrite;
    traceFileToQueue(fileData, findFileLength());
    size_t const traceLength = fileData.size();
    size_t const iterations = traceLength / WORKSPACE_LEN;

    PinTunnel traceTunnel;
    size_t buffer = 0;

    //Write to tunnel
    for (int i = 0; i < iterations; i++)
    {
        //Break trace into sections
        for (int j = 0; j < WORKSPACE_LEN; j++)
        {
            toWrite.push( fileData.front() );
            fileData.pop();
        }

        printf("\n-->[%d] ", i);
        printf("queue to add length = %lu [mmapWriteTunnel - main]\n", toWrite.size());
        traceTunnel.writeTraceSegment(buffer, toWrite);
        //traceTunnel.clearBuffer(buffer);
    }

    //final write segment
    printf("\nFinal writeTraceSegment.size() = %lu\n", fileData.size());
    traceTunnel.writeTraceSegment(buffer, fileData);
    //traceTunnel.clearBuffer(buffer);

    printf("\nWORKSPACE_LEN = %d\n", WORKSPACE_LEN);
    printf("iterations = %zu\n", iterations);
    printf("q length = %lu ...should be empty\n", fileData.size());


	return 0;
}

void traceFileToQueue(std::queue<intptr_t> &src, int fileLen)
{
        std::string temp;
        std::stringstream stream;
        intptr_t address;
        std::ifstream traceIn(TRACE_FILE);

        if (traceIn.is_open())
        {
                for (int i = 0; i < fileLen; i++)
                {
                        traceIn.ignore(256, 'x');
                        getline(traceIn, temp);
                        address = std::stoul(temp, nullptr, 16);
                        stream << address;
                        stream >> std::hex >> address;

                        src.push(address);
                }
                traceIn.close();
        }
        else
        {
                std::cout << "Error: Unable to open trace file." << std::endl;
        }
}

int findFileLength()
{
        int lineCount = 0;
        std::string temp;
        std::ifstream fin(TRACE_FILE);

        if (fin.is_open())
        {
                while (getline(fin, temp))
                {
                        lineCount++;
                }
                fin.close();

                return lineCount;
        }
        else
        {
                std::cout << "Error: Unable to open trace file." << std::endl;
                return 0;
        }
}
