#ifndef _FILE_PSU_PERSISTENCE_H_
#define _FILE_PSU_PERSISTENCE_H_

#include "ctl_main.h"
#include <core/pm/function_scheduler.h>

#ifdef __cplusplus
extern "C"
{
#endif

void psu_persistence_init(ctl_psu_t* hpsu);
gmp_task_status_t tsk_psu_persistence(gmp_task_t* tsk);

#ifdef __cplusplus
}
#endif

#endif
