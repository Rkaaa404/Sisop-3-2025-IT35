#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void write_log(const char *agent, const char *nama, const char *alamat, const char *jenis) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", t);

    fprintf(log, "[%s] [AGENT %s] %s package delivered to %s in %s\n",
            timestamp, agent, jenis, nama, alamat);
    fclose(log);
}

int main(int argc, char *argv[]) {
    key_t key1 = ftok("delivery_order.csv", 65);
    key_t key2 = ftok("delivery_order.csv", 66);
    int shmid1 = shmget(key1, sizeof(Order) * MAX_ORDERS, 0666);
    int shmid2 = shmget(key2, sizeof(int), 0666);

    if (shmid1 == -1 || shmid2 == -1) {
        perror("Shared memory belum tersedia. Jalankan ./delivery_agent dulu.");
        exit(1);
    }

    Order *orders = (Order *)shmat(shmid1, NULL, 0);
    int *order_count = (int *)shmat(shmid2, NULL, 0);

    if (argc == 3 && strcmp(argv[1], "-deliver") == 0) {
        char *user = getenv("USER");
        if (!user) user = "unknown";
        char *target = argv[2];
        int found = 0;

        for (int i = 0; i < *order_count; i++) {
            if (strcmp(orders[i].nama, target) == 0 &&
                strcmp(orders[i].jenis, "Reguler") == 0 &&
                orders[i].delivered == 0) {
                
                orders[i].delivered = 1;
                strcpy(orders[i].agen, user);
                write_log(user, orders[i].nama, orders[i].alamat, "Reguler");

                printf("Pesanan Reguler atas nama %s telah diantar oleh AGENT %s\n", target, user);
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Pesanan Reguler atas nama %s tidak ditemukan atau sudah dikirim.\n", target);
        }

    } else if (argc == 3 && strcmp(argv[1], "-status") == 0) {
        char *nama = argv[2];
        for (int i = 0; i < *order_count; i++) {
            if (strcmp(orders[i].nama, nama) == 0) {
                if (orders[i].delivered) {
                    printf("Status pesanan untuk %s: Telah diantar oleh Agent %s\n", nama, orders[i].agen);
                } else {
                    printf("Status pesanan untuk %s: Pending\n", nama);
                }
                return 0;
            }
        }
        printf("Pesanan atas nama %s tidak ditemukan.\n", nama);

    } else if (argc == 2 && strcmp(argv[1], "-list") == 0) {
        for (int i = 0; i < *order_count; i++) {
            printf("Pesanan atas nama %s [%s] : %s\n", orders[i].nama, orders[i].jenis,
                   orders[i].delivered ? orders[i].agen : "Pending");
        }

    } else {
        printf("Perintah tidak ditemukan, Coba:\n");
        printf("./dispatcher -deliver [Nama]\n");
        printf("./dispatcher -status [Nama]\n");
        printf("./dispatcher -list\n");
    }

    shmdt(orders);
    shmdt(order_count);
    return 0;
}
