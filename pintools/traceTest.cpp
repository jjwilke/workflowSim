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
#include <iostream>


//Globals
std::queue<intptr_t> traceStorage;

//Additional Functions
bool mmapInit(char *filePath, struct stat &memBuf, void *map, int &fd)
{        
        printf("File: %s\n", filePath);    
        fd = open(filePath, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
             
        if (fd == -1)
        {
                perror("File did not *open* properly");
                return false;  
        }
        if (stat(filePath, &memBuf) == -1)
        {
                perror("File status error (fstat)");
                return false;
        }
        
        map = mmap(NULL, memBuf.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED)
        {
                perror("mmap failed to open");
                return false;
        }

        return true;
}

bool mmapClose(void *map, int &fd, struct stat &buf)
{
        if (munmap(map, buf.st_size) == -1)
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
	//Init mmap
	int pinfd;
	char *pinMap;
	struct stat memBuf;
	char path[] = "/Users/gvanmou/Desktop/workflowProject/bin/mmapPin.out";
	
	mmapInit(path, memBuf, pinMap, pinfd);
	
	for (int i = 0; i < traceStorage.size(); i++)
	{
		write(pinfd, &traceStorage.front(), sizeof(intptr_t));
		traceStorage.pop();
	}
	
	mmapClose(pinMap, pinfd, memBuf);
}


//Pintool
void recordMemRD(void *addr)
{
	intptr_t temp = (intptr_t)addr;
	traceStorage.push(temp);
	//fprintf(trace, "%p\n", addr);
}


void recordMemWR(void *addr)
{
	intptr_t temp = (intptr_t)addr;
	traceStorage.push(temp);
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


int main( int argc, char *argv[] )
{
	PIN_Init(argc, argv);
	
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	
	// Never returns
	PIN_StartProgram();


	return 0;
}
