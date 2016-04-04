# The MIT License (MIT)

# Copyright (c) 2015 Thomas Bertauld <thomas.bertauld@gmail.com>
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

BLUEZ_TOOLS_LIB_DIR = ../lib
BLUEZ_TOOLS_INCLUDE_DIR = ../include

CC = $(CROSS_COMPILE)gcc
CCFLAGS = -L -std=gnu99 -I. -I${BLUEZ_TOOLS_INCLUDE_DIR} -L${BLUEZ_TOOLS_LIB_DIR}

.PHONY: main, server

main:
	$(CC) $(CCFLAGS) main.c matrice.c -o main -lbluez_tools -lbluetooth -lpthread -lm
server:
	$(CC) $(CCFLAGS) server1.c -o server1 -lbluez_tools -lbluetooth -lpthread
	$(CC) $(CCFLAGS) server2.c -o server2 -lbluez_tools -lbluetooth -lpthread
	$(CC) $(CCFLAGS) server3.c -o server3 -lbluez_tools -lbluetooth -lpthread

clean:
	rm -rf main server1 server2 server3