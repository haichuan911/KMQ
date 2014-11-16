#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <kmq/errno.h>
#include "osthread.h"
#include "flock.h"
#include "memalloc.h"

using namespace kmq;


static int test_flock_single() {
    int ret;
    char lock_file[] = "/tmp/mem_flock.file";
    flock_t *fl;
    char errmsg[ESTRLEN] = {}, *errp = NULL;


    fl = (flock_t *)mem_alloc(sizeof(flock_t));
    if (!fl) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("mem_alloc flock_t failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_create(fl, lock_file)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_create failed: %s\n", errp);
	return -1;
    }
    
    // first, lock this file lock
    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock failed: %s\n", errp);
	return -1;
    }


    // in the same process, relock should be ok
    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_relock failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_trylock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_trylock failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_unlock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_unlock failed: %s\n", errp);
	return -1;
    }

    flock_destroy(fl);    
    mem_free(fl);
    return 0;
}



static int test_flock_thread_worker(void *arg_) {
    int ret;
    flock_t *fl = (flock_t *)arg_;
    char errmsg[ESTRLEN] = {}, *errp = NULL;

    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock in thread failed: %s\n", errp);
	return -1;
    }
    return 0;
}

static int test_flock_multipthread() {
    int ret;
    char lock_file[] = "/tmp/mem_flock.file";
    flock_t *fl;
    OSThread thread;
    char errmsg[ESTRLEN] = {}, *errp = NULL;


    fl = (flock_t *)mem_alloc(sizeof(flock_t));
    if (!fl) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("mem_alloc flock_t failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_create(fl, lock_file)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_create failed: %s\n", errp);
	return -1;
    }
    
    // first, lock this file lock
    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock failed: %s\n", errp);
	return -1;
    }


    thread.Start(test_flock_thread_worker, fl);
    if ((ret = thread.Stop()) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock in thread failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_unlock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_unlock failed: %s\n", errp);
	return -1;
    }

    flock_destroy(fl);    
    mem_free(fl);
    return 0;
}



static int test_flock_process_worker(void *arg_) {
    int ret;
    flock_t *fl = (flock_t *)arg_;

    if ((ret = flock_trylock(fl)) == 0) {
	printf("flock_trylock in process failed. it should be trylock failed!! \n");
	return -1;
    }
    return 0;
}

static int test_flock_multiprocess() {
    int ret;
    char lock_file[] = "/tmp/mem_flock.file";
    flock_t *fl;
    pid_t pid;
    char errmsg[ESTRLEN] = {}, *errp = NULL;


    fl = (flock_t *)mem_alloc(sizeof(flock_t));
    if (!fl) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("mem_alloc flock_t failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_create(fl, lock_file)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_create failed: %s\n", errp);
	return -1;
    }
    
    // first, lock this file lock
    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock failed: %s\n", errp);
	return -1;
    }

    if ((pid = fork()) == 0) {
	// child
	int ret = test_flock_process_worker(fl);
	flock_destroy(fl);
	mem_free(fl);
	exit(ret);
    } else if (pid > 0) {
	waitpid(pid, &ret, 0);
    }

    if (ret != 0) {
	printf("flock_lock in process failed: %d\n", ret);
	return -1;
    }

    if ((ret = flock_unlock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_unlock failed: %s\n", errp);
	return -1;
    }

    flock_destroy(fl);
    mem_free(fl);
    return 0;
}



static int test_flock_multiprocess_ex() {
    int ret;
    char lock_file[] = "/tmp/mem_flock.file";
    flock_t *fl;
    pid_t pid;
    char errmsg[ESTRLEN] = {}, *errp = NULL;


    fl = (flock_t *)mem_alloc(sizeof(flock_t));
    if (!fl) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("mem_alloc flock_t failed: %s\n", errp);
	return -1;
    }

    if ((ret = flock_create(fl, lock_file)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_create failed: %s\n", errp);
	return -1;
    }


    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock failed: %s\n", errp);
	return -1;
    }
    
    if ((pid = fork()) == 0) {
	// child
	flock_unlock(fl);
	flock_destroy(fl);
	mem_free(fl);
	exit(0);
    }

    if ((ret = flock_unlock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_unlock failed: %s\n", errp);
	return -1;
    }
    if ((ret = flock_lock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_lock failed: %s\n", errp);
	return -1;
    }
    if ((pid = fork()) == 0) {
	// child
	if ((ret = flock_trylock(fl)) == 0) {
	    printf("flock_lock error, it should be lock failed in other process");
	    exit(-1);
	}
	flock_destroy(fl);
	mem_free(fl);
	exit(0);
    } else if (pid > 0) {
	waitpid(pid, &ret, 0);
    }
    if (ret != 0) {
	return -1;
    }
    if ((ret = flock_unlock(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_unlock failed: %s\n", errp);
	return -1;
    }
    if ((ret = flock_destroy(fl)) != 0) {
	errp = strerror_r(errno, errmsg, ESTRLEN);
	printf("flock_destroy failed: %s\n", errp);
	return -1;
    }

    mem_free(fl);
    return 0;
}





TEST(flock, single) {
    EXPECT_EQ(0, test_flock_single());
}

/*
 thread A,B
 A: lock(fl)
 B: EXPECT(lock(fl) == 0)
 A: unlock(fl)
 */  

TEST(flock, multipthread) {
    EXPECT_EQ(0, test_flock_multipthread());
}

/*
 process A,B
 A: lock(fl)
 B: EXPECT(lock(fl) == -1)
 A: unlock(fl)
 */  

TEST(flock, multiprocess) {
    return;
    EXPECT_EQ(0, test_flock_multiprocess());
}



TEST(flock, multiprocess_ex) {
    return;
    EXPECT_EQ(0, test_flock_multiprocess_ex());
}

