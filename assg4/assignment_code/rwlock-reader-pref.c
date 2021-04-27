#include "rwlock.h"



void InitalizeReadWriteLock(struct read_write_lock * rw)
{
  //	Write the code for initializing your read-write lock.
    rw->readers = 0;
    Sem_init(&rw->lock, 1); 
    Sem_init(&rw->writelock, 1); 
}

void ReaderLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the reader.
    Sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1)
        Sem_wait(&rw->writelock);
    Sem_post(&rw->lock);
}

void ReaderUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the reader.
    Sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
        Sem_post(&rw->writelock);
    Sem_post(&rw->lock);
}

void WriterLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the writer.
    Sem_wait(&rw->writelock);
}

void WriterUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the writer.
    Sem_post(&rw->writelock);
}
