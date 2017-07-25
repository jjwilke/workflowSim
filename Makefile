PROGRAM_NAME = pinForkSim
BOOST_ROOT = /usr/local/boost_1_59_0

CC := g++

INCLUDES := -I$(BOOST_ROOT)/include
CXXFLAGS :=
CPPFLAGS :=
CFLAGS :=
LDFLAGS :=
LIBS := 

TARGET = bin/$(PROGRAM_NAME)
SOURCE = src/$(PROGRAM_NAME).cpp
OBJECT = $(SOURCE: .cpp=.o)

all : $(TARGET)

%.o : %.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

$(TARGET) : $(OBJECT)
	$(CC) -o $@ $+ $(INCLUDES) $(LDFLAGS) $(LIBS)

clean : $(OBJECT)
	rm -f *.o $(TARGET)
	@echo clean complete...
