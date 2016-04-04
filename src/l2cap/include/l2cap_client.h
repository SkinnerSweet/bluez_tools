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
 * @file l2cap_client.h
 * @brief Module bluez_tools.l2cap.l2cap_client giving the basis to create
 * a generic client using the L2CAP communication protocole.
 * 
 * @author Thomas Bertauld
 * @date 03/03/2016
 */

#ifndef __L2CAP_CLIENT_H__
#define __L2CAP_CLIENT_H__

#include "l2cap_socket.h"
#include <bluetooth/bluetooth.h>
#include <stdint.h>
#include "bt_device.h"

/**
 * Structure of a L2CAP client.
 */
typedef struct l2cap_client_t {
	/**
	 * Buffer used to receive L2CAP packets.
	 */
	char *buffer;
	/**
	 * Length of receiving buffer.
	 */
	uint16_t buffer_length;
	/**
	 * Socket used by the client to connect to a
	 * L2CAP server.
	 */
	l2cap_socket_t l2cap_socket;
	/**
	 * Status of the client (connected or not to a server).
	 */
	char connected;
	/**
	 * @brief Function used when a packet is received.
	 * @param client client on which to treat the buffer.
	 */
	void (*treat_buffer)(struct l2cap_client_t client);
	/**
	 * @brief Function used to send a request to a server.
	 * @param client client which desires to send a request.
	 * @param req_type this value can be used to discriminate which
	 * message to send to the server. The implementation of the communication
	 * protocole (i.e of the requests/answers mechanism) is up to
	 * the user of the client/server combo. See given example for more
	 * details.
	 */
	void (*send_request)(struct l2cap_client_t client, uint8_t req_type);
} l2cap_client_t;

/**
 * @brief Creates a new L2CAP client.
 * @param client reference on the client to initialize.
 * @param server_add address of the server on which the client is to be
 * later connected.
 * @param port server's port on which to connect later.
 * @param buffer_length desired length for the receiving buffer.
 * This buffer will handle the server's answers.
 * @param treat_buffer_func function to use when a packet is received
 * by the client. If NULL, a default function, simply displaying
 * the received packet on the standard output, will be used.
 * @param send_request_func function to use to send request to the
 * server. If NULL, a default function, simply sending the string
 * "Request echo.", will be used.
 * @return 0 on success, a value < 0 otherwise.
 */
extern int8_t l2cap_client_create(l2cap_client_t *client, bt_address_t *server_add, uint16_t port, 
			   uint16_t buffer_length, 
			   void (*treat_buffer_func)(l2cap_client_t client),
			   void (*send_request_func)(l2cap_client_t client, uint8_t req_type));

/**
 * @brief Connects a previously initialized client to its server.
 * @param client to connect.
 * @return 0 on success, a value < 0 otherwise. Note that in case of success,
 * the field client.connected will also be set to 1.
 */
extern int8_t l2cap_client_connect(l2cap_client_t *client);

/**
 * @brief Sends a request to the server.
 * The client must have been previously successfully connected
 * to said server.
 * Once the request has been sent, the timeout value is used to 
 * wait for the server to answer the request. If the answer arrives
 * before the timeout is reached, the client then calls its {@code treat_buffer_func}
 * function.
 * @param client client willing to send a request.
 * @param timeout timeout for the server's answer to arrive.
 * @param req_type type of the request to send.
 * @return 0 in case of success, a value < 0 otherwise.
 */
extern int8_t l2cap_client_send(l2cap_client_t *client, int16_t timeout, uint8_t req_type);

/**
 * @brief Destroys a client.
 * WARNING: this function DOES NOT terminate the connection with
 * the server, it only frees the memory taken by the client and
 * closes its socket. It may be something to work on ?
 * Moreover this function does not perform any kind of checking on the
 * given client. Use with caution.
 * @param client the client to destroy.
 */
extern void l2cap_client_close(l2cap_client_t *client);

#endif // __L2CAP_CLIENT_H__
