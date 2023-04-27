#ifndef __MODULE_WAKEUP_LOCK_H__
#define __MODULE_WAKEUP_LOCK_H__

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0))
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/sched.h>
enum {
	WAKE_LOCK_SUSPEND, /* Prevent suspend */
	WAKE_LOCK_TYPE_COUNT
};

struct wake_lock {
	struct wakeup_source ws;
};

static inline void wake_lock_init(struct wake_lock *lock, int type,
				  const char *name)
{
    lock->ws.name = name;
	wakeup_source_add(&lock->ws);
}

static inline void wake_lock_destroy(struct wake_lock *lock)
{
    wakeup_source_remove(&lock->ws);
	// wakeup_source_drop(&lock->ws);
}

static inline void wake_lock(struct wake_lock *lock)
{
	__pm_stay_awake(&lock->ws);
}

static inline void wake_lock_timeout(struct wake_lock *lock, long timeout)
{
	__pm_wakeup_event(&lock->ws, jiffies_to_msecs(timeout));
}

static inline void wake_unlock(struct wake_lock *lock)
{
	__pm_relax(&lock->ws);
}

static inline int wake_lock_active(struct wake_lock *lock)
{
	return lock->ws.active;
}
#else
#include <linux/wakelock.h>
#endif

#endif