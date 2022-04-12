#pragma once

#include <sys/socket.h>
#include <sys/capability.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>

#define ON 1
#define OFF 0
#define RET_FAILED	-1
#define RET_SUCCESS	0
#define SCAN_TIME_TICK	8
#define MAX_BLUEZ_RESPONSE 5
#define MAX_BLUEZ_CONNECTION 5
#define MAX_BLUEZ_NAME_LENGHT	21
#define BLE_ADDRESS_LENGHT 30
#define LINE_FEED	10


// Privates typedef
typedef enum _BLE_prot{
	RFCOMM_PROT = 0,
	L2CAP_PROT,
	SDP_PROT
}BLE_prot;


typedef uint8_t _u8;


//extern functions
void getUpperString(char* _dest, const char* _src, size_t size_of_dest);

//privates function
void delay_ms(uint32_t milliseconds);

//BLE function prototype
int bluez_dev_setup(void);
int bluez_dev_scan(inquiry_info** ii, _u8 flags);
int enable_bluetooth_hci(void);
int enable_bluetooth_service(void);
int rfcomm_client(bdaddr_t bdadd);
int rfcomm_server(void);
int l2cap_client(bdaddr_t bdadd);
int l2cap_server(void);
int bluez_dev_connect(BLE_prot protocol,const char* remote_dev_name);
int ble_device_search(char* sdp_remote_addr);
