/// @file shared_memory.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#include <sys/shm.h>
#include <sys/stat.h>

#include "err_exit.h"
#include "shared_memory.h"

int alloc_shared_memory(key_t shmkey, size_t size){
    //creazione segmento di shared memory
    int shmid = shmget(shmkey, size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmid == -1)
        ErrExit("shmget failed");
    
    return shmid;
}

void *get_shared_memory(int shmid, int shmflg){
    void *ptr_sh = (int *)shmat(shmid, NULL, shmflg);
    if (ptr_sh == (void *)-1)
        ErrExit("shmat failed");

    return ptr_sh;
}

void free_shared_memory(void *ptr_sh){
    // libera il segmento di shared memory
    if(shmdt(ptr_sh) == 1)
        ErrExit("shmdt failed");
}

void remove_shared_memory(int shmid){
    // rimuove il segmento di shared memory
    if(shmctl(shmid, IPC_RMID, NULL) == -1)
        ErrExit("shmctl failed");
}