# usb_push
USB push for S3C2440

This utility is used to push the binary to the MINI2440 RAM in SuperVivi mode. The mode is triggered by setting the switch to NOR mode and booting the S3C2440.

Once the file is pushed to the RAM it can be written to the NAND flash. Generally it used to push a bootloader so that it can written to the NAND memory. Once written the bootloader can be booted by toggling the switch to NAND.

This utility is inspired by Harald Welte's original program qt2410_boot_usb and modified program usbpush by Dario Vazquez.
