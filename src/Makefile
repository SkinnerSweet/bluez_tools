# The MIT License (MIT)

# Copyright (c) 2016 Thomas Bertauld <thomas.bertauld@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# ITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
 

# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

x_filter_out = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))

CC = $(CROSS_COMPILE)gcc
LD = $(CC)

LIBTYPE ?= so
INSTALL_LIB_PATH ?= /usr/local/lib
INSTALL_H_PATH ?= /usr/local/include

BUILDDIR = ../build
LIBDIR = ../lib

SRC_DIRECTORIES := $(sort $(dir $(call rwildcard,.,*/)))
BUILD_DIRECTORIES := $(BUILDDIR)
BUILD_DIRECTORIES += $(addprefix $(BUILDDIR)/,$(SRC_DIRECTORIES))
BUILD_DIRECTORIES := $(call x_filter_out,include/, $(BUILD_DIRECTORIES))

HCI_DIR = ./hci
L2CAP_DIR = ./l2cap
BT_DIR = ./bt
DATA_STRUCT_DIR = ./misc/data_struct
LOG_DIR = ./misc/log

HCI_SRC := $(call rwildcard,$(HCI_DIR)/,*.c)
HCI_OBJS := $(addprefix $(BUILDDIR)/,$(HCI_SRC:%.c=%.o))
HCI_HEADERS := $(call rwildcard,$(HCI_DIR)/,*.h)

L2CAP_SRC := $(call rwildcard,$(L2CAP_DIR)/,*.c)
L2CAP_OBJS := $(addprefix $(BUILDDIR)/,$(L2CAP_SRC:%.c=%.o))
L2CAP_HEADERS := $(call rwildcard,$(L2CAP_DIR)/,*.h)

BT_SRC := $(call rwildcard,$(BT_DIR)/,*.c)
BT_OBJS := $(addprefix $(BUILDDIR)/,$(BT_SRC:%.c=%.o))
BT_HEADERS := $(call rwildcard,$(BT_DIR)/,*.h)
BT_INC := $(sort $(dir $(BT_HEADERS)))
BT_INC := $(addprefix -I,$(BT_INC))

DATA_STRUCT_SRC := $(call rwildcard,$(DATA_STRUCT_DIR)/,*.c)
DATA_STRUCT_OBJS := $(addprefix $(BUILDDIR)/,$(DATA_STRUCT_SRC:%.c=%.o))
DATA_STRUCT_HEADERS := $(call rwildcard,$(DATA_STRUCT_DIR)/,*.h)
DATA_STRUCT_INC := $(sort $(dir $(DATA_STRUCT_HEADERS)))
DATA_STRUCT_INC := $(addprefix -I,$(DATA_STRUCT_INC))

LOG_SRC := $(call rwildcard,$(LOG_DIR)/,*.c)
LOG_OBJS := $(addprefix $(BUILDDIR)/,$(LOG_SRC:%.c=%.o))
LOG_HEADERS := $(call rwildcard,$(LOG_DIR)/,*.h)
LOG_INC := $(sort $(dir $(LOG_HEADERS)))
LOG_INC := $(addprefix -I,$(LOG_INC))

OBJS := $(BT_OBJS) $(DATA_STRUCT_OBJS) $(LOG_OBJS) $(HCI_OBJS) $(L2CAP_OBJS)

CINCLUDE = -I. $(DATA_STRUCT_INC) $(LOG_INC) $(BT_INC)

CCFLAGS = -std=c99 -O2 $(CINCLUDE) -Wall -D_GNU_SOURCE
SOFLAGS = -fpic

ifeq ($(LIBTYPE),so)
	CCFLAGS := $(CCFLAGS) $(SOFLAGS)
endif

LDFLAGS = -lbluetooth -lpthread

RM = rm -rf
MKDIR = mkdir

TARGET = bluez_tools

.PHONY: all

all: build_dir lib clean

build_dir: $(BUILD_DIRECTORIES)

$(BUILD_DIRECTORIES):
ifeq ($(wildcard $@),)
	$(MKDIR) $@
endif

lib: $(LIBDIR) $(TARGET).$(LIBTYPE)

$(TARGET).so: $(OBJS) 
	$(LD) -o $(LIBDIR)/lib$@ -shared $^ $(LDFLAGS)

$(TARGET).a: $(OBJS)
	$(AR) $(LIBDIR)/lib$@ $^

$(BUILDDIR)/$(HCI_DIR)/%.o : $(HCI_DIR)/%.c $(HCI_HEADERS)
	$(CC) $(CCFLAGS) -I $(dir $<)/include -c $< -o $@
	
$(BUILDDIR)/$(L2CAP_DIR)/%.o : $(L2CAP_DIR)/%.c $(L2CAP_HEADERS)
	$(CC) $(CCFLAGS) -I $(dir $<)/include -c $< -o $@
	
$(BUILDDIR)/$(BT_DIR)/%.o : $(BT_DIR)/%.c $(BT_HEADERS)
	$(CC) $(CCFLAGS) -I $(dir $<)/include -c $< -o $@
	
$(BUILDDIR)/$(DATA_STRUCT_DIR)/%.o : $(DATA_STRUCT_DIR)/%.c $(DATA_STRUCT_HEADERS)
	$(CC) $(CCFLAGS) -I $(dir $<)/include -c $< -o $@
	
$(BUILDDIR)/$(LOG_DIR)/%.o : $(LOG_DIR)/%.c $(LOG_HEADERS)
	$(CC) $(CCFLAGS) -I $(dir $<)/include -c $< -o $@

install: 
	cp $(LIBDIR)/lib$(TARGET).$(LIBTYPE) $(INSTALL_LIB_PATH)
	cp ../include/* $(INSTALL_H_PATH)
	touch /etc/ld.so.conf.d/libbluez_tools.conf
	echo $(INSTALL_LIB_PATH) > /etc/ld.so.conf.d/libbluez_tools
	ldconfig
	
clean:
	$(RM) $(OBJS)
	
mrproper: clean
	$(RM) $(LIBDIR)/lib$(TARGET).so 
	$(RM) $(LIBDIR)/lib$(TARGET).a 
