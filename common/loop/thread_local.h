#ifndef COMMON_LOOP_THREAD_LOCAL_H_
#define COMMON_LOOP_THREAD_LOCAL_H_

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

#define THREAD_LOCAL_VAR thread_local

#ifdef __GNUC__
#define THREAD_LOCAL_POD_VAR __thread
#else
#define THREAD_LOCAL_POD_VAR thread_local
#endif

TONY_CAT_SPACE_END

#endif  // COMMON_LOOP_THREAD_LOCAL_H_
