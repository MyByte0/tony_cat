#ifndef COMMON_THREAD_LOCAL_H_
#define COMMON_THREAD_LOCAL_H_

#include "core_define.h"

SER_NAME_SPACE_BEGIN

#define THREAD_LOCAL_VAR			thread_local

#ifdef __GNUC__
	#define THREAD_LOCAL_POD_VAR	__thread
#else
	#define THREAD_LOCAL_POD_VAR	thread_local
#endif

SER_NAME_SPACE_END

#endif  // COMMON_THREAD_LOCAL_H_
