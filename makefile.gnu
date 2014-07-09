# ------------------------------------------------------------------------
#
#   makefile.gnu
# 	 
#   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)
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
.PHONY: all clean

# ------------------------------------------------------------------------
#
# Define directories and file names
#
# ------------------------------------------------------------------------
MAKEFILE := makefile.gnu
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

#
# Define SRCROOT to point to the directory above the Webalizer project 
# directory. For example, if the Webalizer source files are located in 
# ~/projects/webalizer, SRCROOT should be defined as ~/projects: 
#
#	make -f makefile.gnu SRCROOT=~/projects
#
# If SRCROOT is not defined, use the parent directory as a default
# value. In this case the variable must be exported to be available 
# to the second-level make (only existing environment variables and 
# variables defined on the command line are passed automatically). 
#
ifndef SRCROOT
SRCROOT  := $(dir $(CURDIR))
SRCROOT  := $(SRCROOT:%/=%)
export   SRCROOT
endif

#
# If the project name is not defined, use the current directory name
#
ifndef PRJNAME
PRJNAME	:= $(notdir $(CURDIR))
export	PRJNAME
endif

ifndef ETCDIR
ETCDIR   := /etc
export   ETCDIR
endif

PRJDIR	 := $(SRCROOT)/$(PRJNAME)
BLDDIR   := $(PRJDIR)/build

VPATH    := $(PRJDIR)

INCDIRS  := $(PRJDIR)
LIBDIRS  := $(BLDDIR)

# precompiled header file must be first in the list
SRCS     := $(PCHSRC) tstring.cpp linklist.cpp hashtab.cpp \
	    output.cpp graphs.cpp preserve.cpp lang.cpp \
	    parser.cpp mutex_pthread.cpp thread_pthread.cpp \
	    util.cpp event_pthread.cpp logrec.cpp tstamp.cpp \
	    webalizer.cpp dns_resolv.cpp history.cpp tmranges.cpp \
	    anode.cpp ccnode.cpp dlnode.cpp hnode.cpp \
	    inode.cpp rcnode.cpp rnode.cpp snode.cpp \
	    spnode.cpp tnode.cpp unode.cpp vnode.cpp \
	    danode.cpp keynode.cpp scnode.cpp sysnode.cpp \
	    daily.cpp hourly.cpp totals.cpp queue_nodes.cpp \
	    hashtab_nodes.cpp config.cpp serialize.cpp \
	    html_output.cpp xml_output.cpp dump_output.cpp \
	    database.cpp GeoIP.cpp logfile.cpp

LIBS     := stdc++ dl pthread db_cxx gd z

DEPLIBS  :=

# ------------------------------------------------------------------------
#
# Define compiler options
#
# ------------------------------------------------------------------------
CCFLAGS  = -DHAVE_CXX_STDHEADERS \
     -DETCDIR=\"$(ETCDIR)\" \
     -fexceptions -fno-implicit-templates \
     -Wno-multichar -Winvalid-pch \
     $(MYCCFLAGS)

# turn on optimization for non-debug builds
ifeq ($(findstring -g,$(CCFLAGS)),)
CCFLAGS  += -O3
endif

ifdef CPUARCH
CCFLAGS  += -march=$(CPUARCH)
endif

# flags passed to the linker through $(CC)
CC_LDFLAGS = 

# ------------------------------------------------------------------------
#
# Create additional variables and modify some existing for the command line
#
# ------------------------------------------------------------------------
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

RPOS := $(OBJS:.o=.rpo)

DEPS := $(OBJS:.o=.d)

INCDIRS := $(addprefix -I,$(INCDIRS))
LIBDIRS := $(addprefix -L,$(LIBDIRS))

LIBS := $(LIBS:lib%.a=%)
LIBS := $(LIBS:lib%.so=%)
LIBS := $(LIBS) $(DEPLIBS:lib%.a=%)
LIBS := $(LIBS) $(DEPLIBS:lib%.so=%)
LIBS := $(addprefix -l,$(LIBS))


# ------------------------------------------------------------------------
#
# Targets
#
# ------------------------------------------------------------------------
all:
	@echo \*\*\* Making Stone Steps Webalizer...
	@if [[ ! -e $(BLDDIR) ]]; then mkdir -p $(BLDDIR); fi
	@$(MAKE) --no-print-directory -C $(BLDDIR) -f $(PRJDIR)/$(MAKEFILE) $(TARGET)

$(TARGET): $(OBJS) $(addprefix $(BLDDIR)/,$(DEPLIBS))
	$(CC) -o $(TARGET) $(CC_LDFLAGS) $(LIBDIRS) $(LIBS) $(OBJS)

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
	@if [[ -e $(PCHOUT) ]]; then rm $(PCHOUT); fi
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
%.d : %.cpp
	set -e; $(CC) -MM $(CCFLAGS) $(INCDIRS) $< | \
	sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

%.d : %.c
	set -e; $(CC) -MM $(CCFLAGS) $(INCDIRS) $< | \
	sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

%.o : %.cpp
	$(CC) -c $(CCFLAGS) $(INCDIRS) $< -o $@	

%.o : %.c
	$(CC) -c $(CCFLAGS) $(INCDIRS) $< -o $@	

#
# Build the precompiled header file and then the object file
#
$(PCHOBJ) : $(PCHSRC)
	@if [[ -e $(PRJDIR)/$(PCHOUT) ]]; then rm $(PRJDIR)/$(PCHOUT); fi
	$(CC) -c -x c++-header $(CCFLAGS) $(INCDIRS) $(PRJDIR)/$(PCHHDR)
	$(CC) -c $(CCFLAGS) $(INCDIRS) $< -o $@

# ------------------------------------------------------------------------
#
# Dependencies
#
# ------------------------------------------------------------------------
ifneq (0,$(MAKELEVEL))
include $(DEPS)
endif

