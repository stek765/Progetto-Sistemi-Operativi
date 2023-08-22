/************************************
* Matricole : VR456187  -  VR422276  -  VR456008
* Nomi e Cognomi : Oualid Maftah  -  Valeria Stuani  -  Stefano Zanolli
* Data realizzazione : 02/05/2023
*************************************/

#include "F4Client_functions.h"

unsigned short arr[3] = {0, 0, 0};

int server_pid;

int sem_key, sem_num=3, sem_id;
int msq_key, msq_id;
int shm_key, shm_id;
char *shm_ptr;

void usage(){
    printf("usage\n");
    printf("./F4Client <nome del giocatore>\n");
    printf("Nome giocatore : massimo 50 caratteri");
}

void err_exit(char* str){
    perror(str);
    exit(EXIT_FAILURE);
}

void check_client_args(int argc, char** argv){
    if (argc != 2) {
        usage();
        err_exit("[ERROR]: Mi hai passato un numero errato di argomenti.\n");
    } else if (strcmp(argv[1], "*") == 0) {
        printf("(opz) <Notifica il server di duplicarsi e lanciare una copia automatica del client>");
    } else if (strlen(argv[1]) > 50) {
        err_exit("[ERROR]: Il nome del giocatore è troppo lungo!");
    }
}

player_info_t initialize_player_info(char** argv){
    player_info_t playerInfo = (player_info_t) {
            .mtype=PLAYER_INFO,
            .name=argv[1],
            .pid=getpid()
    };
    return playerInfo;
}

void print_player_info(player_info_t playerInfo){
    printf("PLAYER INFO\n");
    printf("NAME: %s\n", playerInfo.name);
    printf("PID: %d\n", playerInfo.pid);
}

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
    signal(SIGUSR2, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGALRM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);

}

void signal_handler(int sig){
    switch (sig) {
        case SIGUSR1:
            //printf("\nSIGUSR1\n");
            win_t win = receive_win();
            //printf("win value: %d\nwin pid: %d\nmy pid: %d\n", win.value,win.pid,getpid());
            if(win.value==1){
                if(win.pid==getpid()){
                    printf("Hai vinto!\n");
                    exit(0);
                } else {
                    printf("Hai perso!\n");
                    exit(0);
                }
            } else if (win.value==-2) {
                printf("Hai pareggiato!\n");
                exit(0);
            }
            break;
        case SIGUSR2:
            //printf("\nSIGUSR2\n");
            printf("L'altro client ha abbandonato!\n");
            printf("Hai vinto!\n");
            //sblocca(SERVER);
            exit(0);
            break;
        case SIGINT:
            //printf("\nSIGINT\n");
            printf("\nAbbandono la partita (...)\n");
            //printf("server_pid : %d\n", server_pid);
            kill(server_pid,SIGUSR1);
            printf("\n");
            player_info_t playerInfo = (player_info_t) {(long) PLAYER_INFO,"",getpid()};
            send_player_info(playerInfo);
            exit(0);
            break;
        case SIGALRM:
            //printf("\nSIGALRM\n");
            break;
        case SIGHUP:
            //printf("\nSIGHUP\n");
            break;
        case SIGTERM:
            //printf("\nSIGTERM\n");
            printf("Il server è stato chiuso\n");
            sblocca(SERVER);
            exit(0);
            break;
        default:
            printf("\nSEGNALE NON GESTITO\n");
            break;
    }
}

void connect_semaphore_set(){
    sem_key = ftok("../.", 'a');
    sem_id = semget(sem_key,sem_num, 0777);
    if (sem_id == -1) {
        if (errno == EEXIST) {
            err_exit("SEMGET EEXIST ERROR\n");
        } else {
            err_exit("SEMGET UNKNOWN ERROR\n");
        }
    }
}

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

void connect_message_queue(){
    msq_key = ftok("../.", 'b');
    msq_id = msgget(msq_key,0666);
    if (msq_id == -1) {
        if (errno == EEXIST) {
            err_exit("MSGGET EEXIST ERROR\n");
        } else {
            err_exit("MSGGET UNKNOWN ERROR\n");
        }
    }
}

void connect_shared_memory(matrix_info_t matrixInfo){
    shm_key = ftok("../.", 'c');
    size_t size = matrixInfo.rows*matrixInfo.cols*sizeof(char);
    shm_id = shmget(shm_key, size, 0777);
    shm_ptr = (char *) shmat(shm_id,NULL,0);
}

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
        if(ret==-1){
            printf("ERRORE SEMOP\n");
            if(errno == EIDRM){
                err_exit("Non c'è più il set di semafori");
            } else {
                err_exit("SEMOP");
            }
        };
    } while(ret==-1 && errno==EINTR);
}

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
        if(ret==-1){
            printf("ERRORE SEMOP\n");
            if(errno == EIDRM){
                err_exit("Non c'è più il set di semafori");
            } else {
                err_exit("SEMOP");
            }
        };
    } while(ret==-1 && errno==EINTR);
}

matrix_info_t receive_matrix_info(){
    matrix_info_t matrixInfo;
    size_t size = sizeof(matrix_info_t)- sizeof(long);
    msgrcv(msq_id, &matrixInfo, size, (long) MATRIX_INFO, 0);
    return matrixInfo;
}

void print_matrix_info(matrix_info_t matrixInfo){
    printf("MATRIX INFO\n");
    printf("ROWS: %d\n", matrixInfo.rows);
    printf("COLS: %d\n", matrixInfo.cols);
    printf("SYM1: %c\n", matrixInfo.sym1);
    printf("SYM2: %c\n", matrixInfo.sym2);
}

void send_player_info(player_info_t playerInfo){
    size_t size = sizeof(player_info_t) - sizeof(long);
    msgsnd(msq_id,&playerInfo,size,0);
}

void send_move(move_t move){
    size_t size = sizeof(move_t) - sizeof(long);
    msgsnd(msq_id,&move,size,0);
}

move_t inserisci_colonna(matrix_info_t matrixInfo){
    char (*matrix)[matrixInfo.cols] = (void*) shm_ptr;
    int col=0;
    move_t mossa = (move_t){(long)MOVE,0,0};
    do {
        printf("\nInserisci una colonna (tra 1 e %d): ",matrixInfo.cols);
        // SALVO LA MOSSA //
        scanf("%d", &col);
        if(matrix[0][col-1]!=' '){
            printf("La colonna è piena!\n");
        } else{
            mossa = (move_t){3,col,getpid()};
        }
    } while(col<1 || col>matrixInfo.cols || matrix[0][col-1]!=' ' );
    return mossa;
}

win_t receive_win(){
    win_t win;
    size_t size = sizeof(win_t)- sizeof(long);
    msgrcv(msq_id, &win, size, (long) WIN, 0);
    //printf("value : %d\npid: %d\n",win.value,win.pid);
    return win;
};

void detach_shared_memory(){
    if(shmdt(shm_ptr)==-1){
        err_exit("Detach Shared Memory Failed\n");
    }
};

void close_client(){
    printf("Closing Client ...\n");
    printf("Detach of shared memory\n");
    detach_shared_memory();
    sblocca(SERVER);
};

