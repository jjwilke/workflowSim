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
#include <semaphore.h>
#include <sstmutex.h>
#include <myCircularBuffer.h>
#include <macros.h>

using namespace SST::Core::Interprocess;

//Globals
std::queue<TRACE_TYPE> traceData;
UINT64 icount = 0;
static BUFFER pinBuffer(WORKSPACE_LEN);
static int pinfd;
static BUFFER* pinMap;

//Declarations
void recordMemRD(void *addr);
void recordMemWR(void *addr);
void Instruction( INS ins, void *v );
void Fini( INT32 code, void *v );
bool mmapInit();
bool mmapClose();
bool bufferTransfer();
bool bufferCheckAndClear();
bool PINTOOL_Init();


int main( int argc, char *argv[] )
{
	PINTOOL_Init();	
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
	if ( bufferCheckAndClear() )
	{
		printf("Buffer has been written to and cleared\n");
		return;
	}
	traceData.push( (TRACE_TYPE)addr );
	icount++;
	//fprintf(trace, "%p\n", addr);
}


void recordMemWR(void *addr)
{
	if ( bufferCheckAndClear() )
	{
		printf("Buffer has been written to and cleared\n");
		return;
	}
	traceData.push( (TRACE_TYPE)addr );
	icount++;
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
	//One last transfer to clear up any straglers
	printf("Before buffer transfer...\n");
	bufferTransfer();
	
    mmapClose();

	printf("\nPIN is finished...goodbye...\n");
}


//Additional Functions
bool PINTOOL_Init()
{
	//Init mmap
	if ( mmapInit() )
	{
        //init locks
        if (  )
		{
			perror("Semaphores were not created in parent process");
			return false;
		}
		return true;
	}
	return false;
}

bool bufferCheckAndClear()
{
	if ( icount < (WORKSPACE_LEN-1) )
	{
		return false;
	}

	//WRITE TO BUFFER
	return bufferTransfer();
}

bool bufferTransfer()
{
	//write to buffer
	printf("\nSize of traceData = %lu\n", traceData.size());
	int transferCount = pinBuffer.write(traceData);	
	printf("Before clear...\n");

	//-----------------------
	//Critical Region
    //-----------------------
    pinMap[0] = pinBuffer;
	//-----------------------

	size_t transferSizeActual = transferCount * sizeof(TRACE_TYPE);
	printf("Actual number of bytes transferred = %lu\n", transferSizeActual);
	icount = 0;
	return true;
}

bool mmapInit()
{        
    printf("File: %s\n", MMAP_PATH);
    pinfd = open(MMAP_PATH, O_RDWR|O_CREAT|O_TRUNC, (mode_t)0600);
	     
	if (pinfd == -1)
	{
	        perror("File did not *open* properly");
	        return false;  
	}

	//touch virtual address to map to physical
	if (lseek(pinfd, BUFFER_SIZE, SEEK_SET) == -1)
	{
	        perror("Unable to appropriately size the file");
	        return false;
	}
	write(pinfd, "", 1); //needed to set the size

	size_t totalSize = BUFFER_SIZE + WORKSPACE_SIZE;	
	printf("File size is...%lu bytes\n", totalSize); 
	pinMap = (BUFFER *)mmap( NULL, totalSize, PROT_READ|PROT_WRITE, MAP_SHARED, pinfd, 0 );
	if (pinMap == MAP_FAILED)
	{
	        perror("mmap failed to open");
	        return false;
	}
	printf("[Child] Map pointer atOPEN = %p\n", (void*)pinMap);
	
	return true;
}

bool mmapClose()
{
	printf("[Child] Map pointer atCLOSE = %p\n", (void*)pinMap);
	if ( munmap(pinMap, BUFFER_SIZE) == -1 )
    	{
		perror("munmap failed");
            	return false;
	}      
	if (close(pinfd) == -1)
	{
		perror("file did not close");
		return false;
	}

    return true;
}
