
#include "testBluetooth.h"

int main(int argc, char** argv){
	inquiry_info* ii = NULL ;
	BLE_prot dev_ble_prot= RFCOMM_PROT;
	char prot_str[15]="UNKNOW_PROT";
	_u8  flags = IREQ_CACHE_FLUSH;
	if(argc != 2){
		usage(argv[0]);
	}
	if((argv[1][0] != '0') && (argv[1][0] != '1') && (argv[1][0] != '2')){
		usage(argv[0]);
	}
	switch(atoi(argv[1])){
		case 0:
			dev_ble_prot = RFCOMM_PROT;
			strncpy(prot_str, "RFCOMM_PORT", sizeof(prot_str));
			break;
		case 1:
			dev_ble_prot = L2CAP_PROT;
			strncpy(prot_str, "L2CAP_PORT", sizeof(prot_str));
			break;
		case 2:
			dev_ble_prot = SDP_PROT;
			strncpy(prot_str, "SDP_PORT", sizeof(prot_str));
			break;
		default:
			strncpy(prot_str, "UNKNOW_PROT", sizeof(prot_str));
			break;
		}
	
	fprintf(stdout, "\n=====================================================================\n");
	fprintf(stdout, "\nStarting the program with bluetooth protocol %s\n",prot_str);
	fprintf(stdout, "\n=====================================================================\n");
	
	ret_code = bluez_dev_setup();

	if(ret_code < 0)
		return ret_code;
	// Allocate ressources for bluetooth service
	ii = (inquiry_info*)calloc(MAX_BLUEZ_RESPONSE, sizeof(inquiry_info));
	
	//Scan for nearby bluetooth devices
	bluez_dev_scan(&ii, flags);
	//scan input for remote device name to connect to
	fprintf(stdout, "Write the device name you want to connect to and press [Enter]\n");
	if(!fgets(remote_device_name, MAX_BLUEZ_NAME_LENGHT, stdin)) perror("fgets");
	
	fprintf(stdout, "Chosen device name %s\n", remote_device_name);
	
	if(((int)remote_device_name[0])==LINE_FEED){//user has enter an empty string
		bluez_dev_connect(dev_ble_prot, NULL);
	}
	else {
		bluez_dev_connect(dev_ble_prot, (const char*)remote_device_name);		
	}
	
	//open bluetooth server
	rfcomm_server();
	
	free(ii);
	return ret_code;
}

void usage(char* pgm_name){
	fprintf(stdout, "\n Bad arguments\n");
	fprintf(stdout, "Usage: %s \"Bluetooth_Protocol\" \n \t Bluetooth_Protocol: 0 for RFCOMM, 1 for L2CAP, 2 for SDP\n",pgm_name);
	exit(EXIT_FAILURE);
}

void getUpperString(char* _dest, const char* _src, size_t size_of_dest){
	size_t i=0;
	for(char* c = (char*)_src; *c != '\0' && i< size_of_dest; c++){
		_dest [i] = (char)toupper((int)_src[i]);
		i++;
	}
}

void __attribute__ ((constructor)) init_func(void) {
	cap_t pcaps;
	pid_t pid;
	char* caps_str;
	ret_code =0;
	ble_socket = -1;
	fprintf(stdout, "Welcome to rpi bluetooth test \n");
	dev_id=0;
	for (int i=0;i<MAX_BLUEZ_RESPONSE; i++){
		strcpy(bluez_name[i],"");
		strcpy(bluez_addr[i],"");
	}
	pid = getpid();
	pcaps = cap_get_pid(pid);
	if(!pcaps)
	fprintf(stderr,"Failed to get process capabilities\n");
	caps_str = cap_to_text(pcaps, NULL);
	fprintf(stdout, "Process capabilities are: %s\n", caps_str);
	cap_free(caps_str);
	}

void __attribute__ ((destructor)) quit_func(void){	
		close(ble_socket);
		close(ble_client);
		fprintf(stdout, "Exiting the main function... \n");
}
	
