SRCS := $(wildcard *.cc)
BINS := $(SRCS:.cc=.out)

all: $(BINS)

%.out: %.cc
	@mkdir -p build/
	@g++ $< -o build/$@ -O0

run: $(BINS)
	@for bin in $(BINS); do cd build/ && ./$$bin; done

clean:
	rm -rf build/
