# HR bluetooth sample extended for DFU

## Description:

This is the default HR bluetooth sample project. I have extended to write DFU updates. This project includes following features. Project description is written under the assumption that you know how to compile samples from nRFconnect SDK with dev kits like nRF5340Dk.

- App core DFU update over BLE
- App core DFU update over UART1
- Net core DFU update over BLE
- Net core DFU update over UART1
- DFU packages will be stored in external flash
- 256 Bytes of Shared SRAM is dedicated for the firmware version communication between app core and net core

## Modification Added:
BLE and DFU configuration parameters added in prj.conf and mcuboot configuration file. UART 1 is used for the DFU over UART. UART pins can be modified in overlay file.

hci_rpmsg sample is modified to write netcore firwmare version to allocated shared SRAM on boot up.
modify the shared SRAM partitions to include shared SRAM 256 bytes for firmware version communicaiton between appcore and netcore.

```
/ {
	chosen {
		/* shared memory reserved for the inter-processor communication */
		zephyr,ipc_shm = &sram0_shared;
	};

	reserved-memory {
		#address-cells = <1>;
		#size-cells = <1>;
		ranges;

		// /delete-node/ memory@20070000 ;
		sram0_shared: memory@20070000 {
		  reg = <0x20070000 0xFF00>;
		};
		sram_netcore_fw_data: memory@2007FF00 {
		  reg = <0x2007FF00 0x100>;
		};
	};
	aliases {
		sramnetcorefwdata = &sram_netcore_fw_data;
	};
};
```

## Compilation:
This project is tested with u-blox EVK NORA-B10. Which includes the nRF5340 sip. To compile this project some modifications will be required.

First we need to update the hci_rpmsg sample in SDK. As this sample is included for netcore on compilation.
```
- rename “<nrfConnectSDK Location>/zephyr/samples/bluetooth/hci_rpmsg” to “<nrfConnectSDK Location>/zephyr/samples/bluetooth/hci_rpmsg_orignal”
- Copy “<project directory>/hci_rpmsg”  to “<nrfConnectSDK Location>/zephyr/samples/bluetooth/hci_rpmsg
```
U-Blox EVK NORA-B10 board files are included in this project. But if you want to compile for other boards changes mentioned above related SRAM partitions need to be added.

## How to test DFU over BLE:

### Testing APP core DFU over BLE:
After compilation is successfull the "app_update.bin" file is generated under directory "build/zephyr/". This file should be used for the DFU update.
To run DFU over BLE you can use the nRFConnect mobile app or nRF device manager mobile app.

### Testing Net core DFU over BLE:
After compilation is successfull the "net_core_app_update.bin" file is generated under directory "build/zephyr/". This file should be used for the DFU update.
To run DFU over BLE you can use the nRFConnect mobile app or nRF device manager mobile app.


## Test DFU over UART1:
To test the DFU update over UART. we need to setup the connection and UART packet communication over UART. For testing I used python script "TransferFileOVerComport/TransferFile.py" to transfer DFU files to nRF5340 over UART. Here is how my setup was connected.

My Laptop -----> USB to TTL -----> EVK-NORA-B10

Here the structure of packets sent over UART:
```
DFU Header packet:  <0x24> <4-bytes Packet number> <0x02> <x00, 0x00, 0x00, 0x04> <4-Bytes File Size> <0x23>
DFU Data Packet:    <0x24> <4-bytes Packet number> <0x03> <4-bytes data size> <data> <0x23>
```
### Testing APP core DFU over UART1:
Compile the source code with following flag in "main.c"

```
#define TESTING_NET_CORE_DFU 0
```
After compilation is successfull the "app_update.bin" file is generated under directory "build/zephyr/". This file should be used for the DFU update. Send this file over UART to nRF5340. After transfer complete device will reboot and firmware update will be appllied.

### Testing APP core DFU over UART1:
Compile the source code with following flag in "main.c"

```
#define TESTING_NET_CORE_DFU 1
```
After compilation is successfull the "net_core_app_update.bin" file is generated under directory "build/zephyr/". This file should be used for the DFU update. Send this file over UART to nRF5340. After transfer complete device will reboot and firmware update will be appllied.


Useful Links:
- https://devzone.nordicsemi.com/guides/nrf-connect-sdk-guides/b/software/posts/ncs-dfu#dfu_from_chip
- https://devzone.nordicsemi.com/f/nordic-q-a/96482/nrf5340-firmware-update-from-external-nor-qspi-flash
- https://devzone.nordicsemi.com/f/nordic-q-a/106520/add-dfu-support-to-custom-hci_rpmsg-netcore-image


Note: Incase of any help required you can email me at: dilawarali5@gmail.com