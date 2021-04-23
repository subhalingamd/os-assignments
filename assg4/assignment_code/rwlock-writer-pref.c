#include "rwlock.h"

void InitalizeReadWriteLock(struct read_write_lock * rw)
{
  //	Write the code for initializing your read-write lock.
	rw->readers = 0; rw->writers = 0;
    Sem_init(&rw->lock, 1); Sem_init(&rw->lock1, 1); 
    Sem_init(&rw->writelock, 1); 
    Sem_init(&rw->readlock, 1); 
}

void ReaderLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the reader.
	Sem_wait(&rw->readlock);                 //Indicate a reader is trying to enter
	Sem_wait(&rw->lock);                  //lock entry section to avoid race condition with other readers
	rw->readers++;                 //report yourself as a reader
	if (rw->readers == 1)          //checks if you are first reader
	  Sem_wait(&rw->writelock);              //if you are first reader, lock  the writelock
	Sem_post(&rw->lock);                  //release entry section for other readers
	Sem_post(&rw->readlock);                 //indicate you are done trying to access the writelock

}

void ReaderUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the reader.
	Sem_wait(&rw->lock);                  //reserve exit section - avoids race condition with readers
	rw->readers--;                 //indicate you're leaving
	if (rw->readers == 0)          //checks if you are last reader leaving
	  Sem_post(&rw->writelock);              //if last, you must release the locked writelock
	Sem_post(&rw->lock); 
}

void WriterLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the writer.
	Sem_wait(&rw->lock1);                  //reserve entry section for writers - avoids race conditions
	rw->writers++;                //report yourself as a writer entering
	if (rw->writers == 1)         //checks if you're first writer
	  Sem_wait(&rw->readlock);               //if you're first, then you must lock the readers out. Prevent them from trying to enter CS
	Sem_post(&rw->lock1);                  //release entry section
	Sem_wait(&rw->writelock);                //reserve the writelock for yourself - prevents other writers from simultaneously editing the shared writelock
	
}

void WriterUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the writer.
	  Sem_post(&rw->writelock);                //release file
	  Sem_wait(&rw->lock1);                  //reserve exit section
	  rw->writers--;                //indicate you're leaving
	  if (rw->writers == 0)         //checks if you're the last writer
	    Sem_post(&rw->readlock);               //if you're last writer, you must unlock the readers. Allows them to try enter CS for reading
	  Sem_post(&rw->lock1);  
	
}
