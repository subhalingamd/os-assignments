/*
Source: https://stackoverflow.com/a/27847103
*/


#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

struct _sem {
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
#endif
};


static inline void
Sem_init(struct _sem *s, uint32_t value)
{
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
#else
    sem_init(&s->sem, 0, value);
#endif

return;
};

static inline void
Sem_wait(struct _sem *s)
{

#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
            r = sem_wait(&s->sem);
    } while (r == -1 && errno == EINTR);
#endif

return;
};

static inline void
Sem_post(struct _sem *s)
{

#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
#else
    sem_post(&s->sem);
#endif

return;
};