/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <stdio.h>
#include <stdbool.h> 
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
#include <linux/limits.h>
#include <dirent.h>
#include <errno.h>

#include "defines.h"
#include "err_exit.h"
#include "shared_memory.h"
#include "fifo.h"
#include "semaphore.h"

#define MAX_READ 4000

char save_file[100][100];
int countfile = 0;
int cont0 = 0; // conto caratteri SHM

size_t append2Path(char *directory);
void search();


bool termina = false;  // indica se è arrivato SIGUSR1
bool continua = false; // indica se è arrivato SIGINT
bool segnale = false;  // indica se è arrivato uno dei due segnali
char cdir[250];

struct myMsg{
  long mtype;
  char mtext[500];
} m;

struct fifoMsg{
  int pid;
  char ftext[500];
} fifom;


// sighandler
void sigHandlerParent(int sig){
  // se arriva SIGUSR1
  if(sig == SIGUSR1){
    printf("Client_0 ha ricevuto SIGUSR1\n");
    termina = true;
    segnale = true;
  }
  // se arriva SIGINT
  if(sig == SIGINT){
    printf("Client_0 ha ricevuto SIGINT\n");
    continua = true;
    segnale = true;
  }
}

int create_sem_set(key_t semkey, int num){
  int semid = semget(semkey, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
  if(semid == -1) ErrExit("Client: semget fallita\n");

  union semun arg;
  arg.val = num;

  if(semctl(semid, 0, SETVAL, arg) == -1) ErrExit("Client: semctl SETALL fallita\n");

  return semid;
}

int main(int argc, char * argv[]) {  

  key_t shmkey = 01101101;  // shared memory
  key_t shmkey1 = 01101111; // shared memory per cont0
  key_t msqkey = 01110001;  // message queue
  key_t semkey = 01110011;  // semaphores
  key_t semkey1 = 01110100; // children semaphores
  //key_t semkey2 = 01110101; // ipcs semaphore

  char strcountfile[10];
  char *fifop1 = "/tmp/fifo1";
  char *fifop2 = "/tmp/fifo2";
  

  char dir[] = "";
  strcpy(dir, argv[1]); // copio la directory in input dentro la stringa dir

  char cwdir[PATH_MAX]; // stringa che conterrà la directory attuale di Client_0
  char uid[PATH_MAX];   // stringa che conterrà l'user

  int pausa;

  sigset_t mySet;                            // set di segnali non inizializzato
  sigfillset(&mySet);                        // inizializzo pieno
  sigdelset(&mySet, SIGUSR1);                // tolgo SIGUSR1
  sigdelset(&mySet, SIGINT);                 // tolgo SIGINT
  sigprocmask(SIG_SETMASK, &mySet, NULL);    // creo la maschera

  printf("Sono Client_0 e mi metto in attesa di un segnale\n");
  // setto la signal handler per essere eseguita con SIGINT e SIGUSR1
  if(signal(SIGINT, sigHandlerParent) == SIG_ERR || signal(SIGUSR1, sigHandlerParent) == SIG_ERR)
    ErrExit("errore signal handler\n");

  do{
    
    pausa = pause();

  }while(!segnale);
  //printf("Sono Client_0 e ho ricevuto un segnale\n");

  if(!termina){
    if(continua){
      //printf("Sono Client_0, ho ricevuto SIGINT e ora continuo\n");
      // modifico la maschera in modo che blocchi tutti i signals
      sigaddset(&mySet, SIGUSR1);      // aggiungo SIGUSR1
      sigaddset(&mySet, SIGINT);       // aggiungo SIGINT
      sigprocmask(SIG_SETMASK, &mySet, NULL);
      //printf("Sono Client_0 e ho modificato la maschera\n");

      int msqid = msgget(msqkey, S_IRUSR | S_IWUSR);

      //printf("La directory da cambiare è %s\n", dir);
      /*int new_dir = mkdir(dir, S_IRUSR | S_IXUSR);  // nel caso si debba creare myDir
      if(new_dir == 0)
        printf("La directory myDir è stata crata\n");*/

      if(chdir(dir) == -1)             // cambio directory corrente
        ErrExit("Errore Client_0 in cambio directory\n");
      
      getcwd(cwdir, PATH_MAX);         // ottengo la directory attuale
      //printf("L'attuale directory di Client_0 è: %s\n", cwdir);
      getlogin_r(uid, PATH_MAX);       // ottengo l'user
      //printf("L'utente è: %s\n", uid);
      
      printf("Ciao %s, ora inizio l'invio dei file contenuti in %s\n", uid, cwdir);

      strcpy(cdir, cwdir);
      search();       // cerco i file nella directory

      /*printf("Ho trovato %d file che rispettano i requisiti:\n", countfile);
      for(int i=0; i<countfile; i++)
        printf("\t\t\t\t\t%s\n", save_file[i]);*/
      sprintf(strcountfile, "%d", countfile);   // trasformo il numero di file trovati da int a string con la sprintf

      // apro la fifo1
      printf("Client apre FIFO1\n");
      int fifoclient = open(fifop1, O_WRONLY | O_APPEND);
      if(fifoclient == -1) ErrExit("Client: errore apertura FIFO1\n");
      // apro la fifo2
      int fifoclient2 = open(fifop2, O_WRONLY | O_APPEND);
      if(fifoclient2 == -1) ErrExit("Client: errore apertura FIFO1\n");

      // scrivo il numero di file trovati nella fifo
      if(write(fifoclient, &strcountfile, strlen(strcountfile)) == -1)
        ErrExit("Client: errore write fifo1\n");

      //close(fifoclient);

      // allocazione shared memory
      int shmid = alloc_shared_memory(shmkey, 500);
      // ottengo il puntatore alla shared memory
      char *shm = (char *)get_shared_memory(shmid, 0);
      char *shm1 = shm;
      // shared memory per cont0
      int shmid1;          
      shmid1 = alloc_shared_memory(shmkey1, sizeof(int *));
      int *shm2 = (int *)get_shared_memory(shmid1, 0);
      // ottengo i semafori
      int semid = semget(semkey, 2, S_IRUSR | S_IWUSR);
      int semid1 = create_sem_set(semkey1, countfile);
      //int semfifo = create_sem_set(semkey2, 0);
      
      // client 0 si blocca qui finchè server non sblocca il semaforo dopo aver scritto la conferma in SHM
      semOp(semid, 0, -1, 0);
      // creo una stringa in cui salvare il contenuto della SHM
      char start[10];
      
      // salvo il contenuto della SHM nella stringa start
      while(shm1[cont0] != '\0'){
        start[cont0] = shm1[cont0];
        cont0++;
      }
      start[cont0] = '\0';
      cont0++;
      *shm2 = cont0;

      // controllo se server ha scritto in SHM la conferma
      if(strcmp(start, "START") == 0){
        // inizio a inviare le informazioni a server
        //printf("Client: posso iniziare!\n");
        /*
        **
        **CODICE
        **
        */

        // ciclo for per creare i figli 
        for(int i=0; i<countfile; i++){

          int pid;
          pid = fork();
          if(pid == -1) ErrExit("Errore creazione figlio\n");
          else if(pid == 0){
            /* CODICE FIGLIO */
            char filepath[100];
            strcpy(filepath, save_file[i]);                                   // salvo il path del file su cui deve lavorare il figlio

            int fd;
            fd = open(filepath, O_RDONLY, S_IRWXU);                           // apro il file
            if(fd == -1) ErrExit("Errore nella open di uno dei figli\n");

            char buffer[MAX_READ + 1];                                        // stringa che conterrà il contenuto del file  

            ssize_t numRead = read(fd, buffer, MAX_READ);                     // leggo il contenuto del file
            if(numRead == -1) ErrExit("Errore nella read di uno dei figli\n");

            

            //printf("Figlio %d: contenuto file: %s\n", i, buffer);

            int totalchar = strlen(buffer)-1;                                 // determino il numero di caratteri totali nel file;
            //printf("i caratteri del file sono: %d\n", totalchar);
            int charpieces = totalchar/4;                                     // divido in 4 parti uguali il file
            if((totalchar%4) != 0) charpieces++;                              // se i caratteri non sono divisibili per 4 aumento di 1 la quantità di caratteri contenuta dalle prime 3 parti

            char string1[charpieces+1];                                       // creo le stringhe che conterranno i caratteri
            char string2[charpieces+1];
            char string3[charpieces+1];
            char string4[charpieces+1];

            int j1 = 0;
            int j2 = 0;
            int j3 = 0;
            int j4 = 0;
            for(int j=0; j<totalchar; j++){                                   // inserisco i caratteri nelle stringhe
              if(j < charpieces){
                string1[j1] = buffer[j];
                j1++;
              }
              else if(j >= charpieces && j < (charpieces * 2)){
                string2[j2] = buffer[j];
                j2++;
              }
              else if(j >= (charpieces * 2) && j < (charpieces * 3)){
                string3[j3] = buffer[j];
                j3++;
              }
              else if(j >= (charpieces * 3)){
                string4[j4] = buffer[j];
                j4++;
              }
            }

            string1[j1] = '\0';   // carattere fine stringa
            string2[j2] = '\0';   // carattere fine stringa
            string3[j3] = '\0';   // carattere fine stringa
            string4[j4] = '\0';   // carattere fine stringa

            //printf("Figlio %d:\n%s\n%s\n%s\n%s\n\n", i, string1, string2, string3, string4);  // controllo se la divisione del file è avvenuta correttamente


            /******************************************/
            /* CREO I 4 MESSAGGI DA INVIARE AL SERVER */
            /******************************************/
            /* messaggio da inviare su FIFO 1 */
            char msg1[200];
            sprintf(msg1, "%d", getpid());
            strcat(msg1, "\n1\n");
            strcat(msg1, filepath);
            strcat(msg1, "\n");
            strcat(msg1, string1);
            /* messaggio da inviare su FIFO 2 */
            char msg2[200];
            char msg2_2[200];
            //msg2[0] = 'M';
            sprintf(msg2_2, "%d", getpid());
            strcat(msg2, msg2_2);
            strcat(msg2, "\n2\n");
            strcat(msg2, filepath);
            strcat(msg2, "\n");
            strcat(msg2, string2);
            /* messaggio da inviare su MS QUEUE */
            char msg3[200];
            sprintf(msg3, "%d", getpid());
            strcat(msg3, "\n3\n");
            strcat(msg3, filepath);
            strcat(msg3, "\n");
            strcat(msg3, string3);
            /* messaggio da inviare su SH MEMORY */
            char msg4[200];
            char msg4_2[200];
            msg4[0] = 'M';
            sprintf(msg4_2, "%d", getpid());
            strcat(msg4, msg4_2);
            strcat(msg4, "\n4\n");
            strcat(msg4, filepath);
            strcat(msg4, "\n");
            strcat(msg4, string4);

            /*printf("Figlio %d, messaggio 1: %s\n", i, msg1);  // controlli per vedere se i messaggi sono stati salvati correttamente
            printf("Figlio %d, messaggio 2: %s\n", i, msg2);
            printf("Figlio %d, messaggio 3: %s\n", i, msg3);
            printf("Figlio %d, messaggio 4: %s\n", i, msg4);*/


            //printf("Figlio %d si mette in attesa\n", i);
            semOp(semid1, 0, -1, 0);   // il figlio diminuisce il valore del semaforo
            semOp(semid1, 0, 0, 0);    // se il valore non è 0 rimane bloccato
            //printf("Figlio %d si è sbloccato\n", i);

            int inviofile = 0;    // controllo per invio dei 4 file

            int invio_msg1 = 0;   // controllo invio messaggi
            int invio_msg2 = 0;
            int invio_msg3 = 0;
            int invio_msg4 = 0;
            

            while(inviofile < 4){

              /*************************************************************** INVIO SU FIFO 1 ********************************************************************/

              if(invio_msg1 != 1){
                int x_fifo1 = semOp1(semid, 4, -1);
                if(x_fifo1 == -1){
                  ErrExit("Figlio: errore semop fifo1\n");
                }
                else if(x_fifo1 == 2){    // fifo 1 libera
                  /*
                  ** CODICE **
                  */
                  fifom.pid = getpid();
                  memcpy(fifom.ftext, msg1, strlen(msg1)+1);
                  size_t fSize = sizeof(struct fifoMsg);
                  if(write(fifoclient, &fifom, fSize) == -1){
                    ErrExit("Client: errore write fifo1\n");
                  }
                  else{
                    printf("Figlio%d: inviato msg1 su fifo1\n", i);
                    inviofile++;
                    invio_msg1 = 1;
                  }
                }
                x_fifo1 = semOp1(semid, 4, 1);
              }

              /****************************************************************************************************************************************************/

              /*************************************************************** INVIO SU FIFO 2 ********************************************************************/

              if(invio_msg2 != 1){
                int x_fifo2 = semOp1(semid, 1, -1);
                if(x_fifo2 == -1){
                  ErrExit("Figlio: errore semop fifo2\n");
                }
                else if(x_fifo2 == 2){    // fifo 1 libera
                  /*
                  ** CODICE **
                  */
                  fifom.pid = getpid();
                  memcpy(fifom.ftext, msg2, strlen(msg2)+1);
                  size_t fSize = sizeof(struct fifoMsg);
                  if(write(fifoclient2, &fifom, fSize) == -1){
                    ErrExit("Client: errore write fifo1\n");
                  }
                  else{
                    printf("Figlio%d: inviato msg2 su fifo2\n", i);
                    inviofile++;
                    invio_msg2 = 1;
                  }
                }
                x_fifo2 = semOp1(semid, 1, 1);
              }

              /****************************************************************************************************************************************************/

              /*********************************************************** INVIO SU MESSAGE QUEUE *****************************************************************/

              if(invio_msg3 != 1){
                int x_msq = semOp1(semid, 3, -1);
                if(x_msq == -1){
                  ErrExit("Figlio: errore semop MSQ\n");
                }
                else if(x_msq == 2){    // MSQ libera
                  m.mtype = 1;
                  memcpy(m.mtext, msg3, strlen(msg3)+1);
                  size_t mSize = sizeof(struct myMsg) - sizeof(long);

                  if(msgsnd(msqid, &m, mSize, IPC_NOWAIT) == -1){
                    if(errno != EAGAIN){
                      ErrExit("Errore scrittura su Message Queue\n");
                    }
                  }
                  else{
                    printf("Figlio%d: inviato msg3 su MSQ\n", i);
                    invio_msg3 = 1;
                    inviofile++;
                  }
                }
                x_msq = semOp1(semid, 3, 1);
              }

              /****************************************************************************************************************************************************/

              /*********************************************************** INVIO SU SHARED MEMORY *****************************************************************/

              if(invio_msg4 != 1){
                int x1 = semOp1(semid, 2, -1);
                if(x1 == -1){   // errore semop
                  ErrExit("Figlio: errore semop SHM\n");
                }
                else if(x1 == 2){   // SHM libera
                int cont1 = *shm2;
                  for(int ii=0; ii<strlen(msg4); ii++){
                    shm1[cont1] = msg4[ii];
                    cont1++;
                  }
                  printf("Figlio%d: inviato msg4 su SHM\n", i);
                
                  shm1[cont1] = '\0';
                  *shm2 = cont1+1;

                  inviofile++;
                  invio_msg4 = 1;
                
                }
                x1 = semOp1(semid, 2, 1);
              }

              /****************************************************************************************************************************************************/ 

            }
            
            _exit(0);   // il figlio termina
          }            
        }
              
      }else{        // ERRORE
        printf("Client: ERRORE, server mi ha inviato su SH una conferma sbagliata: %s\n", start);
      }
      
      close(fifoclient);
      close(fifoclient2);
      
    }
  }
  // PADRE attende che i figli terminino
  pid_t child;
  while((child = wait(NULL)) != -1){
    //printf("Figlio %d ha terminato\n", child);
  }

  printf("\nSono Client_0 e adesso termino\n");  
  
  return 0;
}





size_t append2Path(char *directory) {
    size_t lastPathEnd = strlen(cdir);
    // extends current seachPath: seachPath + / + directory
    strcat(strcat(&cdir[lastPathEnd], "/"), directory);
    return lastPathEnd;
}
int checkFileSize(char *pathname, off_t size) {     // funzione per controllare la grandezza dei file
  int check = 0;
  struct stat statbuf;
  if (stat(pathname, &statbuf) == -1)
    return 0;
  if(statbuf.st_size <= size)
    check = 1;

  return check;
}

void search(){    // funzione per la ricerca dei file sendme_

  int match;
  
  
  DIR *dirp = opendir(cdir);
  if(dirp == NULL) return;

  errno = 0;

  struct dirent *dentry;
  while ((dentry = readdir(dirp)) != NULL){
    if (strcmp(dentry->d_name, ".") == 0 ||
      strcmp(dentry->d_name, "..") == 0)
    {  continue;  }

    if(dentry->d_type == DT_REG){
      size_t lastPath = append2Path(dentry->d_name);

      match = 0;
      char filename[] = "sendme_";
      if(strncmp(filename, dentry->d_name, 7) == 0){
        if(checkFileSize(cdir, 4000) == 1){
          match = 1;
        }
      }
      if(match == 1){
        strcpy(save_file[countfile], cdir);
        countfile++;
      }
      cdir[lastPath] = '\0';
    }else if(dentry->d_type == DT_DIR){
      size_t lastPath = append2Path(dentry->d_name);
      search();
      cdir[lastPath] = '\0';
    }
    errno = 0;
  }
  if(errno != 0)
    ErrExit("readdir failed\n");
  if(closedir(dirp) == -1)
    ErrExit("closeddir faild\n");
}
