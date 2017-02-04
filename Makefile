CXX = g++
CPP_FLAGS = -fpermissive -g -m32
CPP_SOURCES = generated/x86_parser.cpp generated/mips32_parser.cpp $(wildcard *.cpp)
HEADERS = $(wildcard *.h) 
OBJ = ${CPP_SOURCES:.cpp=.o}
INCLUDE = .
LIBS = -ledit -ldl
TARGET = EasyASM

all: ${TARGET}

${TARGET}: ${OBJ}
ifeq ($(LIBEDIT_DIR),)
	${CXX} -m32 -o $@ $^ ${LIBS}
else
	${CXX} -L${LIBEDIT_DIR} -m32 -o $@ $^ ${LIBS}
endif

%.o: %.cpp
	${CXX} -c -I ${INCLUDE} ${CPP_FLAGS} -o $@ $<

generated/x86_parser.cpp: x86_parser.y
	lemon -Tlempar.c $<
	mkdir -p generated
	mv x86_parser.c $@

generated/mips32_parser.cpp: mips32_parser.y
	lemon -Tlempar.c $<
	mkdir -p generated
	mv mips32_parser.c $@

clean:
	rm -f *.o
	rm -fr generated/