CC=gcc
CXX=g++
RM=rm -f

CPPFLAGS +=
CXXFLAGS += -Wall -Werror -O2 -fPIC
LDFLAGS  += -shared -Wl,-soname,$(SONAME) -Wl,-O,2 -Wl,--as-needed
LDLIBS   +=

LIBNAME = librrd
SONAME  = $(LIBNAME).so
ANAME   = $(LIBNAME).a

SRCS = $(filter-out example.cpp, $(wildcard *.cpp))
OBJS = $(SRCS:.cpp=.o)

all: $(SONAME) tests

$(SONAME): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJS): $(SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $(@:.o=.cpp)

$(LIBNAME).a: $(OBJS)
	$(AR) -r $@ $^

example: example.o $(ANAME)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

tests: $(ANAME)
	$(MAKE) -C tests

clean:
	$(RM) $(OBJS) $(SONAME) $(ANAME) example.o example
	$(MAKE) -C tests clean

.PHONY: clean tests
