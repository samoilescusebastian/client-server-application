
#include "message.h"
void sendMessage(const char* buffer, int fd, int bytes_count) {
	int bytes_remaining, bytes_send;
	
	bytes_remaining = bytes_count;

	bytes_send = 0;
	// fortare pentru ca tot bufferul sa fie alipit catre cel al kernelului
	do {
		int sent = send(fd, &buffer[bytes_send], bytes_count, 0);
		DIE(sent < 0, "SEND ERROR\n");
		bytes_send += sent;
		bytes_remaining -= sent;
	}while (bytes_remaining > 0);
}

void convert_UDPmessage_to_buffer(struct special_buffer& s_buffer, struct udp_message& udp_message,
							      uint32_t& ip, uint16_t& port) {
	int copied = 0;
	s_buffer.ip = ip;
	s_buffer.port = port;
	s_buffer.data_type = udp_message.data_type;
	s_buffer.topic_size = strlen(udp_message.topic) + 1;
	memcpy(s_buffer.buffer + copied, udp_message.topic, s_buffer.topic_size);
	copied += s_buffer.topic_size;
	int content_length = 0;
	// selectare marime content ului in functie de tipul mesajului
	if (udp_message.data_type == 0) {
		content_length = 5;
	} else if(udp_message.data_type == 1){
		content_length = 2;
	} else if(udp_message.data_type == 2){
		content_length = 6;
	} else if(udp_message.data_type == 3){
		content_length = strlen(udp_message.content) + 1;
	}
	s_buffer.content_size = content_length;
	memcpy(s_buffer.buffer + copied, udp_message.content, content_length);
	

}