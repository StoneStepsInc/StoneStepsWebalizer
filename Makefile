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
.PHONY: all clean install test package

# ------------------------------------------------------------------------
#
# Webalizer variables
#
# ------------------------------------------------------------------------
TARGET   := webalizer

#
# Precompiled header file names.
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

# include and library search paths
INCDIRS  := 
LIBDIRS  := 

# list of source files (precompiled header file must be first in the list)
SRCS     := $(PCHSRC) tstring.cpp linklist.cpp hashtab.cpp \
	output.cpp graphs.cpp preserve.cpp lang.cpp \
	parser.cpp logrec.cpp tstamp.cpp \
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
	platform/thread_pthread.cpp platform/console_linux.cpp \
	encoder.cpp p2_buffer_allocator.cpp char_buffer_stack.cpp \
	cp1252.cpp hckdel.cpp fmt_impl.cpp \
	util_http.cpp util_ipaddr.cpp util_path.cpp util_string.cpp \
	util_time.cpp util_url.cpp

# list all libraries we use
LIBS     := stdc++ dl pthread db_cxx gd z maxminddb

# generate a list of object files
OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

# and a list oof dependency make files
DEPS := $(OBJS:.o=.d)

# and a list of template repository files
RPOS := $(OBJS:.o=.rpo)

# ------------------------------------------------------------------------
#
# Unit test variables
#
# ------------------------------------------------------------------------
TEST     := test

# precompiled header file names
TEST_PCHHDR	 := pchtest.h
TEST_PCHSRC	 := $(TEST_PCHHDR:.h=.cpp)
TEST_PCHOUT	 := $(TEST_PCHHDR:.h=.h.gch)
TEST_PCHOBJ	 := $(TEST_PCHHDR:.h=.o)

# unit test source directory
TEST_SRCDIR := src/test
TEST_RSLT_DIR := test-results
TEST_RPT_FILE := test-report.xml

TEST_SRC := $(TEST_PCHSRC) main.cpp ut_caseconv.cpp ut_formatter.cpp ut_hostname.cpp \
	ut_ipaddr.cpp ut_lang.cpp ut_linklist.cpp ut_normurl.cpp \
	ut_strcmp.cpp ut_strfmt.cpp ut_strsrch.cpp ut_tstamp.cpp

TEST_LIBS     := stdc++ pthread gtest

#
# List unit test object files and some from the main project to link against. 
# Note that unit test files are prefixed with ut_ and will not collide with
# any main project files, so we don't need to place them into their own build
# directory.
#
TEST_OBJS := $(TEST_SRC:.cpp=.o) \
	char_buffer.o char_buffer_stack.o cp1252.o cp1252_ucs2.o \
	encoder.o formatter.o hashtab.o hckdel.o lang.o linklist.o \
	pch.o serialize.o tstamp.o tstring.o unicode.o fmt_impl.o \
	util_http.o util_ipaddr.o util_path.o util_string.o util_time.o \
	util_url.o

TEST_DEPS := $(TEST_OBJS:.o=.d)
TEST_RPOS := $(TEST_OBJS:.o=.rpo)

# ------------------------------------------------------------------------
#
# Package variables
#
# ------------------------------------------------------------------------
PKG_NAME  := webalizer-$$(build/webalizer -v -Q | sed -e s/\\./-/g).tar
PKG_OWNER := --owner=root --group=root
PKG_FILES := sample.conf src/webalizer_highcharts.js src/webalizer.css \
	src/webalizer.js README CHANGES COPYING Copyright
PKG_LANG  := catalan croatian czech danish dutch english estonian finnish french \
	galician german greek hungarian icelandic indonesian italian japanese \
	korean latvian malay norwegian polish portuguese portuguese_brazilian \
	romanian russian serbian simplified_chinese slovak slovene spanish \
	swedish turkish ukrainian

# ------------------------------------------------------------------------
#
# Define compiler options
#
# ------------------------------------------------------------------------
CCFLAGS  = -DHAVE_CXX_STDHEADERS \
     -DETCDIR=\"$(ETCDIR)\" \
     -fexceptions \
     -Wno-multichar -Winvalid-pch \
     -std=c++14 \
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
all: $(BLDDIR)/$(TARGET) $(BLDDIR)/$(TEST)

#
# build/webalizer
#
$(BLDDIR)/$(TARGET): $(BLDDIR)/$(PCHOUT) $(addprefix $(BLDDIR)/,$(OBJS)) 
	$(CC) -o $@ $(CC_LDFLAGS) $(addprefix -L,$(LIBDIRS)) \
		$(addprefix $(BLDDIR)/,$(OBJS)) $(addprefix -l,$(LIBS)) 

#
# build/test
#
$(BLDDIR)/$(TEST): $(BLDDIR)/$(TEST_PCHOUT) $(BLDDIR)/$(TARGET) $(addprefix $(BLDDIR)/,$(TEST_OBJS))
	$(CC) -o $@ $(CC_LDFLAGS) $(addprefix -L,$(LIBDIRS)) \
		$(addprefix $(BLDDIR)/,$(TEST_OBJS)) $(addprefix -l,$(TEST_LIBS))

#
# run unit tests and generate an XML results file in buikd/test-results/
#
test: $(BLDDIR)/$(TEST)
	$(BLDDIR)/$(TEST) --gtest_output=xml:$(BLDDIR)/$(TEST_RSLT_DIR)/$(TEST_RPT_FILE)

install:
	@echo
	@echo "The install target is not implemented"
	@echo

clean:
	@echo "Removing object files..."
	@for obj in $(OBJS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@for obj in $(TEST_OBJS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@echo "Removing dependency files..."
	@for obj in $(DEPS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@for obj in $(TEST_DEPS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@echo "Removing the precompiled header..."
	@if [[ -e $(BLDDIR)/$(PCHOUT) ]]; then rm $(BLDDIR)/$(PCHOUT); fi
	@if [[ -e $(BLDDIR)/$(TEST_PCHOUT) ]]; then rm $(BLDDIR)/$(TEST_PCHOUT); fi
	@echo "Removing template repository files..."
	@for obj in $(RPOS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@for obj in $(TEST_RPOS); do if [[ -e $(BLDDIR)/$$obj ]]; then rm $(BLDDIR)/$$obj; fi; done
	@echo "Removing executables..."
	@if [[ -e $(BLDDIR)/$(TARGET) ]]; then rm $(BLDDIR)/$(TARGET); fi
	@if [[ -e $(BLDDIR)/$(TEST) ]]; then rm $(BLDDIR)/$(TEST); fi
	@echo "Removing test results..."
	@if [[ -e $(BLDDIR)/$(TEST_RSLT_DIR)/$(TEST_RPT_FILE) ]]; then rm $(BLDDIR)/$(TEST_RSLT_DIR)/$(TEST_RPT_FILE); fi
	@echo "Done"

package: $(BLDDIR)/$(TARGET)
	@echo "Adding Webalizer files..." 
	@strip --strip-debug build/webalizer
	@tar $(PKG_OWNER) -cf $(PKG_NAME) -C $(BLDDIR) $(TARGET)
	@tar $(PKG_OWNER) -rf $(PKG_NAME) $(PKG_FILES) 
	@echo "Adding language files..."
	@for lang in $(PKG_LANG); do tar $(PKG_OWNER) -rf $(PKG_NAME) lang/webalizer_lang.$$lang; done
	@echo "Compressing..."
	@gzip $(PKG_NAME)
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
$(BLDDIR)/%.d : $(SRCDIR)/%.cpp
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); fi
	set -e; $(CC) -MM $(CCFLAGS) $(addprefix -I,$(INCDIRS)) $< | \
	sed 's/^[ \t]*\($(subst /,\/,$*)\)\.o/$(BLDDIR)\/\1.o $(BLDDIR)\/\1.d/g' > $@

$(BLDDIR)/%.d : $(TEST_SRCDIR)/%.cpp
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); fi
	set -e; $(CC) -MM $(CCFLAGS) $(addprefix -I,$(INCDIRS)) $< | \
	sed 's/^[ \t]*\($(subst /,\/,$*)\)\.o/$(BLDDIR)\/\1.o $(BLDDIR)\/\1.d/g' > $@

#
# Rules to compile source file
#
$(BLDDIR)/%.o : $(SRCDIR)/%.cpp
	$(CC) -c $(CCFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

$(BLDDIR)/%.o : $(TEST_SRCDIR)/%.cpp
	$(CC) -c $(CCFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@	

#
# Build the precompiled header file
#
$(BLDDIR)/$(PCHOUT) : $(SRCDIR)/$(PCHHDR) $(SRCDIR)/$(PCHSRC)
	@if [[ -e $(BLDDIR)/$(PCHOUT) ]]; then rm $(BLDDIR)/$(PCHOUT); fi
	$(CC) -c -x c++-header $(CCFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

$(BLDDIR)/$(TEST_PCHOUT) : $(TEST_SRCDIR)/$(TEST_PCHHDR) $(TEST_SRCDIR)/$(TEST_PCHSRC)
	@if [[ -e $(BLDDIR)/$(TEST_PCHOUT) ]]; then rm $(BLDDIR)/$(TEST_PCHOUT); fi
	$(CC) -c -x c++-header $(CCFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

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
include $(addprefix $(BLDDIR)/, $(DEPS)) $(addprefix $(BLDDIR)/, $(TEST_DEPS))
else ifneq ($(filter-out clean install package,$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(DEPS)) $(addprefix $(BLDDIR)/, $(TEST_DEPS))
endif
