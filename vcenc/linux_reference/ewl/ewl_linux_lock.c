/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Description : Locking semaphore for hardware sharing
--
------------------------------------------------------------------------------*/
#ifdef LINUX_LOCK_BUILD_SUPPORT

#include <errno.h>
#include "ewl_linux_lock.h"

#ifdef USE_SYS_V_IPC
#include <sys/ipc.h>
#include <sys/sem.h>
#endif

#ifdef _SEM_SEMUN_UNDEFINED
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};
#endif

/* Obtain a binary semaphore's ID, allocating if necessary.  */

#ifdef USE_POSIX_SEMAPHORE

#else /*SYSTEM_V*/

int binary_semaphore_allocation(key_t key, int nsem, int sem_flags)
{
    return semget(key, nsem, sem_flags);
}

/* Deallocate a binary semaphore.  All users must have finished their
   use.  Returns -1 on failure.  */
int binary_semaphore_deallocate(int semid)
{
    return semctl(semid, 0, IPC_RMID);
}

/* Wait on a binary semaphore.  Block until the semaphore value is
   positive, then decrement it by one.  */
int binary_semaphore_wait(int semid, int sem_num)
{
    int ret;
    struct sembuf operations[1];

    /* Use 'sem_num' semaphore from the set.  */
    operations[0].sem_num = sem_num;
    /* Decrement by 1.  */
    operations[0].sem_op = -1;
    /* Permit undo'ing.  */
    operations[0].sem_flg = SEM_UNDO;

    /* signal safe */
    do {
        ret = semop(semid, operations, 1);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

/* Post to a binary semaphore: increment its value by one */
int binary_semaphore_post(int semid, int sem_num)
{
    int ret;
    struct sembuf operations[1];

    /* Use 'sem_num' semaphore from the set.  */
    operations[0].sem_num = sem_num;
    /* Increment by 1.  */
    operations[0].sem_op = 1;
    /* Permit undo'ing.  */
    operations[0].sem_flg = SEM_UNDO;

    /* signal safe */
    do {
        ret = semop(semid, operations, 1);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

/* Initialize a binary semaphore with a value of one.  */
int binary_semaphore_initialize(int semid, int semnum)
{
    union semun argument;

    argument.val = 1;
    return semctl(semid, semnum, SETVAL, argument);
}

/* Get the value of a binary semaphore  */
int binary_semaphore_getval(int semid, int semnum)
{
    return semctl(semid, semnum, GETVAL);
}
#endif

#endif