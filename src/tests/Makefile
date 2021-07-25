CC=gcc
CXX=g++
RM=rm -f

CPPFLAGS += -I..
CXXFLAGS += -Wall -Werror -O2
LDFLAGS  += -Wl,-O,2 -Wl,--as-needed
LDLIBS   +=

LIBNAME = librrd
ANAME   = $(LIBNAME).a
TARGET  = test_$(LIBNAME)

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS) ../$(ANAME)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJS): $(SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $(@:.o=.cpp)

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: clean
