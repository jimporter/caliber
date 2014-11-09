CXXFLAGS := -std=c++1y
PREFIX := /usr

-include config.mk

TESTS := $(patsubst %.cpp,%,$(wildcard test/*.cpp))
COMPILATION_TESTS := $(wildcard test/compilation/*.cpp)
SOURCES := $(wildcard src/*.cpp)
ALL_TESTS := $(TESTS) "./caliber $(COMPILATION_TESTS)"

# Include all the existing dependency files for automatic #include dependency
# handling.
-include $(TESTS:=.d)
-include $(SOURCES:.cpp=.d)

all: caliber

# Build .o files and the corresponding .d (dependency) files. For more info, see
# <http://scottmcpeak.com/autodepend/autodepend.html>.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@
	$(eval TEMP := $(shell mktemp))
	@$(CXX) $(CXXFLAGS) -MM -Iinclude $< > $(TEMP)
	@sed -e 's|.*:|$*.o:|' < $(TEMP) > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(TEMP) | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $(TEMP)

test/test_paths: src/paths.o
test/test_test_compiler: src/paths.o src/test_compiler.o

$(TESTS): %: %.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -lmettle -o $@

tests: $(TESTS)

caliber: MY_LDFLAGS := $(LDFLAGS) -lboost_program_options -lboost_iostreams
caliber: $(SOURCES:.cpp=.o)
	$(CXX) $(CXXFLAGS) $^ -L. -lmettle $(MY_LDFLAGS) -o $@

.PHONY: install
install: all
	cp caliber $(PREFIX)/bin/caliber

.PHONY: test
test: tests caliber
	$(eval TEST_DATA := $(shell readlink -f test/test-data))
	TEST_DATA=$(TEST_DATA) mettle --output=verbose --color $(ALL_TESTS)

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
