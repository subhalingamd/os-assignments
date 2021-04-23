#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


#include "mysemaphore.h"


struct read_write_lock
{
	struct _sem writelock; // for readers priority + writers priority
    struct _sem lock;
    int readers, writers;
    struct _sem readlock; // for writers priority
    struct _sem lock1; // additional for writers priority
};


void InitalizeReadWriteLock(struct read_write_lock * rw);
void ReaderLock(struct read_write_lock * rw);
void ReaderUnlock(struct read_write_lock * rw);
void WriterLock(struct read_write_lock * rw);
void WriterUnlock(struct read_write_lock * rw);
