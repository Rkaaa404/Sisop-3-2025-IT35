#ifndef SHOP_H
#define SHOP_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

Weapon shop[] = {
    {"Random Stick", 300, 80, 0, 1, "Auto kill when enemy HP at 10%"},
    {"Sword", 1000, 200, 0, 0,""},
    {"Axe", 500, 130, 0, 0,""},
    {"Amboi", 350, 80, 0, 0,""},
    {"Berserker Fury", 400, 100, 0, 1, "+30% Crit Chance"}
};

static int shop_size = sizeof(shop) / sizeof(shop[0]);

void displayShop(int client_sock) {
    char buffer[1024] = "\n==== Weapon Shop ====\n";
    for (int i = 0; i < shop_size; i++) {
        if (shop[i].boolPassive == 1 && shop[i].passive[0] != '\0') {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                     "[ %d ] %s - \033[0;33mPrice:\033[0m %d, \033[0;31mDamage:\033[0m %d \033[0;34m(Passive: %s)\033[0m\n",
                     i + 1, shop[i].name, shop[i].price, shop[i].damage, shop[i].passive);
        } else {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                     "[ %d ] %s - \033[0;33mPrice:\033[0m %d, \033[0;31mDamage:\033[0m %d\n",
                     i + 1, shop[i].name, shop[i].price, shop[i].damage);
        }
    }    
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\033[0;32mChoose weapon number to buy: \033[0m");
    send(client_sock, buffer, strlen(buffer), 0);
    
}

void buyWeapon(Player *player, int client_sock){
    char buffer[1024] = {0};
    if (recv(client_sock, buffer, sizeof(buffer), 0) <= 0) {
        close(client_sock);
        pthread_exit(NULL); 
    }

    Weapon *w = NULL;
    if (strncmp(buffer, "1", 1) == 0){
        w = &shop[0];
    } else if (strncmp(buffer, "2", 1) == 0){
        w = &shop[1];
    } else if (strncmp(buffer, "3", 1) == 0){
        w = &shop[2];
    } else if (strncmp(buffer, "4", 1) == 0){
        w = &shop[3];
    } else if (strncmp(buffer, "5", 1) == 0){
        w = &shop[4];
    } else {
        snprintf(buffer, sizeof(buffer), "\033[0;31mWhat you gonna buy dude?\033[0m \n");
        send(client_sock, buffer, strlen(buffer), 0);
        return;
    }

    if (w->bought) {
        snprintf(buffer, sizeof(buffer), "\033[0;31mYou already own this weapon.\033[0m \n");
        send(client_sock, buffer, strlen(buffer), 0);
        return;
    }

    if (player->gold < w->price) {
        snprintf(buffer, sizeof(buffer), "\033[0;31mNot enough gold.\033[0m \n");
        send(client_sock, buffer, strlen(buffer), 0);
        return;
    }

    w->bought = 1;
    player->inv[player->invCount] = *w;
    player->invCount += 1;    
    player->gold -= w->price;
    snprintf(buffer, sizeof(buffer), "You bought %s \n", w->name);
    send(client_sock, buffer, strlen(buffer), 0);
}
#endif
