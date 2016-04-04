/* The MIT License (MIT)
 * Copyright (c) 2016 Thomas Bertauld <thomas.bertauld@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file l2cap_server.h
 * @brief Module bluez_tools.l2cap.l2cap_server giving the basis to create
 * a generic server using the L2CAP communication protocole.
 * 
 * @author Thomas Bertauld
 * @date 03/03/2016
 */


#ifndef __L2CAP_SERVER_H__
#define __L2CAP_SERVER_H__

#include "l2cap_socket.h"
#include "bt_device.h"
#include <bluetooth/bluetooth.h>
#include <stdint.h>

/**
 * Generic request to receive from a client to indicate
 * the end of the communication.
 */
#define L2CAP_SERVER_UNIVERSAL_STOP "STOP"

/* NOTE : The L2CAP dynamic ports go from 4097 (0x1001) to 32765,
 * but only work with even numbers!
 */

/**
 * Structure of a client as seen by the server.
 */
typedef struct l2cap_client_proxy_t {
	/**
	 * Id of the established connection between a server and this client.
	 */
	int8_t conn_id;
	/**
	 * Client's device address.
	 */
	l2cap_sockaddr_t rem_addr;
	/**
	 * Buffer associated with the connection.
	 */
	char *buffer; 
} l2cap_client_proxy_t;

/**
 * Structure of a L2CAP server.
 */
typedef struct l2cap_server_t {
	/**
	 * Indicates if the server has at least one live connection.
	 */
	char launched; 
	/**
	 * Socket on which the clients can connect.
	 */
	l2cap_socket_t socket; 
	/**
	 * Length of each client's buffer.
	 * 
	 * @see l2cap_client_proxy_t
	 */
	uint16_t buffer_length; 
	/** 
	 * Maximum number of clients simultaneously treatable.
	 */
	uint8_t max_clients; 
	/**
	 * List of the connected clients.
	 */
	l2cap_client_proxy_t *clients;
	/**
	 * @brief Function used when a request is received on a
	 * client's buffer.
	 * @param server server on which to treat the request.
	 * @param client_id client to treat.
	 */
	void (*treat_buffer)(struct l2cap_server_t *server, uint8_t client_id);
	/**
	 * @brief Function used to send an answer to a client.
	 * @param server server desiring to answer.
	 * @param client_id client to answer to.
	 * @param res_type this value can be used to discriminate which
	 * message to send to the client. The implementation of the communication
	 * protocole (i.e of the requests/answers mechanism) is up to
	 * the user of the client/server combo. See given example for more
	 * details.
	 */
	void (*send_response)(struct l2cap_server_t *server, uint8_t client_id, uint8_t res_type);
} l2cap_server_t;

/**
 * @brief Creates a new L2CAP server.
 * @param server reference on the server to initialize.
 * @param adapter address of the adapter the server will use to 
 * communicate.
 * @param port port on which the connections will be allowed.
 * @param max_clients number of clients to be treated simultaneously.
 * @param buffer_length desired length for the receiving buffer on each clients.
 * Those buffers will handle the clients requests. 
 * @param treat_buffer_func function to use when a request is received
 * from a client. If NULL, a default function, simply displaying
 * the received packet on the standard output, will be used.
 * @param send_response_func function to use to send an answer to a
 * client. If NULL, a default function, simply sending the string
 * "Response echo.", will be used.
 * @return 0 on success, a value < 0 otherwise.
 */
extern int8_t l2cap_server_create(l2cap_server_t *server, bt_address_t *adapter, uint16_t port, 
				  uint8_t max_clients, uint16_t buffer_length, 
				  void (*treat_buffer_func)(l2cap_server_t *server, uint8_t num_client),
				  void (*send_response_func)(l2cap_server_t *server, uint8_t num_client, uint8_t res_type)); 

/**
 * @brief Launches a server.
 * Launches a server by making it listen on its socket.
 * WARNING: for now, for each connection accepted, a NEW
 * thread is created to handle the client. If this is not the
 * behavior you expect, please modify the implementation.
 * @param server server to launch.
 * @param timeout timeout for a client's request to arrive.
 * @param max_req maximum number of requests to handle for each client.
 * A value of -1 indicates that no limit will be set.
 * @return 0 on success, a value < 0 otherwise.
 */
extern int8_t l2cap_server_launch(l2cap_server_t *server, int16_t timeout, uint16_t max_req);

/**
 * @brief Destroys a server.
 * WARNING: this function DOES NOT terminate the connections with
 * the clients, it only frees the memory taken by the server and
 * closes its socket. It may be something to work on ?
 * Moreover this function does not perform any kind of checking on the
 * given server. Use with caution.
 * @param server the server to destroy.
 */
extern void l2cap_server_close(l2cap_server_t * server);

// TODO Create function to close specific clients?

#endif // __L2CAP_SERVER_H__ 
