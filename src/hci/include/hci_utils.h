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
 * @file hci_utils.h
 * @brief Module bluez_tools.hci.hci_utils bringing functions to deal with 
 * internal informations of an hci_controller.
 * 
 * @author Thomas Bertauld
 * @date 03/03/2016
 */

#ifndef __HCI_UTILS_H__ 
#define __HCI_UTILS_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <stdint.h>
#include <stdarg.h>

/**
 * @brief Computes an hci_filter using the given event types.
 * hci_filters are used to filter out only the events one is
 * interested in. For a list of complete suported events,
 * please refer to {@code include/net/bluetooth/hci.h}.
 * 
 * @param flt reference on the filter to compute;
 * @param ... the events to be enabled to pass through
 * the computed filter.
 */
extern void hci_compute_filter(struct hci_filter *flt, ...);

/**
 * @brief Displays on the standard output the (real) supported states of an
 * hci_controller. 
 * The parameter {@code states} is a binary mask giving the supported states
 * of a controller. This filter has to have previously been retrieved from a real
 * hci_controller by using the function {@code hci_LE_read_supported_states} in the
 * {@code hci_controller} module.
 * 
 * @see hci_LE_read_supported_states
 * 
 * @param states mask representing the supported states.
*/
extern void hci_display_LE_supported_states(uint64_t states);

#endif // __HCI_UTILS_H__
