SST_CORE = $(HOME)/install/sstcore-7.1.0
PIN_ROOT = $(HOME)/install/pin-3.2-81205-clang-mac
PROG_ROOT = $(HOME)/Desktop/workflowProject

PINTOOL_NAME = traceTest

CC := g++

INCLUDES := \
  -I$(PIN_ROOT)/source/include/pin \
  -I$(PIN_ROOT)/source/include/pin/gen \
  -I$(PIN_ROOT)/extras/components/include \
  -I$(PIN_ROOT)/extras/xed-intel64/include/xed \
  -I$(PIN_ROOT)/extras/stlport/include \
  -I$(PIN_ROOT)/source/tools/InstLib \
  -I$(PIN_ROOT)/extras/crt/include \
  -I$(SST_CORE)/include/sst/core/interprocess \
  -I$(PROG_ROOT)/include

CPPFLAGS := \
  -isystem $(PIN_ROOT)/extras/stlport/include \
  -isystem $(PIN_ROOT)/extras/libstdc++/include \
  -isystem $(PIN_ROOT)/extras/crt/include \
  -isystem $(PIN_ROOT)/extras/crt/include/arch-x86_64 \
  -isystem $(PIN_ROOT)/extras/crt/include/kernel/uapi \
  -isystem $(PIN_ROOT)/extras/crt/include/kernel/uapi/asm-x86 \
  -D__PIN__=1 \
  -DPIN_CRT=1 \
  -DBIGARRAY_MULTIPLIER=1 \
  -DTARGET_IA32E \
  -DHOST_IA32E \
  -DTARGET_MAC \
  -D__DARWIN_ONLY_UNIX_CONFORMANCE=1 \
  -D__DARWIN_UNIX03=0

CXXFLAGS := \
  -std=c++11 \
  -O0 -g \
  -fPIC \
  -fno-stack-protector \
  -fno-exceptions \
  -funwind-tables \
  -fno-rtti \
  -fomit-frame-pointer \
  -fno-strict-aliasing

LDFLAGS := \
  -nostdlib \
  -w -Wl,-exported_symbols_list,$(PIN_ROOT)/source/include/pin/pintool.exp \
  -L$(PIN_ROOT)/intel64/runtime/pincrt \
  -L$(PIN_ROOT)/intel64/lib \
  -L$(PIN_ROOT)/intel64/lib-ext \
  -L$(PIN_ROOT)/extras/xed-intel64/lib

LIBS := \
  -lpin \
  -lblas \
  -llapack\
  -lxed \
  -nostdlib \
  -lpin3dwarf \
  -lstlport-dynamic \
  -lm-dynamic \
  -lc-dynamic \
  -lunwind-dynamic

TARGET=$(PROG_ROOT)/bin/$(PINTOOL_NAME).dylib

all : $(TARGET)

SOURCE=$(PROG_ROOT)/pintools/$(PINTOOL_NAME).cpp
OBJECT=$(SOURCE:.cpp=.o)

%.o : %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $^ 

$(TARGET): $(OBJECT) $(PIN_ROOT)/intel64/runtime/pincrt/crtbeginS.o
	$(CC) -shared -o $@ $+ $(INCLUDES) $(LDFLAGS) $(LIBS)

clean : $(OBJECT)
	rm -f *.o $(OBJECT) $(TARGET)
	@echo pintool clean completed...	

