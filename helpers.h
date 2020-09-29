#ifndef _HELPERS_H
#define _HELPERS_H

#define IP_PORT_SIZE 6
#define IP_SIZE 4
#define PORT_SIZE 2
#define DATA_TYPE_SIZE 1
#define COMMAND_LEN 100
#define MESSAGE_LEN 1600
#define TOPIC_SIZE 50
#define TYPE_SIZE 15
#define MAX_CONTENT_SIZE 1500
#define BUFF_MAX 87380
#define MAX_DECIMALS 15
#define HEADER_SIZE 15 
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


#endif