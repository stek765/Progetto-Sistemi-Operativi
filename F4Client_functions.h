/************************************
* Matricole : VR456187  -  VR422276  -  VR456008
* Nomi e Cognomi : Oualid Maftah  -  Valeria Stuani  -  Stefano Zanolli
* Data realizzazione : 02/05/2023
*************************************/

#ifndef FORZA_4_CLIENT_FUNCTIONS_H
#define FORZA_4_CLIENT_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define SERVER 0
#define CLIENT 1

#define MATRIX_INFO 1
#define PLAYER_INFO 2
#define MOVE 3
#define WIN 4

typedef struct {
    long mtype;
    int rows;
    int cols;
    char sym1;
    char sym2;
    int server_pid;
} matrix_info_t;
typedef struct {
    long mtype;
    char* name;
    int pid;
} player_info_t;
typedef struct {
    long mtype;
    int col;
    int pid;
} move_t;
typedef struct {
    long mtype;
    int value;
    int pid;
} win_t;

typedef union {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
} semun;

void check_client_args(int argc, char** argv); //ok
player_info_t initialize_player_info(char** argv); //ok
void print_player_info(player_info_t playerInfo); //ok
void handle_signals(); //ok
void signal_handler();
void connect_semaphore_set(); //ok
void get_semaphore_set_values(unsigned short* arr); //ok
void print_semaphore_set_values(unsigned short* arr); //ok
void sblocca(int sem); //ok
void connect_message_queue(); //ok
matrix_info_t receive_matrix_info();
void print_matrix_info(matrix_info_t matrixInfo); // ok
void connect_shared_memory(matrix_info_t matrixInfo); //ok
void print_matrix(matrix_info_t matrixInfo); //ok
void send_player_info(player_info_t playerInfo); //ok
void send_move(move_t move);
move_t inserisci_colonna(matrix_info_t matrixInfo);
win_t receive_win();
void usage(); //ok
void err_exit(char* str); //ok
void signal_handler(int sig); //ok
void blocca(int sem); //ok
void close_client();

#endif //FORZA_4_CLIENT_FUNCTIONS_H
