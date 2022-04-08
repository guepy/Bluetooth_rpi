#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "highLevel_ble_func.h"


// global variables
int ret_code;
int ble_socket;
int ble_client;
int n_ble_dev_found = 0;
char remote_device_name[MAX_BLUEZ_NAME_LENGHT] = "no_device_chosen";
char bluez_addr[MAX_BLUEZ_RESPONSE][BLE_ADDRESS_LENGHT],bluez_name[MAX_BLUEZ_RESPONSE][MAX_BLUEZ_NAME_LENGHT];

int dev_id;

//privates function
void usage(char* pgm_name);
void getUpperString(char* _dest, const char* _src, size_t size_of_dest);

