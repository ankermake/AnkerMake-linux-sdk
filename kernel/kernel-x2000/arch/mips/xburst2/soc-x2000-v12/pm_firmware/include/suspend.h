#ifndef __SUSPEND_H__
#define __SUSPEND_H__

typedef int suspend_state_t;

#define PM_SUSPEND_ON		((suspend_state_t) 0)
#define PM_SUSPEND_FREEZE	((suspend_state_t) 1)
#define PM_SUSPEND_STANDBY	((suspend_state_t) 2)
#define PM_SUSPEND_MEM		((suspend_state_t) 3)
#define PM_SUSPEND_MIN		PM_SUSPEND_FREEZE
#define PM_SUSPEND_MAX		((suspend_state_t) 4)

#endif
