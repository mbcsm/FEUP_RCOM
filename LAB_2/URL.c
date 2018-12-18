#include "URL.h"

void initURL(url* url) {
	memset(url->user, 0, sizeof(url_content));
	memset(url->password, 0, sizeof(url_content));
	memset(url->host, 0, sizeof(url_content));
	memset(url->path, 0, sizeof(url_content));
	memset(url->filename, 0, sizeof(url_content));
	url->port = 21;
}

int parseURL(url* url, const char* str) {

	char*   url_temp, 
            *element, 
            *expression = (char*) "ftp://([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";

	int userPassMode = FALSE;
	regex_t* regex;

	size_t str_size = strlen(str);
	regmatch_t regex_match[str_size];
    

	url_temp = (char*) malloc(strlen(str));
	element = (char*) malloc(strlen(str));

	memcpy(url_temp, str, strlen(str));

    char url_identifier = url_temp[6];

	if (url_identifier == '[') {
		expression = (char*) "ftp://([([A-Za-z0-9])*:([A-Za-z0-9])*@])*([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";
		userPassMode = TRUE;
	}

	regex = (regex_t*) malloc(sizeof(regex_t));

	if (regcomp(regex, expression, 1) != 0) {
		perror("ERROR: url not correct");
		return 1;
	}

	regexec(regex, url_temp, str_size, regex_match, 1);

	//free(regex);

	strcpy(url_temp, url_temp + 6);

	if (userPassMode == TRUE) {
		strcpy(url_temp, url_temp + 1);

		strcpy(element, getSubStr(url_temp, ':'));
		memcpy(url->user, element, strlen(element));

		strcpy(element, getSubStr(url_temp, '@'));
		memcpy(url->password, element, strlen(element));
		strcpy(url_temp, url_temp + 1);
	}

	strcpy(element, getSubStr(url_temp, '/'));
	memcpy(url->host, element, strlen(element));

	char* path = (char*) malloc(strlen(url_temp));
	int startPath = 1;
	while (strchr(url_temp, '/')) {
		element = getSubStr(url_temp, '/');
		if (startPath) {
			startPath = 0;
			strcpy(path, element);
		} else
			strcat(path, element);

		strcat(path, "/");
	}
	strcpy(url->filename, url_temp);
	strcpy(url->path, path);

	//free(url_temp);
	//free(element);


	return 0;
}

int getIpByHost(url* url) {
	struct hostent* h;

	h = gethostbyname(url->host);

	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

	return 0;
}

char* getSubStr(char* str, char element) {
	char* str_temp = (char*) malloc(strlen(element));
	int index = strlen(str) - strlen(strcpy(str_temp, strchr(str, element)));

	str_temp[index] = '\0';
	strncpy(str_temp, str, index);
    int new_string_size = str + strlen(str_temp) + 1;
	strcpy(str, new_string_size);

	return str_temp;
}
