# FINAL PROJECT SISTEM OPERASI IT28
## server.c
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

void list_users(int client_socket) {
    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        perror("Gagal membuka file users.csv");
        return;
    }

    char line[256];
    char response[1024] = "";
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *username = strtok(line, ",");
        strcat(response, username);
        strcat(response, " ");
    }
    fclose(file);

    send(client_socket, response, strlen(response), 0);
}

void edit_user(int client_socket, char *command) {
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

    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        perror("Gagal membuka file users.csv");
        return;
    }

    char temp_path[] = "DiscorIT/users_temp.csv";
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Gagal membuka file temp");
        fclose(file);
        return;
    }

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *saved_username = strtok(line, ",");
        char *saved_password = strtok(NULL, ",");
        char *role = strtok(NULL, ",");

        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            fprintf(temp_file, "%s,%s,%s\n", new_value, saved_password, role);
            found = 1;
        } else {
            fprintf(temp_file, "%s,%s,%s\n", saved_username, saved_password, role);
        }
    }
    fclose(file);
    fclose(temp_file);

    if (found) {
        remove(FILE_PATH);
        rename(temp_path, FILE_PATH);
        char response[256];
        snprintf(response, sizeof(response), "%s berhasil diubah menjadi %s\n", username, new_value);
        send(client_socket, response, strlen(response), 0);
    } else {
        remove(temp_path);
        send(client_socket, "Username tidak ditemukan\n", 25, 0);
    }
}

void remove_user(int client_socket, char *username) {
    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        perror("Gagal membuka file users.csv");
        return;
    }

    char temp_path[] = "DiscorIT/users_temp.csv";
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Gagal membuka file temp");
        fclose(file);
        return;
    }

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *saved_username = strtok(line, ",");
        char *saved_password = strtok(NULL, ",");
        char *role = strtok(NULL, ",");

        if (saved_username != NULL && strcmp(saved_username, username) != 0) {
            fprintf(temp_file, "%s,%s,%s\n", saved_username, saved_password, role);
        } else if (saved_username != NULL) {
            found = 1;
        }
    }
    fclose(file);
    fclose(temp_file);

    if (found) {
        remove(FILE_PATH);
        rename(temp_path, FILE_PATH);
        char response[256];
        snprintf(response, sizeof(response), "%s berhasil dihapus\n", username);
        send(client_socket, response, strlen(response), 0);
    } else {
        remove(temp_path);
        send(client_socket, "Username tidak ditemukan\n", 25, 0);
    }
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);
    char buffer[1024];
    int read_size;
    
    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0';
        printf("Diterima dari client: %s\n", buffer);

        if (strncmp(buffer, "LIST USER", 9) == 0) {
            list_users(client_socket);
        } else if (strncmp(buffer, "EDIT WHERE", 10) == 0) {
            //strtok(buffer, " ");
            strtok(buffer, " "); 
            strtok(NULL, " "); 
            edit_user(client_socket, strtok(NULL, ""));
        } else if (strncmp(buffer, "REMOVE", 6) == 0) {
            strtok(buffer, " ");
            remove_user(client_socket, strtok(NULL, " "));
        } else {
            char response[1024];
            snprintf(response, sizeof(response), "Server menerima: %s", buffer);
            send(client_socket, response, strlen(response), 0);
        }

        memset(buffer, 0, sizeof(buffer));
    }
    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, client_socket, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t tid[MAX_CLIENTS];
    int i = 0;

    // Membuat socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket gagal");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Binding
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind gagal");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("Listen gagal");
        exit(EXIT_FAILURE);
    }

    printf("Server berjalan pada port %d\n", PORT);

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) > 0) {
        printf("Koneksi baru diterima\n");

        new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        if (pthread_create(&tid[i++], NULL, handle_client, (void *)new_sock) != 0) {
            perror("Gagal membuat thread");
        }

        if (i >= MAX_CLIENTS) {
            i = 0;
            while (i < MAX_CLIENTS) {
                pthread_join(tid[i++], NULL);
            }
            i = 0;
        }
    }

    if (client_socket < 0) {
        perror("Accept gagal");
        exit(EXIT_FAILURE);
    }

    return 0;
}
```


## discorit.c
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

void create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}

int is_file_empty(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 1; 
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    return size == 0;
}

int login_user(const char *username, const char *password) {
    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        perror("Gagal membuka file users.csv");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *saved_username = strtok(line, ",");
        char *saved_password = strtok(NULL, ",");
        char *role = strtok(NULL, ",");

        if (saved_username != NULL && saved_password != NULL && strcmp(saved_username, username) == 0) {
            char *encrypted_password = crypt(password, saved_password);
            if (strcmp(saved_password, encrypted_password) == 0) {
                fclose(file);
                printf("%s berhasil login\n", username);
                return 1; //login berhasil
            } else {
                fclose(file);
                printf("Login gagal: username atau password salah\n");
                return 0; //password salah
            }
        }
    }

    fclose(file);
    printf("Login gagal: username tidak ditemukan\n");
    return 0;
}

void register_user(const char *username, const char *password) {
    create_directory_if_not_exists(DIRECTORY);
    
    FILE *file = fopen(FILE_PATH, "a+");
    if (!file) {
        perror("Gagal membuka file users.csv");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; 
        char *saved_username = strtok(line, ",");
        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            printf("Username sudah terdaftar\n");
            fclose(file);
            return;
        }
    }

    //bcrypt
    char salt[] = "$6$XXXX"; 
    char *encrypted_password = crypt(password, salt);

    if (encrypted_password == NULL) {
        perror("Gagal mengenkripsi password");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    const char *role = is_file_empty(FILE_PATH) ? "ROOT" : "USER";
    fprintf(file, "%s,%s,%s\n", username, encrypted_password, role);
    fclose(file);
    printf("%s berhasil register sebagai %s\n", username, role);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s REGISTER username -p password\n", argv[0]);
        printf("Usage: %s LOGIN username -p password\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "REGISTER") == 0) {
        if (argc != 5 || strcmp(argv[3], "-p") != 0) {
            printf("Usage: %s REGISTER username -p password\n", argv[0]);
            return -1;
        }
        register_user(argv[2], argv[4]);
    } else if (strcmp(argv[1], "LOGIN") == 0) {
        if (argc != 5 || strcmp(argv[3], "-p") != 0) {
            printf("Usage: %s LOGIN username -p password\n", argv[0]);
            return -1;
        }
        if (login_user(argv[2], argv[4])) {
            char command[1024];
            int sock = 0;
            struct sockaddr_in serv_addr;
            char buffer[1024] = {0};
            
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                printf("\n Socket creation error \n");
                return -1;
            }

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(SERVER_PORT);
            
            if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                return -1;
            }

            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nConnection Failed \n");
                return -1;
            }

            printf("[%s] ", argv[2]);
            while (fgets(command, sizeof(command), stdin)) {
                command[strcspn(command, "\r\n")] = 0; 
                send(sock, command, strlen(command), 0);
                read(sock, buffer, 1024);
                printf("%s\n", buffer);
                printf("[%s] ", argv[2]);
                memset(buffer, 0, sizeof(buffer));
            }
            close(sock);
        }
    }

    return 0;
}
```
