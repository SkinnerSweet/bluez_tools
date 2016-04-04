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

#include "bt_device.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cfuhash.h"

/**
 * Hash table used to store the couples (@, bt_device)
 */
static cfuhash_table_t *bt_devices_table = NULL;

//------------------------------------------------------------------------------------

char bt_compare_addresses(const bt_address_t *a1, const bt_address_t *a2) {
	return (memcmp((const void *)a1, (const void *)a2, sizeof(bt_address_t)) == 0);
}

//------------------------------------------------------------------------------------

char bt_already_registered_device(bt_address_t add) {
	if (!bt_devices_table) {
		bt_devices_table = cfuhash_new_with_initial_size(200);
	}
	char string_add[18]; 
	memset(string_add, 0, 18);
	ba2str((const bt_address_t *)&(add), string_add);

	return (char)(cfuhash_exists(bt_devices_table, string_add));
}

//------------------------------------------------------------------------------------

bt_device_t *bt_register_device(bt_device_t bt_device) {
	if (!bt_devices_table) {
		bt_devices_table = cfuhash_new_with_initial_size(200);
	}
	char string_add[18]; 
	memset(string_add, 0, 18);
	ba2str((const bt_address_t *)&(bt_device.mac), string_add);

	bt_device_t *tmp = malloc(sizeof(bt_device_t));
	memcpy(tmp, &bt_device, sizeof(bt_device_t));

	return (bt_device_t *)cfuhash_put(bt_devices_table, string_add, (void *)tmp);
} 

//------------------------------------------------------------------------------------

bt_device_t bt_get_device(bt_address_t add) {
	bt_device_t *tmp;

	if (!bt_devices_table) {
		bt_devices_table = cfuhash_new_with_initial_size(200);
	}

	char string_add[18]; 
	memset(string_add, 0, 18);
	ba2str((const bt_address_t *)&(add), string_add);

	tmp = (bt_device_t *)cfuhash_get(bt_devices_table, string_add);

	return *tmp;
}

//------------------------------------------------------------------------------------

void bt_destroy_device_table(void) {
	if (bt_devices_table) {
		cfuhash_destroy_with_free_fn(bt_devices_table, free);
	}
	bt_devices_table = NULL;
}

//------------------------------------------------------------------------------------

bt_device_t bt_device_create(bt_address_t mac, bt_address_type_t add_type, 
			     const char *real_name, const char *custom_name) {
	bt_device_t res = {0};
	res.mac = mac;
	res.add_type = add_type;
	if (real_name) {
		strncpy(res.real_name, real_name, BT_NAME_LENGTH);
	} else {
		strncpy(res.real_name, "UNKNOWN", BT_NAME_LENGTH);
	}
	if (custom_name) {
		strncpy(res.custom_name, custom_name, BT_NAME_LENGTH);
	} else {
		strncpy(res.custom_name, "UNKNOWN", BT_NAME_LENGTH);
	}

	bt_register_device(res);

	return res;
}		

//------------------------------------------------------------------------------------

void bt_device_display(bt_device_t device) {
	
	char tmp[18]; 
	memset(tmp, 0, 18);
	ba2str((const bt_address_t *)&(device.mac), tmp);

	char address_type_mess[6] = {0};
	switch(device.add_type) {
	case PUBLIC_DEVICE_ADDRESS: 
		strcpy(address_type_mess, "[PDA]");
		break;
	case RANDOM_DEVICE_ADDRESS: 
		strcpy(address_type_mess, "[RDA]");
		break;
	default:
		strcpy(address_type_mess, "[???]");
		break;
	}

	fprintf(stdout, "%s [%s] : %s a.k.a %s\n", 
		address_type_mess, 
		tmp,
		device.real_name,
		device.custom_name);
}

//------------------------------------------------------------------------------------

void bt_device_table_display(bt_device_table_t device_table) {
	for (uint32_t i = 0; i < device_table.length; i++) {
		bt_device_display(device_table.device[i]);
	}
}
