#pragma once

#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#define FALSE 0
#define TRUE 1

typedef char url_content[256];

typedef struct URL {
	url_content user;
	url_content password;
	url_content host;
	url_content ip;
	url_content path;
	url_content filename;
	int port;
} url;

void initURL(url* url);
int parseURL(url* url, const char* str);
int getIpByHost(url* url);

char* getSubStr(char* str, char chr);