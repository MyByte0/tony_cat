#ifndef COMMON_UTILITY_ANONYMOUS_VAR_H_
#define COMMON_UTILITY_ANONYMOUS_VAR_H_

#ifndef _

#define _PLACEHOLDER_COMB(x, y) x##y
#define _PLACEHOLDER(x, y) _PLACEHOLDER_COMB(x, y)

#define _ _PLACEHOLDER(_PlaceholderVar, __COUNTER__)

#endif

#endif  // COMMON_UTILITY_ANONYMOUS_VAR_H_
