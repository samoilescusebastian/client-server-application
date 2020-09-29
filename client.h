#ifndef _CLIENT_H_
#define _CLIENT_H_
#include <iostream>
#include <vector>
#include <queue>
// structura pentru a retine informatii client
struct client
{
    std::string id;
    int socket;
    bool online;
    std::queue<struct special_buffer*> messages; // mesajele primite in modul offline
    client(std::string client_id, int client_socket) {
        id = client_id;
        socket = client_socket;
        online = true;
    }
};
bool is_subscribed(std::vector<std::pair<struct client*,bool>>* , client*);
void remove_clients(std::vector<struct client*>&);
struct client* get_client(std::vector<struct client*>& clients, std::string id);
struct client* get_client(std::vector<struct client*>&, int);

#endif // !_CLIENT_H_
