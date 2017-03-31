#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>

#include <libusb-1.0/libusb.h>
#include "usb_push.h"


static libusb_context *usb_context;

static struct libusb_device * find_s3c2440_device(void)
{
	ssize_t device_list_size = 0;
	struct libusb_device_descriptor desc;
	int i = 0;

	libusb_device **usb_dev_list;

	device_list_size =
			libusb_get_device_list(usb_context,
				&usb_dev_list);

	printf("Found %lu devices \n",device_list_size);

	while(usb_dev_list[i] != NULL) {

		libusb_get_device_descriptor(usb_dev_list[i], &desc);

		if((desc.idVendor == S3C2440_VENDOR_ID) &&
			(desc.idProduct == S3C2440_PRODUCT_ID)) {
			return usb_dev_list[i];
		}

		i++;
	}

	return NULL;
}

int main(int argc, char **argv)
{
	struct libusb_device *s3c_usb_dev = NULL;
	libusb_device_handle *handle = NULL;
	int retval = 0;

	libusb_init(&usb_context);

	s3c_usb_dev = find_s3c2440_device();

	if(s3c_usb_dev == NULL) {
		printf("S3C2440 device not found\n");
		goto end;
	}

	printf("S3C2440 device found\n");

	if((retval = libusb_open(s3c_usb_dev,
				&handle)) != 0) {
		printf("Could not open device\n");

		switch(retval) {
			case LIBUSB_ERROR_ACCESS:
				printf("No access\n");
				break;
			case LIBUSB_ERROR_NO_DEVICE:
				printf("No device\n");
				break;
			case LIBUSB_ERROR_NO_MEM:
				printf("No memory\n");
				break;
		}
		goto end;
	}

	/* Claiming interface 0 as per lsusb there is only 1 interface starting with 0*/
	if(libusb_claim_interface(handle,0) != 0) {
		printf("Could not claim interface\n");
		goto end;
	}

	printf("Claimed interface\n");



end:
//	libusb_exit(usb_context);

	return 0;
}
