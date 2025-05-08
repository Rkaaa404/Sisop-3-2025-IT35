# Sisop-3-2025-IT35

### Anggota Kelompok:  
- Rayka Dharma Pranandita (*5027241039*)
- Bima Aria Perthama (*5027241060*)
- Gemilang Ananda Lingua (*5027241072*)  

### Table of Contents :
- [Nomor 1](#nomor-1-aria)
- [Nomor 2](#nomor-2-gilang)
- [Nomor 3](#nomor-3-rayka)
- [Nomor 4](#nomor-4-rayka)

### Nomor 1 (Aria)

### Nomor 2 (Gilang)
### Nomor 2 (Gilang)
### Downloading and Preparing
Diberikan sebuah link download file dari Google Drive yang berupa csv. Isi file ini merupakan list pelanggan, metode pengiriman (Reguler/Express), dan alamat pengiriman. Selanjutnya, gunakan tools wget untuk menyimpan file tersebut di Directory
```bash
wget https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9
```
Pada sub soal ini, diperintahkan untuk mengupload file csv yang disediakan ke Shared memory dengan fungsi : 
```bash
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
```

### Pengiriman bertipe Express 
Selanjutnya, sub soal ini menginginkan kita memiliki 3 agen pengiriman utama (Agent A, Agent B, Agent C). Setiap agent memiliki thread yang berbeda yang selanjutnya Agent-Agent ini secara otomatis melakukan pengiriman bertipe Express.  
```bash
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
```
Fungsi diatas merupakan fungsi untuk menerima input berupa jenis Agent, yang selanjutnya akan dimasukkan kepada delivery.log. Fungsi log : 
```bash
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
```
Yang terakhir, menggabungkan fungsi-fungsi diatas yang berfungsi membuat dan menjalankan 3 Thread Agent (A,B,C)
```bash
Int main() {
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
```

### Pengiriman bertipe Reguler
sub soal ini berbeda dengan Express, untuk order bertipe Reguler, pengiriman dilakukan secara manual oleh user. 
```bash
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
}
```
Fungsi ini menangani pengiriman Reguler secara manual oleh user, bukan otomatis seperti Express. Pada fungsi tersebut, menggunakan getenv("USER") untuk mengambil nama agen (yaitu nama user yang menjalankan program). Karena pada soal, diharapkan output pengirim tipe Reguler adalah nama User sendiri.

### Mengecek status pesanan
Pada sub soal ini, diharapkan Dispatcher ini dapat memeriksa status pesanan setiap orang, dengan fungsi : 
```bash
else if (argc == 3 && strcmp(argv[1], "-status") == 0) {
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
}
```
Fungsi diatas melakukan pencarian dalam array shared memory orders. 
- Jika delivered == 1, tampilkan agen yang mengantarkan.
- Jika delivered == 0, tampilkan status Pending.
- Jika tidak ada yang cocok, tampilkan pesan tidak ditemukan.

### Melihat Daftar Semua Pesanan
Untuk memudahkan monitoring, sub soal ini mengharapkan program dispatcher bisa menjalankan perintah list untuk melihat semua order disertai nama dan statusnya, dengan fungsi : 
```bash
else if (argc == 2 && strcmp(argv[1], "-list") == 0) {
    for (int i = 0; i < *order_count; i++) {
        printf("Pesanan atas nama %s [%s] : %s\n", orders[i].nama, orders[i].jenis,
               orders[i].delivered ? orders[i].agen : "Pending");
    }
}
```
Fungsi ini menampilkan semua pesanan dari shared memory:
- Nama
- Jenis (Express/Reguler)
- Status (sudah diantar oleh agen siapa, atau Pending)
Mengecek field delivered untuk menentukan status:
- Jika 1, tampilkan nama agen (orders[i].agen)
- Jika 0, tampilkan "Pending"

### Nomor 3 (Rayka)
### Entering the dungeon
### Sightseeing
### Status Check
### Weapon Shop
### Handy Inventory
### Enemy Encounter
### Other Battle Logic
#### Health & Rewards
#### Damage Equation
#### Passive
### Error Handling
### Nomor 4 (Rayka)
