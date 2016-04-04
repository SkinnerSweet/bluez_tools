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

#ifndef __L2CAP_SOCKET_H__
#define __L2CAP_SOCKET_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <stdint.h>
#include "list.h"
#include "bt_device.h"

/**
 * @file l2cap_server.h
 * @brief Module bluez_tools.l2cap.l2cap_socket giving the basis to manage L2CAP sockets.
 * 
 * @author Thomas Bertauld
 * @date 03/03/2016
 */

/* --------------
   - STRUCTURES -
   --------------
*/

typedef struct sockaddr_l2 l2cap_sockaddr_t; 

/**
 * L2CAP socket strucutre.
 */
typedef struct l2cap_socket_t {
	/**
	 * Socket id.
	 * -1 indicates that an error occured during initialization.
	 */
	int8_t sock; 
	/**
	 * Internal socket structure.
	 */
	l2cap_sockaddr_t sockaddr;
} l2cap_socket_t;

/**
 * @brief Creates a new L2CAP socket.
 * @param adapter address of the adapter on which the socket
 * is to be opened.
 * @param port port on which the connections will be allowed.
 * @param to_bind indicates whether or not this socket should
 * be bind directly upon creation.
 * WARNING : only this function should be called to create new
 * L2CAP sockets to ensure the consistency of the (hidden) sockets list
 * maintained by the application.
 * @return the created socket. If an error occured, the {@sock}
 * field of the structure will be set to -1.
 */
extern l2cap_socket_t open_l2cap_socket(bt_address_t *adaptater, uint16_t port, char to_bind);

/**
 * @brief Closes a previously created socket.
 * @param l2cap_socket the socket to be closed.
 * WARNING : only this function should be called to close previously created
 * L2CAP sockets to ensure the consistency of the (hidden) sockets list
 * maintained by the application.
 */
extern void close_l2cap_socket(l2cap_socket_t *l2cap_socket);

/**
 * @brief Returns the list of all opened L2CAP sockets
 * managed by the application.
 */
extern list_t *get_l2cap_socket_list(void);

/**
 * @brief Closes all opened L2CAP sockets
 * managed by the application.
 */
extern void close_all_l2cap_sockets(void);

/**
 * @brief Displays on the standard output a list of all opened L2CAP sockets
 * managed by the application.
 */
extern void display_l2cap_socket_list(void);

#endif // __L2CAP_SOCKET_H__
