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
#include <sys/stat.h>
#include <sys/mman.h>

#define SIZE sizeof(intptr_t)
#define PAGE_SIZE 4096

const std::string TRACE_FILE = "test/trace.out";

//Declarations
size_t findTotalSize(size_t targetSize);
bool mmapInit(char *filePath, size_t fileSize, intptr_t *&map, int &fd);
bool mmapClose(intptr_t *&map, int &fd, size_t fileSize);
void traceFileToQueue(std::queue<intptr_t> &src, int fileLen);
int findFileLength();


//simulates the pintool
int main (int argc, char** argv)
{
	//-------------------
	//Init queue
	//-------------------
	std::queue<intptr_t> q;
	traceFileToQueue(q, findFileLength()); 
	//int qLen = 100000;
	//int range = 5000;
	//srand(time(nullptr));
	//q.push(999999999999);
	//for (int i = 0; i < qLen-2; i++)
	//{
	//	q.push( (intptr_t)((rand() % range) * 10000));
	//}
	//q.push(777777777777);

	
	//-------------------
	//Init mmap
	//-------------------
	int pinfd;
        char *fileMem;
        intptr_t *pinMap;
        size_t traceLen = q.size();
        size_t fileSize = traceLen * SIZE;
        size_t totalSize = findTotalSize(fileSize);
        char path[] = "/Users/gvanmou/Desktop/workflowProject/test/mmapPin.out";

        mmapInit(path, totalSize, pinMap, pinfd);


	//-------------------
	//Copy queue to mmap
	//-------------------
        printf("writing to file of length '%ld' bytes...\n", traceLen);
        printf("Page size = '%d' bytes...\n", PAGE_SIZE);
        for (size_t i = 0; i < traceLen; i++)
        {
                //printf("    Trace Size = %ld\n", sizeof(traceStorage.front()));
                printf("Before pinMap[%lu] = %lu\n", i, pinMap[i]);
                pinMap[i] = q.front();
                printf("After  pinMap[%lu] = %lu\n", i, pinMap[i]);

                q.pop();
        }

        printf("Map pointer middle = %p\n", (void*)pinMap);
	int random = rand() % traceLen;
	printf("Random mmap access: mmap[%d] = %ld\n", random, pinMap[random]);

	//-------------------
	//End mmap in READ...for final sim
	//-------------------
        mmapClose(pinMap, pinfd, totalSize);
	printf("q length = %lu\n", q.size());


	return 0;
}

size_t findTotalSize(size_t targetSize)
{       
        int i = 1;
        while ( (PAGE_SIZE * i) < targetSize )
        {       
                i++;
        }
        return (size_t)(PAGE_SIZE * i);
}

bool mmapInit(char *filePath, size_t fileSize, intptr_t *&map, int &fd)
{        
        printf("File: %s\n", filePath);    
        fd = open(filePath, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
             
        if (fd == -1)
        {       
                perror("File did not *open* properly");
                return false;
        }
	//stretch file according to mmap size
        if (lseek(fd, fileSize, SEEK_SET) == -1)
        {       
                perror("Unable to appropriately size the file");
                return false;
        }
        write(fd, "", 1); //needed to set the size
        
        printf("File size is...%lu bytes\n", fileSize); 
        map = (intptr_t *)mmap(NULL, fileSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED)
        {       
                perror("mmap failed to open");
                return false;
        }
        printf("Map pointer init = %p\n", (void*)map);
        
        return true;
}

bool mmapClose(intptr_t *&map, int &fd, size_t fileSize)
{       
        printf("Map pointer close = %p\n", (void*)map);
        if (munmap(map, fileSize) == -1)
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
