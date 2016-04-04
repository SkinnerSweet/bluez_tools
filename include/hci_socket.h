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

#ifndef __HCI_SOCKET_H__
#define __HCI_SOCKET_H__

/**
 * @file hci_socket.h
 * @brief Module bluez_tools.hci.hci_socket bringing functions to deal with 
 * hci_sockets. Those sockets are "intern" sockets and are only used to communicate
 * with a LOCAL bt adapter (typically a bt dongle).
 * 
 * @author Thomas Bertauld
 * @date 03/03/2016
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <stdint.h>
#include "list.h"
#include "bt_device.h"

/* --------------
   - STRUCTURES -
   --------------
*/

// TODO : cf ioctl FIONBIO : donner possibilité d'avoir dd non bloquant ?
// => On pourrait par exemple lancer un scan en asynchrone, et, tout en maintenant
// un groupe de n données dans un buffer, récupérer n données quand bon nous semble sans jamais
// avoir à relancer de scan ?

/**
 * HCI socket strucutre.
 */
typedef struct hci_socket_t {
	/**
	 * Socket id.
	 * -1 indicates that an error occured.
	 */
	int8_t sock;
	/**
	 * Bluetooth controller id.
	 * NOTE : we can retrieve the bt@ of the controller with the 
	 * "hci_devba(int dev_id, bt_address_t *bdaddr)" function
	 * */
	int8_t dev_id;
} hci_socket_t;

//------------------------------------------------------------------------------------

/* --------------
   - PROTOTYPES -
   --------------
*/

/**
 * @brief Opens an hci_socket on the given controller.
 * If the controller address is NULL, the first available controller on the system
 * is taken.
 On success, the returned socket must have its both fields ("sock"
 and "dev_id") non-negative.
 * @param controller address of the controller on which the socket is to
 * be opened.
 * @return the newly created socket. Upon succes, both its fiels should be non-negative.
 */
extern hci_socket_t open_hci_socket(bt_address_t *controller);

/**
 * @brief Closes a previsouly opened hci_socket. 
 * If the given socket's reference is invalid or if the socket 
 * is already closed, this function should print a warning 
 * message on the standard error output.
 * @param hci_socket socket to close.
*/
extern void close_hci_socket(hci_socket_t *hci_socket);

/**
 * @deprecated
 * @brief Closes all the hci_sockets stored in a list.
 * Because this function calls "close_hci_socket", if one of the references
 * stored in the list is invalid or if one of the contained sockets was already
 * closed, a warning message should be displayed on the standard error output.
 * @param hci_socket_list reference on a sockets list.
*/
extern void close_all_hci_sockets(list_t **hci_socket_list);

/**
 * @brief Retrieves the current socket filter applied to the given hci_socket.
 * Upon success the filter referenced by the {@code old_flt} parameter is filled
 * with the current filter applied to the given socket and the function returns 0.
 * If an error occured, the filter has not been retrieved and -1 is returned.
 * @param hci_socket the socket from which the filter is to be retrieved.
 * @param old_flt reference on the retrieved filter.
 * @return 0 upon success.
*/
extern int8_t get_hci_socket_filter(hci_socket_t hci_socket, struct hci_filter *old_flt);

/**
 * @brief Sets the filter of the given hci_socket by using the given filter's reference.
 * The reference has to be a valid one. 
 * @param hci_socket socket on which the filter is to be set.
 * @param flt filter to apply.
 * @return upon success, the filter has been set on the socket and the function returns 0.
 * If an error occured, the filter has bot been set and -1 is returned.
*/
extern int8_t set_hci_socket_filter(hci_socket_t hci_socket, struct hci_filter *flt);

/**
 * @brief Displays all hci_sockets stored in an hci_socket
 * list on the standard output.
 * The list's reference has to be a valid one.
 * @param hci_socket_list list to display.
*/
extern void display_hci_socket_list(list_t *hci_socket_list);

#endif // __HCI_SOCKET_H__
