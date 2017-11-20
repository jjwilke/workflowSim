#ifndef MY_CIRCULARBUFFER_H
#define MY_CIRCULARBUFFER_H

using trace_entry_t = intptr_t;
constexpr trace_entry_t const END_OF_TRACE = 717171;

#include <mySSTMutex.h>
#include <queue>
#include <vector>


template <typename T>
class CircularBuffer {

private:
    SSTMutex bufferMutex;
    size_t buffSize;
    size_t readIndex;
    size_t writeIndex;
    bool atEndOfTrace; //set to true after last write
    T buffer[0];


public:
	CircularBuffer(size_t mSize = 0) : atEndOfTrace(false)
    {
		buffSize = mSize;
		readIndex = 0;
		writeIndex = 0;
	}

	/**
     * @brief      Reads the buffer data to an output vector
     *
     * @param      result  The resulting vector
     *
     * @return     A bool indicating a successful read.
     *              Consider changing to an int that returns
     *              a count of the items read.   
     */
    bool read(std::vector<T> &result) 
	{
        // printf("in read()... {myCircularBuffer}\n");

        int loop_counter = 0;
        //Make sure the collection vector is empty
		result.clear();

		while( true ) 
        {
            // printf("waiting to read... {myCircularBuffer}\n");   
            //wait until the buffer is full
            while ( !bufferIsFull() )
			{
                //if at the last read, the buffer will never be full
                if ( atEndOfTrace )
                {
                    break;
                }

                //waiting...
                lockGuard g(bufferMutex);
				bufferMutex.processorPause(loop_counter++);
			}

            //needs initial read to avoid skipping the last item
            lockGuard g(bufferMutex);
            result.push_back( buffer[readIndex] );

            // printf("_________ READING DATA... {myCircularBuffer}\n");
            //reads data until the buffer is empty
            while ( !bufferIsEmpty() )
            {
                readIndex = (readIndex + 1) % buffSize;
                result.push_back( buffer[readIndex] );	
            }

            return true;
		}
	}

    //NEED TO EDIT
	bool readNB(T* result) {
        if( bufferMutex.try_lock() )
        {
            if( readIndex != writeIndex )
            {
				*result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return true;
			} 

			bufferMutex.unlock();
		}

		return false;
	}


    /**
     * @brief      Writes a queue of data to the buffer
     *
     * @param      src   The queue that contains the data
     *
     * @return     An int containing the number of items written
     */
    int write(std::queue<T> &src)
	{
        int loop_counter = 0;
        int item_count = 0;

        // printf("in write()... {myCircularBuffer}\n");
	
		while( true ) 
		{

            // printf("waiting for buffer to be empty... {myCircularBuffer}\n");
            //wait until read is complete
            while ( !bufferIsEmpty() )
			{
                lockGuard g(bufferMutex);
                bufferMutex.processorPause(loop_counter++);
			}
			
            //needs initial write to avoid skipping the last item
            lockGuard g(bufferMutex);
            buffer[writeIndex] = src.front();
            src.pop();
            item_count++;

            // printf("_________ WRITING DATA... {myCircularBuffer}\n");
            //write until the buffer is full
            while ( !bufferIsFull() )
            {
                //increment first to avoid skipping last item
                writeIndex = (writeIndex + 1) % (buffSize);

                //if src is empty before buffer is full indicates last write
                if ( src.empty() )
                {
                    //set and mark last write so read knows when to stop
                    atEndOfTrace = true;
                    buffer[writeIndex] = END_OF_TRACE;
                    item_count++;

                    __sync_synchronize();
                    return item_count;
                }

                //normal write
                buffer[writeIndex] = src.front();
                src.pop();
                item_count++;
			}

            __sync_synchronize();
            return item_count;
		}
	}

    ~CircularBuffer() {}

    /*
    * Helper functions
    */
    void clearBuffer()
    {
		lockGuard g(bufferMutex);
		readIndex = writeIndex;
		__sync_synchronize();
	}

    bool bufferIsEmpty()
    {
        if (readIndex == writeIndex)
        {
            return true;
        }
        return false;
    }

    bool bufferIsFull()
    {
        if ( ((writeIndex + 1) % (buffSize) == readIndex) )
        {
            return true;
        }
        return false;
    }

    size_t getBufferLength()
    {
        return buffSize;
    }

    size_t getReadIndex()
    {
        return readIndex;
    }

    void setBufferLength(const size_t bufferLength)
    {
        if ( buffSize != 0 )
        {
                fprintf(stderr, "Already specified size for buffer\n");
                exit(1);
        }

        buffSize = bufferLength;
        __sync_synchronize();
    }


};


#endif
