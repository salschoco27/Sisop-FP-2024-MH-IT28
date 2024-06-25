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
