CC := g++
SRCDIR := src
BUILDDIR := build
TARGET := ptta2
 
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -Wall
LIB :=#-pthread -lmongoclient -L lib -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt
INC :=#-I include

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
