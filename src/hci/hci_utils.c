/* The MIT License (MIT)
 Copyright (c) 2016 Thomas Bertauld <thomas.bertauld@gmail.com>
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "hci_utils.h"

#include <string.h>
#include <stdio.h>
#include <bluetooth/hci_lib.h>
#include "trace.h"

typedef struct hci_states_map {
	uint64_t mask;
	char *description;
} hci_states_map_t;

static hci_states_map_t states_map[29] = {
	{htobs(0x0000000000000001), "Non-connectable Advertising State"},
	{htobs(0x0000000000000002), "Scannable Advertising State"},
	{htobs(0x0000000000000004), "Connectable Advertising State"},
	{htobs(0x0000000000000008), "Directed Advertising State"},
	{htobs(0x0000000000000010), "Passive Scanning State"},
	{htobs(0x0000000000000020), "Active Scanning State"},
	{htobs(0x0000000000000040), "Initiating State and Connection State in the Master Role"},
	{htobs(0x0000000000000080), "Connection State in Slave Role"},
	{htobs(0x0000000000000100), "Non-connectable Advertising State and Passive Scanning State combination"},
	{htobs(0x0000000000000200), "Scannable Advertising State and Passive Scanning State combination"},
	{htobs(0x0000000000000400), "Connectable Advertising State and Passive Scanning State combination"},
	{htobs(0x0000000000000800), "Directed Advertising State and Passive Scanning State combination"},
	{htobs(0x0000000000001000), "Non-connectable Advertising State and Active Scanning State combination"},
	{htobs(0x0000000000002000), "Scannable Advertising State and Active Scanning State combination"},
	{htobs(0x0000000000004000), "Connectable Advertising State and Active Scanning State combination"},
	{htobs(0x0000000000008000), "Directed Advertising State and Active Scanning State combination"},
	{htobs(0x0000000000010000), "Non-connectable Advertising State and Initiating State combination"},
	{htobs(0x0000000000020000), "Scannable Advertising State and Initiating State combination"},
	{htobs(0x0000000000040000), "Non-connectable Advertising State and Master Role combination"},
	{htobs(0x0000000000080000), "Scannable Advertising State and Master Role combination"},
	{htobs(0x0000000000100000), "Non-connectable Advertising State and Slave Role combination"},
	{htobs(0x0000000000200000), "Scannable Advertising State and Slave Role combination"},
	{htobs(0x0000000000400000), "Passive Scanning State and Initiating State combination"},
	{htobs(0x0000000000800000), "Active Scanning State and Initiating State combination"},
	{htobs(0x0000000001000000), "Passive Scanning State and Master Role combination"},
	{htobs(0x0000000002000000), "Active Scanning State and Master Role combination"},
	{htobs(0x0000000004000000), "Passive Scanning State and Slave Role combination"},
	{htobs(0x0000000008000000), "Active Scanning State and Slave Role combination"},
	{htobs(0x0000000010000000), "Initiating State and Master Role combination and Master Role and Master Role combination"}
};

//------------------------------------------------------------------------------------

void hci_compute_filter(struct hci_filter *flt, ...) {
	va_list  pa;
	uint32_t event;
	
	if (flt == NULL) {
		print_trace(TRACE_ERROR, "hci_compute_filter : invalid filter.\n");
		return;
	}
	
	hci_filter_clear(flt); 
	hci_filter_set_ptype(HCI_EVENT_PKT, flt); 
	// HCI_EVENT_PKT = HCI Event Packet.
	
	va_start(pa, flt);
	event = va_arg(pa, uint32_t);
	while (event != 0) {
		hci_filter_set_event(event, flt);
		event = va_arg(pa, uint32_t);
	}
	va_end(pa);
}

//------------------------------------------------------------------------------------

void hci_display_LE_supported_states(uint64_t states) { // p1096
	fprintf(stdout, "Device supported states : 0x%016llX\n", (long long unsigned int)states);
	uint8_t offset = 0;
	while (offset < 29) {
		if (states & states_map[offset].mask) {
			fprintf(stdout, " -> %s supported. \n", states_map[offset].description);
		} else {
			fprintf(stdout, " -> %s UNsupported. \n", states_map[offset].description);
		}
				
		offset++;
	}
}
