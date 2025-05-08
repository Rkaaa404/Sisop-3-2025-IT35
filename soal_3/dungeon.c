#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#define PORT 8080
#define MAX_INV 10

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
    Weapon inv[MAX_INV];
    int kills;
    int invCount;
} Player;

typedef struct {
    int health;
    int max_health;
    int gold;
} Enemy;

#include "shop.h" // disini soalnya struct weapon sama player biar terdefinisi dulu

void *handleClient(void *arg);  // ubah deklarasi
void showStats(Player *player, int client_sock);
void displayInventories(Player *player, int client_sock);
void battleMode(Player *player, int client_sock);
Enemy generateEnemy();
int calculateDamage(Player *player, int *isCritical);

int main() {
    int server_fd;
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
        if (pthread_create(&tid, NULL, handleClient, (void *)client_sock_ptr) != 0) {
            perror("Thread creation failed");
            close(new_socket);
            free(client_sock_ptr);
            continue;
        }

        pthread_detach(tid);
    }

    return 0;
}

void *handleClient(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);  // bebaskan memori setelah digunakan
    char buffer[2048];

    // inisiasi stats client
    Player player = {200, {"Fist", 0, 50, 1, 0, ""}, {}, 0, 1};
    player.inv[0] = player.weapon;
    player.inv[0].bought = 1;

    while (1) {
        snprintf(buffer, sizeof(buffer), "\n==== Welcome to the Dungeon! ====\n"
                                     "1. Show Player Stats\n"
                                     "2. Visit Shop (Buy Weapon)\n"
                                     "3. View Inventories and Pick Weapon\n"
                                     "4. Battle Mode\n"
                                     "5. Exit\n"
                                     "\033[0;32mChoose an option: \033[0m");
        send(client_sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
        if (recv(client_sock, buffer, sizeof(buffer), 0) <= 0) {
            close(client_sock);
            pthread_exit(NULL);
        }

        // Proses pilihan
        if (strncmp(buffer, "1", 1) == 0) {
            showStats(&player, client_sock);
            continue;
        } else if (strncmp(buffer, "2", 1) == 0) {
            displayShop(client_sock);
            buyWeapon(&player, client_sock);
            continue;
        } else if (strncmp(buffer, "3", 1) == 0) {
            displayInventories(&player, client_sock);
            continue;
        } else if (strncmp(buffer, "4", 1) == 0) {
            battleMode(&player, client_sock);
            continue;
        } else if (strncmp(buffer, "5", 1) == 0) {
            snprintf(buffer, sizeof(buffer), "\033[0;32mExiting... Goodbye!\033[0m");
            send(client_sock, buffer, strlen(buffer), 0);
            close(client_sock);
            return NULL;
        } else {
            snprintf(buffer, sizeof(buffer), "\033[0;32mInvalid option. Please choose again. \033[0m");
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
}

void showStats(Player *player, int client_sock) {
    char buffer[1024];
    if (player->weapon.boolPassive == 1){
        snprintf(buffer, sizeof(buffer), "\n==== Player Stats ====\n \033[0;33mGold:\033[0m %d | \033[0;34mWeapon:\033[0m %s | \033[0;31mBase Damage:\033[0m %d | \033[0;31mKills:\033[0m %d | \033[0;34mPassive:\033[0m %s \n", player->gold, player->weapon.name, player->weapon.damage, player->kills, player->weapon.passive);
    } else {
        snprintf(buffer, sizeof(buffer), "\n==== Player Stats ====\n \033[0;33mGold:\033[0m %d | \033[0;34mWeapon:\033[0m %s | \033[0;31mBase Damage:\033[0m %d | \033[0;31mKills:\033[0m %d \n", player->gold, player->weapon.name, player->weapon.damage, player->kills);
    }
    send(client_sock, buffer, strlen(buffer), 0);
}

void displayInventories(Player *player, int client_sock) {
    char buffer[2048] = {0};
    snprintf(buffer, sizeof(buffer), "\n==== Player's Inventory ====\n");
    for (int i = 0; i < player->invCount; i++) {
        if (player->inv[i].boolPassive) {
            if (strcmp(player->inv[i].name, player->weapon.name) == 0) {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                         "[ %d ] %s - \033[0;31mDamage:\033[0m %d \033[0;34m(Passive: %s)\033[0m \033[0;32m(EQUIPPED)\033[0m\n",
                         i, player->inv[i].name, player->inv[i].damage, player->inv[i].passive);
            } else {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                         "[ %d ] %s - \033[0;31mDamage:\033[0m %d \033[0;34m(Passive: %s)\033[0m\n",
                         i, player->inv[i].name, player->inv[i].damage, player->inv[i].passive);
            }
        } else {
            if (strcmp(player->inv[i].name, player->weapon.name) == 0) {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                         "[ %d ] %s - \033[0;31mDamage:\033[0m %d \033[0;32m(EQUIPPED)\033[0m\n",
                         i, player->inv[i].name, player->inv[i].damage);
            } else {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                         "[ %d ] %s - \033[0;31mDamage:\033[0m %d\n",
                         i, player->inv[i].name, player->inv[i].damage);
            }
        }
    }
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\033[0;32mChoose weapon number to use: \033[0m");
    send(client_sock, buffer, strlen(buffer), 0);

    memset(buffer, 0, sizeof(buffer));
    if (recv(client_sock, buffer, sizeof(buffer), 0) <= 0) {
        close(client_sock);
        pthread_exit(NULL); // atau return NULL
    }

    int choice = atoi(buffer);
    if ((choice > player->invCount - 1) || (choice < 0)){
        snprintf(buffer, sizeof(buffer), "\033[0;31mYou don't have that dude...\033[0m");
        send(client_sock, buffer, strlen(buffer), 0);
        return;
    } else {
        player->weapon = player->inv[choice];
    }
}

Enemy generateEnemy() {
    Enemy new_enemy;
    new_enemy.max_health = rand() % 151 + 50; // random 50-200 HP
    new_enemy.health = new_enemy.max_health; // buat track nyawa
    new_enemy.gold = rand() % 151 + 50; // random 50-200 gold reward
    return new_enemy;
}

int calculateDamage(Player *player, int *isCritical){
    int x = rand() % 5 + 1;
    int baseDamage = player->weapon.damage;
    int randomizedDamage = (baseDamage / x) - rand() % 5;
    int damageDealt = baseDamage + randomizedDamage;
    if (damageDealt <= 3) damageDealt = 3;
    
    // crit chance
    int y;
    if (strcmp(player->weapon.name, shop[4].name) == 0) {
        y = rand() % 2; // 0-1, 50% crit chance
    } else {
        y = rand() % 5; // 0-4, 20% crit chance
    }
    
    if (y == 1) {
        *isCritical = 1;
        return damageDealt * 2;
    }

    *isCritical = 0; 
    return damageDealt;
}

void battleMode(Player *player, int client_sock){ 
    char buffer[2048];
    Enemy currentEnemy = generateEnemy();
    snprintf(buffer, sizeof(buffer), "\n==== Battle Started ====\nAn enemy approaches!\n");
    send(client_sock, buffer, strlen(buffer), 0);

    while (1){
        memset(buffer, 0, sizeof(buffer));

        if (currentEnemy.health > 0){
            int health_bar_length = 20;
            int filled_length = (int)(((float)currentEnemy.health / currentEnemy.max_health) * health_bar_length);
            if (filled_length < 0) filled_length = 0;

            snprintf(buffer, sizeof(buffer), "\nEnemy Health: [");
            for (int i = 0; i < health_bar_length; i++) {
                if (i < filled_length) {
                    strcat(buffer, "\033[0;31m#\033[0m");
                } else {
                    strcat(buffer, " ");
                }
            }
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "] (%d/%d)\n", currentEnemy.health < 0 ? 0 : currentEnemy.health, currentEnemy.max_health);
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "Type \033[0;31m'attack'\033[0m to attack or \033[0;32m'exit'\033[0m to leave the battle \n");
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\n > ");
            send(client_sock, buffer, strlen(buffer), 0);
        }

        memset(buffer, 0, sizeof(buffer));
        if (recv(client_sock, buffer, sizeof(buffer), 0) <= 0) {
            close(client_sock);
            pthread_exit(NULL); // atau return NULL
        }
        
        if (strncmp(buffer, "attack", 6) == 0){
            memset(buffer, 0, sizeof(buffer));
            int isCritical = 0;
            int turnDamage = calculateDamage(player, &isCritical);

            if (strcmp(player->weapon.name, shop[4].name) == 0){
                snprintf(buffer, sizeof(buffer), "\n\033[0;34m==== Passive Activated ==== \nBerserker Fury increased the crit chance by 30%%...\033[0m \n");
                send(client_sock, buffer, strlen(buffer), 0);
                memset(buffer, 0, sizeof(buffer));
            }

            if (isCritical){
                snprintf(buffer, sizeof(buffer), "\n==== Critical Hit! ====\nYou dealt \033[0;31m%d\033[0m damage!\n", turnDamage);
            } else {
                snprintf(buffer, sizeof(buffer), "\nYou dealt \033[0;31m%d\033[0m damage!\n", turnDamage);
            }
            send(client_sock, buffer, strlen(buffer), 0);

            currentEnemy.health -= turnDamage;

            int healthPercentage = currentEnemy.health * 100 / currentEnemy.max_health;

            if ((strcmp(player->weapon.name, shop[0].name) == 0) && (healthPercentage <= 10)){
                currentEnemy.health = 0;
                snprintf(buffer, sizeof(buffer), "\n\033[0;34m==== Passive Activated ==== \nRandom Stick just did an auto kill, well that's great...\033[0m \n");
                send(client_sock, buffer, strlen(buffer), 0);
            }

            if (currentEnemy.health <= 0){
                snprintf(buffer, sizeof(buffer), "\n\033[0;31mEnemy defeated!\033[0m \n");
                send(client_sock, buffer, strlen(buffer), 0);

                player->gold += currentEnemy.gold;
                player->kills++;

                snprintf(buffer, sizeof(buffer), "\n\033[0;34m====\033[0m \033[0;33mReward\033[0m \033[0;34m====\033[0m\n You received \033[0;33m%d\033[0m gold!\n", currentEnemy.gold);
                send(client_sock, buffer, strlen(buffer), 0);

                currentEnemy = generateEnemy();
                snprintf(buffer, sizeof(buffer), "\n\033[0;34mA new enemy appears!\033[0m \n");
                send(client_sock, buffer, strlen(buffer), 0);
                continue;
            }
        } else if (strncmp(buffer, "exit", 4) == 0) {
            break;
        } else {
            snprintf(buffer, sizeof(buffer), "\033[0;31mThat's not an option sir\033[0m\n");
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
}
