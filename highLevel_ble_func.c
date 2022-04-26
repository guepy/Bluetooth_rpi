
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include "highLevel_ble_func.h"


// extern variables
extern int ret_code;
extern int ble_socket;
extern int ble_client;
extern char bluez_addr[MAX_BLUEZ_RESPONSE][BLE_ADDRESS_LENGHT],bluez_name[MAX_BLUEZ_RESPONSE][MAX_BLUEZ_NAME_LENGHT];
extern int n_ble_dev_found;
extern int dev_id;


void delay_ms(uint32_t milliseconds){
	clock_t tick;
	
	tick = clock() + CLOCKS_PER_SEC * milliseconds / 1000;
	while(clock() < tick); 
}

int bluez_dev_setup(void ){
	int ret_code = RET_SUCCESS;
	int ctr=0;
	char cmd[128];
	enable_bluetooth_hci();
	enable_bluetooth_service();	

	fprintf(stdout, "Initializing bluetooth device...\n");
	//looking for available bluetooth interface
OPEN_DEV:	
	dev_id = hci_get_route(NULL);
	if(dev_id < 0){
		perror("hci_get_route:");
		fprintf(stdout,"Can not access Bluetooth device,dev_id is %d. It may be off\n", dev_id);
		delay_ms(1000); 
		ctr++;
		fprintf(stdout,"Trying again %d\n", ctr);
		if(!(ctr%3)) {
			// list wireless devices
			fprintf(stdout, "Restarting bluetooth daemon service...\n");
			strcpy(cmd, "sudo systemctl daemon-reload");
			ret_code = system(cmd);
			if(ret_code < 0){
				perror("Can not list radio devices");
				return ret_code;
			}	
			memset(cmd, 0, sizeof(cmd));
			strcpy(cmd, "sudo systemctl restart bluetooth");
			ret_code = system(cmd);
			if(ret_code < 0){
				perror("Can not list radio devices");
				return ret_code;
			}	
		}
		if(ctr>10) return RET_FAILED;
		goto OPEN_DEV;
	}
	fprintf(stdout,"Bluetooth device id is %u \n", dev_id);
	//open a communication socket with the bluetooth µc
	ble_socket = hci_open_dev(dev_id);
	if(ble_socket < 0){
		fprintf(stdout,"Can not open Bluetooth device, The program may not have sufficient capabilities\n");
		return RET_FAILED;		
	}
	if(ble_socket<0){
		perror("open dev failed");
		fprintf(stdout, "No bluetooth module found\nThe app required bluetooth been activated to run\nThis device may not have one\n");
		ret_code = RET_FAILED;
		return ret_code;
	}
	fprintf(stdout, "[Done]\n");
	return ret_code;
}
int enable_bluetooth_hci(void){
	int ret_code = 0, ctr =0;
	char cmd[128];
	int gc, hciStatusStrSize=0;
	char* hciStatusStr, *searchPtr,*hciStatusSubStr;
	FILE* fd;
	bool hci_state;
	hci_state = OFF;
		
	fprintf(stdout, "Checking bluetooth hardware controler interface state...\n");
	
	// list wireless devices
	fprintf(stdout, "Listing Radio devices...\n");
	strcpy(cmd, "rfkill list > hciBLEConf.txt");
	ret_code = system(cmd);
	if(ret_code < 0){
		perror("Can not list radio devices");
		return ret_code;
	}
	
	// count the number of character in the configuration file
	fd = fopen("hciBLEConf.txt","r");
	if(!fd){
		fprintf(stderr, "Can not open hciBLEConf.txt file\n"); 
		ret_code = -1;
		return ret_code;
	}
	
	while((gc=fgetc(fd)) != EOF) hciStatusStrSize++;
	fseek(fd, 0, SEEK_SET);
	
	if(!hciStatusStrSize){
		fprintf(stderr, "Can not read hciBLEConf.txt file\n"); 
		fclose(fd);
		ret_code = -1;
		return ret_code;
	}
	ret_code = 0;
	
	// allocate memory to save the configuration file content
	hciStatusStr = (char*)calloc(hciStatusStrSize, sizeof(char));
	
	if(!hciStatusStr){
		fprintf(stderr, "Can not allocate memory\n"); 
		ret_code = -1;
		goto HCI_ON;
	}	
	// read hci configuration file
	if(fread(hciStatusStr, sizeof(char), hciStatusStrSize,  fd) <= 0){
		perror("hciStatusStr:");
		fprintf(stderr, "Can not read hciBLEConf.txt file\n"); 
		ret_code = -1;	
		fclose(fd);	
		goto HCI_ON;
	}	
	
	fclose(fd);
	// make each character uppercase 
	for(int i=0; i < hciStatusStrSize; i++){
		hciStatusStr[i] = (char)toupper((int)hciStatusStr[i]);
	}
	
	searchPtr = strstr(hciStatusStr, "BLUETOOTH");	
	if(!searchPtr) {
		ret_code = -1;
		goto HCI_ON;
	}
	
	ctr= hciStatusStrSize- (int)(searchPtr-hciStatusStr);
	
	hciStatusSubStr = (char*)calloc(ctr, sizeof(char));
	strncpy(hciStatusSubStr, searchPtr, ctr );
	
	fd = fopen("hciBLEConf.txt","w");
	fwrite(hciStatusSubStr,sizeof(char), ctr, fd); 
	fclose(fd);
	
	searchPtr = strstr(hciStatusSubStr, "SOFT BLOCKED: ");
	if(!searchPtr){
		ret_code = -1;
		goto HCI_ON;
	}
	searchPtr = searchPtr + sizeof("SOFT BLOCKED: ") -1;
	//hciStatusStr = (char*)realloc(hciStatusStr, 4 * sizeof(char)); --> memory leak as we ar no longer able to free original hciStatus 
	memset(hciStatusStr , 0,  hciStatusStrSize);
	strncpy(hciStatusStr, searchPtr, 2 );
	free(hciStatusSubStr);
	
HCI_ON:	if(!strncmp(hciStatusStr, "NO",2)){
		hci_state = ON;
		fprintf(stdout, "Bluetooth hci is ON\n");
	}
	else {
		hci_state = OFF;
		if(ret_code ==-1){
			fprintf(stdout, "Can not check bluetooth hci state due to some errors\n");
		}
		else{
			fprintf(stdout, "Bluetooth hci is OFF\n");
		}
	}
	if(!hci_state) {
		fprintf(stdout, "Enabling bluetooth hardware controler interface...\n"); 
		strcpy(cmd, "rfkill unblock bluetooth");
		ret_code = system(cmd);
		if(ret_code < 0){
			perror("Can not  bluetooth hardware controler interface");
			free(hciStatusStr);
			return ret_code;
		}

		fprintf(stdout, "[Done]\n");
	}
	free(hciStatusStr);
	return ret_code;
}

int enable_bluetooth_service(void){
	int ret_code = 0, ctr =0;
	char cmd[128], bleStatusStr[64];
	FILE* fd;
	char* str, *src;
	bool ble_state;
	ble_state = OFF;
	
	fprintf(stdout, "Checking bluetooth state...\n");
	
	strcpy(cmd, "service bluetooth status > testBLEConf.txt");
	ret_code = system(cmd);
	if(ret_code < 0){
		perror("Can not get bluetooth status");
		return ret_code;
	}
	
	strcpy(cmd, "grep -i Status: testBLEConf.txt > BLEstatus.txt");
	ret_code = system(cmd);
	if(ret_code < 0){
		perror("grep:");
		goto BLE_ON;
	}
	fd = fopen("BLEstatus.txt","r");
	if(!fd){
		perror("Open failed");
	}
	ret_code = fread(bleStatusStr,sizeof(char), sizeof(bleStatusStr),fd);
	if(ret_code <= 0){
		fprintf(stderr, "Can not read the file BLEstate.txt\n"); 
		goto BLE_ON;
	}
	src = strdup(bleStatusStr);
	memset(bleStatusStr,0,sizeof(bleStatusStr));
	
	for(str = strtok(src, ":"); str; str = strtok(NULL, "\"")){
		if(ctr==2){
			strncpy(bleStatusStr, str, 20);
			break;
		}
		ctr++;
	}
	ret_code =0;
	for(char* c = bleStatusStr; *c != '\0' && c < bleStatusStr + sizeof(bleStatusStr); c++){
		bleStatusStr[ret_code] = (char)toupper((int)*c);
		ret_code++;
	}
	
	
BLE_ON:	
	if(!strcmp(bleStatusStr, "RUNNING")){
		ble_state= ON;
		fprintf(stdout, "Bluetooth service is ON\n");
	}
	else{
		ble_state= OFF;		
		fprintf(stdout, "Bluetooth service is OFF\n");
	}
	
	if(!ble_state) {
		fprintf(stdout, "Starting Bluetooth service...\n Enter your password in the pop-up windows on the screen to allow app enabling bluetooth service\n");
		strcpy(cmd, "service bluetooth start");
		ret_code = system(cmd);
		if(ret_code < 0){
			perror("Can not enabled bluetooth service");
			fprintf(stderr, "The command %s can not be executed on the target system\n",cmd);
			return ret_code;
		}
		fprintf(stdout, "[Done]\n");
	}
	return ret_code;
}


int bluez_dev_scan(inquiry_info** ii, _u8 flags){
	int ret_code = 0;

	fprintf(stdout, "scanning nearby devices...\n");
	n_ble_dev_found = hci_inquiry(dev_id, SCAN_TIME_TICK , MAX_BLUEZ_RESPONSE, NULL, ii, flags);
	if(n_ble_dev_found<0){
		perror("failed to scan nearby devices");
		ret_code = RET_FAILED;
		return ret_code;
	}
	n_ble_dev_found = (n_ble_dev_found<MAX_BLUEZ_RESPONSE)?(n_ble_dev_found):MAX_BLUEZ_RESPONSE;
	fprintf(stdout, "--------------------------------------------------------------------------------\n");
	fprintf(stdout, "Number of remote device found: %d\n", n_ble_dev_found);
	if(!(n_ble_dev_found)) {
		fprintf(stdout, "No bluetooth device found\n");
		goto No_device;
	}
	for(int i=0;i<n_ble_dev_found;i++){
		ba2str(&((*ii)+i)->bdaddr, bluez_addr[i]);
		if(hci_read_remote_name(ble_socket, (const bdaddr_t*)(&((*ii)+i)->bdaddr), MAX_BLUEZ_NAME_LENGHT-1, bluez_name[i], 0)<0){
			strcpy(bluez_name[i], "unknow");
		}
		fprintf(stdout, "%s %s\n", bluez_name[i], bluez_addr[i]);
		getUpperString(bluez_name[i], (const char*)bluez_name[i], sizeof(bluez_name[i]));
		
	}
	fprintf(stdout, "--------------------------------------------------------------------------------\n");
No_device:	fprintf(stdout, "[Done]\n");
	return ret_code;
	
}

int bluez_dev_connect(BLE_prot protocol,const char* remote_dev_name){
	bdaddr_t bdadd;
	ret_code =0;
	int i=0;
	if(!remote_dev_name){
		fprintf(stdout, "You have not provide any device name. I will try to get connected to the first one available\n");
		for(int i=0;i<n_ble_dev_found;i++){
			fprintf(stdout, "\nAttempting to connect to %s, BLE Address %s ...\n", bluez_name[i], bluez_addr[i]);
			str2ba((char*)bluez_addr[i], &bdadd);
			switch (protocol) {
				case RFCOMM_PROT:
					ret_code = rfcomm_client(bdadd);
				break;
				case L2CAP_PROT:
					ret_code = l2cap_client(bdadd);
				break;
				case SDP_PROT:
					ble_device_search(bluez_addr[i]);
				break;
				default:
				break;
			}	
			
		}
	}
	else{
		char* remote_dev_name_cpy = strdup(remote_dev_name);
			
		// make each character uppercase 
		getUpperString(remote_dev_name_cpy, remote_dev_name, sizeof(remote_dev_name));

		for (i=0; i<n_ble_dev_found; i++){
			if(!strncmp(bluez_name[i], remote_dev_name_cpy,sizeof(remote_dev_name)-2)){//take into account line feed and end of line
				break;
			}
		}
		if(i>=n_ble_dev_found){
			fprintf(stdout, "Unknow device name \'%s\' ..\n", remote_dev_name);
			fprintf(stdout, "connection canceled \n");
			free(remote_dev_name_cpy);
			ret_code = -1;
			return ret_code;
			
		}
		else {
			fprintf(stdout, "Attempting to connect to %s, BLE Address %s ...\n", bluez_name[i], bluez_addr[i]);
			str2ba((char*)bluez_addr[i], &bdadd);
			switch (protocol) {
					case RFCOMM_PROT:
						ret_code = rfcomm_client(bdadd);
					break;
					case L2CAP_PROT:
						ret_code = l2cap_client(bdadd);
					break;
					case SDP_PROT:
						ble_device_search(bluez_addr[i]);
					break;
					default:
					break;
				}				
		}
		free(remote_dev_name_cpy);
	}
	return ret_code;
	
}


int rfcomm_server(void){
	int ret_code=0;
	struct sockaddr_rc loc_addr = {0}, rem_addr = {0};
	char buf[1024] = {0};
	int bytes_read;
	socklen_t opt = sizeof(rem_addr);
	
	//Allocate socket ressources
	ble_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	//bind socket s to port 1 of the first available local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = SOCK_STREAM;
	bind(ble_socket, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
	//put the socket in listening mode
	fprintf(stdout, "Waiting for incomming connexion...\n");
	listen(ble_socket,MAX_BLUEZ_CONNECTION);
	//accept a new conection 
	ble_client = accept(ble_socket, (struct sockaddr*)&rem_addr, &opt);
	
	ba2str(&rem_addr.rc_bdaddr, buf);
	fprintf(stdout, "New connexion accepted: %s\n", buf);
	memset(buf, 0, sizeof(buf));
	//read data from client
	bytes_read = read(ble_client, buf, sizeof(buf));
	if(bytes_read>0){
		fprintf(stdout, "bytes received %s\n", buf);
	}
	return ret_code;
}

int l2cap_server(void){
	int ret_code=0;
	struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
	char buf[1024] = {0};
	int bytes_read;
	socklen_t opt = sizeof(rem_addr);
	
	//Allocate socket ressources
	ble_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	//bind socket s to port 1 of the first available local bluetooth adapter
	loc_addr.l2_family = AF_BLUETOOTH;
	loc_addr.l2_bdaddr = *BDADDR_ANY;
	loc_addr.l2_psm = htobs(0x1001);
	bind(ble_socket, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
	//put the socket in listening mode
	fprintf(stdout, "Waiting for incomming connexion...\n");
	listen(ble_socket,dev_id);
	//accept a new conection 
	ble_client = accept(ble_socket, (struct sockaddr*)&rem_addr, &opt);
	
	ba2str(&rem_addr.l2_bdaddr, buf);
	fprintf(stdout, "New connexion accepted: %s\n", buf);
	memset(buf, 0, sizeof(buf));
	//read data from client
	bytes_read = read(ble_client, buf, sizeof(buf));
	if(bytes_read>0){
		fprintf(stdout, "bytes received %s\n", buf);
	}
	//close connection 
		
	
	return ret_code;
}


int rfcomm_client(bdaddr_t bdadd) {
	
	int ret_code =0;
	struct sockaddr_rc loc_addr={0};
	
	//allocate socket
	ble_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if(ble_socket < 0){
		perror("socket:");
		fprintf(stderr, "Failed to allocate ressources for socket\n");
	}
	//set the connection parameters(who to connect to)
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_channel = SOCK_STREAM;
	loc_addr.rc_bdaddr = bdadd;
	
	//connect to the server
	ret_code = connect(ble_socket, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
	if(ret_code < 0){
		perror("connect:");
		fprintf(stderr, "Failed to connect to the remote bluetooth server\n");
		if(errno == ECONNREFUSED){
			fprintf(stderr, "The remote device may not be supporting the chosen ble protocol\n");			
		}
	}
	else{
		write(ble_socket, "hello\n", 6);
	}
	
	return ret_code;
}
	

int l2cap_client(bdaddr_t bdadd) {
	
	int ret_code =0;
	struct sockaddr_l2 loc_addr={0};
	
	//allocate socket
	ble_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if(ble_socket < 0){
		perror("socket:");
		fprintf(stderr, "Failed to allocate ressources for socket\n");
	}
	//set the connection parameters(who to connect to)
	loc_addr.l2_family = AF_BLUETOOTH;
	loc_addr.l2_psm = htobs(0x1001);
	loc_addr.l2_bdaddr = bdadd;
	
	//connect to the server
	ret_code = connect(ble_socket, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
	if(ret_code < 0){
		perror("connect:");
		fprintf(stderr, "Failed to connect to the remote bluetooth server\n");
		if(errno == ECONNREFUSED){
			fprintf(stderr, "The remote device may not be supporting the chosen ble protocol\n");			
		}
	}
	else{
		fprintf(stderr, "successfully connected to the remote bluetooth server\n");
		write(ble_socket, "hello\n", 6);
	}	
	
	return ret_code;
}

int ble_device_search(char* sdp_remote_addr){
	_u8 svc_uuid_int[] = {0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0x0, 0x1};//sdp
	uuid_t svc_uuid;
	bdaddr_t bdaddr;
	sdp_list_t* rp_list = NULL, *search_list, *attrid_list;
	sdp_session_t *sdp_session = NULL;
	
	if(sdp_remote_addr == NULL){
		goto FAIL;
	}
	str2ba(sdp_remote_addr, &bdaddr);
	//Connect to the sdp server running on the remote device
	fprintf(stdout, "Connecting to the sdp server running on the remote device %s \n", sdp_remote_addr);
	sdp_session = sdp_connect(BDADDR_ANY, &bdaddr, SDP_RETRY_IF_BUSY);
	if(!sdp_session){
		fprintf(stdout, "SDP can not connect to the remote address %s \n", sdp_remote_addr);
		return RET_FAILED;
	}
	fprintf(stdout, "[Done]\n");
		
	// Specify the UUID of the application we are searching for
	sdp_uuid128_create(&svc_uuid,&svc_uuid_int);
	search_list = sdp_list_append(NULL, &svc_uuid);
	
	// Specify we want the complete list of applications the remote device offer
	uint32_t app_range = 0xffff;
	attrid_list = sdp_list_append(NULL, &app_range);
	// get list of service records that have the specify UUID
	fprintf(stdout, "Searching for list of service records that have the specify UUID\n"); 
	ret_code = sdp_service_search_attr_req(sdp_session, search_list, SDP_ATTR_REQ_RANGE, attrid_list, &rp_list);
	if(!rp_list){
		fprintf(stderr, "No service found with the given UUID \n");
		sdp_close(sdp_session);
		return RET_FAILED;
	}
	sdp_list_t* r = rp_list;
	fprintf(stdout, "[Done]\n");
	fprintf(stdout, "Parsing search results...\n"); 
	//Parsing and interpreting search result
	//go trough each of the service record
	for(;r;r= r->next){
		sdp_record_t *record_data = (sdp_record_t*)r->data;
		sdp_list_t *proto_list;
		//get a list of protocol sequences
		fprintf(stdout, "Getting list of protocol sequences...\n"); 
		if(!sdp_get_access_protos(record_data, &proto_list)){
			sdp_list_t *p = proto_list;
			//go through each protocol sequence
			for(;p; p = p->next){
				sdp_list_t *pds = (sdp_list_t*)p->data;
				//go through each protocol list of the protocol sequence
				for(;pds;pds = pds->next){
					//check the proto attributes
					sdp_data_t *d = (sdp_data_t*)pds->data;					
					uint32_t proto = 0;
					for(;d;d=d->next){
						switch(d->dtd){
							case SDP_UUID16:
								break;
							case SDP_UUID32:
								break;
							case SDP_UUID128:
								proto = sdp_uuid_to_proto(&d->val.uuid);
								if(proto == 0xfd2d){
									fprintf(stdout,"Redmi device detected: %u\n",proto);
								}
								break;
							case SDP_UINT8:
								if(proto == RFCOMM_UUID){
									fprintf(stdout,"RFCOMM channel: %d\n",d->val.int8);
								}
								break;
							default:
								fprintf(stdout,"Unknow channel: \n");
								break;
						}
					
					}
				}
				sdp_list_free((sdp_list_t*)p->data,0);
			}
			sdp_list_free(proto_list,0);
			fprintf(stdout, "Protocol sequences listing done\n");
		}
		else{
			fprintf(stdout, "No protocol sequence found\n");
		}
		fprintf(stdout,"Found service record: 0x%X\n",record_data->handle);
		sdp_record_free(record_data);
	}
	fprintf(stdout, "Parsing done\n");
	sdp_close(sdp_session);
	return RET_SUCCESS;
FAIL:

	fprintf(stdout, "Bad sdp remote address \n");
	return RET_FAILED;
}
