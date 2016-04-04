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
 * @file hci_controller.h
 * @brief Module bluez_tools.hci.hci_controller bringing an "upper-layer" to the BlueZ's HCI
 * (Host Controller Interface) functions
 *
 * The goal of this module is to provide more reliable and adaptative functions. 
 * All the following functions are using an "hci_controller" structure in order to
 * abstract entirely the BlueZ methods.
 * 
 * In all that follows, we call "adapter" the device on which we want to perform HCI control.
 * It could be for instance a Bluetooth dongle.
 * 
 * @author Thomas Bertauld
 * @date 03/03/2016
 */

#ifndef __HCI_CONTROLLER_H__
#define __HCI_CONTROLLER_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include "hci_socket.h"
#include "hci_utils.h"
#include "bt_device.h"
#include "list.h"

/**
 * Default timeout used to communicate with the adapter using HCI.
*/
#define HCI_CONTROLLER_DEFAULT_TIMEOUT 3000

/**
 * @brief Possible states of the hci_controller.
 * The hci_controller is a state machine.
 * That is to say that a virtual state is associated 
 * to each real used state of the adapter. 
 * For instance, when an adapter is scanning devices, the state
 * machine will be in the {@code HCI_STATE_SCANNING} state.
 * If you want to add more states, please follow the naming convention "HCI_STATE_..."
 * for convenience.
 * The default state for a controller (idle) is {@code HCI_STATE_OPEN}.
*/
typedef enum {
	HCI_STATE_CLOSED = 0,
	HCI_STATE_OPEN = 1, // Default state (the adapter doesn't do anything).
	HCI_STATE_SCANNING = 2,
	HCI_STATE_ADVERTISING = 3,
	HCI_STATE_READING = 4,
	HCI_STATE_WRITING = 5} hci_state_t;

/**
 * hci_controller structure : 
*/
typedef struct hci_controller_t {
	/**
	 * bt_device corresponding to the physical adapter.
	 */
	bt_device_t device;
	/**
	 * List of opened sockets on this adapter.
	 */
	list_t *sockets_list;
	/**
	 * Current state of the controller.
	 */
	hci_state_t state;
	/**
	 * Error indicator : if an error occured during a request,
	 * the adapter could be stuck in a bad state. By putting
	 * this indicator to true (1), we can see that an error
	 * occured and then try to solve it (i.e try to
	 * put the controller in the default state) by using
	 * the {@code hci_resolve_interruption} function to be able to use
	 * it properly again.
	 * 
	 * @see hci_resolve_interruption
	 */
	char interrupted;  
} hci_controller_t;
	
/** 
 * @brief Creates a new hci_controller instance using  the adapter 
 * given by the {@code mac} reference.
 * If this reference is NULL, we take the first available adapter in
 * the system.
 * The field {@code name} is used to describe in an user-friendly way the adapter.
 * If NULL then the name of the adapter will be "UNKNOWN".
 * @param mac address of the adapter to use.
 * @param name user-friendly name used to describe the adapter.
 * @return the newly created instance of the adapter's hci_controller.
*/
extern hci_controller_t hci_open_controller(bt_address_t *mac, char *name);

/**
 * @brief Tries to resolve a past interruption of a controller in order to put it
 * in the default state to be able to use it properly again.
 * The field {@code hci_socket} must refers to either NULL, so a new socket is opened
 * on the given controller, or a valid opened socket on the given controller.
 * The field {@code hci_controller} has to be a valid reference on an opened 
 * hci_controller.
 * @param hci_socket a reference on either a valid opened socket on the controller
 * or NULL. In the later case, a new one is opened.
 * @param hci_controller a valid reference on a hci_controller.
 * @return 0 if the interruption has been resolved, a value < 0 otherwise.
*/
extern int8_t hci_resolve_interruption(hci_socket_t *hci_socket, hci_controller_t *hci_controller);

/**
 * @brief Closes and destroys an hci_controller instance. 
 * More precisely, it closes all the opened sockets on 
 * this controller and frees all the used memory.
 * The {@code hci_controller} reference has to be valid.
 * @param hci_controller reference on the controller to finalize.
 * @return 0 on success, < 0 otherwise.
*/
extern int8_t hci_close_controller(hci_controller_t *hci_controller);

/**
 * @brief Opens a new socket on the given hci_controller.
 * The {@code hci_controller} reference has to be valid.
 * WARNING : only this function should be used to open a socket on an hci_controller
 * for the socket to properly be added to the sockets list of the controller.
 * Indeed, if you use the classic {@code open_hci_socket} function, the socket will not be
 * added to the controller and it could result to an UNSTABLE system state when using
 * a socket non-created with this function.
 * 
 * @see open_hci_socket
 * 
 * @param hci_controller reference on the controller on which the socket
 * is to be opened.
 * @return the newly created instance of the socket. If an error occured during
 * the creation of the socket, its {@code sock} field is set to -1 and the socket is
 * not added to the sockets list on the controller.
*/
extern hci_socket_t hci_open_socket_controller(hci_controller_t *hci_controller);

/**
 * @brief Closes one of the opened sockets of an hci_controller.
 * The {@code hci_controller} reference has to be valid (the referenced controller
 * should be opened and initialized).
 * The {@code hci_socket} field has to refer to an opened socket on the given controller.
 * WARNING : only this function should be used to close a socket on an hci_controller
 * for the socket to properly be removed from the sockets list of the controller.
 * Using the classic {@code close_hci_socket} function will result in the socket still
 * being referenced in the list and could be used (while closed) in another context
 * thus leading to errors.
 * 
 * @see close_hci_socket
 * 
 * @param hci_controller controller on which to close the socket.
 * @param hci_socket socket to close.
 * @return 0 on success, < 0 otherwise.
*/
extern int8_t hci_close_socket_controller(hci_controller_t *hci_controller, hci_socket_t *hci_socket);

/**
 * @brief Performs a basic Bluetooth scan to recognize nearby devices.
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * @param hci_socket socket to be used to perform the scan.
 * @param hci_controller controller emitting the scan.
 * @param duration the scan will last at most {@code duration}*1,28s.
 * @param max_rsp max number of devices to be scanned.
 * @param flags inquiry flags.
*/
extern bt_device_table_t hci_scan_devices(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
					  uint8_t duration, uint16_t max_rsp, long flags);

/**
 * @brief Get the name of a remote device.
 * Send an inquiry to the remote device to get its name.
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * @param hci_socket socket to be used to perform the inquiry.
 * @param hci_controller controller emitting the inquiry.
 * @param bt_device device from which the name is to be asked.
 * @return 0 on success, in which case the name of the device
 * is successfully stored under its {@code real_name} field, < 0
 * if an error occured, in which case its {@code real_name} becomes
 * {@code [UNKNOWN}}.
*/
extern int8_t hci_compute_device_name(hci_socket_t *hci_socket, hci_controller_t *hci_controller, bt_device_t *bt_device);

/**
 * @brief Performs a RSSI measurement on a remote device.
 * This function is in charge of sending RSSI inquiries and
 * retrieving them to/from the remote device. The computed results
 * can be stored inside a file if needed. Even though the filters
 * apply to the socket are changed during the process, they are 
 * reset to their initial values in the end.
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * @param hci_socket socket to be used to send and retrieve RSSI inquiries.
 * @param hci_controller local controller.
 * @param file_descriptor (optional) if set, the RSSI values will be written in the
 * corresponding file.
 * @param mac address of the device from which we want the RSSI values.
 * @param duration duration of the RSSI scan.
 * @param max_rsp maximum RSSI values to receive.
 * @return the computed RSSI values stored in a string.
 */
extern char *hci_get_RSSI(hci_socket_t *hci_socket, hci_controller_t *hci_controller, int8_t *file_descriptor,
			 bt_address_t *mac, uint8_t duration, uint16_t max_rsp);	

/**
 * @brief Performs a RSSI measurement on a remote device (LE version).
 * This function is in charge of sending RSSI inquiries and
 * retrieving them to/from the remote device. The computed results
 * can be stored inside a file if needed. Even though the filters
 * apply to the socket are changed during the process, they are 
 * reset to their initial values in the end.
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * For a detailed description of the last five parameters, please refer to the official
 * Bluetooth documentation and the provided examples.
 * @param hci_socket socket to be used to send and retrieve RSSI inquiries.
 * @param hci_controller local controller.
 * @param file_descriptor (optional) if set, the RSSI values will be written in the
 * corresponding file.
 * @param mac address of the device from which we want the RSSI values.
 * @param duration duration of the RSSI scan.
 * @param max_rsp maximum RSSI values to receive.
 * @param scan_type @see hci_le_set_scan_parameters
 * @param scan_interval @see hci_le_set_scan_parameters
 * @param scan_window @see hci_le_set_scan_parameters
 * @param own_add_type @see hci_le_set_scan_parameters
 * @param scan_filter_policy @see hci_le_set_scan_parameters
 * @return the computed RSSI values stored in a string.
 */
extern char *hci_LE_get_RSSI(hci_socket_t *hci_socket, hci_controller_t *hci_controller, int8_t *file_descriptor,
			    bt_address_t *mac, uint16_t max_rsp, uint8_t scan_type, uint16_t scan_interval,
			    uint16_t scan_window, uint8_t own_add_type, uint8_t scan_filter_policy);


/**
 * @brief Clears the white list of a Bluetooth adapter. 
 * The white list is the list of all devices from which the adapter can
 * take answers while performing LE inquiries.
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * @param hci_socket socket used to access the controller's list.
 * @param hci_controller controller from which the list is to be cleared.
 * @return 0 upon success, <0 otherwise.
*/
extern int8_t hci_LE_clear_white_list(hci_socket_t *hci_socket, hci_controller_t *hci_controller);

/**
 * @brief Adds a device to the white list of a Bluetooth adapter. 
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * @param hci_socket socket used to access the controller's list.
 * @param hci_controller controller from which the list is to be modified.
 * @param bt_device device to add.
 * @return 0 upon success, <0 otherwise.
*/
extern int8_t hci_LE_add_white_list(hci_socket_t *hci_socket, hci_controller_t *hci_controller, const bt_device_t bt_device);

/**
 * @brief Removes a device to the white list of a Bluetooth adapter. 
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * @param hci_socket socket used to access the controller's list.
 * @param hci_controller controller from which the list is to be modified.
 * @param bt_device device to remove.
 * @return 0 upon success, <0 otherwise.
 */
extern int8_t hci_LE_rm_white_list(hci_socket_t *hci_socket, hci_controller_t *hci_controller, const bt_device_t bt_device);

/**
 * @brief Reads the size of the white list of a Bluetooth adapter. 
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@code hci_controller} field has to refer to a valid opened hci_controller.
 * @param hci_socket socket used to access the controller's list.
 * @param hci_controller controller from which the list is to be accessed.
 * @param size reference on the size of the list.
 * @return 0 upon success, ensuring that the {@code size} size parameter now
 * contains the size of the white list, <0 otherwise.
 */
extern int8_t hci_LE_get_white_list_size(hci_socket_t *hci_socket, hci_controller_t *hci_controller, uint8_t *size);

/**
  * @brief Lists the supported LE features of a Bluetooth adapter. 
  * Since the official Bluetooth core specification doesn't give any
  * information on this feature, expect that most of the values returned are RFU,
  * this function has no warranty to work well.
  * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
  * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
  * The {@hci_controller} field has to refer to a valid opened hci_controller.
  * @param hci_socket socket used to access the controller's list.
  * @param hci_controller controller from which the list is to be accessed.
  * @param features binary mask used to store the list of supported features.
  * @return 0 upon success, ensuring that the {@code features} parameter now
  * contains the list of all supported LE features, <0 otherwise.
*/
extern int8_t hci_LE_read_local_supported_features(hci_socket_t *hci_socket, hci_controller_t *hci_controller, uint8_t *features);

/**
 * @brief Reads the supported (real) states of a Bluetooth adapter.
 * The {@code hci_socket} field can either be a valid opened socket on a valid Bluetooth adapter
 * or NULL, in which case a new socket is opened on the given {@code hci_controller}.
 * The {@hci_controller} field has to refer to a valid opened hci_controller.
 * On success, the function should return a non-negative value and the variable pointed by {@code states}
 * should have been set to an unsigned int representing a binary filter of the available states.
 * For an user-friendly view, the states contained in this binary filter can be displayed 
 * on the standard output by using the function {@code hci_display_LE_states} from the
 * {@code hci_utils} module.
 * @see hci_display_LE_states
 * @param hci_socket socket used to access the controller's list.
 * @param hci_controller controller from which the list is to be accessed.
 * @param states binary mask used to store the list of supported states.
 * @return 0 upon success, ensuring that the {@code states} parameter now
 * contains the list of all supported LE states, <0 otherwise.
*/
extern int8_t hci_LE_read_supported_states(hci_socket_t *hci_socket, hci_controller_t *hci_controller, uint64_t *states);

#endif // __HCI_CONTROLLER_H__
