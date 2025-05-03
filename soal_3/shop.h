#ifndef SHOP_H
#define SHOP_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

Weapon shop[] = {
    {"Random Stick", 100, 5, 0, 1, "5% chance to give +1 damage"},
    {"Sword", 300, 50, 0, 0,""},
    {"Axe", 150, 15, 0, 0,""},
    {"Amboi", 100, 10, 0, 0,""},
    {"Berserker Fury", 120, 8, 0, 1, "+30% Crit Chance"}
};

static int shop_size = sizeof(shop) / sizeof(shop[0]);

void buy_weapon(Player *player, int choice, int client_sock) {
    char buffer[1024];
    if (choice < 0 || choice >= shop_size) {
        snprintf(buffer, sizeof(buffer), "\033[0;31mInvalid choice.\033[0m \n");
        send(client_sock, buffer, strlen(buffer), 0);
        return;
    }

    Weapon *w = &shop[choice];

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

    snprintf(buffer, sizeof(buffer), "\033[0;31mWeapon purchased successfully!\033[0m \n");
    send(client_sock, buffer, strlen(buffer), 0);
}

#endif
