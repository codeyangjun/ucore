#include <stdio.h>
#include <monitor.h>
#include <kmalloc.h>
#include <assert.h>

// Initialize monitor.
void monitor_init(monitor_t * mtp, size_t num_cv)
{
	int i;
	assert(num_cv > 0);
	mtp->next_count = 0;
	mtp->cv = NULL;
	sem_init(&(mtp->mutex), 1); //unlocked
	sem_init(&(mtp->next), 0);
	mtp->cv = (condvar_t *) kmalloc(sizeof(condvar_t) * num_cv);
	assert(mtp->cv!=NULL);
	for (i = 0; i < num_cv; i++)
	{
		mtp->cv[i].count = 0;
		sem_init(&(mtp->cv[i].sem), 0);
		mtp->cv[i].owner = mtp;
	}
}

// Unlock one of threads waiting on the condition variable. 
void cond_signal(condvar_t *cvp)
{
	//LAB7 EXERCISE1: YOUR CODE
	cprintf(
			"cond_signal begin: cvp %x, cvp->count %d, cvp->owner->next_count %d\n",
			cvp, cvp->count, cvp->owner->next_count);
	/*
	 *      cond_signal(cv) {
	 *          if(cv.count>0) {
	 *             mt.next_count ++;
	 *             signal(cv.sem);
	 *             wait(mt.next);
	 *             mt.next_count--;
	 *          }
	 *       }
	 */
	//1、首先进程B判断cv.count，如果不大于0，则表示当前没有执行cond_wait而睡眠的进程，因此就没有被唤醒的对象了，直接函数返回即可
	if (cvp->count > 0)
	{
		//2、大于0，这表示当前有执行cond_wait而睡眠的进程A，因此需要唤醒等待在cv.sem上睡眠的进程A。
		cvp->owner->next_count++;
		up(&(cvp->sem));
		//3、由于只允许一个进程在管程中执行，所以一旦进程B唤醒了别人（进程A），那么自己就需要睡眠。
		//   故让monitor.next_count加一，且让自己（进程B）睡在信号量monitor.next上
		down(&(cvp->owner->next));
		cvp->owner->next_count--;
	}
	cprintf(
			"cond_signal end: cvp %x, cvp->count %d, cvp->owner->next_count %d\n",
			cvp, cvp->count, cvp->owner->next_count);
}

// Suspend calling thread on a condition variable waiting for condition Atomically unlocks 
// mutex and suspends calling thread on conditional variable after waking up locks mutex.
// Notice: mp is mutex semaphore for monitor's procedures
void cond_wait(condvar_t *cvp)
{
	//LAB7 EXERCISE1: YOUR CODE
	cprintf(
			"cond_wait begin:  cvp %x, cvp->count %d, cvp->owner->next_count %d\n",
			cvp, cvp->count, cvp->owner->next_count);
	/*
	 *         cv.count ++;
	 *         if(mt.next_count>0)
	 *            signal(mt.next)
	 *         else
	 *            signal(mt.mutex);
	 *         wait(cv.sem);
	 *         cv.count --;
	 */
	//1、等待条件不为真，需要睡眠
	cvp->count++;
	//2.1、有大于等于1个进程执行cond_signal函数且睡着了，就睡在了monitor.next信号量上
	if (cvp->owner->next_count > 0)
	{
		up(&(cvp->owner->next));
	}
	//2.2、目前没有进程执行cond_signal函数且睡着了，那需要唤醒的是由于互斥条件限制而无法进入管程的进程，所以要唤醒睡在monitor.mutex上的进程
	else
	{
		up(&(cvp->owner->mutex));
	}
	//3、进程A睡在cv.sem上，如果睡醒了，则让cv.count减一
	down(&(cvp->sem));
	cvp->count--;

	cprintf(
			"cond_wait end:  cvp %x, cvp->count %d, cvp->owner->next_count %d\n",
			cvp, cvp->count, cvp->owner->next_count);
}
