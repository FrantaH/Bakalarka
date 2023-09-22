CC= gcc -Wall
DEFS= 
PROGNAME= argvparsertest
INCLUDES=  
LIB=libargvparser.a
 


DEFINES= $(INCLUDES) $(DEFS)
CFLAGS= -g $(DEFINES)

SRCS = tests.cpp argvparser.cpp 

OBJS = argvparser.o 

all: $(PROGNAME) $(LIB)

$(PROGNAME) : $(OBJS)
	$(CC) $(CFLAGS) -o $(PROGNAME) $(OBJS)

$(LIB) : argvparser.o
	ar -cru $@ $^
	ranlib $@

clean:
	rm $(OBJS) $(PROGNAME) $(LIB)

depend:
	-$(CXX) -M $(CXXFLAGS) *.cpp > .depend.mk

-include .depend.mk	
