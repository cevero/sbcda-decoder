CC := gcc
SRCDIR := src
BUILDDIR := build
TARGET := inputSignalAndufft
 
SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS :=-Wall -g -fopenmp
LIB :=lib/ufft/fft-dit.c lib/ufft/ift-dit.c -lm -fopenmp#-pthread
INC :=-Ilib/ufft

$(BUILDDIR)/$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $@ $(LIB)"; $(CC) $^ -o $@ $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR)"; $(RM) -r $(BUILDDIR)

.PHONY: clean
