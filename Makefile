CXXFLAGS=
LIBS=-lboost_filesystem -lboost_system -lboost_program_options
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: converter

converter: $(OBJS)
	$(CXX) $^ $(LIBS) -o ${@}

clean:
	$(RM) $(OBJS) converter

