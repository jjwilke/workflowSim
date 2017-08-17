PROG_ROOT = $(HOME)/Desktop/workflowProject
SST_CORE = $(HOME)/install/sstcore-7.1.0
PROGRAM_NAME = mmapWrite

CC := g++

INCLUDES := \
	-I$(PROG_ROOT)/include \
	-I$(SST_CORE)/include/sst/core/interprocess
CXXFLAGS := 
CPPFLAGS :=
CFLAGS :=
LDFLAGS := -std=c++11
LIBS := 

TARGET = $(PROGRAM_NAME)
SOURCE = $(PROGRAM_NAME).cpp
OBJECT = $(SOURCE: .cpp=.o)

all : $(TARGET)

%.o : %.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

$(TARGET) : $(OBJECT)
	$(CC) -o $@ $+ $(INCLUDES) $(LDFLAGS) $(LIBS)

clean : $(OBJECT)
	rm -f *.o $(TARGET)
	@echo clean complete...
