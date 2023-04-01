#ifndef COMMON_UTILITY_DEFER_H_
#define COMMON_UTILITY_DEFER_H_

template <typename Function>
struct _DoDefer {
    Function fun;
    _DoDefer(Function f) : fun(f) {}
    ~_DoDefer() { fun(); }
};

template <typename Function>
_DoDefer<Function> _Deferer(Function fun) {
    return _DoDefer<Function>(fun);
}

#define _DEFER_1(x, y) x##y
#define _DEFER_2(x, y) _DEFER_1(x, y)
#define _DEFER_0(x) _DEFER_2(x, __COUNTER__)

// notice: not promise stack order execute on multiple defers define
#define defer(_expr) auto _DEFER_0(__defered_item) = _Deferer([&]() { _expr; })

#endif  // COMMON_UTILITY_DEFER_H_
