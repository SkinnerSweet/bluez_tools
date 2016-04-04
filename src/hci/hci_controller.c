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

#include "hci_controller.h"
#include "trace.h"
#include "hci_utils.h"
#include "bt_device.h"
#include <bluetooth/hci_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define RSSI_STRING_LENGTH 10

/*------------------
 - HELPER ROUTINES -
 -------------------*/

#define CHECK_HCI_CONTROLLER_PTR(__hci_controller, __function_name)\
do {\
	if (!__hci_controller) {\
		print_trace(TRACE_ERROR, "%s : invalid controller reference.\n", __function_name);\
		return -1;\
	}\
} while(0);
// CHECK_HCI_CONTROLLER_PTR

#define CHECK_HCI_SOCKET_PTR(__hci_controller, __function_name)\
do {\
	if (!__hci_controller) {\
		print_trace(TRACE_ERROR, "%s : invalid socket reference.\n", __function_name);\
		return -1;\
	}\
} while(0);
// CHECK_HCI_SOCKET_PTR

#define CHECK_HCI_CONTROLLER_INTERRUPTED(__hci_controller, __hci_socket)\
do {\
	if (__hci_controller->interrupted) {\
		hci_resolve_interruption(__hci_socket, __hci_controller);\
	}\
} while(0);
// CHECK_HCI_CONTROLLER_INTERRUPTED

#define CHECK_HCI_CONTROLLER_OPEN(__hci_controller, __function_name)\
do {\
	if (__hci_controller->state != HCI_STATE_OPEN) {\
		print_trace(TRACE_ERROR, "%s : busy or closed controller.\n", __function_name);\
		return -1;\
	}\
} while(0);
// CHECK_HCI_CONTROLLER_OPEN

//------------------------------------------------------------------------------------

/*--------------------
  - GLOBAL VARIABLES -
  --------------------*/

static pthread_mutex_t hci_controller_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t hci_state_mutex = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------------

/*--------------------
  - STATIC FUNCTIONS -
  --------------------*/

/* Static function verifying if the given socket is a valid one or not.
   If not, we try to open a new one using the given hci_controller.
*/
static inline void check_hci_socket_ptr(hci_socket_t **hci_socket, hci_controller_t *controller,
					char *new_socket, char *err) {
	*err = 0;
	if (*hci_socket == NULL) { // We take the first socket we can :
		if (controller->sockets_list) {
			*new_socket = 0;
			*hci_socket = ((hci_socket_t *)controller->sockets_list->val);
		} else {
			*new_socket = 1;
			*hci_socket = malloc(sizeof(hci_socket_t));
			*(*hci_socket) = open_hci_socket(&(controller->device.mac));
		}}
	if ((*hci_socket)->sock < 0) {
		if (*new_socket) {
			free(*hci_socket);
			*err = 1;
			*new_socket = 0;
		}
	}
}

//---------------------------------

/* Static function trying to catch if a CMD_COMPLETE event has occured or not. Please note
   that, due to BlueZ implementation, the huge majority of functions don't need to catch
   such an event. In fact, each BlueZ function using "hci_send_req" already does this verification.
   Nevertheless, if we use a function calling "hci_send_cmd" we have to do the verification by ourselves.
   To know wether or not a function needs to be verified, please refer to the BlueZ file "hci.c".
*/
static char check_cmd_complete(hci_socket_t *hci_socket, hci_controller_t *hci_controller) {
	//http://forum.hardware.fr/hfr/Programmation/C/poll-sockets-sujet_86846_1.htm
	//http://code.metager.de/source/xref/linux/bluetooth/bluez/lib/hci.c
	struct pollfd p;
	int n;
	unsigned char buf[HCI_MAX_EVENT_SIZE];
	hci_event_hdr *hdr;

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		goto fail;
	}

	uint8_t try = 10;
	while (try--) {
		p.fd = hci_socket->sock; p.events = POLLIN;
		while ((n = poll(&p, 1, HCI_CONTROLLER_DEFAULT_TIMEOUT)) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			perror("check_cmd_complete : error while polling socket");
			goto fail;
		}
	
		if (!n) {
			errno = ETIMEDOUT;
			perror("check_cmd_complete : error while polling socket");
			goto fail;
		}

		int8_t len;
		while ((len = read(hci_socket->sock, buf, sizeof(buf))) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			perror("check_cmd_complete : error while reading socket");
			goto fail;
		}

		hdr = (void *) (buf + 1);
		switch (hdr->evt) {
		case EVT_CMD_COMPLETE:
			return 1;
			break;
		default:
			print_trace(TRACE_WARNING, "check_cmd_complete warning : an unknown event occured.\n");
			break;
		}
	}
	errno = ETIMEDOUT;

 fail:	
	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 
	
	return 0;

}

//---------------------------------

static inline int8_t hci_change_state(hci_controller_t *hci_controller, hci_state_t state) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_change_state");
	
	pthread_mutex_lock(&hci_state_mutex);
	print_trace(TRACE_DEBUG, "Controller %s state changing from %i to %i\n", hci_controller->device.custom_name,
		    hci_controller->state, state);
	hci_controller->state = state;
	pthread_mutex_unlock(&hci_state_mutex);
	
	return 0;
}
	

//------------------------------------------------------------------------------------

/*-----------------------------
  - CONTROLLER TYPE FUNCTIONS -
  -----------------------------*/

hci_controller_t hci_open_controller(bt_address_t *mac, char *name) {
	hci_controller_t res = {0};
	res.state = HCI_STATE_CLOSED;
	struct hci_dev_info info;
	hci_socket_t hci_socket = open_hci_socket(mac);
	char real_name[8] = "UNKNOWN";

	if (hci_socket.sock < 0) {
		return res;
	}
	list_push(&(res.sockets_list), &hci_socket, sizeof(hci_socket_t));

	if (hci_devinfo(hci_socket.dev_id, &info) >= 0) {
		strncpy(real_name, info.name, 8);
		if (!mac) {
			*mac = info.bdaddr;
		}
	}
	res.device = bt_device_create(*mac, PUBLIC_DEVICE_ADDRESS, real_name, name);
	res.state = HCI_STATE_OPEN;
	res.interrupted = 0;

	return res;
}

//---------------------------------

int8_t hci_close_controller(hci_controller_t *hci_controller) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_close_controller");
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_close_controller");

	close_all_hci_sockets(&(hci_controller->sockets_list));
	hci_change_state(hci_controller, HCI_STATE_CLOSED);
	
	return 0;
}

//---------------------------------

hci_socket_t hci_open_socket_controller(hci_controller_t *hci_controller) {
	
	hci_socket_t tmp;

	if (!hci_controller) {
		print_trace(TRACE_ERROR, "hci_close_socket_controller : invalid controller reference.\n");
		tmp.sock = -1;
		return tmp;
	}

	tmp = open_hci_socket(&(hci_controller->device.mac));
	int8_t status = tmp.sock;
	if (status < 0) {
		tmp.sock = -1;
		return tmp;
	}
	list_push(&(hci_controller->sockets_list), &tmp, sizeof(hci_socket_t));
	
	return tmp;
}

//---------------------------------

int8_t hci_close_socket_controller(hci_controller_t *hci_controller, hci_socket_t *hci_socket) {
	
	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_close_socket_controller");
	CHECK_HCI_SOCKET_PTR(hci_socket, "hci_close_socket_controller");

	hci_socket_t *listed_socket = list_search(&(hci_controller->sockets_list),
						  (const void *)hci_socket,
						  sizeof(hci_socket_t));
	if (!listed_socket) {
		print_trace(TRACE_WARNING, "hci_close_socket_controller : unexisting socket.\n");
		return -1;
	}
	close_hci_socket(listed_socket);
	free(listed_socket);
	
	return 0;
}

//------------------------------------------------------------------------------------

/*------------------------------------
  - CONTROLLER INTERACTION FUNCTIONS -
  ------------------------------------*/

int8_t hci_resolve_interruption(hci_socket_t *hci_socket, hci_controller_t *hci_controller) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_resolve_interruption");

	if (!hci_controller->interrupted) {
		print_trace(TRACE_WARNING, "hci_resolve_interruption : nothing to resolve.\n");
		return 0;
	}

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return -1;
	}

	pthread_mutex_lock(&hci_controller_mutex);
	switch (hci_controller->state) {
	case HCI_STATE_SCANNING :
		print_trace(TRACE_INFO, "The controller was previsouly blocking on the scanning state\n");
		if (hci_le_set_scan_enable(hci_socket->sock, htobs(0x00), htobs(0x00), 0) < 0) {
			perror("set_scan_disable");
		} else {
			hci_controller->interrupted = 0;
			hci_change_state(hci_controller, HCI_STATE_OPEN);
		}
		break;
	default:
		print_trace(TRACE_ERROR, "hci_resolve_interruption : unrecognized state.\n");
		break;
	}
	pthread_mutex_unlock(&hci_controller_mutex);

	if (!hci_controller->interrupted) {
		print_trace(TRACE_INFO, "hci_resolve_interruption : interruption resolved.\n");
		return 0;
	} else {
		print_trace(TRACE_INFO, "hci_resolve_interruption : unable to resolve the interruption.\n");
		return -1;
	}
}

//------------------------------------------------------------------------------------

// RFU (Reserved for Future Use) : the only information available yet is the LE_Encryption...
int8_t hci_LE_read_local_supported_features(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
					    uint8_t *features) { //p1056

	char new_socket = 0;

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_LE_read_local_supported_features");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_LE_read_local_supported_features");

	if (!features) {
		print_trace(TRACE_ERROR, "hci_LE_read_local_supported_features : invalid reference.\n");
		goto fail;
	}

	struct hci_request rq;
	memset(&rq, 0, sizeof(rq));
	le_read_local_supported_features_rp rp;
	memset(&rp, 0, sizeof(rp));
	
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_READ_LOCAL_SUPPORTED_FEATURES;
	rq.rparam = &rp;
	rq.rlen = LE_READ_LOCAL_SUPPORTED_FEATURES_RP_SIZE;
	
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		goto fail;
	}

	hci_change_state(hci_controller, HCI_STATE_READING);
	if (hci_send_req(hci_socket->sock, &rq, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_LE_read_local_supported_features");
		goto fail;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (rp.status) {
		print_trace(TRACE_ERROR, "hci_LE_read_supported_features : 0x%X\n", rp.status);
		goto fail;
	}

	memcpy(features, rp.features, 8);

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	return 0;

 fail:

	hci_change_state(hci_controller, HCI_STATE_OPEN);
	if (new_socket) {
		hci_close_socket_controller(hci_controller, hci_socket);
	} 
	return -1;

}

//------------------------------------------------------------------------------------

int8_t hci_LE_read_supported_states(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
				    uint64_t *states) { // p1096

	char new_socket = 0;
	
	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_LE_read_local_supported_states");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_LE_read_local_supported_states");
    
	if (!states) {
		print_trace(TRACE_ERROR, "hci_LE_read_local_supported_features : states invalid reference.\n");
		goto fail;
	}


	struct hci_request rq;
	memset(&rq, 0, sizeof(rq));
	le_read_supported_states_rp rp;
	memset(&rp, 0, sizeof(rp));

	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_READ_SUPPORTED_STATES;
	rq.rparam = &rp;
	rq.rlen = LE_READ_SUPPORTED_STATES_RP_SIZE;

	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		goto fail;
	}

	hci_change_state(hci_controller, HCI_STATE_READING);
	if (hci_send_req(hci_socket->sock, &rq, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_LE_read_supported_states");
		goto fail;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (rp.status) {
		print_trace(TRACE_ERROR, "hci_LE_read_supported_states : 0x%X\n", rp.status);
		goto fail;
	}

	*states = rp.states;

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	return 0;

 fail:
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 
	return -1;

}

//------------------------------------------------------------------------------------

int8_t hci_LE_clear_white_list(hci_socket_t *hci_socket, hci_controller_t *hci_controller) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_LE_clear_white_list");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_LE_clear_white_list");

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return -1;
	}

	hci_change_state(hci_controller, HCI_STATE_WRITING);
	if (hci_le_clear_white_list(hci_socket->sock, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_LE_clear_white_list");
		hci_change_state(hci_controller, HCI_STATE_OPEN);
		if (new_socket) {
			close_hci_socket(hci_socket);
			free(hci_socket);
		}
		return -1;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	return 0;
}

//------------------------------------------------------------------------------------

int8_t hci_LE_add_white_list(hci_socket_t *hci_socket, hci_controller_t *hci_controller, 
			     const bt_device_t bt_device) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_LE_add_white_list");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_LE_add_white_list");

	char new_socket = 0;
	uint8_t add_type = bt_device.add_type;

	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return -1;
	}

	if (add_type == UNKNOWN_ADDRESS_TYPE) {
		add_type = PUBLIC_DEVICE_ADDRESS;
	}

	hci_change_state(hci_controller, HCI_STATE_WRITING);
	if (hci_le_add_white_list(hci_socket->sock, &(bt_device.mac), add_type, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_LE_add_white_list");
		hci_change_state(hci_controller, HCI_STATE_OPEN);
		if (new_socket) {
			close_hci_socket(hci_socket);
			free(hci_socket);
		}
		return -1;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (!bt_already_registered_device(bt_device.mac)) {
		bt_register_device(bt_device);
	}

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	return 0;
}

//------------------------------------------------------------------------------------

int8_t hci_LE_rm_white_list(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
			    const bt_device_t bt_device) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_LE_rm_white_list");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_LE_rm_white_list");

	uint8_t add_type = bt_device.add_type;

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return -1;
	}

	if (add_type == UNKNOWN_ADDRESS_TYPE) {
		add_type = PUBLIC_DEVICE_ADDRESS;
	}

	hci_change_state(hci_controller, HCI_STATE_WRITING);
	if (hci_le_rm_white_list(hci_socket->sock, &(bt_device.mac), add_type, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_LE_rm_white_list");	
		hci_change_state(hci_controller, HCI_STATE_OPEN);
		if (new_socket) {
			close_hci_socket(hci_socket);
			free(hci_socket);
		}
		return -1;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (!bt_already_registered_device(bt_device.mac)) {
		bt_register_device(bt_device);
	}

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	return 0;
}

//------------------------------------------------------------------------------------

int8_t hci_LE_get_white_list_size(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
				  uint8_t *size) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_LE_get_white_list_size");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_LE_get_white_list_size");

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return -1;
	}

	hci_change_state(hci_controller, HCI_STATE_READING);
	if (hci_le_read_white_list_size(hci_socket->sock, size, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_LE_get_white_list_size");
		hci_change_state(hci_controller, HCI_STATE_OPEN);
		if (new_socket) {
			close_hci_socket(hci_socket);
			free(hci_socket);
		}
		return -1;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	return 0;
}

//------------------------------------------------------------------------------------

int8_t hci_compute_device_name(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
			     bt_device_t *bt_device) {

	CHECK_HCI_CONTROLLER_PTR(hci_controller, "hci_compute_device_name");
	CHECK_HCI_CONTROLLER_INTERRUPTED(hci_controller, hci_socket);
	CHECK_HCI_CONTROLLER_OPEN(hci_controller, "hci_compute_device_name");
	
	if (!bt_device) {
		print_trace(TRACE_ERROR, "hci_compute_device_name : invalid device's reference.\n");
		return -1;
	}

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		strcpy(bt_device->real_name, "[UNKNOWN]");
	}

	hci_change_state(hci_controller, HCI_STATE_SCANNING);
	if (hci_read_remote_name(hci_socket->sock, &(bt_device->mac), BT_NAME_LENGTH, 
				 bt_device->real_name, HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		perror("hci_read_remote_name");
		strcpy(bt_device->real_name, "[UNKNOWN]");
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} 

	
	return 0;
}

//------------------------------------------------------------------------------------

bt_device_table_t hci_scan_devices(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
				    uint8_t duration, uint16_t max_rsp, long flags) {
	
	bt_device_table_t res;
	res.device = NULL;
	res.length = 0;

	if (!hci_controller) {
		print_trace(TRACE_ERROR, "hci_scan_devices : invalid controller reference.\n");
		return res;
	}

	if (hci_controller->state != HCI_STATE_OPEN) {
		print_trace(TRACE_ERROR, "hci_scan_devices : busy or closed controller.\n");
		return res;
	}

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return res;
	}

	// Creation of the inquiry_info table :
	inquiry_info *ii = calloc(max_rsp, sizeof(inquiry_info));
	
	// Starting the inquiry :
	print_trace(TRACE_INFO, "Starting the scanning inquiry...");
	hci_change_state(hci_controller, HCI_STATE_SCANNING);
	int16_t num_rsp = hci_inquiry(hci_socket->dev_id, duration, max_rsp, NULL, &ii, flags);
	if(num_rsp <= 0) {
		print_trace(TRACE_STDOUT, " No device found.\n");
		goto end;
	}
	
	print_trace(TRACE_INFO, " [DONE]\n"); 
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	bt_device_t *device_table = calloc(num_rsp, sizeof(bt_device_t));

	for (uint16_t i = 0; i < num_rsp; i++) {
		memset(&(device_table[i]), 0, sizeof(bt_device_t));
		device_table[i].mac = ii[i].bdaddr;
		hci_compute_device_name(hci_socket, hci_controller, &(device_table[i]));
		device_table[i].add_type = UNKNOWN_ADDRESS_TYPE;
		strcpy(device_table[i].custom_name, "UNKNOWN");
		if (!bt_already_registered_device(device_table[i].mac)) {
			bt_register_device(device_table[i]);
		}
	}


	res.device = device_table;
	res.length = num_rsp;

 end :	
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	free(ii);

	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	}

	return res;
}

//------------------------------------------------------------------------------------

// TODO : add the duplicate packets support
char *hci_get_RSSI(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
		   int8_t *file_descriptor,
		   bt_address_t *mac, uint8_t duration, uint16_t max_rsp) {

	if (!hci_controller) {
		print_trace(TRACE_ERROR, "hci_get_RSSI : invalid controller reference.\n");
		return NULL;
	}

	if (hci_controller->interrupted) {
		hci_resolve_interruption(hci_socket, hci_controller);
	}

	if (hci_controller->state != HCI_STATE_OPEN) {
		print_trace(TRACE_ERROR, "hci_get_RSSI : busy or closed controller.\n");
		return NULL;
	}

	// Resulting string :
	char *res = NULL;

	// HCI_Filter structure :
	struct hci_filter flt;	
	struct hci_filter old_flt;

	// Command parameter for an "OCF_WRITE_INQUIRY_MODE" :
	write_inquiry_mode_cp write_cp;
	memset(&write_cp, 0, sizeof(write_cp));

	// Command parameter for a standard inquiry : "OCF_INQUIRY" :
	inquiry_cp cp;
	memset (&cp, 0, sizeof(cp));

	/* Events buffer : table of unsigned char (range 0-255) allowing us
	   to catch an HCI-event packet coming in response to one of our
	   commands packet (thanks to the "read()" function).
	*/
	unsigned char buf[HCI_MAX_EVENT_SIZE];
	unsigned char *event_parameter;
 
	hci_event_hdr event_header;

	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return NULL;
	}

	// Creating our filter :
	hci_compute_filter(&flt, EVT_CMD_COMPLETE,
			   EVT_INQUIRY_RESULT_WITH_RSSI,
			   EVT_INQUIRY_COMPLETE,
			   0);

	/* Saving the old filter.
	   This can be helpful in a case where we want to use a same socket for different purposes
	   (even with multiple threads), we can replace the old filter and re-use it later.
	*/
	char saved_flt = 1;
	if (!new_socket) {
		if (get_hci_socket_filter(*hci_socket, &old_flt) < 0) {
			saved_flt = 0;
		}
	}

	// Applying the new filter :
	if (set_hci_socket_filter(*hci_socket, &flt) < 0) {
		goto end;
	}

	// Configuring the inquiry mode on the open HCI socket :
	print_trace(TRACE_DEBUG, "Configuring the inquiry mode...");
	write_cp.mode = htobs(0x01); // Inquiry Result format with RSSI (cf p878 spec').
	
	pthread_mutex_lock(&hci_controller_mutex);
	hci_change_state(hci_controller, HCI_STATE_WRITING);
	if (hci_send_cmd(hci_socket->sock, OGF_HOST_CTL, OCF_WRITE_INQUIRY_MODE, 
			 WRITE_INQUIRY_MODE_CP_SIZE, &write_cp) < 0) {
		print_trace(TRACE_ERROR, " [ERROR]\n");
		perror("Can't set inquiry mode");
		goto end;
	}
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	pthread_mutex_unlock(&hci_controller_mutex);
	
	if (check_cmd_complete(hci_socket, hci_controller)) {
		print_trace(TRACE_DEBUG, " [DONE]\n"); 
	} else {
		print_trace(TRACE_DEBUG, " [ERROR]\n");
	}

	// Setting the command parameters for our rssi inquiry :
	cp.lap[2] = htobs(0x9e); 
	cp.lap[1] = htobs(0x8b);
	cp.lap[0] = htobs(0x33);
	cp.num_rsp = max_rsp; 
	cp.length = duration; 

	print_trace(TRACE_INFO, "Starting inquiry with RSSI...");

	/* Sending the inquiry's command to the HCI socket :
	   TRACE_WARNING : According to the spec (p 706), no Command Complete 
	   event will be sent to the host by using this command. 
	   This will only generate an Inquiry Complete event.
	*/
	pthread_mutex_lock(&hci_controller_mutex);
	hci_change_state(hci_controller, HCI_STATE_SCANNING);
	if (hci_send_cmd(hci_socket->sock, OGF_LINK_CTL, OCF_INQUIRY, INQUIRY_CP_SIZE, &cp) < 0) {
		print_trace(TRACE_ERROR, " [ERROR]\n");
		perror("Can't start inquiry");
		goto end;
	}
	print_trace(TRACE_INFO, " [DONE]\n"); 
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	pthread_mutex_unlock(&hci_controller_mutex);

	// Connection poll structure, not initialized yet :
	struct pollfd p;
	
	// Initializing the connection poll on our HCI socket :
	p.fd = hci_socket->sock;
	p.events = POLLIN;

	char canceled = 0;
	int8_t n = 0;
	int16_t len = 0;

	res = calloc(6*max_rsp, sizeof(char));
	res[0] = '\0';
	while(!canceled) {
		p.revents = 0;
		n = 0;
		len = 0;

		// Polling the BT device for an event :
		if ((n = poll(&p, 1, HCI_CONTROLLER_DEFAULT_TIMEOUT)) < 0) {
			if (errno == EAGAIN || EINTR) {
				continue;
			}
			perror("hci_get_RSSI : error while polling the socket");
			canceled = 1;
			goto end;
		}

		if (!n) {
			errno = ETIMEDOUT;
			perror("hci_get_RSSI : error while polling the socket");
			break;
		}

		while ((len = read(hci_socket->sock, buf, sizeof(buf))) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			perror("hci_get_RSSI : error while reading the socket");
			canceled = 1;
			goto end;
		}

		if (len == 0) {
			print_trace(TRACE_WARNING, "hci_get_RSSI : nothing to read on the socket.\n");
			break;
		}


		// WHAT IS buf[0] ? Maybe a parameter added by "read()" ???
		event_header.evt = buf[1];
		event_header.plen = buf[2];
	
		event_parameter = buf + HCI_EVENT_HDR_SIZE + 1;
			
		int num_results = event_parameter[0];
		event_parameter = (void *)(event_parameter + 1); // We switch to the next byte.
		
		switch (event_header.evt) {
		case EVT_CMD_COMPLETE: // Corresponding to the last "hci_send_cmd"
			print_trace(TRACE_WARNING, "hci_get_RSSI : untreated \"Command Complete\" event.\n");
			break;

		case EVT_INQUIRY_RESULT_WITH_RSSI: // Code 0x22 
			for (uint16_t i = 0; i < num_results; i++) {
				bt_device_t bt_device;
				bt_address_t *rsp_mac = (bt_address_t *)(event_parameter + 6*i); // @ on 6 bytes.
				if (!bt_already_registered_device(*rsp_mac)) {
					bt_device.mac = *rsp_mac;	
					hci_compute_device_name(hci_socket, hci_controller, &bt_device);
					strcpy(bt_device.custom_name, "UNKNOWN");
					bt_device.add_type  = UNKNOWN_ADDRESS_TYPE;
					bt_register_device(bt_device);
				} else {
					bt_device = bt_get_device(*rsp_mac);
				}

				if (mac != NULL && !(bt_compare_addresses(mac, rsp_mac))) {
					continue;
				}

				/* To understand the meaning of the following offsets, please refer to
				   the spec' p.1002. The structure of the HCI Packet "inquiry result with
				   RSSI" is shown and explained. To sum up, during the inquiry period,
				   several devices may respond and one event packet may contain several
				   informations. But these informations are not stored device per device
				   but rather "feature by feature". That is to say that the event packet
				   contains first all of the mac@, then all of the 
				   "page_scan_repetition_mode" etc...
				*/
				int8_t *rssi = (int8_t *)(event_parameter + (6+1+1+3+2)*num_results + i); 
			
				bt_device_display(bt_device);
				if (file_descriptor && (*file_descriptor >= 0)) {
					char rssi_string[RSSI_STRING_LENGTH] = {0};
					snprintf(rssi_string, RSSI_STRING_LENGTH, "%i \n", *rssi);
					if (write(*file_descriptor, rssi_string, RSSI_STRING_LENGTH) < RSSI_STRING_LENGTH) {
						print_trace(TRACE_WARNING, "Unable to write rssi value into given fd\n");
					}
				} 
				char rssi_string_val[5] = {0};
				snprintf(rssi_string_val, 5, "%i;", *rssi);
				strcat(res, rssi_string_val);
			}
			break;

		case EVT_INQUIRY_COMPLETE:
			print_trace(TRACE_INFO, "Inquiry complete !\n");
			canceled = 1;
			break;
				
		default:
			print_trace(TRACE_WARNING, "hci_get_RSSI : an unknown event occurred : 0x%X\n", event_header.evt);
			break;
		}
			
	}


 end :
	if (!hci_controller->interrupted) {
		hci_change_state(hci_controller, HCI_STATE_OPEN);
	}
	pthread_mutex_trylock(&hci_controller_mutex);
	pthread_mutex_unlock(&hci_controller_mutex);
	
	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} else {
		// Restoring the old filter :
		if (saved_flt) {
			set_hci_socket_filter(*hci_socket, &old_flt);
		}
	}

	return res;
}

//------------------------------------------------------------------------------------

char *hci_LE_get_RSSI(hci_socket_t *hci_socket, hci_controller_t *hci_controller,
		      int8_t *file_descriptor,
		      bt_address_t *mac, uint16_t max_rsp, uint8_t scan_type, uint16_t scan_interval,
		      uint16_t scan_window, uint8_t own_add_type, uint8_t scan_filter_policy) {

	if (!hci_controller) {
		print_trace(TRACE_ERROR, "hci_LE_get_RSSI : invalid controller reference.\n");
		return NULL;
	}

	if (hci_controller->interrupted) {
		hci_resolve_interruption(hci_socket, hci_controller);
	}

	if (hci_controller->state != HCI_STATE_OPEN) {
		print_trace(TRACE_ERROR, "hci_LE_get_RSSI : busy or closed controller.\n");
		return NULL;
	}

	// Resulting string :
	char *res = NULL;

	FILE *file = NULL;

	print_trace(TRACE_INFO, "1. Opening socket...");
	char new_socket = 0;
	char socket_err = 0;
	check_hci_socket_ptr(&hci_socket, hci_controller, &new_socket, &socket_err);
	if (socket_err) {
		return NULL;
	}

	print_trace(TRACE_INFO, " [DONE]\n");

	struct pollfd p;
	// Initializing the connection poll on our HCI socket :
	p.fd = hci_socket->sock;
	p.events = POLLIN;
	unsigned char buf[HCI_MAX_EVENT_SIZE]; 

	hci_event_hdr event_header;
	unsigned char *event_parameter;

	struct hci_filter flt;	
	struct hci_filter old_flt;

	// Creating our filter :
	hci_compute_filter(&flt, EVT_CMD_COMPLETE,
			   EVT_LE_META_EVENT,
			   EVT_LE_ADVERTISING_REPORT,
			   0);

	//hci_filter_all_events(&flt);

	/* Saving the old filter.
	   This can be helpful in a case where we want to use a same socket for different purposes
	   (even with multiple threads), we can replace the old filter and re-use it later.
	*/
	print_trace(TRACE_INFO, "2. Saving old filter...");
	char saved_flt = 1;
	if (!new_socket) {
		if (get_hci_socket_filter(*hci_socket, &old_flt) < 0) {
			saved_flt = 0;
		}
	}
	print_trace(TRACE_INFO, " [DONE]\n");

	// Applying the new filter :
	print_trace(TRACE_INFO, "3. Applying new filter...");
	if (set_hci_socket_filter(*hci_socket, &flt) < 0) {
		goto end;
	}
	print_trace(TRACE_INFO, " [DONE]\n");

	print_trace(TRACE_INFO, "4. Setting scan parameters...");
	hci_change_state(hci_controller, HCI_STATE_WRITING);
	pthread_mutex_lock(&hci_controller_mutex);
	if (hci_le_set_scan_parameters(hci_socket->sock, scan_type, scan_interval, // cf p 1066 spec
				      scan_window, own_add_type, scan_filter_policy, 2*HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) { 
		// last parameter is timeout (for reaching the controler) 0 = infinity.
		print_trace(TRACE_ERROR, " [ERROR] \n");
		perror("set_scan_parameters");
		goto end;
	}
	print_trace(TRACE_INFO, " [DONE]\n");
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	pthread_mutex_unlock(&hci_controller_mutex);

	print_trace(TRACE_INFO, "5. Enabling scan...");
	pthread_mutex_lock(&hci_controller_mutex);
	hci_change_state(hci_controller, HCI_STATE_SCANNING);
	if (hci_le_set_scan_enable(hci_socket->sock, htobs(0x01), htobs(0x00), 2*HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) { // Duplicate filtering ? (cf p1069)
		print_trace(TRACE_ERROR, " [ERROR] \n");
		perror("set_scan_enable");
		goto end;
	}
	print_trace(TRACE_INFO, " [DONE]\n"); 
	pthread_mutex_unlock(&hci_controller_mutex);

	char canceled = 0;
	int k = 0;

	print_trace(TRACE_INFO, "6. Checking response events...\n");

	int8_t n = 0;
	int16_t len = 0;

	res = calloc(6*max_rsp, sizeof(char));
	res[0] = '\0';

	while(!canceled && (!(max_rsp > 0) || (k < max_rsp))) {
		p.revents = 0;
		n = 0;
		len = 0;

		// Polling the BT device for an event :
		while ((n = poll(&p, 1, 5000)) < 0) {
			if (errno == EAGAIN || EINTR) {
				continue;
			}
			perror("hci_LE_get_RSSI : error while polling the socket");
			canceled = 1;
		}
		if (canceled) {
			break;
		}

		if (!n) {
			errno = ETIMEDOUT;
			perror("hci_LE_get_RSSI : error while polling the socket");
			break;
		}

		while ((len = read(hci_socket->sock, buf, sizeof(buf))) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			perror("hci_LE_get_RSSI : error while reading the socket");
			canceled = 1;
		}
		if (canceled) {
			break;
		}

		if (len == 0) {
			print_trace(TRACE_WARNING, "hci_LE_get_RSSI : nothing to read on the socket.\n");
			break;
		}

		// WHAT IS buf[0] ? Maybe a parameter added by "read()" ???
		event_header.evt = buf[1];
		event_header.plen = buf[2];
			
		event_parameter = (void *)(buf + HCI_EVENT_HDR_SIZE + 1);
		uint8_t subevent_code = *(event_parameter);
		event_parameter = (void *)(event_parameter + 1);
		int num_reports = *event_parameter;
		event_parameter = (void *)(event_parameter + 1);

		switch (event_header.evt) {
		case EVT_LE_META_EVENT: // Code 0x3E
			switch (subevent_code) {
			case EVT_LE_ADVERTISING_REPORT:
				for (uint16_t i = 0; i < num_reports; i++) {
					bt_device_t bt_device;
					
					// Address field :
					bt_address_t *rsp_mac = (bt_address_t *)(event_parameter +
									 num_reports*(1+1) +
									 6*i); // @ on 6 bytes.
					if (!bt_already_registered_device(*rsp_mac)) {
						uint8_t *address_type;
						// Address type field :
						address_type = (uint8_t *)(event_parameter + 1*num_reports + i);
						bt_device.mac = *rsp_mac;
						bt_device.add_type = *address_type;
						strcpy(bt_device.custom_name, "UNKNOWN");
						bt_register_device(bt_device);
					} else {
						bt_device = bt_get_device(*rsp_mac);
					}

					if (mac != NULL && !(bt_compare_addresses(mac, rsp_mac))) {
						continue;
					}

					// Length_data field :
					uint8_t *length_data = (uint8_t *)(event_parameter +
									   num_reports*(1+1+6) + i);

					// RSSI field :
					int8_t *rssi = (int8_t *)(event_parameter + 
								  (1+1+6+1+(*length_data))*num_reports + i); // rssi on 1byte
					if (*rssi == 127) {
						print_trace(TRACE_WARNING, "hci_LE_get_rssi : RSSI measure unavailable.\n");
					} else if (*rssi >= 21) {
						print_trace(TRACE_ERROR, "hci_LE_get_rssi : error while reading RSSI measure.\n");
					}

					// Display info :
					bt_device_display(bt_device);

					if (file_descriptor && *file_descriptor >= 0) {
						char rssi_string[RSSI_STRING_LENGTH] = {0};
						snprintf(rssi_string, RSSI_STRING_LENGTH, "%i \n", *rssi);
						if (write(*file_descriptor, rssi_string, RSSI_STRING_LENGTH) < RSSI_STRING_LENGTH) {
							print_trace(TRACE_WARNING, "Unable to write rssi value into given fd\n");
						}
					}
					if (file) {
						fprintf(file, "%i\n", *rssi);
					}
					char rssi_string_val[5] = {0};
					snprintf(rssi_string_val, 5, "%i ;", *rssi);
					strcat(res, rssi_string_val);
					k++;
				}
				break;
					
			default:
				print_trace(TRACE_WARNING, "hci_LE_get_rssi : an unknown LE sub-event occured : 0x%X \n",
					    subevent_code);
				break;
			}
			break;
		default:
			print_trace(TRACE_WARNING, "hci_LE_get_rssi : an unknown LE sub-event occured : 0x%X \n",
				    subevent_code);
			break;
		}
	}

	print_trace(TRACE_INFO, "Scan complete !\n");
	
	print_trace(TRACE_INFO, "7. Disabling scan...");
	pthread_mutex_lock(&hci_controller_mutex);
	if (hci_le_set_scan_enable(hci_socket->sock, htobs(0x00), htobs(0x00), 2*HCI_CONTROLLER_DEFAULT_TIMEOUT) < 0) {
		print_trace(TRACE_ERROR, " [ERROR] \n");
		perror("set_scan_disable");
		hci_controller->interrupted = 1;
		goto end;
	}
	print_trace(TRACE_INFO, " [DONE]\n"); 
	hci_change_state(hci_controller, HCI_STATE_OPEN);
	pthread_mutex_unlock(&hci_controller_mutex);

 end :	
	if (!hci_controller->interrupted) {
		hci_change_state(hci_controller, HCI_STATE_OPEN);
	}
	pthread_mutex_trylock(&hci_controller_mutex);
	pthread_mutex_unlock(&hci_controller_mutex);
	
	if (new_socket) {
		close_hci_socket(hci_socket);
		free(hci_socket);
	} else {			
		// Restoring the old filter :
		if (saved_flt) {
			set_hci_socket_filter(*hci_socket, &old_flt);
		}
	}

	if (file) {
		fclose(file);
	}

	return res;
}

