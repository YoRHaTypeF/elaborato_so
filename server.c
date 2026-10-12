/// @file server.c
/// @brief Contiene l'implementazione del server.

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "defines.h"
#include "err_exit.h"
#include "shared_memory.h"
#include "fifo.h"
#include "semaphore.h"

#define LEN 3

int cont0 = 0; // lunghezza SHM

struct myMsg{
  long mtype;
  char mtext[500];
} m;

struct fifoMsg{
  int pid;
  char ftext[500];
} fifom;

typedef struct messaggio{
  char pid[10];
  char number[5];
  char text[1100];
  char path[100];
  int sort;
};

int create_sem_set(key_t semkey){
  int semid = semget(semkey, 5, IPC_CREAT | S_IRUSR | S_IWUSR);
  if(semid == -1) ErrExit("Server: semget fallita\n");

  union semun arg;

  // 0 = fifo | 1 = fifo2 | 2 = SHM | 3 = MSQ | 4 fifo1
  unsigned short values[] = {0, 1, 1, 1, 1};
  arg.array = values;

  if(semctl(semid, 0, SETALL, arg) == -1) ErrExit("Server: semctl SETALL fallita\n");

  return semid;
}

struct messaggio create_string(char msg[], int type){
  //printf("Funzione creazione messaggio\n\n");
  char s_pid[100];
  int i_pid = 0;
  char num[100];
  int i_num = 0;
  char dir[100];
  int i_dir = 0;
  char message[4000];
  int i_message = 0;
  char sort[100];
  int i_sort;

  struct messaggio messaggio_s;

  char stringa[5000];

  int cont = 0;

  while(msg[cont] != '\n'){
    if(msg[cont] != 'M'){
      s_pid[i_pid] = msg[cont];
      cont++;
      i_pid++;
    }
    else{
      cont++;
    }
  }
  
  //printf("%s\n", s_pid);
  cont++;

  while(msg[cont] != '\n'){
    num[i_num] = msg[cont];
    cont++;
    i_num++;
  }
  strcpy(sort, s_pid);
  strcat(sort, num);
  strcat(sort, "\0");

  num[i_num] = '\0';
  s_pid[i_pid] = '\0';
  //printf("%s\n", num);
  cont++;

  while(msg[cont] != '\n'){
    dir[i_dir] = msg[cont];
    cont++;
    i_dir++;
  }
  dir[i_dir] = '\0';
  //printf("%s\n", dir);
  cont++;

  while(msg[cont] != '\0'){
    message[i_message] = msg[cont];
    cont++;
    i_message++;
  }
  message[i_message] = '\0';
  //printf("%s\n", message);

  strcpy(stringa, "[Parte ");
  strcat(stringa, num);
  strcat(stringa, ", del file ");
  strcat(stringa, dir);
  strcat(stringa, ", spedita dal processo ");
  strcat(stringa, s_pid);
  if(type == 1) strcat(stringa, " tramite FIFO1]\n");
  if(type == 2) strcat(stringa, " tramite FIFO2]\n");
  if(type == 3) strcat(stringa, " tramite MSQ]\n");
  if(type == 4) strcat(stringa, " tramite SHM]\n");
  
  strcat(stringa, message);

  strcpy(messaggio_s.pid, s_pid);
  strcpy(messaggio_s.number, num);
  strcpy(messaggio_s.text, stringa);

  // metto _out in fondo al nome del file
  int leng = strlen(dir);
  
  dir[leng] = '_';
  dir[leng+1] = 'o';
  dir[leng+2] = 'u';
  dir[leng+3] = 't';
  dir[leng+4] = '\0';
  strcpy(messaggio_s.path, dir);

  i_sort = atoi(sort);
  messaggio_s.sort = i_sort;

  return messaggio_s;

  //printf("%s\n\n", stringa);

  //return stringa;


}

int main(int argc, char * argv[]) {

  int numb_list = 0;
  struct messaggio messaggi;
  

  // chiavi ipc
  key_t shmkey = 01101101; // shared memory
  key_t msqkey = 01110001; // message queue
  key_t semkey = 01110011; // semaphore
  //key_t semkey2 = 01110101; // fifo1 semaphore

  // creazione FIFO
  int fifo1;
  int fifo2;
  char *fifop1 = "/tmp/fifo1"; // path della fifo 1
  char *fifop2 = "/tmp/fifo2"; // path della fifo 2
  // creazione fifo 1
  fifo1 = mkfifo(fifop1, S_IRUSR | S_IWUSR | O_APPEND);
  if(fifo1 == -1) ErrExit("SERVER: errore creazione FIFO 1\n");
  //printf("Server: FIFO1 creata\n");
  // creazione fifo 2
  fifo2 = mkfifo(fifop2, S_IRUSR | S_IWUSR | O_APPEND);
  if(fifo2 == -1)
    ErrExit("SERVER: errore creazione FIFO 2\n");

  // creazione SHARED MEMEORY
  int shmid;          // id shared memory
  shmid = alloc_shared_memory(shmkey, 5000);
  char *shm = (char *)get_shared_memory(shmid, 0);
  char *shm1 = shm;

  // creazione message queue
  int msqid;
  msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR);
  if(msqid == -1)
    ErrExit("SERVER: errore creazione message queue\n");

  // creazione semafori
  int semid = create_sem_set(semkey);
  //int semid1 = semget(semkey2, 1, S_IRUSR | S_IWUSR);

  // server apre la prima fifo
  //printf("Server apre FIFO1\n");
  int fifoserver = open(fifop1, O_RDONLY | O_APPEND);
  if(fifoserver == -1) ErrExit("Server: errore apertura FIFO1\n");

  int fifoserver2 = open(fifop2, O_RDONLY | O_APPEND);
  if(fifoserver2 == -1) ErrExit("Server: errore apertura FIFO2\n");

  // leggo dalla fifo
  char readfifo1[LEN];
  read(fifoserver, readfifo1, LEN);

  printf("Server: il numero di file è: %s\n", readfifo1);

  int number_of_files = atoi(readfifo1);
  
  // scrivo nella shared memory la conferma per client 0
  shm[0] = 'S';
  shm[1] = 'T';
  shm[2] = 'A';
  shm[3] = 'R';
  shm[4] = 'T';
  shm[5] = '\0';
  cont0 = 6;
  printf("Server: ho inviato START a Client\n");
  semOp(semid, 0, 1, 0);     // sblocco il primo semaforo in modo che client possa scrivere sulla SH

  
  struct messaggio lista_messaggi[number_of_files*4];   // array di struct messaggio
  int posizione_lista = 0;
  int prova = 0;
  while(prova < number_of_files*4){   // number_of_files = number_of_files * 4 per quando server leggerà da fifo1, fifo2, msgq, shm
    /***************************************************************** LETTURA SU FIFO1 ****************************************************************/

    int x_fifo1 = semOp1(semid, 4, -1);

    if(x_fifo1 == -1){
      ErrExit("Server: errore lettura messaggio su fifo1\n");
    }
    else if(x_fifo1 == 2){    // fifo1 libera
      if(read(fifoserver, &fifom, sizeof(struct fifoMsg)) > 0){
        lista_messaggi[posizione_lista] = create_string(fifom.ftext, 1);
        posizione_lista++;
        //printf("\nServer FIFO1: %s\n", fifom.ftext);
        prova++;
      }
    }
    x_fifo1 = semOp1(semid, 4, 1);

    /***************************************************************************************************************************************************/

    /***************************************************************** LETTURA SU FIFO2 ****************************************************************/

    int x_fifo2 = semOp1(semid, 1, -1);

    char msg_fifo2[LEN];

    if(x_fifo2 == -1){
      ErrExit("Server: errore lettura messaggio su fifo2\n");
    }
    else if(x_fifo2 == 2){    // fifo1 libera
      if(read(fifoserver2, &fifom, sizeof(struct fifoMsg)) > 0){
        lista_messaggi[posizione_lista] = create_string(fifom.ftext, 2);
        posizione_lista++;
        //printf("\nServer FIFO2: %s\n", fifom.ftext);
        prova++;
      }
    }
    x_fifo2 = semOp1(semid, 1, 1);

    /***************************************************************************************************************************************************/

    /************************************************************** LETTURA SU MESSAGE QUEUE ***********************************************************/

    int x_msq = semOp1(semid, 3, -1);

    size_t mSize = sizeof(struct myMsg);

    if(x_msq == -1){
      ErrExit("Server: errore lettura messagio su MSQ\n");
    }
    else if(x_msq == 2){    // MSQ libera
      if(msgrcv(msqid, &m, mSize, 1, IPC_NOWAIT) == -1){
        if(errno == ENOMSG){
          // non ha trovato messaggi con type = 1
        }
        else{
          ErrExit("Server: msgrcv fallita in lettura MSQ\n");
        }
      }
      else{
        /* lettura da MSQ */
        lista_messaggi[posizione_lista] = create_string(m.mtext, 3);
        posizione_lista++;
        //printf("\nServer MSQ: %s\n", m.mtext);
        prova++;

      }
    }
    x_msq = semOp1(semid, 3, 1);

    /***************************************************************************************************************************************************/

    /*************************************************************** LETTURA DA SHARED MEMORY **********************************************************/

    char msg4[100] = "";
    //sleep(3);
    int x1 = semOp1(semid, 2, -1);
    if(x1 == -1){
      ErrExit("Server: errore lettura messagio su SHM\n");
    }
    else if(x1 == 2){   // SHM libera
      //printf("\n\nLeggo da SHM\n\n");
      int cont1 = 0;
      do{               // salva il messaggio in SHM
        msg4[cont1] = shm[cont0];
        if(msg4[0] == 'M'){
          cont0++;
          cont1++;
        }
      }while(shm[cont0] != '\0');
      //cont0++;
      msg4[cont1] = '\0';
      
      if(msg4[0] == 'M'){
        //char messaggio[] = create_string(msg4);
        //printf("\nServer SHM: %s\n", msg4); //create_string(msg4)
        lista_messaggi[posizione_lista] = create_string(msg4, 4);
        posizione_lista++;
        //create_string(msg4);
        cont0++;
        prova++;
      }
      strcpy(msg4, "");
      
    }
    x1 = semOp1(semid, 2, 1);

    /***************************************************************************************************************************************************/
  }

  /*printf("Lista non ordinata:\n");
  for(int i=0; i<number_of_files*4; i++){
    printf("\n%s", lista_messaggi[i].text);
  }*/

  //char pid_list[100][1500];

  /*
  ** riordino i messaggi in base al processo che li ha inviati
  ** e alla parte di file che corrispondono
  */
  struct messaggio temp_messaggio;
  for(int i=0; i<(number_of_files*4); i++){
    for(int j=i+1; j<(number_of_files*4); j++){
      if(lista_messaggi[i].sort > lista_messaggi[j].sort){
        temp_messaggio = lista_messaggi[i];
        lista_messaggi[i] = lista_messaggi[j];
        lista_messaggi[j] = temp_messaggio;
      }
    }
  }

  /*printf("\n\n\nLista ordinata:\n");
  for(int i=0; i<number_of_files*4; i++){
    printf("\n%s\n", lista_messaggi[i].text);
  }*/

  char complete_msg[number_of_files][1000];
  int ii = 0;
  int contt = 0;
  for(int i=0; i<number_of_files*4; i++){
    strcat(complete_msg[ii], lista_messaggi[i].text);
    strcat(complete_msg[ii], "\n\n");
    contt++;
    if(contt == 4){
      contt = 0;
      ii++;
    }
  }

  /*for(int i=0; i<number_of_files; i++){
    printf("\n%s\n", complete_msg[i]);
  }*/

  int iii = 0;
  for(int i=0; i<number_of_files; i++){
    int fd;
    fd = open(lista_messaggi[iii].path, O_WRONLY | O_APPEND | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(fd == -1){
      if(errno != EEXIST) ErrExit("Server: errore open file\n");
    }

    ssize_t numWrite = write(fd, complete_msg[i], strlen(complete_msg[i]));
    if(numWrite != strlen(complete_msg[i])) ErrExit("Server: errore write nel file\n");
    iii += 4;

    close(fd);
  }

  


  close(fifoserver);
  close(fifoserver2);
  
  unlink(fifop1);
  unlink(fifop2);

  return 0;
}