# FINAL PROJECT SISTEM OPERASI IT28
## server.c

Library dan define path
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define FILE_PATH "DiscorIT/users.csv"
```

Fungsi untuk list user untuk ROOT
```c
// fopen Membuka file users.csv dalam mode baca (r)
void list_users(int client_socket) {
    FILE *file = fopen(FILE_PATH, "r");

        // Jika file tidak bisa dibuka (gaada/file==null), tampilkan pesan error lalu keluar dari fungsi tanpa eksekusi lebih lanjut
    if (!file) {
        perror("Gagal membuka file users.csv");
        return;
    }

     // simpan satu baris data dari file CSV
    char line[256];
     // Menyimpan respons yang akan dikirim ke klien
    char response[1024] = "";

     // fgets Membaca file baris per baris
    while (fgets(line, sizeof(line), file)) {
        // strcspn Menghapus karakter newline (`\r\n` atau `\n`)dari akhir baris (jika ada)
        // strtok memisahkan baris berdasarkan koma (,) dan mengambil token pertama sebagai username.
        line[strcspn(line, "\r\n")] = 0;
        char *username = strtok(line, ",");
        // strcat Menambahkan username ke dalam respons
        strcat(response, username);
        strcat(response, " ");
    }
    // tutup file setelah selesai membaca
    fclose(file);

    // Mengirim respons ke klien melalui socket
    send(client_socket, response, strlen(response), 0);
}
```

Fungsi untuk edit username dan password user
```c
void edit_user(int client_socket, char *command) {
//strtok untuk memisahkan command menjadi username, field, & new_value
//Jika username, field, atau new_value tidak ditemukan atau formatnya salah, fungsi mengirim pesan error dan keluar.

    char *username = strtok(command, " "); 
    if (username == NULL) {
        send(client_socket, "Format salah\n", 13, 0);
        return;
    }

    char *field = strtok(NULL, " ");
    if (field == NULL || strcmp(field, "-u") != 0) {
        send(client_socket, "Format salah\n", 13, 0);
        return;
    }

    char *new_value = strtok(NULL, " ");
    if (new_value == NULL) {
        send(client_socket, "Format salah\n", 13, 0);
        return;
    }

    //fopen membuka file users.csv dalam mode baca ("r") dan file sementara users_temp.csv dalam mode tulis ("w").
    FILE *file = fopen(FILE_PATH, "r");

//Jika salah satu file gagal dibuka, fungsi mencetak pesan error dan keluar.*1
    if (!file) {
        perror("Gagal membuka file users.csv");
        return;
    }

    char temp_path[] = "DiscorIT/users_temp.csv";
    FILE *temp_file = fopen(temp_path, "w");
//sama seperti *1
    if (!temp_file) {
        perror("Gagal membuka file temp");
        fclose(file);
        return;
    }

//baca & tulis file
    char line[256];
    int found = 0;
    //fgets membaca file users.csv baris demi baris.
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
            //strtok mengubah tiap line jadi saved_username, saved_password, dan role.
        char *saved_username = strtok(line, ",");
        char *saved_password = strtok(NULL, ",");
        char *role = strtok(NULL, ",");
            //kalau username ada, data yg diubah masuk ke temp_file
        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            fprintf(temp_file, "%s,%s,%s\n", new_value, saved_password, role);
            found = 1;
            //kalau tidak ada, data asli yang masuk
        } else {
            fprintf(temp_file, "%s,%s,%s\n", saved_username, saved_password, role);
        }
    }
    fclose(file);
    fclose(temp_file);

//untuk ganti/rewrite file
    //kalau username ada = file asli hapus, temp_file jadi file asli
    //lalu kirim pesan sukses ke klien
    if (found) {
        remove(FILE_PATH);
        rename(temp_path, FILE_PATH);
        char response[256];
        snprintf(response, sizeof(response), "%s berhasil diubah menjadi %s\n", username, new_value);
        send(client_socket, response, strlen(response), 0);
    }

    //kalo tidak ada ya, yowes temp_file dihapus
    //lalu kirim pesan error/gagal ke klien
    else {
        remove(temp_path);
        send(client_socket, "Username tidak ditemukan\n", 25, 0);
    }
}
```

Fungsi untuk remove user
```c
//untuk membuka file
    //buka users.csv di mode baca (r)
    //kalau gagal dibuka (file == null), perror mengirim pesan error dan stop eksekusi 
void remove_user(int client_socket, char *username) {
    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        perror("Gagal membuka file users.csv");
        return;
    }

    //membuat temp_file untuk menulis data yang diubah
        //fopen buka temp_file di mode tulis (w)
        //kalau gagal buka file = perror mengirim pesan error lalu stop eksekusi
    char temp_path[] = "DiscorIT/users_temp.csv";
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Gagal membuka file temp");
        fclose(file);
        return;
    }

//mencari user yang ingin dihapuskan

    char line[256];
    int found = 0; //penanda apakah username ditemukan atau tidak
        //fgets membaca isi users.csv
        //strcspn menghapus newline di akhir baris
        //strok memisah tiap baris jadi saved_username, saved_password, role
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *saved_username = strtok(line, ",");
        char *saved_password = strtok(NULL, ",");
        char *role = strtok(NULL, ",");

        //kalau tidak cocok dengan yang akan dihapus, data asli masuk ke temp_file
        if (saved_username != NULL && strcmp(saved_username, username) != 0) {
            fprintf(temp_file, "%s,%s,%s\n", saved_username, saved_password, role);
        //kalau cocok dengan yg akan dihapus, ditandai jadi 1 untuk penanda pengguna ditemukan
        } else if (saved_username != NULL) {
            found = 1;
        }
    }
    fclose(file);
    fclose(temp_file);

//kalau ketemu, file asli dihapus, temp_file berubah jadi file asli lalu pesan sukses dikirim ke klien
    if (found) {
        remove(FILE_PATH);
        rename(temp_path, FILE_PATH);
        char response[256];
        snprintf(response, sizeof(response), "%s berhasil dihapus\n", username);
        send(client_socket, response, strlen(response), 0);
    }
//kalau tidak ditemukan, temp_file dihapus lalu pesan error dikirim ke klien
else {
        remove(temp_path);
        send(client_socket, "Username tidak ditemukan\n", 25, 0);
    }
}
```

Menyambungkan ke discorit.c
```c
void *handle_client(void *arg) {
    // mendapatkan socket klien dari argumen dan melepaskan memori
    int client_socket = *((int *)arg);
    free(arg);
    
    // buffer untuk menyimpan data yang diterima dari klien
    char buffer[1024];
    int read_size;

    // loop untuk terus menerima data dari klien selama masih ada data yang diterima
    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        // menambahkan null-terminator di akhir data yang diterima agar dapat diproses sebagai string
        buffer[read_size] = '\0';
        printf("Diterima dari client: %s\n", buffer);

        // memeriksa apakah data yang diterima adalah perintah "LIST USER"
        if (strncmp(buffer, "LIST USER", 9) == 0) {
            list_users(client_socket); // memanggil fungsi untuk daftar pengguna
        }
        // memeriksa apakah data yang diterima adalah perintah "EDIT WHERE"
        else if (strncmp(buffer, "EDIT WHERE", 10) == 0) {
            strtok(buffer, " "); // mengabaikan kata pertama "EDIT"
            strtok(NULL, " "); // mengabaikan kata kedua "WHERE"
            // memanggil fungsi untuk mengedit pengguna dengan argumen yang tersisa
            edit_user(client_socket, strtok(NULL, ""));
        }
        // memeriksa apakah data yang diterima adalah perintah "REMOVE"
        else if (strncmp(buffer, "REMOVE", 6) == 0) {
            strtok(buffer, " "); // mengabaikan kata pertama "REMOVE"
            // memanggil fungsi untuk menghapus pengguna dengan argumen yang tersisa
            remove_user(client_socket, strtok(NULL, " "));
        }
        // jika perintah tidak dikenal, kirimkan kembali pesan yang diterima ke klien
        else {
            char response[1024];
            // membuat balasan dengan menambahkan teks "Server menerima: " di depan pesan klien
            snprintf(response, sizeof(response), "Server menerima: %s", buffer);
            // mengirim balasan ke klien
            send(client_socket, response, strlen(response), 0);
        }

        // mengosongkan buffer untuk siap menerima data berikutnya
        memset(buffer, 0, sizeof(buffer));
    }

    // menutup socket klien setelah selesai berkomunikasi
    close(client_socket);
    return NULL; // mengembalikan NULL karena fungsi ini harus mengembalikan void*
}

```

Main Function
```c
int main() {
    int server_socket, client_socket, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t tid[MAX_CLIENTS]; // array untuk menyimpan thread ID
    int i = 0;

    // membuat socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket gagal");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET; // menggunakan protokol IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // mendengarkan pada semua interface
    server_addr.sin_port = htons(PORT); // mengatur port yang digunakan

    // binding
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind gagal");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(server_socket, 3) < 0) {
        perror("listen gagal");
        exit(EXIT_FAILURE);
    }

    printf("server berjalan pada port %d\n", PORT);

    // loop untuk menerima koneksi klien
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) > 0) {
        printf("koneksi baru diterima\n");

        // alokasi memori untuk socket klien
        new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        // membuat thread untuk menangani klien
        if (pthread_create(&tid[i++], NULL, handle_client, (void *)new_sock) != 0) {
            perror("gagal membuat thread");
        }

        // jika jumlah thread melebihi MAX_CLIENTS, menunggu thread untuk selesai
        if (i >= MAX_CLIENTS) {
            i = 0;
            while (i < MAX_CLIENTS) {
                pthread_join(tid[i++], NULL);
            }
            i = 0;
        }
    }

    // jika accept gagal
    if (client_socket < 0) {
        perror("accept gagal");
        exit(EXIT_FAILURE);
    }

    return 0;
}
```


## discorit.c
Library dan define path
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <crypt.h>
#include <arpa/inet.h>

#define DIRECTORY "DiscorIT"
#define FILE_PATH DIRECTORY "/users.csv"
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
```

Membuat directory yang dibutuhkan apa bila directory belum ada
```c
// memeriksa apakah direktori ada, jika tidak, membuat direktori dengan izin 0700
void create_directory_if_not_exists(const char *path) {
    struct stat st = {0}; // mendeklarasikan struktur untuk memeriksa status file atau direktori
    if (stat(path, &st) == -1) { // jika status path tidak dapat diperoleh (direktori tidak ada)
        mkdir(path, 0700); // membuat direktori dengan izin 0700 (pemilik dapat membaca, menulis, dan mengeksekusi)
    }
}

// memeriksa apakah file kosong dengan memeriksa ukurannya
int is_file_empty(const char *filename) {
    FILE *file = fopen(filename, "r"); // membuka file untuk membaca
    if (!file) { // jika file tidak dapat dibuka (misalnya, tidak ada)
        return 1; // mengembalikan 1 yang menunjukkan file kosong atau tidak ada
    }
    fseek(file, 0, SEEK_END); // menggerakkan pointer file ke akhir file
    long size = ftell(file); // mendapatkan ukuran file dengan mendapatkan posisi pointer di akhir
    fclose(file); // menutup file
    return size == 0; // mengembalikan 1 jika ukuran file 0 (file kosong), jika tidak mengembalikan 0
}
```

Mengatur untuk fitur LOGIN
```c
int login_user(const char *username, const char *password) {
    FILE *file = fopen(FILE_PATH, "r"); // membuka file users.csv untuk membaca
    if (!file) { // jika file tidak dapat dibuka
        perror("Gagal membuka file users.csv"); // mencetak pesan kesalahan
        exit(EXIT_FAILURE); // keluar dari program dengan kode kesalahan
    }

    char line[256]; // buffer untuk membaca baris dari file
    while (fgets(line, sizeof(line), file)) { // membaca baris demi baris dari file
        line[strcspn(line, "\r\n")] = 0; // menghapus karakter newline dari akhir baris
        char *saved_username = strtok(line, ","); // memisahkan nama pengguna
        char *saved_password = strtok(NULL, ","); // memisahkan kata sandi yang disimpan
        char *role = strtok(NULL, ","); // memisahkan peran (role) pengguna, jika ada

        if (saved_username != NULL && saved_password != NULL && strcmp(saved_username, username) == 0) { // memeriksa apakah username cocok
            char *encrypted_password = crypt(password, saved_password); // mengenkripsi kata sandi yang diberikan
            if (strcmp(saved_password, encrypted_password) == 0) { // membandingkan kata sandi yang disimpan dengan kata sandi terenkripsi
                fclose(file); // menutup file setelah berhasil login
                printf("%s berhasil login\n", username); // mencetak pesan bahwa login berhasil
                return 1; // mengembalikan 1 jika login berhasil
            } else {
                fclose(file); // menutup file jika password salah
                printf("Login gagal: username atau password salah\n"); // mencetak pesan bahwa login gagal karena password salah
                return 0; // mengembalikan 0 jika password salah
            }
        }
    }

    fclose(file); // menutup file jika username tidak ditemukan
    printf("Login gagal: username tidak ditemukan\n"); // mencetak pesan bahwa login gagal karena username tidak ditemukan
    return 0; // mengembalikan 0 jika username tidak ditemukan
}
```

Fungsi untuk REGISTER 
```c
void register_user(const char *username, const char *password) {
    create_directory_if_not_exists(DIRECTORY);
    
    FILE *file = fopen(FILE_PATH, "a+");
    if (!file) {
        perror("gagal membuka file users.csv");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; 
        char *saved_username = strtok(line, ",");
        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            printf("username sudah terdaftar\n");
            fclose(file);
            return;
        }
    }

    // bcrypt
    char salt[] = "$6$XXXX"; 
    char *encrypted_password = crypt(password, salt);

    if (encrypted_password == NULL) {
        perror("gagal mengenkripsi password");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // menentukan peran (role) pengguna
    const char *role = is_file_empty(FILE_PATH) ? "ROOT" : "USER";

    // menulis informasi pengguna baru ke dalam file
    fprintf(file, "%s,%s,%s\n", username, encrypted_password, role);
    fclose(file);
    printf("%s berhasil register sebagai %s\n", username, role);
}
```

Main Function
```c
int main(int argc, char *argv[]) {
    // memeriksa jumlah argumen command line
    if (argc < 2) {
        printf("usage: %s register username -p password\n", argv[0]);
        printf("usage: %s login username -p password\n", argv[0]);
        return -1;
    }

    // mode register
    if (strcmp(argv[1], "register") == 0) {
        // memeriksa argumen untuk registrasi
        if (argc != 5 || strcmp(argv[3], "-p") != 0) {
            printf("usage: %s register username -p password\n", argv[0]);
            return -1;
        }
        // panggil fungsi register_user dengan username dan password yang diberikan
        register_user(argv[2], argv[4]);
    }

    // mode login
    else if (strcmp(argv[1], "login") == 0) {
        // memeriksa argumen untuk login
        if (argc != 5 || strcmp(argv[3], "-p") != 0) {
            printf("usage: %s login username -p password\n", argv[0]);
            return -1;
        }
        // memeriksa hasil dari fungsi login_user
        if (login_user(argv[2], argv[4])) {
            // inisialisasi variabel untuk koneksi socket
            int sock = 0;
            struct sockaddr_in serv_addr;
            char buffer[1024] = {0};

            // membuat socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                printf("\n socket creation error \n");
                return -1;
            }

            // mengisi informasi server
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(server_port);
            if(inet_pton(af_inet, server_ip, &serv_addr.sin_addr) <= 0) {
                printf("\ninvalid address/ address not supported \n");
                return -1;
            }

            // melakukan koneksi ke server
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nconnection failed \n");
                return -1;
            }

            // memulai komunikasi dengan server
            printf("[%s] ", argv[2]);
            char command[1024];
            while (fgets(command, sizeof(command), stdin)) {
                command[strcspn(command, "\r\n")] = 0; // menghapus newline dari input
                send(sock, command, strlen(command), 0); // mengirim perintah ke server
                read(sock, buffer, 1024); // menerima respons dari server
                printf("%s\n", buffer); // menampilkan respons dari server
                printf("[%s] ", argv[2]); // menampilkan prompt untuk input berikutnya
                memset(buffer, 0, sizeof(buffer)); // mengosongkan buffer
            }

            // menutup soket setelah selesai
            close(sock);
        }
    }

    return 0;
}
```
## Command untuk menjalankan program
``` ./discorit REGISTER <username> -p <password> ```

Jika dalam system belum tercatat username manapun, maka otomatis akun pertama yang register akan memiliki tipe ROOT yang dapat mengubah username, menghapus, dan mengganti password USER lain. 

``` ./discorit LOGIN <username> -p <password> ```

## Dokumentasi
![Screenshot_2024-06-28_13_07_46](https://github.com/salschoco27/Sisop-FP-2024-MH-IT28/assets/151063684/ce1abe7c-af06-47b4-9217-e90cc09c136a)
![Screenshot_2024-06-28_13_07_51](https://github.com/salschoco27/Sisop-FP-2024-MH-IT28/assets/151063684/b371ac86-bf0c-4dbd-b03e-eb0e5f5a56be)

## Dokumentasi setelah saya ubah
![Screenshot_2024-06-28_13_24_10](https://github.com/salschoco27/Sisop-FP-2024-MH-IT28/assets/151063684/67cc6d70-2c75-4a47-a642-faa77f4abb45)
