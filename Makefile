CXXFLAGS := -std=c++1y
PREFIX := /usr

-include config.mk

TESTS := $(wildcard test/*.cpp)
SOURCES := $(wildcard src/*.cpp)

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

caliber: MY_LDFLAGS := $(LDFLAGS) -lboost_program_options -lboost_iostreams
caliber: $(SOURCES:.cpp=.o)
	$(CXX) $(CXXFLAGS) $^ -L. -lmettle $(MY_LDFLAGS) -o $@

.PHONY: install
install: all
	cp caliber $(PREFIX)/bin/caliber

.PHONY: test
test: caliber
	./caliber --verbose 2 --color $(TESTS)

.PHONY: clean
clean: clean-bin clean-obj

.PHONY: clean-bin
clean-src:
	rm -f caliber

.PHONY: clean-obj
clean-obj:
	find . -name "*.[od]" -exec rm -f {} +
