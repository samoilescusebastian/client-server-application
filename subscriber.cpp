

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <math.h>
#include "message.h"
int packets;
const char* data_types[4] = {"INT", "SHORT_REAL", "FLOAT", "STRING"};
void print_message(const special_buffer& buffer) {
	packets++;
	char ip_addr[INET_ADDRSTRLEN];
	char topic[TOPIC_SIZE];
	char content[MAX_CONTENT_SIZE];
	int sign = 1;
	int int_part = 0;
	uint8_t exp = 1;
	inet_ntop(AF_INET, &(buffer.ip), ip_addr, INET_ADDRSTRLEN);

	memcpy(topic, buffer.buffer, buffer.topic_size);
	memcpy(content, buffer.buffer + buffer.topic_size, buffer.content_size);
	// convertirea bufferului in informatie in functie de tipul acestuia
	if (buffer.data_type == 0) {
		if (content[0] == 1) {
			sign = -1;
		}
		memcpy(&int_part, content + 1, 4);
		int_part = ntohl(int_part);
		sprintf(content, "%d", sign * int_part);
	} else if (buffer.data_type == 1)  {
		memcpy(&int_part, content, 2);
		int_part = ntohs(int_part);
		sprintf(content, "%hu.%02hu",int_part/100, int_part % 100);
	} else if(buffer.data_type == 2){
		if (content[0] == 1) {
			sign = -1;
		}
		memcpy(&int_part, content + 1, 4);
		int_part = ntohl(int_part);
		memcpy(&exp, content + 5, 1);
		int power = pow(10, exp);
		sprintf(content, "%lf", ((double)sign * int_part / power));
		//convertire catre precizia enuntata in cerinta
		char *p = strchr(content, '.');
		p = p + strlen(p) - 1;
		while (*p == '0') {
			p--;
		}
		if (*p == '.') {
			*p = '\0';
		} else {
			*(p + 1) = '\0';
		}


	}

	printf("%s:%d - %s - %s - %s\n", ip_addr, ntohs(buffer.port), data_types[buffer.data_type], topic, content);	
}
void receive_data(int sockfd, char* buffer, struct special_buffer& s_buffer,
		          int& bytes_in_buffer, bool& hdr_completed) {
	int res = recv(sockfd, (buffer + bytes_in_buffer), BUFF_MAX - bytes_in_buffer, 0);
	DIE(res < 0, "Failed to receive udp message from server.\n");
	if (res == 0) {
		close(sockfd);
		exit(0);
	}
	// mecanism folosit pentru a nu altera bufferul
	bytes_in_buffer += res;
	while (bytes_in_buffer >= HEADER_SIZE) { // cat timp avem informatie pentru a completa un header
		if (!hdr_completed) {				 // incearca sa formezi un pachet
			memcpy(&s_buffer, buffer, HEADER_SIZE);
			memmove(buffer, buffer + HEADER_SIZE, bytes_in_buffer - HEADER_SIZE); // completare header, marime topic
			bytes_in_buffer -= HEADER_SIZE;									      // si marime content
			hdr_completed = true;
		}
		
		int tc_size = s_buffer.topic_size + s_buffer.content_size; // daca ce a ramas in bufffer e de ajuns a completa measjul
		if (bytes_in_buffer >= tc_size && hdr_completed) {		  // completeaza si printeaza informatia
			memcpy(&(s_buffer.buffer), buffer, tc_size);
			memmove(buffer, buffer + tc_size, bytes_in_buffer - tc_size);
			bytes_in_buffer -= tc_size;
			print_message(s_buffer);
			hdr_completed = false;
			memset(&s_buffer, 0, sizeof(struct special_buffer));
		} else {
			break; // daca nu este sufficient, asteapta un alt pachet
		}
	}

}
// mecanism de primire feedback, asemanator cu mecanismele anterioare
void receive_feedback(char* buffer, int &bytes_in_buffer, int socket) {
		int res = recv(socket, buffer + bytes_in_buffer, BUFF_MAX - bytes_in_buffer, 0);
		bytes_in_buffer += res;
		int feedback_size;
		memcpy(&feedback_size, buffer, sizeof(int));

		char feedback[COMMAND_LEN];
		
		memcpy(feedback, buffer + sizeof(int), feedback_size + 1);
		memmove(buffer, buffer + feedback_size + sizeof(int) + 1,
			bytes_in_buffer - (feedback_size + sizeof(int)) + 1);
		bytes_in_buffer -= (feedback_size + sizeof(int) + 1);
		if (feedback_size > 0) {
			printf("[LOG]%s\n", feedback);
			if (std::string(feedback) == "Client already exists, choose another id.") { // daca id ul clientului nu este disponibil
				exit(0);																// inchide clientul
			}

		}
}
void sendCommand(std::string &command, int socket, char*buffer, int &bytes_in_buffer) {
	char command_buffer[COMMAND_LEN];
	int command_size = command.size();
	memcpy(command_buffer, &command_size, sizeof(command_size));
	memcpy(command_buffer + sizeof(command_size), command.c_str(), command_size + 1);
    sendMessage(command_buffer, socket, command_size + sizeof(command_size));
	receive_feedback(buffer, bytes_in_buffer, socket);

}
int main(int argc, char **argv)
{
	int sockfd, n, ret, option, res, stored_bytes = 0;
	struct sockaddr_in subs_addr;
	struct special_buffer s_buffer;
	char buffer[BUFF_MAX];	
	bool hdr_completed = false;
	
	std::string command;
	std::string operation;


	fd_set read_fds;
	fd_set tmp_fds;

	if (argc != 4) {
		DIE(true, "Inccorect number of arguments\n");
        exit(-1);
	}		

    option = 1;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Failed to create tcp socket\n");
    res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));
    DIE(res < 0, "Failed to set no-delay on tcp socket.\n");


    memset((char *) &subs_addr, 0, sizeof(subs_addr));
	subs_addr.sin_family = AF_INET;
	subs_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &subs_addr.sin_addr);
	DIE(ret == 0, "Failed to convert ip\n");

	ret = connect(sockfd, (struct sockaddr*) &subs_addr, sizeof(subs_addr));
	DIE(ret < 0, "Failed to connect to server");
    command = "addclient " + std::string(argv[1]);
	sendCommand(command, sockfd, buffer, stored_bytes);


	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	while (1) {
  		tmp_fds = read_fds;
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			std::getline(std::cin, command);

			if (command == "exit") {
				printf("%d", packets);
				exit(0);
			}
			sendCommand(command, sockfd, buffer, stored_bytes); // trimite comand catre server
		} else 
            if (FD_ISSET(sockfd, &tmp_fds)) {
                receive_data(sockfd, buffer, s_buffer, stored_bytes, hdr_completed); // primeste mesaje udp de la server
            }
	}
}
