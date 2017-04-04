#ifndef USB_PUSH_H_
#define USB_PUSH_H_

/*
 * The following are the MINI2440 vendor id and
 * product id when it is in the NOR firmware
 * mode.
 */

#define MINI2440_VENDOR_ID 	0x5345
#define MINI2440_PRODUCT_ID 	0x1234

/*
 * Endpoint addresses can be got by doing a
 * lsusb -vv and checking out the EP 1 IN and
 * EP 3 OUT bEndpointAddress values.
 */
#define MINI2440_EP_1_IN		0x81
#define MINI2440_EP_3_OUT	0x03
#define MINI2440_USB_IF_NUM	0

#define ARG_SIZE			3

#define RAM_BASE_ADDR 		0x30000000U
#define WRITE_CHUNK_SIZE	100

#endif
