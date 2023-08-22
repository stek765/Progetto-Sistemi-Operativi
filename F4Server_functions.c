/************************************
* Matricole : VR456187  -  VR422276  -  VR456008
* Nomi e Cognomi : Oualid Maftah  -  Valeria Stuani  -  Stefano Zanolli
* Data realizzazione : 02/05/2023
*************************************/

#include "F4Server_functions.h"

unsigned short arr[3] = {0, 0, 0};

/// STRUTTURA PER DISTINGUERE I GIOCATORI
int pids[2][2]={{0,0},
                {1,0}};

int sem_key, sem_num=3, sem_id;
int msq_key, msq_id;
int shm_key, shm_id;
char *shm_ptr;

void usage(){
    printf("usage\n");
    printf("./F4Server <righe> <colonne> <simbolo 1> <simbolo 2>\n");
    printf("righe : min 5\n");
    printf("colonne : min 5\n");
    printf("simbolo 1 : max 1 carattere\n");
    printf("simbolo 2 : max 1 carattere\n");
};

void err_exit(char* str){
    perror(str);
    exit(EXIT_FAILURE);
};

void check_server_args(int argc, char** argv){
    if (argc != 5) {
        usage();
        err_exit("[ERROR]: Mi hai passato un numero errato di argomenti.\n");
    } else if (atoi(argv[1]) < 5) {
        usage();
        err_exit("[ERROR]: La matrice non può avere meno di 5 righe.\n");
    } else if (atoi(argv[2]) < 5) {
        usage();
        err_exit("[ERROR]: La matrice non può avere meno di 5 colonne.\n");
    } else if (strlen(argv[3]) != sizeof(char) || strlen(argv[4]) != sizeof(char)) {
        usage();
        err_exit("[ERROR]: i simboli devono essere lunghi 1 carattere ciascuno.\n");
    } else if (strcmp(argv[3], argv[4]) == 0) {
        err_exit("[ERROR]: i simboli devono essere diversi.\n");
    }
};

matrix_info_t initialize_matrix_info(char** argv){
    matrix_info_t matrixInfo = (matrix_info_t) {
            .mtype=MATRIX_INFO,
            .rows=atoi(argv[1]),
            .cols=atoi(argv[2]),
            .sym1=argv[3][0],
            .sym2=argv[4][0],
            .server_pid=getpid()
    };
    return matrixInfo;
};

void print_matrix_info(matrix_info_t matrixInfo){
    printf("MATRIX INFO\n");
    printf("ROWS: %d\n", matrixInfo.rows);
    printf("COLS: %d\n", matrixInfo.cols);
    printf("SYM1: %c\n", matrixInfo.sym1);
    printf("SYM2: %c\n", matrixInfo.sym2);
};

void handle_signals(){
    sigset_t sigset;
    sigfillset(&sigset);

    sigdelset(&sigset, SIGUSR1);
    sigdelset(&sigset, SIGUSR2);
    sigdelset(&sigset, SIGINT);
    sigdelset(&sigset, SIGALRM);
    sigdelset(&sigset, SIGHUP);
    sigdelset(&sigset, SIGTERM);

    sigprocmask(SIG_SETMASK, &sigset, NULL);

    signal(SIGUSR1, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGALRM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
};

void signal_handler(int sig){
    static int sigint_count=0;

    switch (sig) {
        case SIGUSR1:
            //printf("SIGUSR1\n");
            printf("Un client ha abbandonato\n");
            player_info_t playerInfo = receive_player_info();
            int other_pid = (pids[0][1] == playerInfo.pid) ? pids[1][1] : pids[0][1];
            if(other_pid!=0){
                printf("Segnalo la vincita all'altro client\n");
                kill(other_pid, SIGUSR2);
                blocca(SERVER);
                blocca(SERVER);
                exit(0);
                printf("\n");
            } else {
                printf("Non è collegato nessun altro client\n");
                exit(0);
            }
            break;
        case SIGUSR2:
            //printf("SIGUSR2\n");
            break;
        case SIGINT:
            //printf("SIGINT\n");
            if(sigint_count==0){
                printf("[WARNING]: CTRL-C 1: premi un'altra volta per terminare il server\n");
                sigint_count++;
                alarm(10);
            } else {
                printf("[NOTE]: CTRL-C 2: termino il server...\n");
                if(pids[0][1]!=0 || pids[1][1]!=0){
                    kill(pids[0][1], SIGTERM);
                    blocca(SERVER);
                    kill(pids[1][1], SIGTERM);
                    blocca(SERVER);
                }
                exit(0);
            }
            break;
        case SIGALRM:
            //printf("SIGALRM\n");
            sigint_count=0;
            break;
        case SIGHUP:
            //printf("SIGHUP\n");
            if(pids[0][1]!=0 || pids[1][1]!=0){
                kill(pids[0][1], SIGTERM);
                blocca(SERVER);
                kill(pids[1][1], SIGTERM);
                blocca(SERVER);
            }
            exit(0);
            break;
        case SIGTERM:
            //printf("SIGTERM\n");
            break;
        default:
            //printf("DEFAULT\n");
            break;
    }
};

void initialize_semaphore_set(){
    sem_key = ftok("../.", 'a');
    errno=0;
    sem_id = semget(sem_key,sem_num, IPC_CREAT | IPC_EXCL | 0777);
    if (sem_id == -1) {
        if (errno == EEXIST) {
            err_exit("SEMGET EEXIST ERROR\n");
        } else {
            err_exit("SEMGET UNKNOWN ERROR\n");
        }
    }
    set_semaphore_set_values(arr);
};

void remove_semaphore_set(){
    if(semctl(sem_id,IPC_RMID,0)==-1){
        err_exit("Remove Semaphore Set Failed\n");
    }
};

void set_semaphore_set_values(unsigned short* arr){
    semun arg;
    arg.array = arr;
    if(semctl(sem_id,sem_num,SETALL, arg)==-1){
        err_exit("SEMCTL SETALL\n");
    }
};

void get_semaphore_set_values(unsigned short* arr){
    semun arg = {.array=NULL};
    arg.array = arr;
    if(semctl(sem_id,sem_num,GETALL, arg)){
        err_exit("SEMCTL GETALL\n");
    }
}

void print_semaphore_set_values(unsigned short* arr){
    for(int i=0; i<sem_num; i++){
        printf("SEM[%d] = %u\n", i, arr[i]);
    }
}

void initialize_message_queue(){
    msq_key = ftok("../.", 'b');
    errno=0;
    msq_id = msgget(msq_key,IPC_CREAT | IPC_EXCL | 0666);
    if (msq_id == -1) {
        if (errno == EEXIST) {
            err_exit("MSGGET EEXIST ERROR\n");
        } else {
            err_exit("MSGGET UNKNOWN ERROR\n");
        }
    }
};

void remove_message_queue(){
    if(msgctl(msq_id,IPC_RMID,0)==-1){
        err_exit("Remove Message Queue Failed\n");
    }
};

char* initialize_shared_memory(matrix_info_t matrixInfo){
    shm_key = ftok("../.", 'c');
    size_t size = matrixInfo.rows*matrixInfo.cols*sizeof(char);
    errno=0;
    shm_id = shmget(shm_key, size, IPC_CREAT | IPC_EXCL | 0777);
    if (shm_id == -1) {
        if (errno == EEXIST) {
            err_exit("SHMGET EEXIST ERROR\n");
        } else {
            err_exit("SHMGET UNKNOWN ERROR\n");
        }
    }
    shm_ptr = (char *) shmat(shm_id,NULL,0);
    if (shm_ptr == NULL) {
        err_exit("SHMAT UNKNOWN ERROR\n");
    }
    return shm_ptr;
};

void detach_shared_memory(){
    if(shmdt(shm_ptr)==-1){
        err_exit("Detach Shared Memory Failed\n");
    }
};

void remove_shared_memory(){
    if(shmctl(shm_id,IPC_RMID,0)==-1){
        err_exit("Remove Shared Memory Failed\n");
    }
};

void initialize_matrix(matrix_info_t matrixInfo){
    char (*matrix)[matrixInfo.cols] = (void*) shm_ptr;
    for (int i = 0; i < matrixInfo.rows; i++) {
        for (int j = 0; j < matrixInfo.cols; j++) {
            matrix[i][j] = ' ';
        }
    }
};

void print_matrix(matrix_info_t matrixInfo){
    char (*matrix)[matrixInfo.cols] = (void*) shm_ptr;
    for (int i = 0; i < matrixInfo.rows; i++) {
        for (int j = 0; j < matrixInfo.cols; j++) {
            printf("[%c]",matrix[i][j]);
        }
        printf("\n");
    }
}

void blocca(int sem){
    struct sembuf sops = {
            .sem_num=sem,
            .sem_op=-1,
            .sem_flg=0
    };
    //printf("BLOCCO %d\n",sem);
    int ret=-1;
    do{
        ret = semop(sem_id,&sops,1);
        if(ret==-1){ perror("ERRORE SEMOP\n"); }
    } while(ret==-1 && errno==EINTR);
};

void sblocca(int sem){
    struct sembuf sops = {
            .sem_num=sem,
            .sem_op=+1,
            .sem_flg=0
    };
    //printf("SBLOCCO %d\n",sem);
    int ret=-1;
    do{
        ret = semop(sem_id,&sops,1);
        if(ret==-1){ perror("ERRORE SEMOP\n"); };
    } while(ret==-1 && errno==EINTR);
};

void send_matrix_info(matrix_info_t matrixInfo){
    size_t size = sizeof(matrix_info_t) - sizeof(long);
    msgsnd(msq_id,&matrixInfo,size,0);
};

player_info_t receive_player_info(){
    player_info_t playerInfo;
    size_t size = sizeof(player_info_t)- sizeof(long);
    msgrcv(msq_id, &playerInfo, size, (long) PLAYER_INFO, 0);
    return playerInfo;
};

void print_player_info(player_info_t playerInfo){
    printf("PLAYER INFO\n");
    //printf("NAME: %s\n", playerInfo.name);
    printf("PID: %d\n", playerInfo.pid);
};

move_t receive_move(){
    move_t move;
    size_t size = sizeof(move_t)- sizeof(long);
    int ret;
    do {
        ret = msgrcv(msq_id, &move, size, (long) MOVE, 0);
    } while(ret==-1);
    return move;
};

win_t check_win(matrix_info_t matrixInfo, int pid) {
    int i, j;
    char (*matrix)[matrixInfo.cols] = (void *) shm_ptr;
    win_t win = (win_t) {4, 0, pid};

    {
        for (i = 0; i < matrixInfo.rows; i++) {
            for (j = 0; j < matrixInfo.cols - 3; j++) {

                char elem = matrix[i][j];
                int row_win = ((elem != ' ') &&
                               (elem == matrix[i][j + 1]) &&
                               (elem == matrix[i][j + 2]) &&
                               (elem == matrix[i][j + 3]) ? 1 : 0);
                if (row_win == 1) {
                    win.value = 1;
                    return win;
                };
            }
        }
    } // ROW WIN

    {
        for (i = 0; i < matrixInfo.rows - 3; i++) {
            for (j = 0; j < matrixInfo.cols; j++) {

                char elem = matrix[i][j];
                int col_win = ((elem != ' ') &&
                               (elem == matrix[i + 1][j]) &&
                               (elem == matrix[i + 2][j]) &&
                               (elem == matrix[i + 3][j]) ? 1 : 0);
                if (col_win == 1) {
                    win.value = 1;
                    return win;
                };
            }
        }
    } // COL WIN

    {
        for (i = 3; i < matrixInfo.rows; i++) {
            for (j = 0; j < matrixInfo.cols - 3; j++) {
                char elem = matrix[i][j];
                int diag_win = ((elem != ' ') &&
                                (elem == matrix[i - 1][j + 1]) &&
                                (elem == matrix[i - 2][j + 2]) &&
                                (elem == matrix[i - 3][j + 3]) ? 1 : 0);
                if (diag_win == 1) {
                    win.value = 1;
                    return win;
                }
            }
        }
    } // DIAGONALE PRINCIPALE

    {
            for (i = 0; i < matrixInfo.rows - 3; i++) {
            for (j = 0; j < matrixInfo.cols - 3; j++) {
                char elem = matrix[i][j];
                int diag_win = ((elem != ' ') &&
                                (elem == matrix[i + 1][j + 1]) &&
                                (elem == matrix[i + 2][j + 2]) &&
                                (elem == matrix[i + 3][j + 3]) ? 1 : 0);
                if (diag_win == 1) {
                    win.value = 1;
                    return win;
                }
            }
        }
        } // DIAGONALE SECONDARIA

    {
        for (i = 0; i < matrixInfo.rows; i++) {
            for (j = 0; j < matrixInfo.cols; j++) {
                if (matrix[i][j] == ' ') {
                    // printf("Matrice non piena\n");
                    win.value = 0;
                    return win;
                }
            }
        }
    } // NESSUNO HA VINTO, C'È ANCORA POSTO

    {
        win.value = -2;
        return win;
    } // NON C'È PIÙ POSTO, HANNO PAREGGIATO
}

void send_win(win_t win){
    size_t size = sizeof(win_t) - sizeof(long);
    msgsnd(msq_id,&win,size,0);
}

void close_server(){
    printf("Closing server ...\n");

    //printf("Removing semaphore set\n");
    remove_semaphore_set();
    //printf("Removing message queue\n");
    remove_message_queue();
    //printf("Detaching shared memory\n");
    detach_shared_memory();
    //printf("Removing shared memory\n");
    remove_shared_memory();
}