#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ipac_prog.h"
#include "util.h"

// TODO: there is really no reason to keep calling this routine again and again
// TODO: Might as well save away the results and not scan the bus after the initial scan
/* Sets the devices in the *dev_arr */
/* Returns the number of devices found */
int find_ipacs(struct usb_device *dev_arr[], int max_num) {
  struct usb_bus *bus;
  struct usb_device *dev;
  int idx;

  for(idx=0; idx < max_num; idx++) {
     dev_arr[idx] = NULL;
  }

  usb_init();

  usb_find_busses();
  usb_find_devices();

  idx = 0;
  for (bus = usb_busses; bus; bus = bus->next) {

      for (dev = bus->devices; dev; dev = dev->next) {
         if ((dev->descriptor.idVendor  == VENDOR_ID_ULTIMARC) && 
             (dev->descriptor.idProduct == PRODUCT_ID_IPAC_USB)) { 

            //printf("Ultimarc IPAC found\n");
            dev_arr[idx] = dev;
            idx++;
            if (idx >= max_num) {
               break;
            }
         }
      }
  }

  return idx;
}

/* Put the I-PAC in download mode */
/* Send a 0xE9 request */
/* Returns 0 on success */
int enter_program_mode(usb_dev_handle *ipac_hdl) {
   int result;

   result = usb_control_msg(ipac_hdl,                         /* device */
                            USB_REQUEST_TYPE_HD_VENDOR_OTHER, /* request type */
                            IPAC_REQUEST_PROGRAM_MODE,        /* request */
                            1,                                /* value */
                            0,                                /* index */ 
                            NULL,                             /* buffer */
                            0,                                /* num bytes */
                            IPAC_USB_TIMEOUT);                /* timeout */
   
   return result;
}

/* Send a block of 32 bytes of data */
/* Returns 0 on success */
int send_block(usb_dev_handle *ipac_hdl, char *data) {
   int result;

   result = usb_control_msg(ipac_hdl,                       /* device */
                            USB_REQUEST_TYPE_HD_VENDOR_OTHER,  /* request type - host to device other */
                            IPAC_REQUEST_SEND_BLOCK,    /* request */
                            0,                          /* value */
                            0,                          /* index */
                            data,                       /* data bytes */
                            IPAC_BLOCK_LEN,             /* size in bytes */
                            IPAC_USB_TIMEOUT);          /* timeout */

   if (result == IPAC_BLOCK_LEN) {
      result = get_response(ipac_hdl);
   } else {
      printf("Ipac didn't accept entire block: %d\n", result);
   }

   return result;
}

/* called after sending a block of data */
int get_response(usb_dev_handle *ipac_hdl) {

   char response;
   int result, retval = -1;

   /* successful sending 32 bytes - get ipac response*/
   result = usb_control_msg(ipac_hdl,                         /* device */
                            USB_REQUEST_TYPE_DH_VENDOR_OTHER, /* request type - device to host other */
                            IPAC_REQUEST_RESPONSE,            /* request */
                            0,                                /* value */
                            0,                                /* index */
                            &response,                        /* buffer */
                            1,                                /* number bytes */
                            IPAC_USB_TIMEOUT);                /* timeout */
   if (result == 1) {
      //printf("I-PAC returned 0x%x after this block\n", response);

      switch (response) {
        case RESP_BAD:
          /* Something failed... that's all we know */
          retval = response;
          break;
        case RESP_GOOD:
          retval = 0;
          break;
        case RESP_CKSUM_IPAC2: 
        case RESP_CKSUM_IPAC4: 
        case RESP_CKSUM_IPACVE: 
        case RESP_CKSUM_MINIPACVE: 
          //printf("Checksum error detected\n");
          retval = response;
          break;
        case RESP_TOO_MUCH_DATA:
          /* Oops -- tried to send more data than this model supports */
          printf("too much data: 0x%x\n", response);
          break;
        default:
          printf("Unknown response: 0x%x\n", response);
          retval = 0;
      }

   } else {
      printf("Failed to get response byte from ipac\n");
   }

   return retval;
}

/* Send a 96(or 192)-byte data block to the I-PAC */
/* Must start_download before calling this */
/* Returns 0 on success */
int program_ipac(int ipac_num, int board_type, char *datablock, int gen_checksum, char control, FILE *loghdl) {
   int i, result, curpass, passes;
   char *data = datablock;
   char *datablock2 = NULL;
   usb_dev_handle *ipac_hdl;

   if (loghdl == NULL) {loghdl = stdout;}

   switch (board_type) {
      case BOARD_TYPE_IPAC2_28:
         if (gen_checksum) {
            /* Checksum Byte */
            datablock[IPAC2_28_NUM_KEYCODES + 1] = compute_checksum(datablock, IPAC2_28_NUM_KEYCODES);
         }
         /* Control Byte */
         datablock[IPAC2_28_NUM_KEYCODES + 2] = control;
         break;

      case BOARD_TYPE_IPAC2_32:
         if (gen_checksum) {
            /* Checksum Byte */
            datablock[IPAC2_32_NUM_KEYCODES + 1] = compute_checksum(datablock, IPAC2_32_NUM_KEYCODES);
         }
         /* Control Byte */
         datablock[IPAC2_32_NUM_KEYCODES + 2] = control;
         break;

      case BOARD_TYPE_IPAC4_56:
         datablock2 = datablock + IPAC_DATA_LEN;
         if (gen_checksum) {
            /* Checksum Byte */
            datablock[IPAC4_NUM_KEYCODES + 1]  = compute_checksum(datablock, IPAC4_NUM_KEYCODES);
            datablock2[IPAC4_NUM_KEYCODES + 1] = compute_checksum(datablock2, IPAC4_NUM_KEYCODES);
         }
         /* Control Byte */
         datablock[IPAC4_NUM_KEYCODES + 2] = control;
         datablock2[IPAC4_NUM_KEYCODES + 2] = control | CONTROL_SELECT_PAGE_2;
         break;
      case BOARD_TYPE_IPACVE:
         if (gen_checksum) {
            /* Checksum Byte */
            datablock[IPACVE_NUM_KEYCODES + 1] = compute_checksum(datablock, IPACVE_NUM_KEYCODES);
         }
         /* Control Byte */
         datablock[IPACVE_NUM_KEYCODES + 2] = control;
         break;
      case BOARD_TYPE_MINIPACVE:
         if (gen_checksum) {
            /* Checksum Byte */
            datablock[MINIPACVE_NUM_KEYCODES + 1] = compute_checksum(datablock, MINIPACVE_NUM_KEYCODES);
         }
         /* Control Byte */
         datablock[MINIPACVE_NUM_KEYCODES + 2] = control;
         break;
      default:
         printe("Tried to program invalid board type\n");
         return -1;
   }

   passes = board_info[board_type][BOARD_INFO_INDEX_PASSES];

   printl(0,0,loghdl,"Program_ipac: passes: %d\n", passes);

   ipac_hdl = init_ipac(ipac_num, loghdl);
   if (ipac_hdl == NULL) {
      return -1;
   }

   result = enter_program_mode(ipac_hdl);
   if (result != 0) {
      close_ipac(ipac_hdl);
      return -2;
   }
 
   for(curpass=0; curpass < passes; curpass++) { 
      if (curpass) data = datablock2;

      /* Log the data for debugging */
      printl(0,0,loghdl,"program_ipac: pass; %d\n", curpass);
      printl(0,0,loghdl,"Data block:\n");      
      for(i=0; i<IPAC_DATA_LEN; i++) 
      {
         printl(0,0,loghdl,"0x%0.2x ", data[i] & 0xFF);
      }
      printl(0,0,loghdl,"\n");

      /* Send the data in 3 chunks of 32 */
      for(i=0; i<3; i++) {
         result = send_block(ipac_hdl, data);
         data += IPAC_BLOCK_LEN;
         if (result != 0) 
         {
            //printf("send_data failed on block %d\n", i);
            exit_program_mode(ipac_hdl);
            close_ipac(ipac_hdl);
            return result;
            //return -3;
         }
      }
   }

   result = exit_program_mode(ipac_hdl);
   if (result != 0) {
      close_ipac(ipac_hdl);
      return -4;
   }

   close_ipac(ipac_hdl);

   return result; 
}

/* Tell the ipac we're done */
/* returns 0 on success */
int exit_program_mode(usb_dev_handle *ipac_hdl) {

   int result;

   // TODO: constants 
   result = usb_control_msg(ipac_hdl,
                            USB_REQUEST_TYPE_HD_VENDOR_OTHER, 
                            IPAC_REQUEST_PROGRAM_MODE, 
                            0, 
                            0,
                            NULL, 
                            0, 
                            IPAC_USB_TIMEOUT);

   return result;
}

/* Compute the checksum-8 over the data */
char compute_checksum(char *data, int len) {
   int idx, cksum=0;

   for(idx=0; idx<len; idx++) {
       cksum = (*data + cksum) & 0xFF;  // Force to 8 bits to be sure
       //printf("Data byte: 0x%x\tRunning checksum: 0x%x\n", *data, cksum);
       data++;
   }

   return cksum;
}

/* Returns board type # */
/* Returns -1 on error */
/* WARNING: new I-PAC boards have had the detection logic stripped out.  There is no longer
 * any way to detect them as the USB descriptors also have nothing unique to look for.  I am 
 * changing detection to default to I-PAC2 (32 input) when detection fails in a manner consistent
 * with the new behavior
 */
int detect_ipac_model(int ipac_num, FILE *loghdl) {

  int res;
  struct usb_device *ipac; 
  usb_dev_handle *ipac_hdl;
  int boardType = BOARD_TYPE_IPAC2_28;

  //TODO:copy default_ipac2 to a local array
  /* Calculate checksum for each board type and force bad */
  default_ipac2[IPAC2_28_NUM_KEYCODES+1]     = compute_checksum(default_ipac2, IPAC2_28_NUM_KEYCODES) + 1;
  default_ipac2[IPACVE_NUM_KEYCODES+1]    = compute_checksum(default_ipac2, IPACVE_NUM_KEYCODES) + 1;
  default_ipac2[MINIPACVE_NUM_KEYCODES+1] = compute_checksum(default_ipac2, MINIPACVE_NUM_KEYCODES) + 1;


  res = program_ipac(ipac_num, boardType, default_ipac2, 
                     FALSE, CONTROL_RAM_ONLY, loghdl);
  if (res == -1) {
      printf("Detection: board found but did not follow programming sequence\n");
  } else {
      //printf("Detect: found board type %s (%d)\n", board_name_by_resp[res], res);
      switch (res) {
      case RESP_CKSUM_IPAC2:
          res = boardType;
          break;
      case RESP_CKSUM_IPAC4:
          res = BOARD_TYPE_IPAC4_56;
          break;
      case RESP_CKSUM_IPACVE:
          res = BOARD_TYPE_IPACVE;
          break;
      case RESP_CKSUM_MINIPACVE:
          res = BOARD_TYPE_MINIPACVE;
          break;
      case RESP_BAD:
          /* Something was invalid... try the 32-input variant */
          /* More than likely, this indicates that it's a new board which is undetectable */
          res = BOARD_TYPE_UNKNOWN;
          break;
     } /* switch */
  } 

  /* Undo our messing */ 
  /* TODO: this can be pulled when we start copying the data to a local array */
  default_ipac2[IPAC2_28_NUM_KEYCODES]     = 0;
  default_ipac2[IPAC2_32_NUM_KEYCODES]     = 0;
  default_ipac2[IPACVE_NUM_KEYCODES]       = 0;
  default_ipac2[MINIPACVE_NUM_KEYCODES]    = 0;

  return res;
}


/* Returns NULL on error */
usb_dev_handle *init_ipac(int ipac_num, FILE *loghdl) {
  struct usb_device *ipac;
  usb_dev_handle *ipac_hdl;
  struct usb_device *ipac_arr[MAX_IPACS];
  int res;

  res = find_ipacs(ipac_arr, MAX_IPACS);
  if (res < 1) {
     printl(0,0, loghdl, "init_ipac: No ipacs found...\n");
     return NULL;
  }
  if (res < ipac_num) {
     printl(0, 0, loghdl, "init_ipac: Couldn't find the ipac specified.\n");
     return NULL;
  } 
  ipac = ipac_arr[ipac_num];
  
  ipac_hdl = usb_open(ipac);
  if (ipac_hdl == NULL) {
     printl(0, 0, loghdl, "init_ipac: failed to open the stinkin ipac\n");
     usb_close(ipac_hdl);
     return ipac_hdl;
  }

/* I don't believe this is necessary just to send control messages */
/* More importantly, it forces the IPAC to be unplugged/replugged to
 * get the kernel driver to re-attach (until I can find a different way anyway) */
#ifdef CLAIM_INTERFACE
#ifdef LINUX
  /* Non-portable chunk follows.  Have to find a different method on other OSes */
  res = usb_detach_kernel_driver_np(ipac_hdl, 0);
  if (res != 0) {
     printl(0,0,loghdl, "Failed to detach kernel driver: %d\n", res);
     if (res == -61) { 
        printl(0,0,loghdl, "\tOK: no driver claims this device\n");
     }
  }
#endif


  res = usb_claim_interface(ipac_hdl, 0);
  if (res != 0) {
     printl(0,0, loghdl, "init_ipac: Claiming returned: %d\n", res);
     if (res == -EPERM) {
        printl(0,0,loghdl, "didn't have permissions...are you root?\n");
     }
     //usb_close(ipac_hdl);
     //return NULL;
  }
#endif

  return ipac_hdl;
}

int close_ipac(usb_dev_handle *ipac_hdl) {
   usb_release_interface(ipac_hdl, 0);
   usb_close(ipac_hdl);
}

int re_enumerate(int ipac_num, FILE *log_hdl) {

   int res;
   usb_dev_handle *ipac_hdl;

   ipac_hdl = init_ipac(ipac_num, log_hdl);
   if (ipac_hdl == NULL) {
      return -1;
   }

   /* Force re-enumeration to get kernel driver to reclaim the ipac*/
   res = usb_reset(ipac_hdl);

   close_ipac(ipac_hdl);
   
   return res;
}

/* Returns the number of Ipacs found */
/* -1 on error */
int ipac_count() {
  struct usb_device *ipac_arr[MAX_IPACS];
  int res;

  res = find_ipacs(ipac_arr, MAX_IPACS);

  return res;
}
