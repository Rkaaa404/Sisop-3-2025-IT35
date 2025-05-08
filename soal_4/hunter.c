#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50

struct Hunter {
    char username[50];
    int level, exp, atk, hp, def, banned;
    key_t shm_key;
};

struct Dungeon {
    char name[50];
    int min_level, exp, atk, hp, def;
    key_t shm_key;
};

struct SystemData {
    struct Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    struct Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
    int current_notification_index;
};

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

void displayMenu(struct Hunter *h){
    printf("\n==== %s's Menu ====\n", h->username);
}

int register_or_login(struct SystemData *sys_data, struct Hunter **my_hunter) {
    char uname[50];
    printf("Masukkan username: ");
    scanf("%s", uname);

    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, uname) == 0) {
            int id = shmget(sys_data->hunters[i].shm_key, sizeof(struct Hunter), 0666);
            *my_hunter = shmat(id, NULL, 0);
            printf("Login berhasil.\n");
            return 1;
        }
    }

    if (sys_data->num_hunters >= MAX_HUNTERS) return 0;

    struct Hunter new_hunter;
    strcpy(new_hunter.username, uname);
    new_hunter.level = 1;
    new_hunter.exp = 0;
    new_hunter.atk = 10;
    new_hunter.hp = 100;
    new_hunter.def = 5;
    new_hunter.banned = 0;
    new_hunter.shm_key = ftok("/tmp", (rand() % 100) + 101);

    int id = shmget(new_hunter.shm_key, sizeof(struct Hunter), IPC_CREAT | 0666);
    *my_hunter = shmat(id, NULL, 0);
    memcpy(*my_hunter, &new_hunter, sizeof(struct Hunter));
    sys_data->hunters[sys_data->num_hunters++] = new_hunter;
    printf("Registrasi berhasil.\n");
    return 1;
}

void show_available_dungeons(struct SystemData *sys_data, struct Hunter *user) {
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        struct Dungeon *d = &sys_data->dungeons[i];
        if (user->level >= d->min_level) {
            printf("[%d] %s (Lv%d) => EXP:%d ATK:%d HP:%d DEF:%d\n",
                i , d->name, d->min_level, d->exp, d->atk, d->hp, d->def);
        }
    }
}

void raid_dungeon(struct SystemData *sys_data, struct Hunter *user) {
    show_available_dungeons(sys_data, user);
    int choice;
    printf("Pilih dungeon (nomor): ");
    scanf("%d", &choice);
    choice--;
    if (choice < 0 || choice >= sys_data->num_dungeons) return;

    struct Dungeon d = sys_data->dungeons[choice];
    if (user->level < d.min_level) {
        printf("Level not met!\n");
        return;
    }

    user->exp += d.exp;
    user->atk += d.atk;
    user->hp += d.hp;
    user->def += d.def;

    if (user->exp >= 500) {
        user->exp = 0;
        user->level++;
        printf("Levelled up to %d!\n", user->level);
    }

    for (int i = choice; i < sys_data->num_dungeons - 1; i++)
        sys_data->dungeons[i] = sys_data->dungeons[i + 1];
    sys_data->num_dungeons--;

    printf("Dungeon conquered!\n");
}

int calc_power(struct Hunter *h) {
    return h->atk + h->hp + h->def;
}

void battle(struct SystemData *sys_data, struct Hunter *user) {
    if (sys_data->num_hunters == 0){
        return;
    }
    
    printf("\n====Opponent List====\n");
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, user->username) != 0) {
            printf("%s (Power %d)\n", sys_data->hunters[i].username,
                   calc_power(&sys_data->hunters[i]));
        }
    }
    char target[50];
    printf("Choose opponent: ");
    scanf("%s", target);

    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, target) == 0) {
            int id = shmget(sys_data->hunters[i].shm_key, sizeof(struct Hunter), 0666);
            struct Hunter *enemy = shmat(id, NULL, 0);
            int my_power = calc_power(user);
            int enemy_power = calc_power(enemy);
            if (my_power >= enemy_power) {
                user->atk += enemy->atk;
                user->hp += enemy->hp;
                user->def += enemy->def;
                shmctl(id, IPC_RMID, NULL);
                for (int j = i; j < sys_data->num_hunters - 1; j++)
                    sys_data->hunters[j] = sys_data->hunters[j + 1];
                sys_data->num_hunters--;
                printf("You won, absorbing %s stats!\n", enemy->username);
            } else {
                int myid = shmget(user->shm_key, sizeof(struct Hunter), 0666);
                shmctl(myid, IPC_RMID, NULL);
                for (int j = 0; j < sys_data->num_hunters; j++) {
                    if (strcmp(sys_data->hunters[j].username, user->username) == 0) {
                        for (int k = j; k < sys_data->num_hunters - 1; k++)
                            sys_data->hunters[k] = sys_data->hunters[k + 1];
                        sys_data->num_hunters--;
                        break;
                    }
                }
                printf("You lose, now your account is deleted.. huhu..\n");
                exit(0);
            }
            return;
        }
    }
    printf("Hunter not found.\n");
}

int main() {
    srand(getpid());
    key_t key = get_system_key();
    int sys_id = shmget(key, sizeof(struct SystemData), 0666);
    if (sys_id == -1) {
        perror("Failed to get shared memory...");
        return 1;
    }

    struct SystemData *sys_data = shmat(sys_id, NULL, 0);
    struct Hunter *user = NULL;

    // Hunter Main Menu
    while (1) {
        int choice;
        printf("\n==== Main Menu ====\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 1 || choice == 2) {
            if (register_or_login(sys_data, &user)) break;
            else printf("Gagal melakukan aksi.\n");
        } else if (choice == 3) {
            return 0;
        } else {
            printf("Pilihan tidak valid.\n");
        }
    }

    // Hunter system menu
    while (1) {
        int choice;
        printf("\n==== Hunter Menu ====\n");
        displayMenu(user);
        printf("1. Dungeon List  2. Dungeon Raid  3. Hunters Battle  4. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 1) {
            show_available_dungeons(sys_data, user);
        } else if (choice == 2) {
            if (user->banned) {
                printf("You're banned\n");
                sleep(2);
                continue;
            }
            raid_dungeon(sys_data, user); 
        } else if (choice == 3) {
            if (user->banned) {
                printf("You're banned\n");
                sleep(2);
                continue;
            }
            battle(sys_data, user); 
        } else if (choice == 4) {
            break;
        } else {
            printf("Wrong input\n");
        }
    }

    return 0;
}
