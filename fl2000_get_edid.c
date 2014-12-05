/*
 * minimal, experimental program to read EDID from a Fresco Logic FL2000
 *
 * Copyright (C) 2014 cy384
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <sys/types.h>
#include <libusb.h>

/* for control stuff, we only really work with four bytes at a time, but w/e */
#define RESPONSE_BUFFER_SIZE 512

/* convenience macros */
#define EXPECT_FOUR_BYTES \
if (bytes_back != 4) \
{ \
	fprintf(stderr, "%d bytes from transfer?\n", bytes_back); \
	ret = bytes_back; \
	goto cleanup; \
}

#define PRINT_FOUR_BYTES(wIndex, readwrite) \
printf("%d %c: ", wIndex, readwrite); print_data(data);

#define SILENT_TRANSFER_OUT(wIndex) \
bytes_back = libusb_control_transfer(fl2000_handle, 0x40, 65, 0, wIndex, data, 4, 0); \
EXPECT_FOUR_BYTES;

#define SILENT_TRANSFER_IN(wIndex) \
bytes_back = libusb_control_transfer(fl2000_handle, 0xc0, 64, 0, wIndex, data, 4, 0); \
EXPECT_FOUR_BYTES;

#define SET_DATA(byte0, byte1, byte2, byte3) \
data[0] = byte0; data[1] = byte1; data[2] = byte2; data[3] = byte3;

#define DATA_EQ(byte0, byte1, byte2, byte3) \
(byte0 == data[0] && byte1 == data[1] && byte2 == data[2] && byte3 == data[3])

/* make all transfers silent except in debug mode */
#ifdef DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

#if DEBUG

#define TRANSFER_IN(wIndex) SILENT_TRANSFER_IN(wIndex) \
EXPECT_FOUR_BYTES; PRINT_FOUR_BYTES(wIndex, 'R');

#define TRANSFER_OUT(wIndex) SILENT_TRANSFER_OUT(wIndex) \
EXPECT_FOUR_BYTES; PRINT_FOUR_BYTES(wIndex, 'W');

#else

#define TRANSFER_IN(wIndex) SILENT_TRANSFER_IN(wIndex)
#define TRANSFER_OUT(wIndex) SILENT_TRANSFER_OUT(wIndex)

#endif

void print_data(uint8_t* data)
{
	int i;
	for (i = 0; i < 4; i++) printf("%.2x ", data[i]);
	printf("\n");
}

int main(void)
{
	int ret = 0;
	int bytes_back = 0;
	uint8_t edid_offset = 0;
	uint8_t data[RESPONSE_BUFFER_SIZE] = {0};
	libusb_device_handle* fl2000_handle = 0;

	if (DEBUG) printf ("compiled in debug mode\n");

	ret = libusb_init(NULL);
	if (ret < 0) goto cleanup;

	fl2000_handle = libusb_open_device_with_vid_pid(0, 0x1d5c, 0x2000);
	if (!fl2000_handle)
	{
		fprintf(stderr, "couldn't find an fl2000 device\n");
		goto cleanup;
	}

	ret = libusb_set_configuration(fl2000_handle, 1);
	if (ret < 0) goto cleanup;

	ret = libusb_claim_interface(fl2000_handle, 1);
	if (ret < 0) goto cleanup;

	ret = libusb_set_interface_alt_setting(fl2000_handle, 1, 0);
	if (ret < 0) goto cleanup;

	ret = libusb_claim_interface(fl2000_handle, 2);
	if (ret < 0) goto cleanup;

	ret = libusb_set_interface_alt_setting(fl2000_handle, 2, 0);
	if (ret < 0) goto cleanup;

	TRANSFER_IN(32800);

	/* either for priming the interrupt or the EDID stuff? */
	while (!DATA_EQ(0xcc, 0x00, 0x00, 0x8f))
	{
		SET_DATA(0xcc, 0x00, 0x00, 0x10);
		SILENT_TRANSFER_OUT(32800);
		SILENT_TRANSFER_IN(32800);
	}

	/* reset the monitor attachment thing? */
	SET_DATA(0xe1, 0x00, 0x00, 0x00);
	TRANSFER_OUT(32768);
	TRANSFER_IN(32768);

	if (!DATA_EQ(0x00, 0x00, 0x00, 0x00))
	{
		fprintf(stderr, "did you leave your monitor attached?\n");
	}

	/* wait for monitor attach interrupt, should get a 0x01 response */
	bytes_back = 0;
	ret = libusb_interrupt_transfer(fl2000_handle, 0x83, data, 1, &bytes_back, 0);

	if (ret != 0)
	{
		goto cleanup;
	}

	if (bytes_back != 1)
	{
		fprintf(stderr, "%d bytes from interrupt?\n", bytes_back);
	}

	/* now begin the real EDID config and transfer */
	SET_DATA(0xd0, 0x00, 0x00, 0xcf);
	TRANSFER_OUT(32800);
	TRANSFER_IN(32800);

	SET_DATA(0xd0, 0x00, 0x00, 0x48);
	TRANSFER_OUT(32800);
	TRANSFER_IN(32800);

	SET_DATA(0x00, 0x00, 0x00, 0x00);

	for (edid_offset = 4; edid_offset < 132; edid_offset += 4)
	{
		while (!DATA_EQ(0xd0, edid_offset, 0x00, 0xc0))
		{
			SET_DATA(0xd0, edid_offset, 0x00, 0xc0);
			SILENT_TRANSFER_OUT(32800);
			SILENT_TRANSFER_IN(32800);
		}

		TRANSFER_IN(32804);

		if (!DEBUG)
		{
			int i;
			for (i = 0; i < 4; i++) printf("%c", data[i]);
		}

		SET_DATA(0xd0, edid_offset, 0x00, 0x50);
		SILENT_TRANSFER_OUT(32800);
		SILENT_TRANSFER_IN(32800);
	}

	/* clean up before exiting */
	cleanup:

	/* is there a way to check for open interfaces? w/e */	
	if (fl2000_handle)
	{
		libusb_release_interface(fl2000_handle, 1);
		libusb_release_interface(fl2000_handle, 2);
		/* do we need to set configuration back to 0? not really */
		libusb_set_configuration(fl2000_handle, 0);
		libusb_close(fl2000_handle);
	}

	libusb_exit(NULL);
	if (ret < 0) fprintf(stderr, "%s (%d)\n", libusb_error_name(ret), ret);
	return ret;
}

