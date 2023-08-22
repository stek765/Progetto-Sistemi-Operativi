/************************************
* Matricole : VR456187  -  VR422276  -  VR456008
* Nomi e Cognomi : Oualid Maftah  -  Valeria Stuani  -  Stefano Zanolli
* Data realizzazione : 02/05/2023
*************************************/

#include <stdio.h>
#include "F4Client_functions.h"

extern unsigned short arr[3];

extern int server_pid;

int main(int argc, char** argv) {
    atexit(close_client);

    /// ARGOMENTI
    check_client_args(argc,argv); //ok
    /// PLAYER INFO
    player_info_t playerInfo = initialize_player_info(argv); //ok
    // print_player_info(playerInfo); //ok
    /// SEGNALI
    handle_signals(); //ok
    /// SET SEMAFORI
    connect_semaphore_set(); //ok
    get_semaphore_set_values(arr); //ok
    //print_semaphore_set_values(arr); //ok
    /// SBLOCCA SERVER
    sblocca(SERVER); //ok
    /// MESSAGE QUEUE
    connect_message_queue(); //ok
    /// MATRIX INFO
    matrix_info_t matrixInfo = receive_matrix_info(); //ok
    server_pid = matrixInfo.server_pid;
    //("server_pid : %d\n", server_pid);
    //print_matrix_info(matrixInfo); //ok
    /// SHARED MEMORY
    connect_shared_memory(matrixInfo); //ok
    //print_matrix(matrixInfo); //ok
    /// SEND PLAYER
    send_player_info(playerInfo); //ok

    printf("Attendo l'altro client (...)\n");

    win_t win = (win_t) {4,0,0};
    do {
        /// BLOCCA CLIENT
        blocca(CLIENT);
        /// PRINT MATRICE
        print_matrix(matrixInfo);
        /// INSERISCI COLONNA
        move_t mossa = inserisci_colonna(matrixInfo);
        /// SEND MOVE
        send_move(mossa);
        /// RECEIVE WIN
        win = receive_win();
        /// SBLOCCA CLIENT
        sblocca(CLIENT);
    } while(win.value==0);

    return 0;
}