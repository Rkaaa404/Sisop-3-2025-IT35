#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    char name[50];
    int price;
    int damage;
    int bought;
    int boolPassive;
    char passive[100];
} Weapon;

typedef struct {
    int gold;
    Weapon weapon;
    Weapon inv[10];
    int kills;
} Player;

typedef struct {
    int health;
    int max_health;
    int gold;
} Enemy;

#include "shop.h"

void *handle_client(void *arg);
void show_player_stats(Player *player, int client_sock);
void display_shop(int client_sock);
void display_inv(int client_sock, Player *player);
void battle_mode(Player *player, int client_sock);
Enemy generate_enemy();
int calculate_damage(Player *player, int *is_critical);

#define PORT 8080

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    srand(time(NULL));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for clients...\n");

    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected.\n");

        int *client_sock_ptr = malloc(sizeof(int));
        if (client_sock_ptr == NULL) {
            perror("Memory allocation failed");
            close(new_socket);
            continue;
        }

        *client_sock_ptr = new_socket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)client_sock_ptr) != 0) {
            perror("Thread creation failed");
            close(new_socket);
            free(client_sock_ptr);
            continue;
        }

        pthread_detach(tid);
    }

    return 0;
}

void *handle_client(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);
    Player player = {200, {"Fist", 0, 5, 1, 0, ""}, {{"", 0, 0, 0, 0, ""}}, 0};
    player.inv[0] = player.weapon;
    player.inv[0].bought = 1;

    while (1) {
        char buffer[1024];
        int choice;

        snprintf(buffer, sizeof(buffer), "\n====Welcome to the Dungeon!====\n"
                                         "1. Show Player Stats\n"
                                         "2. Visit Shop (Buy Weapon)\n"
                                         "3. View Inventories and Pick Weapon\n"
                                         "4. Battle Mode\n"
                                         "5. Exit\n"
                                         "\033[0;32mChoose an option: \033[0m");
        send(client_sock, buffer, strlen(buffer), 0);

        ssize_t bytes_received = recv(client_sock, &choice, sizeof(int), 0);
        if (bytes_received <= 0) {
            printf("Client disconnected.\n");
            close(client_sock);
            pthread_exit(NULL);
        }

        switch (choice) {
            case 1:
                show_player_stats(&player, client_sock);
                break;
            case 2: {
                display_shop(client_sock);
                int weapon_choice;
                bytes_received = recv(client_sock, &weapon_choice, sizeof(int), 0);
                 if (bytes_received <= 0) {
                    printf("Client disconnected.\n");
                    close(client_sock);
                    pthread_exit(NULL);
                }
                buy_weapon(&player, weapon_choice - 1, client_sock);
                break;
            }
            case 3: {
                display_inv(client_sock, &player);
                int inv_choice;
                bytes_received = recv(client_sock, &inv_choice, sizeof(int), 0);
                 if (bytes_received <= 0) {
                    printf("Client disconnected.\n");
                    close(client_sock);
                    pthread_exit(NULL);
                }
                if (inv_choice >= 0 && inv_choice < 10 && player.inv[inv_choice].bought == 1) {
                    player.weapon = player.inv[inv_choice];
                    snprintf(buffer, sizeof(buffer), "\033[0;32mYou equipped %s!\033[0m\n", player.weapon.name);
                } else {
                    snprintf(buffer, sizeof(buffer), "\033[0;31mInvalid inventory choice.\033[0m\n");
                }
                send(client_sock, buffer, strlen(buffer), 0);
                break;
            }
            case 4:
                battle_mode(&player, client_sock);
                break;
            case 5:
                snprintf(buffer, sizeof(buffer), "\033[0;31mDisconnecting...\033[0m\n");
                send(client_sock, buffer, strlen(buffer), 0);
                close(client_sock);
                pthread_exit(NULL);
            default:
                snprintf(buffer, sizeof(buffer), "\033[0;31mInvalid choice. Try again.\033[0m\n");
                send(client_sock, buffer, strlen(buffer), 0);
                break;
        }
    }
}

void show_player_stats(Player *player, int client_sock) {
    char buffer[1024];
    if (player->weapon.boolPassive == 1 && player->weapon.passive[0] != '\0'){
        snprintf(buffer, sizeof(buffer), "\n====Player Stats:====\n \033[0;33mGold:\033[33m %d | \033[0;34mWeapon:\033[34m %s | \033[0;31mBase Damage:\033[0m %d | Kills: %d | Passive: %s \n", player->gold, player->weapon.name, player->weapon.damage, player->kills, player->weapon.passive);
    } else {
        snprintf(buffer, sizeof(buffer), "\n====Player Stats:====\n \033[0;33mGold:\033[33m %d | \033[0;34mWeapon:\033[34m %s | \033[0;31mBase Damage:\033[0m %d | Kills: %d \n", player->gold, player->weapon.name, player->weapon.damage, player->kills);
    }
    send(client_sock, buffer, strlen(buffer), 0);
}

void display_shop(int client_sock) {
    char buffer[1024] = "\n====Weapon Shop:====\n";
    for (int i = 0; i < shop_size; i++) {
        if (shop[i].boolPassive == 1 && shop[i].passive[0] != '\0'){
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "[ %d ] %s - \033[0;33mPrice:\033[33m %d, \033[0;31mDamage:\033[0m %d  (Passive: %s) \n", i+1, shop[i].name, shop[i].price, shop[i].damage, shop[i].passive);
        } else {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "[ %d ] %s - \033[0;33mPrice:\033[33m %d, \033[0;31mDamage:\033[0m %d \n", i+1, shop[i].name, shop[i].price, shop[i].damage);
        }
    }
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\033[0;32mChoose weapon number to buy: \033[0m");
    send(client_sock, buffer, strlen(buffer), 0);
}

void display_inv(int client_sock, Player *player){
    char buffer[1024] = "\n====Inventory====:\n";
    int items_in_inv = 0;
    for (int i = 0; i < 10; i++) {
        if (player->inv[i].bought == 1){
            items_in_inv++;
            if (player->inv[i].boolPassive == 1 && player->inv[i].passive[0] != '\0'){
                if (strcmp(player->inv[i].name, player->weapon.name) == 0){
                    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\033[0;32m[ %d ] %s (%s) (EQUIPPED)\033[0m\n", i, player->inv[i].name, player->inv[i].passive);
                }else {
                    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "[ %d ] %s (%s) \n", i, player->inv[i].name, player->inv[i].passive);
                }
            } else {
                if (strcmp(player->inv[i].name, player->weapon.name) == 0){
                    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\033[0;32m[ %d ] %s (EQUIPPED) \033[0m\n", i, player->inv[i].name);
                }else{
                    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "[ %d ] %s \n", i, player->inv[i].name);
                }
            }
        }
    }
    if (items_in_inv == 0) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "Inventory is empty.\n");
    }
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\033[0;32mChoose weapon number to use: \033[0m");
    send(client_sock, buffer, strlen(buffer), 0);
}

Enemy generate_enemy() {
    Enemy new_enemy;
    new_enemy.max_health = rand() % 151 + 50;
    new_enemy.health = new_enemy.max_health;
    new_enemy.gold = rand() % 51 + 20;
    return new_enemy;
}

int calculate_damage(Player *player, int *is_critical) {
    int base_damage = player->weapon.damage;
    int random_variation = (rand() % (base_damage / 5 + 1)) - (base_damage / 10);
    int damage_dealt = base_damage + random_variation;

    if (damage_dealt < 1) {
        damage_dealt = 1;
    }

    *is_critical = 0;

    if ((rand() % 100) < 10) {
        *is_critical = 1;
    }

    if (player->weapon.boolPassive == 1) {
        if (strcmp(player->weapon.name, "Random Stick") == 0) {
            if ((rand() % 100) < 5) {
                damage_dealt += 1;
            }
        } else if (strcmp(player->weapon.name, "Berserker Fury") == 0) {
            if ((rand() % 100) < 30) {
                *is_critical = 1;
            }
        }
    }

    if (*is_critical) {
        damage_dealt *= 2;
    }

    return damage_dealt;
}

void battle_mode(Player *player, int client_sock) {
    char buffer[2048];
    Enemy current_enemy = generate_enemy();
    int in_battle = 1;

    snprintf(buffer, sizeof(buffer), "\n====Battle Mode====\nAn enemy approaches!\n");
    send(client_sock, buffer, strlen(buffer), 0);

    while (in_battle) {
        int health_bar_length = 20;
        int filled_length = (int)(((float)current_enemy.health / current_enemy.max_health) * health_bar_length);
        if (filled_length < 0) filled_length = 0;

        snprintf(buffer, sizeof(buffer), "\nEnemy Health: [");
        for (int i = 0; i < health_bar_length; i++) {
            if (i < filled_length) {
                strcat(buffer, "\033[0;31m#\033[0m");
            } else {
                strcat(buffer, "-");
            }
        }
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "] (%d/%d)\n", current_enemy.health < 0 ? 0 : current_enemy.health, current_enemy.max_health);
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "Options: attack | exit\n");
        send(client_sock, buffer, strlen(buffer), 0);

        char action[50];
        ssize_t bytes_received = recv(client_sock, action, sizeof(action) - 1, 0);
        if (bytes_received <= 0) {
             printf("Client disconnected during battle.\n");
            close(client_sock);
            pthread_exit(NULL);
        }
        action[bytes_received] = '\0';
        action[strcspn(action, "\n")] = 0;

        if (strcmp(action, "attack") == 0) {

            if (player->weapon.boolPassive == 1) {
                 if (strcmp(player->weapon.name, "Random Stick") == 0) {
                     if ((rand() % 100) < 5) {
                        snprintf(buffer, sizeof(buffer), "\n====Passive Active====\nRandom Stick: +1 Damage\n");
                        send(client_sock, buffer, strlen(buffer), 0);
                     }
                 } else if (strcmp(player->weapon.name, "Berserker Fury") == 0) {
                     if ((rand() % 100) < 30) {
                        snprintf(buffer, sizeof(buffer), "\n====Passive Active====\nBerserker Fury: +30%% Crit Chance activated!\n");
                        send(client_sock, buffer, strlen(buffer), 0);
                     }
                 }
            }

            int is_critical = 0;
            int damage_dealt = calculate_damage(player, &is_critical);

            snprintf(buffer, sizeof(buffer), "You attack the enemy!\n");
            send(client_sock, buffer, strlen(buffer), 0);
            usleep(500000);

            if (is_critical) {
                snprintf(buffer, sizeof(buffer), "\033[0;33mCritical Hit!\033[0m\n");
                send(client_sock, buffer, strlen(buffer), 0);
                usleep(500000);
            }

            snprintf(buffer, sizeof(buffer), "You dealt %d damage!\n", damage_dealt);
            send(client_sock, buffer, strlen(buffer), 0);
            usleep(500000);

            current_enemy.health -= damage_dealt;

            if (current_enemy.health <= 0) {
                snprintf(buffer, sizeof(buffer), "\nEnemy defeated!\n");
                send(client_sock, buffer, strlen(buffer), 0);
                usleep(500000);

                player->gold += current_enemy.gold;
                player->kills++;
                snprintf(buffer, sizeof(buffer), "You received %d gold and %d kill!\n", current_enemy.gold, 1);
                send(client_sock, buffer, strlen(buffer), 0);
                usleep(500000);

                current_enemy = generate_enemy();
                snprintf(buffer, sizeof(buffer), "\nA new enemy appears!\n");
                send(client_sock, buffer, strlen(buffer), 0);
                usleep(500000);

            }

        } else if (strcmp(action, "exit") == 0) {
            snprintf(buffer, sizeof(buffer), "Exiting Battle Mode.\n");
            send(client_sock, buffer, strlen(buffer), 0);
            in_battle = 0;
        } else {
            snprintf(buffer, sizeof(buffer), "\033[0;31mInvalid action. Choose 'attack' or 'exit'.\033[0m\n");
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
}
