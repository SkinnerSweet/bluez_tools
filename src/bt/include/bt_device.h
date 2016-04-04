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

/**
 * @file bt_device.h
 * @brief Module bluez_tools.bt.bt_device defining the basics of manipulating BT devices.
 *
 * This header defines the functions structures and functions used to manage
 * BT devices. Every BT device used inside the application should be registered
 * with this manager. The module keeps track of registered BT devices using
 * a hash table provided by the CFU module.
 *
 * @author Thomas Bertauld
 * @date 03/03/2016
 */

#ifndef __BT_DEVICE_H__
#define __BT_DEVICE_H__

#include <stdint.h>
#include <bluetooth/bluetooth.h>

/** Max length of a stroed name */
#define BT_NAME_LENGTH 50 

/**
 * Wrapper on the bt_address_t type
 */
typedef bdaddr_t bt_address_t;

/**
 * Addresses types
 */
typedef enum {
	PUBLIC_DEVICE_ADDRESS = 0x00,
	RANDOM_DEVICE_ADDRESS = 0x01,
	UNKNOWN_ADDRESS_TYPE = 0x12
} bt_address_type_t;

/* --------------
   - STRUCTURES -
   --------------
*/

/** Bluetooth Device structure : */
typedef struct {
	/** Mac (Public or not) address of the device.*/
	bt_address_t mac;  
	/** Address type. The following value are allowed :
	 * - 0x00 : Public Device Address (PDA).
	 * - 0x01 : Random Device Address (RDA).
	 * - 0x12 : Unknown address type (personnal code).
	 */
	bt_address_type_t add_type; 
	/** Real "constructor" name of the device.*/
	char real_name[BT_NAME_LENGTH]; 
	/** User-friendly name of the device.*/
	char custom_name[BT_NAME_LENGTH]; 
} bt_device_t;

/** 
 * Bluetooth devices table structure :
 */
typedef struct {
	/** Table of registred devices.*/
	bt_device_t *device; 
	uint16_t length;
} bt_device_table_t;

//------------------------------------------------------------------------------------

/* --------------
   - PROTOTYPES -
   --------------
*/

/** 
 * @brief Allows the comparison of two bt mac addresses.
 * @param a1 first address to compare.
 * @param a2 second address to compare.
 * @return 1 if the the two addresses are the same. Else returns 0.
 */
extern char bt_compare_addresses(const bt_address_t* a1, const bt_address_t *a2);


/**
 * @brief Allows to know if already encoutered (and stored the information about)
 * a bt device.
 * Please note that a hash table is used to store the bt_device's information.
 * @param add the address of the bt device to check.
 * @return 1 if the device was already registered. Else returns 0.
 */
extern char bt_already_registered_device(bt_address_t add);

/**
 * @brief Stores a device in the main data structure (currently an hash table).
 * We store the devices by using a couple (@, bt_device).
 * WARNING : there is no possible double-entries
 * (corresponding to two same @) and so, trying to add a device having the
 * same mac @ that another previously stored device will erase that device.
 * @param bt_device the device to register.
 * @return the previously stored device corresponding to the @ associated
 * to the parameter {@code bt_device} if any. Returns NULL otherwise.
 */
extern bt_device_t *bt_register_device(bt_device_t bt_device);

/**
 * @brief Returns the stored device corresponding to the given address.
 * @param add address of the device to retrieve.
 * @return the device corresponding to the given address if any. Returns
 * NULL otherwise.
 */
extern bt_device_t bt_get_device(bt_address_t add);

/**
 * @brief Function used to destroy and free the structure containing the
 * (@, bt_device) couples.
 * Since the structure will be (re)created by using any function accessing
 * said structure, this function could be used to "reset" the table.
 */
extern void bt_destroy_device_table(void);

/**
 * @brief Creates a new "bt_device" structure filled with the given parameters.
 * If one (or both) of the {@code name} parameters is NULL, the function 
 * will automatically fill the corresponding field with the name "UNKNOWN".
 * @param mac address of the device.
 * @param add_type type of the device's address.
 * @param real_name real (constructor given) name of the device.
 * @param custom_name custom user-friendly name.
 * @returns the created device structure.
 */
extern bt_device_t bt_device_create(bt_address_t mac, bt_address_type_t add_type, 
				    const char *real_name, const char *custom_name);

/**
 * @brief Displays the information of a device on the standard output.
 * @param device device to display.
 */
extern void bt_device_display(bt_device_t device);

/**
 * @brief Displays the information of all the devices contained in
 * the given table on the standard output.
 * @param device_table table to display.
 */
extern void bt_device_table_display(bt_device_table_t device_table);

#endif // __BT_DEVICE_H__