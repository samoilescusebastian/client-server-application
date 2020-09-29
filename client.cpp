#include "client.h"
#include "message.h"
// eliberarea memoriei pentru clientii alocati dinamic
void remove_clients(std::vector<struct client*>& clients) {
    struct special_buffer* buffer = nullptr;
    struct client* client = nullptr;
    for (int i = 0; i < clients.size(); i++){
        client = clients[i];
        while (!client -> messages.empty()) {
            buffer = client -> messages.front();
            client -> messages.pop();
            delete[] buffer;
        }
        delete[] client;
    }
}
struct client* get_client(std::vector<struct client*>& clients, std::string id) {
    for (int i = 0; i < clients.size(); i++) {
        if (clients[i] -> id == id) {
            return clients[i];
        }
    }
    return nullptr;
}
struct client* get_client(std::vector<struct client*>& clients, int socket) {

    for (int i = 0; i < clients.size(); i++) {
        if (clients[i] -> socket == socket) {
            return clients[i];
        }
    }
    return nullptr;
}
bool is_subscribed(std::vector<std::pair<struct client*,bool>>* topic_client , client* client) {
    for (int i = 0; i < topic_client -> size(); i++) {
        if ((*topic_client)[i].first == client) {
            return true;
        }
    }
    return false;
}