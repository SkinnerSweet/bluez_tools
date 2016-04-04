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

#include "l2cap_server.h"
#include "trace.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/errno.h>
#include <bluetooth/l2cap.h>
#include <sys/unistd.h>
#include <pthread.h>

// TODO : send message to client to explain reasons of connection interruption (timeout expired...)
// FIXME : a different thread is created for each accepted client...

struct routine_data_t {
	int16_t timeout;
	uint8_t num_client;
	uint16_t max_req;
	l2cap_server_t *server; 
};
	
//------------------------------------------------------------------------------------

static void treat_buffer_default_func(l2cap_server_t *server, uint8_t num_client) {
	fprintf(stderr, "Buffer trace : %s\n", server->clients[num_client].buffer);
	return;
}

static void send_response_default_func(l2cap_server_t *server, uint8_t num_client, uint8_t res_type) {
	char response[] = "Response echo.";
	if (write(server->clients[num_client].conn_id, response, 15) < 15) {
		print_trace(TRACE_ERROR, "Unable to write response.\n");
	}
	return;
}

//------------------------------------------------------------------------------------

int8_t l2cap_server_create(l2cap_server_t *server, bt_address_t *adapter, uint16_t port, 
			   uint8_t max_clients, uint16_t buffer_length, 
			   void (*treat_buffer_func)(l2cap_server_t *server, uint8_t num_client),
			   void (*send_response_func)(l2cap_server_t *server, uint8_t num_client, uint8_t res_type)) {	

	if (!server) {
		print_trace(TRACE_ERROR, "l2cap_server_create : invalid server reference.\n");
		return -1;
	}

	if (!adapter) {
		print_trace(TRACE_ERROR, "l2cap_server_create : invalid BT adapter reference.\n");
		return -1;
	}

	if (!buffer_length) {
		print_trace(TRACE_WARNING, "l2cap_server_create : null buffer, creation aborted.\n");
		return -1;
	}

	server->launched = 0;
	server->socket = open_l2cap_socket(adapter, port, 1);
	if (server->socket.sock < 0) {
		print_trace(TRACE_ERROR, "Unable to create listening socket on server.\n");
		return -1;
	}
	server->buffer_length = buffer_length;
	server->max_clients = max_clients;
	server->clients = calloc(max_clients, sizeof(l2cap_client_proxy_t));
	for (uint8_t i = 0; i < max_clients; i++) {
		server->clients[i].buffer = calloc(buffer_length, sizeof(char));
	}
	
	if (!treat_buffer_func) {
		server->treat_buffer = &(treat_buffer_default_func);
	} else {
		server->treat_buffer = treat_buffer_func;
	}
	if (!send_response_func) {
		server->send_response = &(send_response_default_func);
	} else {
		server->send_response = send_response_func;
	}

	return 0;
}

//------------------------------------------------------------------------------------

static void *server_thread_routine(void *data) {
	struct routine_data_t *routine_data = (struct routine_data_t *)data;

	uint8_t i = routine_data->num_client;
	l2cap_server_t *server = routine_data->server;
	l2cap_client_proxy_t my_client = server->clients[i];
	int16_t timeout = routine_data->timeout;
	uint16_t max_req = routine_data->max_req;
	char *buffer = server->clients[i].buffer;

	char thread_launched = 1;
	uint16_t num_req = 0;

	while (thread_launched && ((num_req < max_req) || max_req == -1)) {
		memset(buffer, 0, server->buffer_length);
		int8_t bytes_read = 0;
		int8_t n = 0;
		struct pollfd p;
		p.fd = my_client.conn_id;
		p.events = POLLIN;
		
		while ((n = poll(&p, 1, timeout)) < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}
			perror("server_routine : error while polling socket");
			goto end;
		}
		
		if (!n) {
			errno = ETIMEDOUT;
			perror("server_routine : error while polling socket");
			goto end;
		}
		
		while ((bytes_read = read(my_client.conn_id,
					 buffer,
					 server->buffer_length)) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			perror("server_routine : error while reading socket.\n");
			goto end;
		}
		
		if (bytes_read == 0) { // 0 Bytes read means that the connection has been lost.
			print_trace(TRACE_WARNING, "l2cap_server : nothing to read on the socket.\n");
			goto end;
		}
		
		num_req++;
		
		if (strcmp(buffer, L2CAP_SERVER_UNIVERSAL_STOP) == 0) {
			char closeACK[10] = "STOP_ACK";
			if (write(my_client.conn_id, closeACK, 10) < 10) {
				print_trace(TRACE_WARNING, "l2cap_server : unable to send STOP_ACK.\n");
			}
			thread_launched = 0;
		}
		server->treat_buffer(server, i);		
	}

 end:
	print_trace(TRACE_INFO, "l2cap_server : connection %i ended.\n", my_client.conn_id);
	close(my_client.conn_id);

	return NULL;
}

//------------------------------------------------------------------------------------

int8_t l2cap_server_launch(l2cap_server_t *server, int16_t timeout, uint16_t max_req) {
	if (server == NULL) {
		print_trace(TRACE_ERROR, "l2cap_server_launch : invalid server reference.\n");
		return -1;
	}

	if (server->max_clients == 0) {
		print_trace(TRACE_ERROR, "l2cap_server_launch : no socket available to run the server.\n");
		return -1;
	}
	
	if (server->socket.sock < 0) {
		print_trace(TRACE_ERROR, "server_launch_socket_routine : invalid socket.\n");
		return -1;
	}
	
	pthread_t server_threads[server->max_clients];
	struct routine_data_t routine_data[server->max_clients];
	for (uint8_t i = 0; i < server->max_clients; i++) {
		routine_data[i].server = server;
		routine_data[i].timeout = timeout;
		routine_data[i].max_req = max_req;
		routine_data[i].num_client = i;
	}
	
	int8_t sock = server->socket.sock;
	socklen_t sockaddr_len = sizeof(l2cap_sockaddr_t);
	int8_t client_i = 0;
	int client_sock = 0;
	
	listen(sock, server->max_clients);
	print_trace(TRACE_INFO, "Waiting for connection on socket %i...\n", sock);
	
	// Now that the socket is listening, we check connection demands
	struct pollfd p;
	int8_t n = 0;
	p.fd = sock;
	p.events = POLLIN;
	
	while ((n = poll(&p, 1, timeout)) < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			continue;
		perror("server_routine_accept : error while polling socket");
		goto fail;
	}
	
	if (!n) {
		errno = ETIMEDOUT;
		perror("server_routine_accept : error while polling socket");
		goto fail;
	}
	
	// We create a new thread for each client accepted
	while ((client_sock = accept(sock,
			      (struct sockaddr *)&(server->clients[client_i].rem_addr),
			      &sockaddr_len))) {
		
		print_trace(TRACE_INFO, "Connection established on socket %i, connection id : %i...\n", sock, client_sock);
		server->clients[client_i].conn_id = client_sock;
		
		if(pthread_create(&(server_threads[client_i]), NULL, &(server_thread_routine), (void *)&routine_data[client_i]) < 0) {
			perror("could not create thread");
			goto fail;
		}
		client_i++;
		server->launched = 1;
	}
	
	if (client_sock < 0) {
		perror("accept failed");
		goto fail;
	}
	
	for (uint8_t i = 0; i < client_i; i++) {
		pthread_join(server_threads[i], NULL);
	}
	
	return 0;
fail:
	return -1;
}

//------------------------------------------------------------------------------------

void l2cap_server_close(l2cap_server_t * server) {
	for (uint8_t i = 0; i < server->max_clients; i++) {
		free(server->clients[i].buffer);
	}
	if (server->socket.sock >= 0) {
		close(server->socket.sock);
	}
	free(server->clients);
}
