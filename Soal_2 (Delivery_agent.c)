#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define MAX_ORDERS 100
#define NAME_LEN 100

typedef struct {
    char nama[NAME_LEN];
    char alamat[NAME_LEN];
    char jenis[NAME_LEN];
    int delivered;
    char agen[NAME_LEN];
} Order;

Order *orders;
int *order_count;

void write_log(const char *agent, const char *nama, const char *alamat) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", t);

    fprintf(log, "[%s] [AGENT %s] Express package delivered to %s in %s\n",
            timestamp, agent, nama, alamat);
    fclose(log);
}

void *agent_thread(void *arg) {
    char *agent = (char *)arg;

    while (1) {
        int found = 0;

        for (int i = 0; i < *order_count; i++) {
            if (strcmp(orders[i].jenis, "Express") == 0 && orders[i].delivered == 0) {
                orders[i].delivered = 1;
                strcpy(orders[i].agen, agent);
                write_log(agent, orders[i].nama, orders[i].alamat);
                printf("Pesanan express atas nama %s sudah dikirim oleh Agent %s. \n", orders[i].nama, agent);
                sleep(1);
                found = 1;
                break;
            }
        }

        if (!found) break;
    }

    return NULL;
}

void load_csv_if_needed() {
    if (*order_count > 0) {
        printf("Shared memory sudah memiliki data. Lewati loading CSV.\n");
        return;
    }

    FILE *fp = fopen("delivery_order.csv", "r");
    if (!fp) {
        perror("File delivery_order.csv tidak ditemukan");
        exit(1);
    }

    char line[300];
    *order_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *nama = strtok(line, ",\n");
        char *alamat = strtok(NULL, ",\n");
        char *jenis = strtok(NULL, ",\n");

        if (nama && alamat && jenis) {
            strcpy(orders[*order_count].nama, nama);
            strcpy(orders[*order_count].alamat, alamat);
            strcpy(orders[*order_count].jenis, jenis);
            orders[*order_count].delivered = 0;
            strcpy(orders[*order_count].agen, "-");
            (*order_count)++;
        }
    }

    fclose(fp);
    printf("CSV berhasil dimuat ke shared memory oleh delivery_agent.\n");
}

int main() {
    key_t key1 = ftok("delivery_order.csv", 65);
    key_t key2 = ftok("delivery_order.csv", 66);
    int shmid1 = shmget(key1, sizeof(Order) * MAX_ORDERS, 0666 | IPC_CREAT);
    int shmid2 = shmget(key2, sizeof(int), 0666 | IPC_CREAT);

    orders = (Order *)shmat(shmid1, NULL, 0);
    order_count = (int *)shmat(shmid2, NULL, 0);

    load_csv_if_needed();

    pthread_t a, b, c;
    pthread_create(&a, NULL, agent_thread, "A");
    pthread_create(&b, NULL, agent_thread, "B");
    pthread_create(&c, NULL, agent_thread, "C");

    pthread_join(a, NULL);
    pthread_join(b, NULL);
    pthread_join(c, NULL);

    shmdt(orders);
    shmdt(order_count);

    return 0;
}
