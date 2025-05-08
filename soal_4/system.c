#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

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

void generate_random_dungeon(struct SystemData *sys_data) {
    if (sys_data->num_dungeons >= MAX_DUNGEONS) return;

    const char *dungeon_names[] = {
        "Double Dungeon",
        "Demon Castle",
        "Pyramid Dungeon",
        "Red Gate Dungeon",
        "Hunters Guild Dungeon",
        "Busan A-Rank Dungeon",
        "Insects Dungeon",
        "Goblins Dungeon",
        "D-Rank Dungeon",
        "Gwanak Mountain Dungeon",
        "Hapjeong Subway Station Dungeon"
    };
    int name_count = sizeof(dungeon_names) / sizeof(dungeon_names[0]);

    struct Dungeon d;
    strcpy(d.name, dungeon_names[rand() % name_count]);
    d.min_level = rand() % 5 + 1;
    d.atk = rand() % 51 + 100;
    d.hp = rand() % 51 + 50;
    d.def = rand() % 26 + 25;
    d.exp = rand() % 151 + 150;
    d.shm_key = ftok("/tmp", rand() % 100 + 1);

    sys_data->dungeons[sys_data->num_dungeons++] = d;

    printf("Generated Dungeon: %s Lv%d (EXP %d ATK %d HP %d DEF %d)\n",
           d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
}

void show_all_player(struct SystemData *sys_data){
    printf("\n==== Player List====\n");
    for (int i = 0; i < sys_data->num_hunters; i++) {
        struct Hunter *h = &sys_data->hunters[i];
        printf("%s Lv.%d | EXP: %d | ATK: %d | HP: %d | DEF: %d\n",
            h->username, h->level, h->exp, h->atk, h->hp, h->def, h->banned ? " [BANNED]" : "");
    }
}
void show_all_dungeons(struct SystemData *sys_data) {
    printf("\n==== Dungeon List ====\n");
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        struct Dungeon *d = &sys_data->dungeons[i];
        printf("[%d] %s | Min Lv: %d | EXP:%d ATK:%d HP:%d DEF:%d | key: %d\n",
            i+1, d->name, d->min_level, d->exp, d->atk, d->hp, d->def, d->shm_key);
    }
}

void setBanStatus(struct SystemData *sys_data, const char *username) {
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, username) == 0) {
            if (sys_data->hunters[i].banned = 1){
                sys_data->hunters[i].banned = 0;
                printf("%s is now unbanned\n", username);
                return;
            }
            sys_data->hunters[i].banned = 1;
            printf("%s is now banned\n", username);
            return;
        }
    }
    printf("Hunter not found\n");
}

void resetHunter(struct SystemData *sys_data, const char *username) {
    for (int i = 0; i < sys_data->num_hunters; i++) {
        if (strcmp(sys_data->hunters[i].username, username) == 0) {
            struct Hunter *h = &sys_data->hunters[i];
            h->level = 1; h->exp = 0; h->atk = 10; h->hp = 100; h->def = 5;
            printf("%s stats has been reset.\n", username);
            return;
        }
    }
    printf("Hunter not found\n");
}

void cleanAllMemory(struct SystemData *sys_data) {
    for (int i = 0; i < sys_data->num_dungeons; i++) {
        int id = shmget(sys_data->dungeons[i].shm_key, sizeof(struct Dungeon), 0666);
        if (id != -1) shmctl(id, IPC_RMID, NULL);
    }
    for (int i = 0; i < sys_data->num_hunters; i++) {
        int id = shmget(sys_data->hunters[i].shm_key, sizeof(struct Hunter), 0666);
        if (id != -1) shmctl(id, IPC_RMID, NULL);
    }
    int sys_id = shmget(get_system_key(), sizeof(struct SystemData), 0666);
    if (sys_id != -1) shmctl(sys_id, IPC_RMID, NULL);
    printf("All shared memory segments removed.\n");
}

int main() {
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    struct SystemData *sys_data = shmat(shmid, NULL, 0);
    srand(time(NULL));

    while (1) {
        printf("\n====System Menu====\n 1.Hunter Info \n 2.Generate Dungeon\n 3.Show Dungeons\n 4.Ban\n 5.Reset\n 6.Exit\n");
        printf("Choice: ");
        int cmd;
        scanf("%d", &cmd);
        if (cmd == 1) show_all_player(sys_data);
        else if (cmd == 2) generate_random_dungeon(sys_data);
        else if (cmd == 3) show_all_dungeons(sys_data);
        else if (cmd == 4) {
            char u[50]; scanf("%s", u); setBanStatus(sys_data, u);
        } 
        else if (cmd == 5) {
            char u[50]; scanf("%s", u); resetHunter(sys_data, u);
        } else if (cmd == 6) break;
    }

    cleanAllMemory(sys_data);
    return 0;
}
