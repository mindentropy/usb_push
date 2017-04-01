#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <libusb-1.0/libusb.h>
#include "usb_push.h"


static libusb_context *usb_context;
static libusb_device **usb_dev_list;

static struct libusb_device * find_s3c2440_device(void)
{
	ssize_t device_list_size = 0;
	struct libusb_device_descriptor desc;
	int i = 0;

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

static uint16_t checksum(const uint8_t *data,
							uint32_t len)
{
	uint16_t csum = 0;
	int i = 0;

	for(i = 0; i < len; i++) {
		csum += data[i];
	}

	return csum;
}

static int send_data(
					uint32_t ram_base_addr,
					uint8_t *map_addr,
					uint32_t len
					)
{
	int ret_val = 0;
	uint16_t csum = checksum(map_addr, len);

	printf("Check sum : 0x%4x\n",csum);

	return ret_val;
}


void print_usage()
{
	printf("Usage: usb_push <filename> <ram_base_address> \n");
}

int main(int argc, char **argv)
{
	struct libusb_device *s3c_usb_dev = NULL;
	libusb_device_handle *handle = NULL;
	int retval = 0, fd = 0;
	struct stat st;
	uint8_t *file_mm = NULL;

	if(argc != ARG_SIZE) {
		print_usage();
		return 0;
	}

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
	if(libusb_claim_interface(handle,
					S3C2440_USB_IF_NUM) != 0) {
		printf("Could not claim interface\n");
		goto end;
	}

	printf("Claimed interface\n");


	fd = open(argv[1],O_RDONLY);

	if(fd < 0) {
		printf("Could not open file\n");
		goto file_error;
	}

	if((retval = fstat(fd, &st)) < 0) {
		perror("usb_push");
		goto file_error;
	}

	file_mm = mmap(NULL,
					st.st_size,
					PROT_READ,
					MAP_SHARED,
					fd,
					0);

	if(file_mm == NULL) {
		close(fd);
		perror("usb_push");
		goto file_error;
	}

	send_data(RAM_BASE_ADDR,
				file_mm,
				st.st_size);

	munmap(file_mm,
			st.st_size);

file_error:
	libusb_release_interface(handle,
							S3C2440_USB_IF_NUM);
end:
	libusb_free_device_list(usb_dev_list,1);
	libusb_exit(usb_context);

	return 0;
}
