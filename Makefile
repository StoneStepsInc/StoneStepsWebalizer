# ------------------------------------------------------------------------
#
#   Makefile
# 	 
#   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)
#
#   See COPYING and Copyright files for additional licensing and copyright information 
# 
# ------------------------------------------------------------------------

SHELL := /bin/bash

# ------------------------------------------------------------------------
#
#   Usage:  make -f makefile.gnu [[name=value] ...]
#           make -f makefile.gnu clean
#
#   Parameters:
#     CPUARCH={i686|prescott|nocona|...}
#     ETCDIR=/etc
#     MYCCFLAGS=-g
# ------------------------------------------------------------------------

#
# Remove all standard suffix rules and declare phony targets
#
.SUFFIXES:
.PHONY: all clean install

# ------------------------------------------------------------------------
#
# Define directories and file names
#
# ------------------------------------------------------------------------
TARGET   := webalizer

#
# Define precompiled header file names. Make sure the source PCH file 
# is the first in the list of source files, so when it's built, PCH 
# header will be rebuilt before any other source files are processed.
#
PCHHDR	 := pch.h
PCHSRC	 := $(PCHHDR:.h=.cpp)
PCHOUT	 := $(PCHHDR:.h=.h.gch)
PCHOBJ	 := $(PCHHDR:.h=.o)

# define the configuration directory
ifndef ETCDIR
ETCDIR   := /etc
export   ETCDIR
endif

# source and build directories
SRCDIR   := src
BLDDIR   := build

# search paths for source and object files
vpath %.cpp $(SRCDIR) 
vpath %.o   $(BLDDIR)

# include and library search paths
INCDIRS  := $(SRCDIR)
LIBDIRS  := $(BLDDIR)

# list of source files (precompiled header file must be first in the list)
SRCS     := $(PCHSRC) tstring.cpp linklist.cpp hashtab.cpp \
	output.cpp graphs.cpp preserve.cpp lang.cpp \
	parser.cpp	util.cpp logrec.cpp tstamp.cpp \
	webalizer.cpp dns_resolv.cpp history.cpp tmranges.cpp \
	anode.cpp ccnode.cpp dlnode.cpp hnode.cpp \
	inode.cpp rcnode.cpp rnode.cpp snode.cpp \
	spnode.cpp unode.cpp vnode.cpp \
	danode.cpp keynode.cpp scnode.cpp sysnode.cpp \
	daily.cpp hourly.cpp totals.cpp queue_nodes.cpp \
	hashtab_nodes.cpp config.cpp serialize.cpp \
	html_output.cpp dump_output.cpp \
	database.cpp logfile.cpp cp1252_ucs2.cpp \
	char_buffer.cpp unicode.cpp formatter.cpp \
	platform/exception_linux.cpp platform/event_pthread.cpp \
	platform/thread_pthread.cpp \
	encoder.cpp p2_buffer_allocator.cpp char_buffer_stack.cpp \
	cp1252.cpp hckdel.cpp

# list all libraries we use
LIBS     := stdc++ dl pthread db_cxx gd z maxminddb

# generate a list of object files
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

# and a list oof dependency make files
DEPS := $(OBJS:.o=.d)

# and a list of template repository files
RPOS := $(OBJS:.o=.rpo)

# add GCC include and library options to each search path
INCDIRS := $(addprefix -I,$(INCDIRS))
LIBDIRS := $(addprefix -L,$(LIBDIRS))

# add the library option to each library in the list
LIBS := $(addprefix -l,$(LIBS))

# ------------------------------------------------------------------------
#
# Define compiler options
#
# ------------------------------------------------------------------------
CCFLAGS  = -DHAVE_CXX_STDHEADERS \
     -DETCDIR=\"$(ETCDIR)\" \
     -fexceptions \
     -Wno-multichar -Winvalid-pch \
     -std=c++11 \
     $(MYCCFLAGS)

# turn on optimization for non-debug builds
ifeq ($(findstring -g,$(CCFLAGS)),)
CCFLAGS  += -O3
endif

# use the specified CPU architecture if one is defined
ifdef CPUARCH
CCFLAGS  += -march=$(CPUARCH)
endif

# flags passed to the linker through $(CC)
CC_LDFLAGS := 


# ------------------------------------------------------------------------
#
# Targets
#
# ------------------------------------------------------------------------

# default target
all: $(BLDDIR)/$(TARGET) ;

#
# build/webalizer
#
$(BLDDIR)/$(TARGET): $(OBJS)
	$(CC) -o $@ $(CC_LDFLAGS) $(LIBDIRS) $(LIBS) $(addprefix $(BLDDIR)/,$(OBJS))

install:
	@echo
	@echo "The install target is not implemented"
	@echo

clean:
	@echo "Removing object files..."
	@for obj in $(OBJS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@echo "Removing dependency files..."
	@for obj in $(DEPS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@echo "Removing the precompiled header..."
	@if [[ -e $(BLDDIR)/$(PCHOUT) ]]; then rm $(BLDDIR)/$(PCHOUT); fi
	@echo "Removing template repository files..."
	@for obj in $(RPOS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@echo "Removing webalizer..."
	@if [[ -e $(BLDDIR)/$(TARGET) ]]; then rm $(BLDDIR)/$(TARGET); fi
	@echo "Done"

# ------------------------------------------------------------------------
#
# Rules
#
# ------------------------------------------------------------------------

#
# Dependency rules build .d files in the build directory. One of these 
# rules will be executed for each .d file included at the bottom of this 
# file. 
#
# GCC is used to generate lists of includes in each source file:
#
#  gcc -MM src/webalizer.cpp
#
# , which is then piped into sed to insert the .d file in the list of 
# targets of the generated dependency file, so the final result looks 
# like this:
#
#  webalizer.o webalizer.d: webalizer.cpp version.h config.h ...
#
# Note that the target variable $@ will contain the build directory,
# but the stem variable $* will only contain the name of the file. 
# This ensures that file path separators are not misinterpreted as
# sed options.
#
$(BLDDIR)/%.d : %.cpp
	@if [[ ! -e $(BLDDIR)/platform ]]; then mkdir -p $(BLDDIR)/platform; fi
	set -e; $(CC) -MM $(CCFLAGS) $(INCDIRS) $< | \
	sed 's/^[ \t]*\($(subst /,\/,$*)\)\.o/\1.o \1.d/g' > $@

$(BLDDIR)/%.d : %.c
	@if [[ ! -e $(BLDDIR)/platform ]]; then mkdir -p $(BLDDIR)/platform; fi
	set -e; $(CC) -MM $(CCFLAGS) $(INCDIRS) $< | \
	sed 's/^[ \t]*\($(subst /,\/,$*)\)\.o/\1.o \1.d/g' > $@

#
# Rules to compile source file
#
%.o : %.cpp
	$(CC) -c $(CCFLAGS) $(INCDIRS) $< -o $(BLDDIR)/$@

%.o : %.c
	$(CC) -c $(CCFLAGS) $(INCDIRS) $< -o $@	

#
# Build the precompiled header file and the object file
#
$(BLDDIR)/$(PCHOBJ) : $(PCHSRC)
	@if [[ -e $(BLDDIR)/$(PCHOUT) ]]; then rm $(BLDDIR)/$(PCHOUT); fi
	$(CC) -c -x c++-header $(CCFLAGS) $(INCDIRS) $(SRCDIR)/$(PCHHDR) -o $(BLDDIR)/$(PCHOUT)
	$(CC) -c $(CCFLAGS) $(INCDIRS) $< -o $@

# ------------------------------------------------------------------------
#
# Dependencies
#
# ------------------------------------------------------------------------

# 
# Include the dependencies if there is no target specified explicitly (i.e.
# the default target is being built) or if the specified target is not one 
# of the maintenance targets.
#
ifeq ($(MAKECMDGOALS),)
include $(addprefix $(BLDDIR)/, $(DEPS))
else ifneq ($(filter-out clean install,$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(DEPS))
endif

