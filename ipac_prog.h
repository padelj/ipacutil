/*************************************************************************
 * TLV 02/14/2002                                                        *
 * Copyright (C) 2012 Travis Veldkamp                                    *
 *                                                                       *
 * This program is for programming the I-PAC keyboard encoder            *
 * For more information, see http://www.ultimarc.com                     *
 *                                                                       *
 * This is free software; you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * at your option) any later version.                                    *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with Foobar; if not, write to:                                  *
 *                   Free Software Foundation, Inc.                      *
 *                   59 Temple Place                                     *
 *                   Suite 330                                           *
 *                   Boston, MA  02111-1307                              *
 *                   USA                                                 *
 *************************************************************************/


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <usb.h>

/*************************************************************
 * New I-PAC2 (FS32) - 32 inputs, 28 shifted codes           *
 * I-PAC4 - 56 inputs, 56 shifted codes                      *
 * J-PAC - 28 inputs, 28 shifted codes                       *
 * ??? is there a J-PAC 32 input ???                         *
 * I-PACVE - 32 inputs, 28 shifted codes, RAM only, no flash *
 * MINIPac - 32 inputs, 28 shifted - same as I-PAC2          *
 * I believe the MINIPac also came in a 28-input version     *
 *************************************************************/

/* Timeout in ms, used for libusb calls.  A timeout of 0 indicates wait forever */
#define IPAC_USB_TIMEOUT	100

/* All boards are programmed with a 96 byte block of data, sent in 3 chunks */
#define IPAC_DATA_LEN      	96 
#define IPAC_BLOCK_LEN 		32

/* 4 macros for each board, each composed of 4 keystrokes */
#define NUM_MACRO_KEYCODES     (4*4)

/* Responses */
#define RESP_BAD                0   /* Something failed... likely bad checksum on a new undetectable board */
#define RESP_GOOD               1   /* Accepted */
/* 2 is legacy PS/2 response for IPAC-2 init */
#define RESP_CKSUM_IPAC2    	3   /* Checksum error, I-PAC2 */
#define RESP_CKSUM_IPAC4    	4   /* Checksum error, I-PAC4 */
#define RESP_TOO_MUCH_DATA      5   /* Trying to send I-PAC4 data to I-PAC2 */
#define RESP_CKSUM_IPACVE   	7   /* Checksum error, I-PACVE */
#define RESP_CKSUM_MINIPACVE	8   /* Checksum error, MINI-PACVE.  I think this is deprecated */

/* Map checksum response to board name */
static const char *board_name_by_resp[RESP_CKSUM_MINIPACVE+1] = { "invalid",   /* 0 */
                                                                  "invalid",   /* 1 */
                                                                  "invalid",   /* 2 */
                                                                  "ipac2",     /* 3 */
                                                                  "ipac4",     /* 4 */
                                                                  "invalid",   /* 5 */
                                                                  "invalid",   /* 6 */
                                                                  "ipacve",    /* 7 */
                                                                  "minipacve"};/* 8 */

/* Board types */
#define NUM_BOARD_TYPES            6
#define BOARD_TYPE_IPAC2_28        0
#define BOARD_TYPE_IPAC2_32        1
#define BOARD_TYPE_IPAC4_56        2
#define BOARD_TYPE_IPACVE          3
#define BOARD_TYPE_MINIPACVE       4

#define BOARD_TYPE_UNKNOWN         5
#define BOARD_TYPE_INVALID        -1

static const char *board_name_by_type[NUM_BOARD_TYPES] = { "ipac2 (28)",
                                                           "ipac2 (32)",
                                                           "ipac4",
                                                           "ipacve",
                                                           "minipacve",
                                                           "unknown",
                                                         };


/* 2nd D of array is: Number valid (shifted/unshifted) codes, num passes */
#define BOARD_INFO_INDEX_UNSHIFTED_CODES 0
#define BOARD_INFO_INDEX_SHIFTED_CODES   1
#define BOARD_INFO_INDEX_PASSES          2
static const char board_info[NUM_BOARD_TYPES][3] = { {28, 28, 1},   // IPAC2 (28-input)
                                                     {32, 28, 1},   // IPAC2 (32-input)
                                                     {28, 28, 2},   // IPAC4 (56-input)
                                                     {32, 28, 1},   // IPAC VE (32-input)
                                                     {36, 28, 1} }; // MiniPAC VE (36 input)


/* IPAC-2 Legacy Model (28 inputs) */
#define IPAC2_28_NUM_KEYCODES  (board_info[BOARD_TYPE_IPAC2_28][BOARD_INFO_INDEX_UNSHIFTED_CODES] + board_info[BOARD_TYPE_IPAC2_28][BOARD_INFO_INDEX_SHIFTED_CODES])
#define IPAC2_28_PASSES        (board_info[BOARD_TYPE_IPAC2_28][BOARD_INFO_INDEX_PASSES])

/* IPAC-2 New Model (32 inputs) */
#define IPAC2_32_NUM_KEYCODES  (board_info[BOARD_TYPE_IPAC2_32][BOARD_INFO_INDEX_UNSHIFTED_CODES] + board_info[BOARD_TYPE_IPAC2_32][BOARD_INFO_INDEX_SHIFTED_CODES])
#define IPAC2_32_PASSES        (board_info[BOARD_TYPE_IPAC2_32][BOARD_INFO_INDEX_PASSES])

/* IPAC-4 Legacy Model (56 inputs) - two passes of 28 keycodes each */
#define IPAC4_NUM_KEYCODES  (board_info[BOARD_TYPE_IPAC4_56][BOARD_INFO_INDEX_UNSHIFTED_CODES] + board_info[BOARD_TYPE_IPAC4_56][BOARD_INFO_INDEX_SHIFTED_CODES])
#define IPAC4_PASSES        (board_info[BOARD_TYPE_IPAC4_56][BOARD_INFO_INDEX_PASSES])

/* IPAC-VE */
#define IPACVE_NUM_KEYCODES  (board_info[BOARD_TYPE_IPACVE][BOARD_INFO_INDEX_UNSHIFTED_CODES] + board_info[BOARD_TYPE_IPACVE][BOARD_INFO_INDEX_SHIFTED_CODES])
#define IPACVE_PASSES        (board_info[BOARD_TYPE_IPACVE][BOARD_INFO_INDEX_PASSES])

/* MiniPac-VE */
#define MINIPACVE_NUM_KEYCODES  (board_info[BOARD_TYPE_MINIPACVE][BOARD_INFO_INDEX_UNSHIFTED_CODES] + board_info[BOARD_TYPE_MINIPACVE][BOARD_INFO_INDEX_SHIFTED_CODES])
#define MINIPACVE_PASSES        (board_info[BOARD_TYPE_MINIPACVE][BOARD_INFO_INDEX_PASSES])


/* Control byte */
#define CONTROL_SELECT_PAGE_2    (1)
#define CONTROL_FACTORY_DEFAULTS (1 << 1)
#define CONTROL_RAM_ONLY         (1 << 2)

/* Programming order is: shift code, unshifted codes, shifted codes, checksum, control, macros, zero pad to 96 */


/* Default data */
/* For the 28-input I-PAC2, byte 58 is checksum, 59 is control */
/* For the 32-input I-PAC2, byte 61 is checksum, 62 is control */
static char default_ipac2[96] = { 0x10, 0xF5, 0xF4, 0x14, 0xEB, 0x29, 0xF2, 0x1A,   /*  0-7  */
                           0x11, 0x34, 0x12, 0x23, 0x22, 0x2D, 0x2B, 0x1C,   /*  8-15 */
                           0x16, 0x1B, 0x1E, 0x15, 0x2E, 0x1D, 0x36, 0x43,   /* 16-23 */
                           0x21, 0x42, 0x2A, 0x3B, 0x4B, 0x0E, 0x0D, 0x2E,   /* 24-31 */
                           0x5A, 0x00, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 32-39 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00,   /* 40-47 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 48-55 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 56-63 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 64-71 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 72-79 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 80-87 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; /* 88-95 */

// TODO: these are NOT correct
/* 2nd block data, used for IPAC4.  For 56 input version, byte 58 is checksum, 59 is control */
/* 2nd block data, used for IPAC4.  For 64 input version, byte 61 is checksum, 62 is control */
static char default_ipac4[96] = { 0x10, 0xF5, 0xF4, 0x14, 0xEB, 0x29, 0xF2, 0x1A,   /*  0-7  */
                           0x11, 0x34, 0x12, 0x23, 0x22, 0x2D, 0x2B, 0x1C,   /*  8-15 */
                           0x16, 0x1B, 0x1E, 0x15, 0x2E, 0x1D, 0x36, 0x43,   /* 16-23 */
                           0x21, 0x42, 0x2A, 0x3B, 0x4B, 0x0E, 0x0D, 0x2E,   /* 24-31 */
                           0x5A, 0x00, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 32-39 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00,   /* 40-47 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 48-55 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 56-63 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 64-71 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 72-79 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 80-87 */
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; /* 88-95 */

/* RequestType field -- upper bit (direction) */
#define USB_REQ_TYPE_HOST_TO_DEV 0x00
#define USB_REQ_TYPE_DEV_TO_HOST 0x80

/* RequestType field -- bits 5/6 (type) */
#define USB_REQ_TYPE_TYPE_STD    (0)
#define USB_REQ_TYPE_TYPE_CLASS  (1 << 5)
#define USB_REQ_TYPE_TYPE_VENDOR (2 << 5)

/* RequestType field -- bits 0-4 (recipient) */
#define USB_REQ_TYPE_RECIP_DEV       0
#define USB_REQ_TYPE_RECIP_INTERFACE 1
#define USB_REQ_TYPE_RECIP_ENDPOINT  2
#define USB_REQ_TYPE_RECIP_OTHER     3

/* Host to device vendor specific, recipient other -- 0x43 */
#define USB_REQUEST_TYPE_HD_VENDOR_OTHER (USB_REQ_TYPE_HOST_TO_DEV | USB_REQ_TYPE_TYPE_VENDOR | USB_REQ_TYPE_RECIP_OTHER)
/* Device to Host Vendor specific recipient other -- 0xC3 */
#define USB_REQUEST_TYPE_DH_VENDOR_OTHER (USB_REQ_TYPE_DEV_TO_HOST | USB_REQ_TYPE_TYPE_VENDOR | USB_REQ_TYPE_RECIP_OTHER)


#define IPAC_REQUEST_PROGRAM_MODE 0xE9
#define IPAC_REQUEST_SEND_BLOCK   0xEB
#define IPAC_REQUEST_RESPONSE     0xEA


#define VENDOR_ID_ULTIMARC       0xD209
#define PRODUCT_ID_IPAC_USB      0x0301 

/* Number of I-PACs connected to 1 PC that we support */
/* I can't imagine it would ever be more than this... so just use an array instead of llist */
#define MAX_IPACS	5

/* */
int get_response(usb_dev_handle *ipac_hdl);

/* sends one 32 byte block */
int send_block(usb_dev_handle *ipac_hdl, char *data);

int enter_program_mode(usb_dev_handle *ipac_hdl);

int exit_program_mode(usb_dev_handle *ipac_hdl);

int find_ipacs(struct usb_device *dev_arr[], int max_num);

int detect_ipac_model(int ipac_num, FILE *loghdl);

int program_ipac(int ipac_num, int boardtype, char *datablock, int gen_checksum, char control, FILE *loghdl);

char compute_checksum(char *datablock, int len);

usb_dev_handle *init_ipac(int ipac_num, FILE *loghdl);
int close_ipac(usb_dev_handle *ipac_hdl);

int re_enumerate(int ipac_num, FILE *loghdl);

int ipac_count(void);
