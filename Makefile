XR25DIAG_VERSION = 1.1.0
CXXFLAGS = -pipe -O2 -Wall -std=c++14 -DXR25DIAG_VERSION=\"${XR25DIAG_VERSION}\" \
           ${shell pkg-config --cflags gtkmm-3.0}
LDFLAGS = ${shell pkg-config --libs gtkmm-3.0} -pthread
BIN = xr25_diag
OBJS = XR25streamreader.o Parsers.o UI.o CairoGauge.o CairoTSPlot.o main.o

ifdef DEBUG
  CXXFLAGS += -DDEBUG
endif

all: ${BIN}

clean:
	rm -f *~ \#*\# *.o ${BIN}
.PHONY: all clean

${BIN}: ${OBJS}
	g++ ${LDFLAGS} -o $@ $^

%.o: %.cc
	g++ -c ${CXXFLAGS} -o $@ $^
