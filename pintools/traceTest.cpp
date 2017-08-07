#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <queue>
#include <pin.H>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>

#define SIZE sizeof(intptr_t)

//Globals
std::queue<intptr_t> traceStorage;


//Additional Functions
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

void exportTrace()
{
	int length = 20;	

	//Init mmap
	int pinfd;
	char *fileMem;
	intptr_t *pinMap;
	size_t traceLen = traceStorage.size();
	size_t fileSize = traceLen * SIZE;
	size_t totalSize = findTotalSize(fileSize);
	char path[] = "/Users/gvanmou/Desktop/workflowProject/bin/mmapPin.out";
	
	mmapInit(path, totalSize, pinMap, pinfd);
	
	printf("writing to file of length '%ld' bytes...\n", traceLen);
	printf("Page size = '%d' bytes...\n", PAGE_SIZE);
	for (size_t i = 0; i < traceLen; i++)
	{
		printf("Before pinMap[%lu] = %lld\n", i, pinMap[i]);
		pinMap[i] = traceStorage.front();
		printf("After  pinMap[%lu] = %lld\n", i, pinMap[i]);

		traceStorage.pop();
	}

	printf("reading data from file...\n");
	//for (int i = 0; i < traceLen; i++)
	//{
        //        printf("[%d]: %lld\n", i, pinMap[i]);
	//} 
		
	printf("Map pointer middle = %p\n", (void*)pinMap);
	mmapClose(pinMap, pinfd, totalSize);
}


//Pintool
void recordMemRD(void *addr)
{
	traceStorage.push((intptr_t)addr);
	//fprintf(trace, "%p\n", addr);
}


void recordMemWR(void *addr)
{
	traceStorage.push((intptr_t)addr);
	//fprintf(trace, "%p\n", addr);
}


void Instruction( INS ins, void *v )
{
	UINT32 memOperands = INS_MemoryOperandCount(ins);

	for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	{
		if (INS_MemoryOperandIsRead(ins, memOp))
		{
			INS_InsertPredicatedCall ( 
				ins, IPOINT_BEFORE, (AFUNPTR)recordMemRD,
				IARG_MEMORYOP_EA, memOp, IARG_END);
		}
		if (INS_MemoryOperandIsWritten(ins, memOp))
		{
			INS_InsertPredicatedCall (
				ins, IPOINT_BEFORE, (AFUNPTR)recordMemWR,
				IARG_MEMORYOP_EA, memOp, IARG_END);
		}
	}
}


void Fini( INT32 code, void *v )
{
	//Output collected trace to mmap
	exportTrace();
}


int main( int argc, char *argv[] )
{
	PIN_Init(argc, argv);
	
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	
	// Never returns
	PIN_StartProgram();


	return 0;
}

