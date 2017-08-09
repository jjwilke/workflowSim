
#ifndef SST_CORE_INTERPROCESS_CIRCULARBUFFER_H
#define SST_CORE_INTERPROCESS_CIRCULARBUFFER_H

#include "sstmutex.h"
#include <queue>
#include <vector>
#include <macros.h>

namespace SST {
namespace Core {
namespace Interprocess {

template <typename T>
class CircularBuffer {

public:
	CircularBuffer(size_t mSize = 0) {
		buffSize = mSize;
		readIndex = 0;
		writeIndex = 0;
	}

	void setBufferSize(const size_t bufferSize)
    	{
        	if ( buffSize != 0 ) {
	            fprintf(stderr, "Already specified size for buffer\n");
        	    exit(1);
        	}

	        buffSize = bufferSize;
		__sync_synchronize();
    	}

	//Read buffer->result vector after clearing result vector
	bool read(std::vector<T> &result) 
	{
		int loop_counter = 0;
		result.clear();

		while( true ) 
		{
			bufferMutex.lock();
			
			if (readIndex == writeIndex)
			{
				bufferMutex.unlock();
				bufferMutex.processorPause(loop_counter++);
				
				return false;
			}
			while( readIndex != writeIndex ) 
			{
				//Mark the end last section being read
				if ( buffer[readIndex] == 0 )
				{
					result.push_back(END_OF_TRACE);
					bufferMutex.unlock();
					return true;
				}

				if ( buffer[readIndex] != 0 )
				{
					result.push_back( buffer[readIndex] );
				}				
				readIndex = (readIndex + 1) % buffSize;	
			}
	
			bufferMutex.unlock();		
			return true;
		}
	}

	bool readNB(T* result) {
		if( bufferMutex.try_lock() ) {
			if( readIndex != writeIndex ) {
				*result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return true;
			} 

			bufferMutex.unlock();
		}

		return false;
	}

	//modify so write() takes a CONST
	int write(std::queue<T> &v) 	
	{
		int loop_counter = 0;
		int T_count = 0;

		printf("<<< 1 >>>\n");	
		while( true ) 
		{
			bufferMutex.lock();
			
			if ( ((writeIndex + 1) % buffSize) == readIndex )
			{
				bufferMutex.unlock();
				bufferMutex.processorPause(loop_counter++);
				
				return 0;
			}
			while ( ((writeIndex + 1) % buffSize) != readIndex ) 
			{
				if (!v.empty())
				{
					buffer[writeIndex] = v.front();
					v.pop();
				}
				else
				{
					buffer[writeIndex] = 0;
				}
				writeIndex = (writeIndex + 1) % buffSize;
				T_count++;
			}
			__sync_synchronize();
			bufferMutex.unlock();
		
			return T_count;
		}
	}

	~CircularBuffer() {

	}

	void clear() {
		bufferMutex.lock();
		readIndex = writeIndex;
		__sync_synchronize();
		bufferMutex.unlock();
	}

private:
	SSTMutex bufferMutex;
	size_t buffSize;
	size_t readIndex;
	size_t writeIndex;
	T buffer[0];

};

}
}
}

#endif
