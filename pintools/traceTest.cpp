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
#include <sstmutex.h>
#include <myCircularBuffer.h>

using namespace SST::Core::Interprocess;

//Macros
#ifndef TRACE_TYPE
  #define TRACE_TYPE intptr_t
#endif

#ifdef TRACE_TYPE
  #define BUFFER CircularBuffer<TRACE_TYPE>
  #define WORKSPACE_SIZE 7168
  #define WORKSPACE_LEN (WORKSPACE_SIZE / sizeof(TRACE_TYPE))
#endif

#ifndef BUFFER_SIZE
  #define BUFFER_SIZE sizeof(BUFFER)
#endif


//Globals
std::queue<intptr_t> traceStorage;
UINT64 icount = 0;

//Declarations
void recordMemRD(void *addr);
void recordMemWR(void *addr);
void Instruction( INS ins, void *v );
void Fini( INT32 code, void *v );
bool mmapInit(BUFFER* &map, int &fd, char *filePath);
bool mmapClose(BUFFER* &map, int &fd);
bool bufferTransfer();
bool bufferCleared(UINT64 icount);
bool pintoolInit();


int main( int argc, char *argv[] )
{
	PIN_Init(argc, argv);
	
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	
	// Never returns
	PIN_StartProgram();


	return 0;
}


//Pintool
void recordMemRD(void *addr)
{
	traceStorage.push((intptr_t)addr);
	icount++;
	if (bufferCleared(icount))
	{
		printf("Buffer has been written to and cleared\n");
	}
	//fprintf(trace, "%p\n", addr);
}


void recordMemWR(void *addr)
{
	traceStorage.push((intptr_t)addr);
	icount++;
	if (bufferCleared(icount))
	{
		printf("Buffer has been written to and cleared\n");
	}
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
	//exportTrace();
}


//Additional Functions
bool pintoolInit()
{
	//Init buffer
	CircularBuffer<intptr_t> pinBuffer(WORKSPACE_LEN);
	
	//Init mmap
	int pinfd;
	BUFFER* pinMap;
	char path[] = "/Users/gvanmou/Desktop/workflowProject/bin/pinMap.out";
	if ( mmapInit(pinMap, pinfd, path) )
	{
		return true;
	}
	return false;
}

bool bufferTransfer()
{
	return true;
}

bool bufferCleared(UINT64 icount)
{
	if (icount == WORKSPACE_LEN)
	{
		//WRITE TO BUFFER
		bufferTransfer();
		return true;
	}
	return false;
}

bool mmapInit(BUFFER* &map, int &fd, char *filePath)
{        
	printf("File: %s\n", filePath);    
	fd = open(filePath, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	     
	if (fd == -1)
	{
	        perror("File did not *open* properly");
	        return false;  
	}

	//touch virtual address to map to physical
	if (lseek(fd, BUFFER_SIZE, SEEK_SET) == -1)
	{
	        perror("Unable to appropriately size the file");
	        return false;
	}
	write(fd, "", 1); //needed to set the size
	
	printf("File size is...%lu bytes\n", BUFFER_SIZE); 
	map = (BUFFER *)mmap( NULL, BUFFER_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
	if (map == MAP_FAILED)
	{
	        perror("mmap failed to open");
	        return false;
	}
	printf("Map pointer init = %p\n", (void*)map);
	
	return true;
}

bool mmapClose(BUFFER* &map, int &fd)
{
	printf("Map pointer close = %p\n", (void*)map);
    if (munmap(map, BUFFER_SIZE) == -1)
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
