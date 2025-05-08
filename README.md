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
### Downloading and Preparing
Diberikan sebuah link download file dari Google Drive yang berupa csv. Isi file ini merupakan list pelanggan, metode pengiriman (Reguler/Express), dan alamat pengiriman. Selanjutnya, gunakan tools wget untuk menyimpan file tersebut di Directory
```bash
wget https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9
```
Pada sub soal ini, diperintahkan untuk mengupload file csv yang disediakan ke Shared memory dengan fungsi : 
```c
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
```c
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
```c
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
```c
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
```c
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
```c
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
Pada program ini, dungeon.c berfungsi sebagai server yang melakukan semua logic dan menghandle lebih dari satu client, sedangkan untuk player.c berfungsi untuk connect dengan server lalu menerima dan mengirim pesan saja. Setelah sudah terhubung program melakukan infinity loop, untuk mengecek dan menerima pesan melalui fungsi recv(), send() serta memset untuk mengosongkan kembali buffer untuk menyimpan data selanjutnya. Pada *dungeon.c* bagian penagturan konreksi ini, while loopnya adalah menunggu koneksi dengan client baru dan jika ada yag connect maka akan mengirim pesan serta membuat thread baru.

- *dungeon.c*:
```c
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

```

Selain itu, juga terdapat inisialisasi data player atau client yang baru connect, hal ini dituliskan dalam fungsi handleClient():
```c
void *handleClient(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);  // bebaskan memori setelah digunakan
    char buffer[2048];

    // inisiasi stats client
    Player player = {200, {"Fist", 0, 50, 1, 0, ""}, {}, 0, 1};
    player.inv[0] = player.weapon;
    player.inv[0].bought = 1;

    .....
}
```

- *player.c*:
```c
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recv(sock, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) break;  // koneksi ditutup oleh server

        printf("%s", buffer);  // tampilkan pesan dari server
        fflush(stdout);
        
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        send(sock, input, strlen(input), 0);
    }
```
Ketika sudah terkoneksi dengan server, maka akan melaukan infinity loop menerima dan memberi pesan. Dimana pesan disimpan dalam bentuk string dalam variable buffer.
### Sightseeing
Membuat terminal yang akan melakukan infinity loop selama client atau player tidak memilih untuk exit dari server, selain itu juga menerima input user untuk membuka fitur atau menu lainnya. Program ini dituliskan dalam fungsi handleClient() seperti ini:
```c
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
```
Dikarenakan input user merupakan string, maka pemilihan opsi dilakukan menggunakan strncmp yang selanjutnya akan memanggil fungsi dari fitur fitur tersebut, sebenarnya bisa saja menggunakan atoi terlebih dahulu untuk mengubah input user menjadi int, namun pada kasus ini efisiensi tidak begitu penting untuk diperhatikan.
### Status Check   
Menampilkan statistik dari client atau player, disimpan dalam fungsi showStats()
```c
void showStats(Player *player, int client_sock) {
    char buffer[1024];
    if (player->weapon.boolPassive == 1){
        snprintf(buffer, sizeof(buffer), "\n==== Player Stats ====\n \033[0;33mGold:\033[0m %d | \033[0;34mWeapon:\033[0m %s | \033[0;31mBase Damage:\033[0m %d | \033[0;31mKills:\033[0m %d | \033[0;34mPassive:\033[0m %s \n", player->gold, player->weapon.name, player->weapon.damage, player->kills, player->weapon.passive);
    } else {
        snprintf(buffer, sizeof(buffer), "\n==== Player Stats ====\n \033[0;33mGold:\033[0m %d | \033[0;34mWeapon:\033[0m %s | \033[0;31mBase Damage:\033[0m %d | \033[0;31mKills:\033[0m %d \n", player->gold, player->weapon.name, player->weapon.damage, player->kills);
    }
    send(client_sock, buffer, strlen(buffer), 0);
}
```
Pada fungsi ini dilakukan pengecekan, apakah senjata yang sedang digunakan player memiliki passive, jika iya, maka akan menampilkan passive dari senjata itu juga, jiak tidak ada maka akan langsung outpu statstik dasar player.
### Weapon Shop   
Pada Weapon Shop digunakan dua fungsi utama yaitu displayShop() untuk menampilkan shop serta buyWeapon() untuk membeli senjata, berikut untuk penjelasannya:
- *displayShop()*
Sebelum menuju fungsinya langsung, dilakukan pembuatan struct weapon yang bernama shop[], yang berisi array weapon yang dijual dishop:
```c
Weapon shop[] = {
    {"Random Stick", 300, 80, 0, 1, "Auto kill when enemy HP at 10%"},
    {"Sword", 1000, 200, 0, 0,""},
    {"Axe", 500, 130, 0, 0,""},
    {"Amboi", 350, 80, 0, 0,""},
    {"Berserker Fury", 400, 100, 0, 1, "+30% Crit Chance"}
};

static int shop_size = sizeof(shop) / sizeof(shop[0]);
```
Selanjutnya untuk fungsi displayShop adalah sebagai berikut:
```c
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
```
Dimana akan melakukan for loop sebanyak item di shop[], yang jika item memiliki passive (boolPassive == 1), maka akan melakukan snprintf dengan passive-nya juga. Jika sudah semua maka akan mengirim buffer tersebut ke sisi user.
- *buyWeapon()*
```c
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
```
Mmebuat pointer struct *w, yang memiliki menunjuk address shop[i], dimana jika weapon belum dibeli, gold player cukup akan mengubah shop[i].bought menjadi 1 atau true. Selanjutnya menambahkan weapon tersebut ke dalam inventory player, menambah invCount (inventory count) player, mengurangi gold player sesuai harga dan mengirim pesan bahwa user telah membeli barang bernama shop[i].name.
### Handy Inventory    
Membuat sistem inventori yang bisa melihat list senjata dan mengubah senjata yang bisa dipakai:
```c
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

    .....
}
```
Pada bagian awal, akan melakukan output yang dimana dilakukan melalui for loop sebanyak invCount player, dan jika weapon memiliki passive maka akan menunjukkan passivenya juga. Serta jika weapon sedang digunakan maka akan ada text (EQUIPPED) disamping weapon.

Selanjutnya ke bagian penerimaan input untuk pergantian senjata yang sedang digunakan:
```c
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
```
Pertama tama mengosongkan buffer, selanjutnya melakukan recv() input user, sealnjutnya melakukan atoi untuk merubah input yang berupa string menjadi int, dan melakukan pengecekan apakah angka berada dalam range, jika ada maka data di player->weapon akan di overwrite dengan player->inv[choice], dimana choice merupakan pilihan user tadi.
### Enemy Encounter     
**Note: Penjelasan lebih jelas mengenai battle logic ada di bagian (subheading) selanjutnya**    
Fungsi mode battle atau battleMode() terdiri dari beberapa bagian utama yaitu:
 - Awal:
```c
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

        .....
    }
```
Melakukan inisiasi, yaitu generate musuh, mengirim pesan, selanjutnya masuk ke bagian awal while loop yang berupa pembuatan percentage bar HP musuh, yang hanya akan diprint ketika health musuh > 0.   

 - Tengah
```c
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
```
Jika input user adalah attack maka akan menyerang dengan damageDealt berasal dari perhitungan fungsi calculateDamage(), dan jika memiliki passive maka akan ada pesan yang tampil jika passive senjata yang digunakan player aktif.
 - Akhir
```c
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
```
Jika nyawa musuh sudah habis, maka akan menampilkan pesan dan menambah statistik gold dan kill player. Selain itu juga akan memunculkan musuh baru.
### Other Battle Logic
#### Health & Rewards
Ketika battle dilakukan, akan terjadi perandoman gold yang diberikan dan health dari entitas musuh. Hal ini dibuat dalam fungsi:
```c
Enemy generateEnemy() {
    Enemy new_enemy;
    new_enemy.max_health = rand() % 151 + 50; // random 50-200 HP
    new_enemy.health = new_enemy.max_health; // buat track nyawa
    new_enemy.gold = rand() % 151 + 50; // random 50-200 gold reward
    return new_enemy;
}
```
Ketika battle telah usai, maka akan ditambahkan gold enemy tersebut ke gold player dan terdapat pesan yang dikirimkan kepada player, hal ini dituliskan dalam kode program:
```c
snprintf(buffer, sizeof(buffer), "\n\033[0;31mEnemy defeated!\033[0m \n");
send(client_sock, buffer, strlen(buffer), 0);

player->gold += currentEnemy.gold;
player->kills++;

snprintf(buffer, sizeof(buffer), "\n\033[0;34m====\033[0m \033[0;33mReward\033[0m \033[0;34m====\033[0m\n You received \033[0;33m%d\033[0m gold!\n", currentEnemy.gold);
send(client_sock, buffer, strlen(buffer), 0);
```     

#### Damage Equation
```c
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
```
Pada game perhitungan damage dilakukan dalam fungsi calculateDamage(), yang dimana melakukan banyak operasi rand(), perhitungan damage yang diberikan (disini disebut damageDealt), berasal dari dua variable, yaitu baseDamage, yaitu damage dasar dari senjata yang digunakan player. Selanjutnya terdapat randomizedDamage yang berasal dari baseDamage yang dibagi angka random 1-5, kemudian dikurangi angka random 0-4. Selanjutnya, baseDamage ditambahkan dengan randomizedDamage yang menghasilkan damageDealt. Ketika damageDealt <= 3, maka damageDealt bernilai 3. Terakhir akan melalui proses randomize critical nya, jika critical hit, maka damageDealt akan menjadi dua kali lipat, jika tidak maka akan tetap.   

#### Passive
Pada game terdapat dua weapon yang memiliki passive, yaitu Random Stick dengan passive Auto kill when enemy HP at 10% dan Berserker Fury dengan passive +30% Critical Chance.  
Mekanisme passive ini dalam kode program adalah:
- *Random Stick :*
```c
    int healthPercentage = currentEnemy.health * 100 / currentEnemy.max_health;    
    if ((strcmp(player->weapon.name, shop[0].name) == 0) && (healthPercentage <= 10)){
        currentEnemy.health = 0;
        snprintf(buffer, sizeof(buffer), "\n\033[0;34m==== Passive Activated ==== \nRandom Stick just did an auto kill, well that's great...\033[0m \n");
        send(client_sock, buffer, strlen(buffer), 0);
    }
```
- *Berserker Fury:*
```c
    if (strcmp(player->weapon.name, shop[4].name) == 0) {
        y = rand() % 2; // 0-1, 50% crit chance
    } else {
        y = rand() % 5; // 0-4, 20% crit chance
    }
    if (y == 1) {
        *isCritical = 1;
        return damageDealt * 2;
    }
```
Dalam program dilakukan pengecekan apakah senjata yang sedang digunakan player (player->weapon.name) memiliki nama yang sama dengan senjata di shop index ke 4, yang merupakan shop index dari *Berserker Fury*. Dimana perhitungan rand()nya akan memiliki chance 50% untuk critical hit ketika player menggunakan *Berserker Fury* sedangkan jika tidak akan menggunakan akan memiliki chance 20% critical hit. Dikarenakan passive senjata ini selalu aktif, maka ketika melakukan command *attack*, otomatis akan ada pesan:
```c
if (strcmp(player->weapon.name, shop[4].name) == 0){
    snprintf(buffer, sizeof(buffer), "\n\033[0;34m==== Passive Activated ==== \nBerserker Fury increased the crit chance by 30%%...\033[0m \n");
    send(client_sock, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
}
```

### Error Handling   
Pada tiap fungsi yang menerima input, serta pada pengaturan koneksi, penghubungan client dan server telah terdapat pesan error, dan penanganan yang diberlakukan sesuai kasus
### Nomor 4 (Rayka)
