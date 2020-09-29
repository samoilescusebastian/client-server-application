#include<iostream>
#include <string.h>
#include "message.h"
#include "unordered_map"
// functia care trimite feedback in urma unei comenzi reusite/esuate
void send_feedback(const char* message, int socket) {
    char feedback[COMMAND_LEN];                                               // pachetul contine 4 octeti ce reprezinta marimea bufferului
    int feedback_size = strlen(message);                                      // urmatorii octeti sunt reprezentati de informatia din buffer
	memcpy(feedback, &feedback_size, sizeof(feedback_size));
	memcpy(feedback + sizeof(feedback_size), message, feedback_size + 1);
    sendMessage(feedback, socket, feedback_size + sizeof(feedback_size) + 1);
}
void do_command(std::vector<struct client*> &clients, std::unordered_map<std::string, std::vector<std::pair<struct client*,bool>>> &topics,
                std::string &command, int socket, uint32_t ip, uint16_t port) {
    char ip_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip_addr, INET_ADDRSTRLEN);
    size_t pos = 0;
    std::string identifier, first_argument;
    struct client* client;
    struct special_buffer* tcp_buffer;

    pos = command.find(" "); // obtinerea numelui comenzii(subscribe, unsubscribe)
    identifier = command.substr(0, pos);
    command.erase(0, pos + 1);

    pos = command.find(" "); // obtinerea primului argument din comanda
    first_argument = command.substr(0, pos);
    command.erase(0, pos + 1); // in cazul unei comenzi cu doi parametrii, cel de al doilea parametru se retine in command
                               

    if (identifier == "subscribe") {
        std::vector<std::pair<struct client*,bool>>* topic_clients = &topics[first_argument]; // obtinere clienti pe un anumit topic
        client = get_client(clients, socket);
        if (client == nullptr) {
            printf("No such client!\n");
            exit(-1);
        }
        if (is_subscribed(topic_clients, client)) { // daca clientul este deja abonat la topic, se ignora cererea
            send_feedback("", socket);
            return;
        }
        send_feedback(std::string("subscribed " + first_argument).c_str(), socket); // anunta client ca subscribe ul a reusit
        topic_clients -> push_back(std::make_pair(client, command == "1")); // adaugarea unui client catre un topic
    } else if (identifier == "unsubscribe") {

        std::vector<std::pair<struct client*,bool>>* clients = &topics[first_argument];
        for(int i = 0; i < clients -> size(); i++) {
            if ((*clients)[i].first -> socket == socket) {
                clients -> erase(clients -> begin() + i);
                send_feedback(std::string("unsubscribed " + first_argument).c_str(), socket); // stergere client din cadrul topic ului
                return;                                                                       // in cazul in care este abonat
            }
        }
        send_feedback(std::string("client is not subscribed to " + first_argument).c_str(), socket); // trimitere mesaj in cazul in care
                                                                                                    // clientul nu este abonat la topic
    } else if (identifier == "addclient") {   // comanda ce este data atunci cand un client doreste concectarea spre server
        client = get_client(clients, first_argument);
        if (client != nullptr) { // daca clinetul exista si este conectat se trimite mesaj de eroare
            if (client -> online) {
                send_feedback("Client already exists, choose another id.", socket);
                return;
            }
            send_feedback("", socket);
            printf("New client (%s) connected from %s:%d.\n", first_argument.c_str(), ip_addr, port); // conctarea cu succes a unui
            client -> socket = socket; // asociere socket nou                                        // client existent, dar offline
            client -> online = true;   // marcare online
            // trimite mesajele primite cat timp a fost offline
            while (!client -> messages.empty()) { 
                tcp_buffer = client -> messages.front();
                int size = sizeof(struct special_buffer) - MESSAGE_LEN +
                             tcp_buffer -> content_size + tcp_buffer -> topic_size;
                sendMessage((const char*)tcp_buffer, client -> socket, size);
                client -> messages.pop();
                delete []tcp_buffer;
            }

        } else {
            // conectarea unui client nou, inexistent
            client = new struct client(first_argument, socket);
            clients.push_back(client);
            send_feedback("", socket);
            printf("New client (%s) connected from %s:%d.\n", first_argument.c_str(), ip_addr, port);
        }

    } else {
       send_feedback("Unknown command.", socket); // trimitere feedback pentru o comanda invalida
    }
    
}
void send_to_subscribers(std::unordered_map<std::string, std::vector<std::pair<struct client*,bool>>> &topics,
                      struct special_buffer& tcp_buffer, std::string topic) {

    std::vector<std::pair<struct client*,bool>>* clients = &topics[topic];
    int size = sizeof(struct special_buffer) - MESSAGE_LEN +
              tcp_buffer.content_size + tcp_buffer.topic_size;
    for (int i = 0; i < clients -> size(); i++) {
        if ((*clients)[i].first -> online) {  // daca clientul este online, trimite direct mesaj catre client
            sendMessage((const char*)&tcp_buffer, (*clients)[i].first -> socket, size);
        } else if ((*clients)[i].second) {
            struct special_buffer* new_buffer = new special_buffer; // pune pachetele in coada pentru un client offline
            memcpy(new_buffer, &tcp_buffer, sizeof(struct special_buffer)); // si care are setat SF pe 1
            (*clients)[i].first -> messages.push(new_buffer);
        }   
    }
}

int main(int argc, char **argv) {
    int udp_socket, tcp_socket, new_socket;
    int port, res, option = 1, max_socket = 0;
    char command_buffer[COMMAND_LEN];
    int bytes_in_buffer = 0;

    struct sockaddr_in serv_addr, cli_addr;
    struct udp_message udp_message;
    struct special_buffer tcp_buffer;
    socklen_t socket_length = sizeof(serv_addr);
    fd_set all_sockets, temp_sockets, udp_sockets, tcp_sockets;
    std::string command;
    std::vector<struct client*> clients; 
    std::unordered_map<std::string, std::vector<std::pair<struct client*,bool>>> topics;
    FD_ZERO(&all_sockets);
    FD_ZERO(&temp_sockets);
    FD_ZERO(&udp_sockets);
    FD_ZERO(&tcp_sockets);

    if (argc != 2) {
        DIE(true, "Inccorect number of arguments\n");
        exit(-1);
    }


    port = atoi(argv[1]);
	DIE(port == 0, "Invalid port format\n");

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "Failed to create udp socket\n");

    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "Failed to create passive socket\n");

    // dezactivare alg neagle
    res = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));
    DIE(res < 0, "Failed to set no-delay on tcp socket.\n");


	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    res = bind(udp_socket, (struct sockaddr*)&serv_addr, socket_length);
    DIE(res < 0, "UDP bind error\n");

    res = bind(tcp_socket, (struct sockaddr*)&serv_addr, socket_length);
    DIE(res < 0, "TCP bind error\n");

    res = listen(tcp_socket, 5);
	DIE(res < 0, "Failed to listen on tcp socket\n");

    FD_SET(STDIN_FILENO, &all_sockets);
    FD_SET(udp_socket, &all_sockets);
    FD_SET(tcp_socket, &all_sockets);

    FD_SET(udp_socket, &udp_sockets);

    max_socket = std::max(udp_socket, tcp_socket);

    while (true)
    {   
        temp_sockets = all_sockets;
        res = select(max_socket + 1, &temp_sockets, NULL, NULL, NULL);
        DIE(res < 0, "Failed in selecting read sockets\n");
        for (int i = 0; i <= max_socket; i++) {
            if (FD_ISSET(i, &temp_sockets)) {
                if (i == tcp_socket) {
                    socket_length = sizeof(cli_addr);
                    new_socket = accept(tcp_socket, (struct sockaddr*)&cli_addr, &socket_length);
                    DIE(new_socket < 0, "Failed to establish a new TCP connection\n");

                    // dezactivare Neagle
                    res = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));
                    DIE(res < 0, "Failed to set no-delay on tcp socket.\n");

                    FD_SET(new_socket, &all_sockets);
                    FD_SET(new_socket, &tcp_sockets);

                    if (new_socket > max_socket) {
                        max_socket = new_socket;
                    }
                } else if (i == STDIN_FILENO) {
                        std::cin >> command;
                        if (command == "exit") {
                            for (int j = 0; j <= max_socket; j++) {
                                if (FD_ISSET(j, &all_sockets)) {
                                    close(j);
                                }
                            }
                            remove_clients(clients);
                            return 0;
                        }
                } else if(FD_ISSET(i, &udp_sockets)){ // primesc mesaj udp
                    memset(&udp_message, 0, sizeof(udp_message));
                    res = recvfrom(i, (char *)&udp_message, sizeof(struct udp_message), 0, (struct sockaddr*)&cli_addr, &socket_length);
                    convert_UDPmessage_to_buffer(tcp_buffer, udp_message, cli_addr.sin_addr.s_addr, cli_addr.sin_port);          // transforma mesajul udp(structura speciala) intr un sir de octeti(buffer)
                    send_to_subscribers(topics, tcp_buffer, std::string(udp_message.topic)); //trimite mesajul primit catre abonatii topicului
                } else if (FD_ISSET(i, &tcp_sockets)) { //primire comenzi de la client
                    memset(command_buffer , 0, COMMAND_LEN);
                    res = recv(i, command_buffer + bytes_in_buffer, COMMAND_LEN - bytes_in_buffer, 0);
                   
                    DIE(res < 0, "Failed to receive data from a tcp client\n");

                    if (res == 0) {
                        struct client* tcp_client = get_client(clients, i);
                        if (tcp_client == nullptr) { // tratarea cazului in care nu s - a reusit conectarea
                                                    // din cauza unui utilizator cu acelasi id
                            continue;
                        }

                        printf("Client (%s) disconnected\n", tcp_client -> id.c_str());
                        tcp_client -> socket = -1;
                        tcp_client -> online = false;
                        close(i);

                        FD_CLR(i, &all_sockets);
                        FD_CLR(i, &tcp_sockets);
                    } else {
                        // ma asigur de faptul ca citesc din buffer doar cata informatie a fost transmisa
                        bytes_in_buffer += res;
                        int command_length;
                        memcpy(&command_length, command_buffer, sizeof(int));

                        char current_command[COMMAND_LEN];
                    
                        memcpy(current_command, command_buffer + sizeof(int), command_length + 1);
                        memmove(command_buffer, command_buffer + command_length + sizeof(int),
                                 bytes_in_buffer - (command_length + sizeof(int)));
                        bytes_in_buffer -= (command_length + sizeof(int));
                        command = std::string(current_command);
                        do_command(clients, topics, command, i, cli_addr.sin_addr.s_addr, ntohs(cli_addr.sin_port)); // selectare comanda
                    }
                }
            }
        }

    }
    remove_clients(clients);

}