# ------------------------------------------------------------------------
#
#   Makefile
# 	 
#   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)
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
WEBALIZER   := webalizer

#
# Precompiled header file names.
#
# Precompiled headers are always generated from a pch.h/pch.cpp pair of files
# located in the main source directory or sub-directories, such as src/pch.cpp
# or src/test/pch.cpp. These files are expected at the top level of their
# build target source tree (e.g. src/test/pch.cpp) and are included explicitly
# when source files from that tree are compiled (see implicit rules at the end
# of this file).
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

# list of source files (the src/ prefix is assumed in build rules)
SRCS     := $(PCHSRC) tstring.cpp linklist.cpp hashtab.cpp \
	output.cpp graphs.cpp preserve.cpp lang.cpp \
	parser.cpp logrec.cpp tstamp.cpp \
	webalizer.cpp dns_resolv.cpp history.cpp tmranges.cpp \
	anode.cpp ccnode.cpp dlnode.cpp hnode.cpp \
	inode.cpp rcnode.cpp rnode.cpp snode.cpp \
	unode.cpp vnode.cpp ctnode.cpp \
	danode.cpp keynode.cpp scnode.cpp sysnode.cpp \
	daily.cpp hourly.cpp totals.cpp queue_nodes.cpp \
	hashtab_nodes.cpp config.cpp serialize.cpp \
	html_output.cpp dump_output.cpp \
	berkeleydb.cpp database.cpp logfile.cpp cp1252_ucs2.cpp \
	char_buffer.cpp unicode.cpp formatter.cpp \
	platform/exception_linux.cpp platform/event_pthread.cpp \
	platform/thread_pthread.cpp platform/console_linux.cpp \
	encoder.cpp p2_buffer_allocator.cpp char_buffer_stack.cpp \
	cp1252.cpp hckdel.cpp fmt_impl.cpp \
	util_http.cpp util_ipaddr.cpp util_path.cpp util_string.cpp \
	util_time.cpp util_url.cpp

# list all libraries we use
LIBS     := dl pthread db_cxx gd z maxminddb

# generate a list of object files for .c and .cpp source files
OBJS := $(patsubst %.c,%.o,$(filter %.c,$(SRCS))) \
	$(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS)))

# and a list of dependency make files
DEPS := $(OBJS:.o=.d)

# ------------------------------------------------------------------------
#
# Unit tests
#
# ------------------------------------------------------------------------

# pick a name that won't conflict with build/test/
TEST     := utest

# Bitbucket unit test results location
TEST_RSLT_DIR := test-results
TEST_RPT_FILE := test-report.xml

# unit tests source files (the test/ prefix is added right after this assignment)
TEST_SRC := $(PCHSRC) main.cpp ut_caseconv.cpp ut_formatter.cpp ut_hostname.cpp \
	ut_ipaddr.cpp ut_lang.cpp ut_linklist.cpp ut_normurl.cpp \
	ut_strcmp.cpp ut_strfmt.cpp ut_strsrch.cpp ut_tstamp.cpp \
	ut_config.cpp ut_strcreate.cpp ut_hashtab.cpp ut_initseqguard.cpp \
	ut_berkeleydb.cpp ut_unicode.cpp ut_serialize.cpp ut_ctnode.cpp

# add the test/ prefix (will be combined with src/ in build rules)
TEST_SRC := $(addprefix test/,$(TEST_SRC))

TEST_LIBS := $(LIBS) gtest

#
# List unit test object files and some from the main project to link against. 
# Note that unit test files are prefixed with ut_ and will not collide with
# any main project files, so we don't need to place them into their own build
# directory.
#
TEST_OBJS := $(TEST_SRC:.cpp=.o)  \
	char_buffer.o char_buffer_stack.o cp1252.o cp1252_ucs2.o \
	encoder.o formatter.o hashtab.o hckdel.o lang.o linklist.o \
	pch.o serialize.o tstamp.o tstring.o unicode.o fmt_impl.o \
	util_http.o util_ipaddr.o util_path.o util_string.o util_time.o \
	util_url.o tmranges.o config.o anode.o dlnode.o ccnode.o hnode.o \
	rcnode.o vnode.o unode.o snode.o inode.o rnode.o ctnode.o \
	keynode.o hashtab_nodes.o berkeleydb.o

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

# compiler options shared between C and C++ source
CCFLAGS_COMMON := -Werror -pedantic

CFLAGS := -std=gnu99 \
	$(CCFLAGS_COMMON) \
	$(MYCCFLAGS)

CXXFLAGS := -std=c++17 \
	$(CCFLAGS_COMMON) \
	-DHAVE_CXX_STDHEADERS \
	-DETCDIR=\"$(ETCDIR)\" \
	-fexceptions \
	-Wno-multichar \
	-Winvalid-pch \
	$(MYCCFLAGS)

# turn on optimization for non-debug builds
ifeq ($(findstring -g,$(CXXFLAGS)),)
CXXFLAGS += -O3
endif

ifeq ($(findstring -g,$(CFLAGS)),)
CFLAGS += -O3
endif

# flags passed to the linker through $(CXX)
CC_LDFLAGS := 

# ------------------------------------------------------------------------
#
# Targets
#
# ------------------------------------------------------------------------

# default target
all: $(BLDDIR)/$(WEBALIZER)

#
# build/webalizer
#
$(BLDDIR)/$(WEBALIZER): $(BLDDIR)/$(PCHOUT) $(addprefix $(BLDDIR)/,$(OBJS)) 
	$(CXX) -o $@ $(CC_LDFLAGS) $(addprefix -L,$(LIBDIRS)) \
		$(addprefix $(BLDDIR)/,$(OBJS)) $(addprefix -l,$(LIBS)) 

#
# build/utest
#
$(BLDDIR)/$(TEST): $(BLDDIR)/test/$(PCHOUT) $(BLDDIR)/$(WEBALIZER) $(addprefix $(BLDDIR)/,$(TEST_OBJS))
	$(CXX) -o $@ $(CC_LDFLAGS) $(addprefix -L,$(LIBDIRS)) \
		$(addprefix $(BLDDIR)/,$(TEST_OBJS)) $(addprefix -l,$(TEST_LIBS))

#
# run unit tests and generate an XML results file in build/test-results/
#
test: $(BLDDIR)/$(TEST)
	$(BLDDIR)/$(TEST) --gtest_output=xml:$(BLDDIR)/$(TEST_RSLT_DIR)/$(TEST_RPT_FILE)

install:
	@echo
	@echo "The install target is not implemented"
	@echo

clean:
	@echo "Removing object files..."
	@find $(BLDDIR)/ -name '*.o' -type f -delete
	@echo "Removing dependency files..."
	@find $(BLDDIR)/ -name '*.d' -type f -delete
	@echo "Removing the precompiled header..."
	@find $(BLDDIR)/ -name '$(PCHOUT)' -type f -delete
	@echo "Removing executables..."
	@if [[ -e $(BLDDIR)/$(WEBALIZER) ]]; then rm $(BLDDIR)/$(WEBALIZER); fi
	@if [[ -e $(BLDDIR)/$(TEST) ]]; then rm $(BLDDIR)/$(TEST); fi
	@echo "Removing test results..."
	@if [[ -e $(BLDDIR)/$(TEST_RSLT_DIR)/$(TEST_RPT_FILE) ]]; then rm $(BLDDIR)/$(TEST_RSLT_DIR)/$(TEST_RPT_FILE); fi
	@echo "Done"

package: $(BLDDIR)/$(WEBALIZER)
	@echo "Adding Webalizer files..." 
	@strip --strip-debug build/webalizer
	@tar $(PKG_OWNER) -cf $(PKG_NAME) -C $(BLDDIR) $(WEBALIZER)
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
# Dependency rules generate .d files in the build directory, which are
# included at the bottom of this file.
#
# GCC -MM strips the path from the input file, so we need to put it back
# into the dependency targets. For example, this command:
#
#  gcc -MM src/webalizer.cpp
#
# , produces this dependency list:
#
#   webalizer.o: src/webalizer.cpp src/webalizer.h ...
#
# , so we need to inject source paths back in and add the dependency
# files as well, so they are rebuilt when any headers change:
#
#   src/webalizer.o src/webalizer.d: src/webalizer.cpp src/webalizer.h ...
#
$(BLDDIR)/%.d : src/%.cpp
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); fi
	set -e; $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $(addprefix -I,$(INCDIRS)) $< | \
	sed 's/^[ \t]*$(*F)\.o/$(BLDDIR)\/$(subst /,\/,$*).o $(BLDDIR)\/$(subst /,\/,$*).d/g' > $@

#
# Rules to compile source file
#

# include the unit test precompiled header in all unit test source files
$(BLDDIR)/test/%.o : src/test/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -include $(BLDDIR)/test/$(PCHHDR) $(addprefix -I,$(INCDIRS)) $< -o $@

# no precompiled header for C source
$(BLDDIR)/%.o : src/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

# include the main precompiled header in all source files (unit test is handled above)
$(BLDDIR)/%.o : src/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -include $(BLDDIR)/$(PCHHDR) $(addprefix -I,$(INCDIRS)) $< -o $@

#
# Build the precompiled header file
#

# a rule for /src/pch.h.gch, which produces an empty stem (i.e. build//pch.cpp) and is ignored by src/%/pch.h.gch
$(BLDDIR)/$(PCHOUT) : src/$(PCHSRC)
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); elif [[ -e $@ ]]; then rm $@; fi
	$(CXX) -c -x c++-header $(CPPFLAGS) $(CXXFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

# a rule for any pch.h.gch located anywhere else in the source tree
$(BLDDIR)/%/$(PCHOUT) : src/%/$(PCHSRC)
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); elif [[ -e $@ ]]; then rm $@; fi
	$(CXX) -c -x c++-header $(CPPFLAGS) $(CXXFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

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
