/************************************
* Matricole : VR456187  -  VR422276  -  VR456008
* Nomi e Cognomi : Oualid Maftah  -  Valeria Stuani  -  Stefano Zanolli
* Data realizzazione : 02/05/2023
*************************************/

#include "F4Server_functions.h"

extern unsigned short arr[3];
extern int pids[2][2];

int main(int argc, char** argv) {
    atexit(close_server);
    //printf("server_pid : %d\n",getpid());
    /// ARGOMENTI
    check_server_args(argc,argv); //ok
    /// MATRIX INFO
    matrix_info_t matrixInfo = initialize_matrix_info(argv); //ok
    //print_matrix_info(matrixInfo); //ok
    /// SEGNALI
    handle_signals(); //ok
    /// SET SEMAFORI
    initialize_semaphore_set(); //ok
    get_semaphore_set_values(arr); //ok
    //print_semaphore_set_values(arr); //ok
    /// MESSAGE QUEUE
    initialize_message_queue(); //ok
    /// SHARED MEMORY
    char* shm_ptr = initialize_shared_memory(matrixInfo); //ok
    initialize_matrix(matrixInfo); //ok
    //print_matrix(matrixInfo); //ok
    /// BLOCCA SERVER
    blocca(SERVER); //ok
    /// SEND MATRIX INFO
    send_matrix_info(matrixInfo); //ok
    /// BLOCCA SERVER
    blocca(SERVER); //ok
    /// SEND MATRIX INFO
    send_matrix_info(matrixInfo); //ok
    /// RECEIVE PLAYER INFO
    player_info_t playerInfo1 = receive_player_info(); //ok
    //print_player_info(playerInfo1); //ok
    pids[0][1]=playerInfo1.pid;
    /// SBLOCCA CLIENT 1
    sblocca(CLIENT);
    /// RECEIVE PLAYER INFO
    player_info_t playerInfo2 = receive_player_info(); //ok
    //print_player_info(playerInfo2); //ok
    pids[1][1]=playerInfo2.pid;

    move_t move = (move_t){3,0,0};
    win_t win = (win_t){4,0,0};
    int other_pid=0;
    do {
        /// RECEIVE MOVE
       move = receive_move();
       /// CALCOLO PID AVVERSARIO
       other_pid = (move.pid==pids[0][1])?pids[1][1]:pids[0][1];
       //printf("current player pid: %d\n", move.pid);
       //printf("other player pid: %d\n", other_pid);
       /// POSIZIONA GETTONE
       char (*matrix)[matrixInfo.cols] = (void *) shm_ptr;
       for (int row = matrixInfo.rows-1; row >= 0; row--) {
           if (matrix[row][move.col - 1] == ' ') {
               matrix[row][move.col - 1] = (move.pid == pids[0][1]) ? matrixInfo.sym1 : matrixInfo.sym2;
               break;
           }
       }
       /// PRINT MATRIX
       print_matrix(matrixInfo);
       /// CHECK WIN
       win = check_win(matrixInfo,move.pid);
       /// IF WIN
       if(win.value==1 ){ // move.pid ha vinto, other_pid ha perso
           printf("Vincita!\n");
           /// END CLIENTS
           kill(move.pid,SIGUSR1);
           kill(other_pid,SIGUSR1);
           printf("\n");
           send_win(win);
           blocca(SERVER);
           send_win(win);
           blocca(SERVER);
           exit(0);
       } else if (win.value==-2) { // pareggio
           printf("Pareggio");
           kill(move.pid,SIGUSR1);
           kill(other_pid,SIGUSR1);
           printf("\n");
           send_win(win);
           send_win(win);
           exit(0);
       } else {
           /// NO WIN
           printf("La partita continua\n");
           send_win(win);
       }
    } while(win.value==0);

    return 0;
}
