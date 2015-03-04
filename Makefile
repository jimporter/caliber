CXXFLAGS := -std=c++1y
PREFIX := /usr

-include config.mk

ifndef TMPDIR
  TMPDIR := /tmp
endif

TESTS := $(patsubst %.cpp,%,$(wildcard test/*.cpp))
COMPILATION_TESTS := $(wildcard test/compilation/*.cpp)
SOURCES := $(wildcard src/*.cpp)
LIBS := -lboost_program_options -lboost_iostreams -pthread
ALL_TESTS := $(TESTS) "./caliber $(COMPILATION_TESTS)"

all: caliber

# Include all the existing dependency files for automatic #include dependency
# handling.
-include $(TESTS:=.d)
-include $(SOURCES:.cpp=.d)

# Build .o files and the corresponding .d (dependency) files. For more info, see
# <http://scottmcpeak.com/autodepend/autodepend.html>.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -MF $*.d -c $< -o $@
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d

test/test_paths: src/paths.o
test/test_tool: src/paths.o src/tool.o

$(TESTS): %: %.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -lmettle -o $@

tests: $(TESTS)

caliber: MY_LDFLAGS := $(LDFLAGS) $(LIBS)
caliber: $(SOURCES:.cpp=.o)
	$(CXX) $(CXXFLAGS) $^ -L. -lmettle $(MY_LDFLAGS) -o $@

.PHONY: install
install: all
	cp caliber $(PREFIX)/bin/caliber

.PHONY: test
test: tests caliber
	$(eval TEST_DATA := $(shell readlink -f test/test-data))
	TEST_DATA=$(TEST_DATA) mettle --output=verbose --color=auto $(ALL_TESTS)

.PHONY: clean
clean: clean-bin clean-obj

.PHONY: clean-bin
clean-bin:
	rm -f caliber

.PHONY: clean-obj
clean-obj:
	find . -name "*.[od]" -exec rm -f {} +

.PHONY: gitignore
gitignore:
	@echo $(TESTS) | sed -e 's|test/||g' -e 's/ /\n/g' > test/.gitignore
