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
