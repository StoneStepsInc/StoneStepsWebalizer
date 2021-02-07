# ------------------------------------------------------------------------
#
#   Makefile
# 	 
#   Copyright (c) 2004-2021, Stone Steps Inc. (www.stonesteps.ca)
#
#   See COPYING and Copyright files for additional licensing and copyright information 
# 
# ------------------------------------------------------------------------

SHELL := /bin/bash

# ------------------------------------------------------------------------
#
#   Usage:  make [[name=value] ...]
#           make test
#           make package
#           make clean
#           make clean-deps
#           make install
#           make install-info
#           make uninstall
#
#   Parameters:
#     all targets:
#       BLDDIR=/path/to/build/directory (default ./build)
#
#     build:
#       ETCDIR=/etc
#       MYCCFLAGS=-g
#       AZP_BUILD_NUMBER=auto-incremented-build-number
#
#     test:
#       TEST_RSLT_DIR=/path/to/test/results/directory (default BLDDIR)
#       TEST_RSLT_FILE=test-results-file-name (default utest.xml)
#
#     package:
#       PKG_DIR=/path/to/package/directory
#       PKG_OS_ABBR=OS-name (default linux)
#       PKG_ARCH_ABBR=cpu-architecture (default $(uname -p))
# ------------------------------------------------------------------------

#
# Remove all standard suffix rules and declare phony targets
#
.SUFFIXES:
.PHONY: all clean clean-deps install install-info uninstall test package

# ------------------------------------------------------------------------
#
# Webalizer variables
#
# ------------------------------------------------------------------------
WEBALIZER   := webalizer

#
# webalizer            -> /usr/local/bin/webalizer-stonesteps
# webalizer (sym.link) -> /usr/local/bin
# webalizer.css, etc   -> /usr/local/share/webalizer-stonesteps/www
# language files       -> /usr/local/share/webalizer-stonesteps/lang
# dir for MaxMind DBs  -> /usr/local/share/webalizer-stonesteps/maxmind
# README.md, etc       -> /usr/local/share/doc/webalizer-stonesteps
# webalizer.db         -> /var/local/lib/webalizer-stonesteps
#
INSTDIR_BIN := /usr/local/bin/webalizer-stonesteps
INSTDIR_BNL := /usr/local/bin
INSTDIR_SHR := /usr/local/share/webalizer-stonesteps
INSTDIR_WWW := /usr/local/share/webalizer-stonesteps/www
INSTDIR_LNG := /usr/local/share/webalizer-stonesteps/lang
INSTDIR_MDB := /usr/local/share/webalizer-stonesteps/maxmind
INSTDIR_DOC := /usr/local/share/doc/webalizer-stonesteps
INSTDIR_VAR := /var/local/lib/webalizer-stonesteps

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
endif

# source and build directories
SRCDIR   := src

# build all output in ./build, unless asked otherwise
ifeq ($(strip $(BLDDIR)),)
BLDDIR   := build
endif

# include and library search paths
INCDIRS  := 
LIBDIRS  := 

# webalizer source files, relative to $(SRCDIR)
SRCS     := $(PCHSRC) tstring.cpp linklist.cpp hashtab.cpp \
	output.cpp graphs.cpp preserve.cpp lang.cpp \
	parser.cpp logrec.cpp tstamp.cpp \
	webalizer.cpp dns_resolv.cpp history.cpp tmranges.cpp \
	anode.cpp ccnode.cpp dlnode.cpp hnode.cpp \
	inode.cpp rcnode.cpp rnode.cpp snode.cpp \
	unode.cpp vnode.cpp ctnode.cpp asnode.cpp \
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

# webalizer libraries
LIBS     := dl pthread db_cxx gd z maxminddb

OBJS := $(patsubst %.c,%.o,$(filter %.c,$(SRCS))) \
	$(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS)))

DEPS := $(OBJS:.o=.d)

# ------------------------------------------------------------------------
#
# Unit tests
#
# ------------------------------------------------------------------------

# pick a name that won't conflict with build/test/
TEST     := utest

# output test results in the build directory unless asked otherwise
ifeq ($(strip $(TEST_RSLT_DIR)),)
TEST_RSLT_DIR := $(BLDDIR)
endif

# use the name of the unit test executable, unless asked otherwise
ifeq ($(strip $(TEST_RSLT_FILE)),)
TEST_RSLT_FILE := $(TEST).xml
endif

# unit tests source files
TEST_SRC := $(PCHSRC) main.cpp ut_caseconv.cpp ut_formatter.cpp ut_hostname.cpp \
	ut_ipaddr.cpp ut_lang.cpp ut_linklist.cpp ut_normurl.cpp \
	ut_strcmp.cpp ut_strfmt.cpp ut_strsrch.cpp ut_tstamp.cpp \
	ut_config.cpp ut_strcreate.cpp ut_hashtab.cpp ut_initseqguard.cpp \
	ut_berkeleydb.cpp ut_unicode.cpp ut_serialize.cpp ut_ctnode.cpp

# add the test/ prefix, which in turn is relative to $(SRCDIR)
TEST_SRC := $(addprefix test/,$(TEST_SRC))

TEST_LIBS := $(LIBS) gtest

# unit test object files and some from the main project to link against
TEST_OBJS := $(TEST_SRC:.cpp=.o)  \
	char_buffer.o char_buffer_stack.o cp1252.o cp1252_ucs2.o \
	encoder.o formatter.o hashtab.o hckdel.o lang.o linklist.o \
	pch.o serialize.o tstamp.o tstring.o unicode.o fmt_impl.o \
	util_http.o util_ipaddr.o util_path.o util_string.o util_time.o \
	util_url.o tmranges.o config.o anode.o dlnode.o ccnode.o hnode.o \
	rcnode.o vnode.o unode.o snode.o inode.o rnode.o ctnode.o asnode.o \
	keynode.o hashtab_nodes.o berkeleydb.o

TEST_DEPS := $(TEST_OBJS:.o=.d)

# ------------------------------------------------------------------------
#
# Package variables
#
# ------------------------------------------------------------------------

ifeq ($(strip $(PKG_DIR)),)
PKG_DIR       := $(BLDDIR)
endif

ifeq ($(strip $(PKG_OS_ABBR)),)
PKG_OS_ABBR   := linux
endif

ifeq ($(strip $(PKG_ARCH_ABBR)),)
PKG_ARCH_ABBR := $$(uname -p)
endif

PKG_NAME  := webalizer-$(PKG_OS_ABBR)-$(PKG_ARCH_ABBR)-$$($(BLDDIR)/$(WEBALIZER) -v -Q).tar
PKG_OWNER := --owner=root --group=root
PKG_FILES := sample.conf $(SRCDIR)/webalizer_highcharts.js $(SRCDIR)/webalizer.css \
	$(SRCDIR)/webalizer.js README.md CHANGES COPYING Copyright
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

# if CI pipeline build number is provided, add it to the build
ifdef AZP_BUILD_NUMBER
CXXFLAGS += -DBUILD_NUMBER=$(AZP_BUILD_NUMBER)
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
$(BLDDIR)/$(WEBALIZER): $(addprefix $(BLDDIR)/,$(OBJS)) | $(BLDDIR) 
	$(CXX) -o $@ $(CC_LDFLAGS) $(addprefix -L,$(LIBDIRS)) \
		$(addprefix $(BLDDIR)/,$(OBJS)) $(addprefix -l,$(LIBS)) 

#
# build/utest
#
$(BLDDIR)/$(TEST): $(addprefix $(BLDDIR)/,$(TEST_OBJS)) | $(BLDDIR) 
	$(CXX) -o $@ $(CC_LDFLAGS) $(addprefix -L,$(LIBDIRS)) \
		$(addprefix $(BLDDIR)/,$(TEST_OBJS)) $(addprefix -l,$(TEST_LIBS))

#
# build directory
#
$(BLDDIR): 
	@mkdir $(BLDDIR)

#
# run unit tests and generate an XML results file in build/test-results/
#
test: $(BLDDIR)/$(TEST)
	$(BLDDIR)/$(TEST) --gtest_output=xml:$(TEST_RSLT_DIR)/$(TEST_RSLT_FILE)

clean:
	@echo "Removing object files..."
	@rm -f $(addprefix $(BLDDIR)/, $(OBJS))
	@rm -f $(addprefix $(BLDDIR)/, $(TEST_OBJS))
	@echo "Removing dependency files..."
	@rm -f $(addprefix $(BLDDIR)/, $(DEPS))
	@rm -f $(addprefix $(BLDDIR)/, $(TEST_DEPS))
	@echo "Removing the precompiled header..."
	@rm -f $(BLDDIR)/$(PCHOUT) $(BLDDIR)/test/$(PCHOUT)
	@echo "Removing executables..."
	@rm -f $(BLDDIR)/$(WEBALIZER)
	@rm -f $(BLDDIR)/$(TEST)
	@echo "Removing test results..."
	@rm -f $(TEST_RSLT_DIR)/$(TEST_RSLT_FILE)
	@echo "Done"

#
# When a source file is moved or renamed, the next build may fail
# because the previous dependency file may reference a stale source
# file. This target allows us to rebuild just the dependencies
# instead of having to make a full build.
#
clean-deps:
	@echo 'Removing dependencies'
	@rm -f $(addprefix $(BLDDIR)/,$(DEPS)) $(addprefix $(BLDDIR)/,$(TEST_DEPS))
	@echo 'Done'

package: $(BLDDIR)/$(WEBALIZER)
	@echo "Adding Webalizer files..." 
	@strip --strip-debug $(BLDDIR)/$(WEBALIZER)
	@tar $(PKG_OWNER) -cf $(PKG_DIR)/$(PKG_NAME) -C $(BLDDIR) $(WEBALIZER)
	@tar $(PKG_OWNER) -rf $(PKG_DIR)/$(PKG_NAME) $(PKG_FILES) 
	@echo "Adding language files..."
	@for lang in $(PKG_LANG); do tar $(PKG_OWNER) -rf $(PKG_DIR)/$(PKG_NAME) lang/webalizer_lang.$$lang; done
	@echo "Compressing..."
	@gzip $(PKG_DIR)/$(PKG_NAME)
	@echo "Done"

#
# The symbolic link in /usr/local/bin is not created if a file with this
# name already exists because it may be the original webalizer executable.
#
install: $(BLDDIR)/$(WEBALIZER)
	@echo ''
	@if [ ! -d $(INSTDIR_BIN) ]; \
	then \
		mkdir -p $(INSTDIR_BIN); \
	fi
	@cp -f $(BLDDIR)/$(WEBALIZER) $(INSTDIR_BIN)
	@if [ ! -e $(INSTDIR_BNL)/$(WEBALIZER) ]; \
	then \
		ln -s $(INSTDIR_BIN)/$(WEBALIZER) $(INSTDIR_BNL)/$(WEBALIZER); \
	else \
		echo 'Leaving existing file $(INSTDIR_BNL)/$(WEBALIZER) alone'; \
		echo ''; \
	fi
	@mkdir -p $(INSTDIR_VAR)
	@mkdir -p $(INSTDIR_WWW)
	@for file in webalizer.css webalizer.js webalizer_highcharts.js; \
	do \
		cp -f src/$$file $(INSTDIR_WWW); \
	done
	@mkdir -p $(INSTDIR_LNG)
	@cp -f lang/webalizer_lang.* $(INSTDIR_LNG)
	@mkdir -p $(INSTDIR_MDB)
	@mkdir -p $(INSTDIR_DOC)
	@for file in README.md CHANGES Copyright COPYING; \
	do \
		cp -f $$file $(INSTDIR_DOC); \
	done

install-info:
	@echo ''
	@echo 'Set DbPath to $(INSTDIR_VAR)'
	@echo ''
	@echo 'Copy files from $(INSTDIR_WWW)'
	@echo 'to a directory where they can be referenced from HTML reports.'
	@echo 'Set HTMLCssPath and HTMLJsPath to point to that directory.'
	@echo ''
	@echo 'Set LanguageFile to point to a language file of your choice in'
	@echo '$(INSTDIR_LNG)'
	@echo ''
	@echo 'If you enabled GeoIP and/or ASN, copy MaxMind databases to'
	@echo '$(INSTDIR_MDB)'
	@echo 'Set GeoIPDBPath and ASNDBPath to point to the corresponding'
	@echo 'databases.'
	@echo ''
	@echo 'See README.md in $(INSTDIR_DOC)'
	@echo 'for more details.'
	@echo ''

#
# The directory with state databases and MaxMind databases may be
# left behind because they may contain data that cannot be recovered.
#
uninstall:
	@if [ -d $(INSTDIR_DOC) ]; \
	then \
		for file in README.md CHANGES Copyright COPYING; \
		do \
			rm -f $(INSTDIR_DOC)/$$file; \
		done; \
		rmdir $(INSTDIR_DOC); \
	fi
	@if [ -d $(INSTDIR_LNG)/ ]; \
	then \
		rm -f $(INSTDIR_LNG)/webalizer_lang.*; \
		rmdir $(INSTDIR_LNG); \
	fi
	@if [ -d $(INSTDIR_WWW) ]; \
	then \
		for file in webalizer.css webalizer.js webalizer_highcharts.js; \
		do \
			rm -f $(INSTDIR_WWW)/$$file; \
		done; \
		rmdir $(INSTDIR_WWW); \
	fi
	@if [ -d $(INSTDIR_MDB) ]; \
	then \
		rmdir --ignore-fail-on-non-empty $(INSTDIR_MDB); \
	fi
	@if [ -d $(INSTDIR_VAR) ]; \
	then \
		rmdir --ignore-fail-on-non-empty $(INSTDIR_VAR); \
	fi
	@if [ -d $(INSTDIR_SHR) ]; \
	then \
		rmdir $(INSTDIR_SHR); \
	fi
	@if [ -L $(INSTDIR_BNL)/$(WEBALIZER) ]; \
	then \
		if [ $$(ls -l /usr/local/bin | sed -r -n -e 's/^.+ $(WEBALIZER) -> ([-/a-z]+)/\1/p') = '$(INSTDIR_BIN)/$(WEBALIZER)' ]; \
		then \
			rm -f $(INSTDIR_BNL)/$(WEBALIZER); \
		fi; \
	fi
	@rm -f $(INSTDIR_BIN)/$(WEBALIZER)
	@if [ -d $(INSTDIR_BIN) ]; \
	then \
		rmdir $(INSTDIR_BIN); \
	fi

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
#   build/webalizer.o build/webalizer.d: src/webalizer.cpp src/webalizer.h ...
#
$(BLDDIR)/%.d : $(SRCDIR)/%.cpp
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); fi
	set -e; $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $(addprefix -I,$(INCDIRS)) $< | \
	sed 's|^[ \t]*$(*F)\.o|$(BLDDIR)/$*.o $(BLDDIR)/$*.d|g' > $@

#
# Source files
#

# compile unit test source with the unit test precompiled header
$(BLDDIR)/test/%.o : $(SRCDIR)/test/%.cpp $(BLDDIR)/test/$(PCHOUT)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -include $(BLDDIR)/test/$(PCHHDR) $(addprefix -I,$(INCDIRS)) $< -o $@

# no precompiled header for C source
$(BLDDIR)/%.o : $(SRCDIR)/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

# compile main source with its precompiled header
$(BLDDIR)/%.o : $(SRCDIR)/%.cpp	$(BLDDIR)/$(PCHOUT)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -include $(BLDDIR)/$(PCHHDR) $(addprefix -I,$(INCDIRS)) $< -o $@

# webalizer and unit test precompiled header files
$(BLDDIR)/$(PCHOUT) $(BLDDIR)/test/$(PCHOUT): $(BLDDIR)/%$(PCHOUT): $(SRCDIR)/%$(PCHSRC)
	@if [[ ! -e $(@D) ]]; then mkdir -p $(@D); elif [[ -e $@ ]]; then rm $@; fi
	$(CXX) -c -x c++-header $(CPPFLAGS) $(CXXFLAGS) $(addprefix -I,$(INCDIRS)) $< -o $@

# ------------------------------------------------------------------------
#
# Dependencies
#
# ------------------------------------------------------------------------

# webalizer dependencies
ifeq ($(MAKECMDGOALS),)
include $(addprefix $(BLDDIR)/, $(DEPS))
else ifneq ($(filter package,$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(DEPS))
else ifneq ($(filter install,$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(DEPS))
else ifneq ($(filter $(BLDDIR)/$(WEBALIZER),$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(DEPS))
endif

# unit test dependencies
ifneq ($(filter test,$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(TEST_DEPS))
else ifneq ($(filter $(BLDDIR)/$(TEST),$(MAKECMDGOALS)),)
include $(addprefix $(BLDDIR)/, $(TEST_DEPS))
endif
