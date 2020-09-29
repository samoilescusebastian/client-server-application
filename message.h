#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include "client.h"
#include "helpers.h"
// strcutura pentru mesajele provenite de la udp
struct udp_message
{   
    char topic[TOPIC_SIZE];
    unsigned char data_type;
    char content[MAX_CONTENT_SIZE];
};
// structura pentru a transmite mesajele udp
// de la server catre un client tcp
struct special_buffer {
    uint32_t ip;
    uint16_t port;
    uint8_t data_type;
    int topic_size, content_size;
    char buffer[MESSAGE_LEN];
}__attribute__ ((packed));
void sendMessage(const char*, int, int);
void convert_UDPmessage_to_buffer(struct special_buffer&, struct udp_message&, uint32_t&, uint16_t&);
#endif // !_MESSAGE_H_
