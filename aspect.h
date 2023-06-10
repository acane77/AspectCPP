#ifndef _ACANE_ASPECT_H_
#define _ACANE_ASPECT_H_

#include <memory>
#include <tuple>
#include <iostream>
#include <execinfo.h>
#include <vector>
#include <cxxabi.h>

static std::string Demangle(const char* name) {
    int status = -4; // some arbitrary value to eliminate the compiler warning
    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(name, NULL, NULL, &status),
            std::free
    };
    return (status==0) ? res.get() : name ;
}

#define SMART_LOGD(fmt, ...) printf("smart debug: " fmt "\n", ##__VA_ARGS__)

struct AspectProxyDefaultAfter {
    template <class ...Args>
    void After(Args... args) { }
};

struct AspectProxyDefaultBefore {
    template <class ...Args>
    void Before(Args... args) { }
};

struct TimeLogger {
    template <class... Args>
    void Before(Args... args) {
        time_ = clock() * 1.f / CLOCKS_PER_SEC;
    }

    template <class... Args>
    void After(Args... args) {
        double duration = clock() * 1.f / CLOCKS_PER_SEC - time_;
        SMART_LOGD("%s time: %lf ms", name_, duration);
    }

    void SetParams(const char* name) {
        name_ = name;
    }

    double time_;
    const char* name_ = nullptr;
};

template <class T>
struct is_printable {
    template <class T1>
    static constexpr auto test(int) ->
        decltype(operator <<(std::declval<std::ostream>(), std::declval<T1>()), bool{}) {
        return true;
    }

    template <class>
    static constexpr bool test(...) {
        return false;
    }

    static constexpr bool value = test<T>(0);
};

struct ParameterPrinter {
    template <class... Args>
    void Before(Args... args) {
        SMART_LOGD("The function have %zd parameters", sizeof...(args));
        PrintParams<0>(std::forward<Args>(args)...);
    }

    template <int N, class T, class... Rest>
    void PrintParams(const T& obj, Rest... rest) {
        std::cout << "smart debug: [Parameter #" << N << "] ";
        std::cout << "(" << Demangle(typeid(T).name()) << ") ";
        Print(obj);
        std::cout << std::endl;
        PrintParams<N + 1>(std::forward<Rest>(rest)...);
    }

    template <int N>
    void PrintParams() {  }

    template <class ReturnType>
    void After(ReturnType ret) {
        std::cout << "smart debug: [Return value] ";
        std::cout << "(" << Demangle(typeid(ReturnType).name()) << ") ";
        std::cout << ret << std::endl;
    }

    void After() {  }

private:
    template <class T>
    typename std::enable_if<is_printable<T>::value, void>::type
    Print(const T& val) { std::cout << val; }

    template <class T>
    typename std::enable_if<!is_printable<T>::value, void>::type
    Print(const T& val) { std::cout << " *** The value is not printable by std::ostream ***"; }
};

struct CodeProfiler : public AspectProxyDefaultAfter {
    template <class... Args>
    void Before(Args... args) { }

    void SetParams(const char* function_name, const char* file, int line, const char* call_from) {
        SMART_LOGD("[============] =======================================");
        SMART_LOGD("[function    ] %s", function_name);
        SMART_LOGD("[file        ] %s:%d", file, line);
        SMART_LOGD("[call from   ] %s", call_from);
    }
};

struct StackTrace : public AspectProxyDefaultAfter {
    template <class... Args>
    void Before(Args... args) { PrintStackStace(); }

    std::string ProcessStackTrack(char* trace_str) {
        std::string trace;
        int group = 0;
        while (*trace_str && group < 3) {
            while (*trace_str++ == ' ');
            group++;
            while (*trace_str != ' ' && *trace_str) trace_str++;
        };
        if (!*trace_str) return trace;
        trace_str++;
        char* trace_str_end = trace_str;
        while (*trace_str_end != ' ' && *trace_str_end) trace_str_end++;
        *trace_str_end = 0;
        return Demangle(trace_str);
    }

    void PrintStackStace() {
        void* buffer[50];
        int size = backtrace(buffer, 50);
        char** strings = backtrace_symbols(buffer, size);
        SMART_LOGD("[============] =======================================");

        const bool skip_first_frames = true;
        int omitted = 0;
        for (int i = 0; i < size; ++i) {
            // Hide AspectProxy<> and StackTrace(this class) call stack
            if (skip_first_frames) {
                if (strstr(strings[i], "10StackTrace") ||
                    strstr(strings[i], "11AspectProxy") ||
                    strstr(strings[i], "mem_fn") ||
                    strstr(strings[i], "ZNSt3")) {
                    omitted++;
                    continue;
                }
            }
            //skip_first_frames = false;
            std::string trace = ProcessStackTrack(strings[i]);
            SMART_LOGD("[Stack Trace ] [#%-3d] %s", size - i - 1, trace.c_str());
        }
        free(strings);
        //SMART_LOGD("[============] %d frames omitted", omitted);
    }

    void SetParams() { }

};

template <class T>
struct function_helper;

template <class T, class ...Args>
struct function_helper<T(*)(Args...)> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
};

template <class T, class ...Args>
struct function_helper<std::function<T(Args...)>> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
};

template <class T, class ...Args>
struct function_helper<T(Args...)> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
};

template <class T, class C, class ...Args>
struct function_helper<T (C::*)(Args...)> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
    using class_type = C;
    using functional_type = std::function<T (Args...)>;
};

template <class Func>
struct return_type_is_void {
    constexpr const static bool value = std::is_same<typename function_helper<Func>::return_type, void>::value;
};

template <class Type, class... Aspects>
class AspectProxy {
public:
    template <class ...Args>
    explicit AspectProxy(Args... args) { obj_holder_.reset(new Type(std::forward<Args>(args)...)); obj_ = obj_holder_.get(); }

    AspectProxy(Type* obj) { obj_ = obj; }

    Type* operator->() { Before(); return obj_; }

    const Type* operator->() const { Before(); return obj_; }


    /// @brief Invoke a non-member function and returns void
    template <class Func, class... Args>
    typename std::enable_if<!std::is_member_function_pointer<Func>::value &&
                            return_type_is_void<Func>::value, void>::type
    Invoke(Func func, Args... args) {
        Before(std::forward<Args>(args)...);
        func(std::forward<Args>(args)...);
        After();
    }

    /// @brief Invoke a member function and returns something
    template <class Func, class ...Args>
    typename std::enable_if<std::is_member_function_pointer<Func>::value &&
                            !std::is_same<typename function_helper<Func>::return_type, void>::value,
            typename function_helper<Func>::return_type>::type
    Invoke(Func func, Args... args) {
        using return_t = typename function_helper<Func>::return_type;
        Before(std::forward<Args>(args)...);
        return_t ret = InvokeImpl(func, std::forward<Args>(args)...);
        After(ret);
        return ret;
    }

    /// @brief Invoke a member function and returns void
    template <class Func, class ...Args>
    typename std::enable_if<std::is_member_function_pointer<Func>::value &&
                            std::is_same<typename function_helper<Func>::return_type, void>::value, void>::type
    Invoke(Func func, Args... args) {
        Before<Args...>(std::forward(args)...);
        InvokeImpl(func, std::forward<Args>(args)...);
        After();
    }

    /// @brief Invoke a non-member function and returns something
    template <class Func, class... Args>
    typename std::enable_if<!std::is_member_function_pointer<Func>::value &&
                            !return_type_is_void<Func>::value, typename function_helper<Func>::return_type>::type
    Invoke(Func func, Args... args) {
        using return_t = typename function_helper<Func>::return_type;
        Before(std::forward<Args>(args)...);
        return_t ret = func(std::forward<Args>(args)...);
        After(ret);
        return ret;
    }

    template <class Aspect, class... Args>
    AspectProxy& SetParam(Args... args) {
        SetParam<0, sizeof...(Aspects), Aspect>(std::forward<Args>(args)...);
        return *this;
    }

private:
    std::unique_ptr<Type> obj_holder_;
    Type* obj_;
    std::tuple<Aspects...> aspects_;

    template <size_t I, size_t N, class Aspect, class... Args>
    typename std::enable_if<I >= sizeof...(Aspects), void>::type
    SetParam(Args... args) { }

    template <size_t I, size_t N, class Aspect, class... Args>
    typename std::enable_if<
            I < sizeof...(Aspects) &&
            std::is_same<typename std::tuple_element<I, std::tuple<Aspects..., void>>::type, Aspect>::value, void>::type
    SetParam(Args... args) {
        Aspect* obj = &std::get<I>(aspects_);
        obj->SetParams(std::forward<Args>(args)...);
        SetParam<I + 1, N, Aspect>(std::forward<Args>(args)...);
    }

    template <size_t I, size_t N, class Aspect, class... Args>
    typename std::enable_if<
            I < sizeof...(Aspects) &&
            !std::is_same<typename std::tuple_element<I, std::tuple<Aspects..., void>>::type, Aspect>::value, void>::type
    SetParam(Args... args) {
        SetParam<I + 1, N, Aspect>(std::forward<Args>(args)...);
    }

    template <class Func, class ...Args>
    typename std::enable_if<std::is_member_function_pointer<Func>::value,
            typename function_helper<Func>::return_type>::type
    InvokeImpl(Func func, Args... args) {
        auto mem_fn = std::mem_fn(func);
        return mem_fn(obj_, std::forward<Args>(args)...);
    }

    template<int N, class Aspect, class... Rest, class... Args>
    typename std::enable_if<sizeof...(Rest) != 0, void>::type
    InvokeAspectsBefore(Args... args) {
        InvokeAspectsBefore<N, Aspect>(std::forward<Args>(args)...);
        InvokeAspectsBefore<N + 1, Rest...>(std::forward<Args>(args)...);
    }

    template<int N, class Aspect, class... Args>
    void InvokeAspectsBefore(Args... args) {
        //InvokeFunc(&Aspect::Before, std::get<N>(aspects_), std::forward<Args>(args)...);
        Aspect* obj = &std::get<N>(aspects_);
        obj->Before(std::forward<Args>(args)...);
    }

    template <class... Args>
    void Before(Args... args) {
        InvokeAspectsBefore<0, Aspects...>(std::forward<Args>(args)...);
    }

    template<int N, class Aspect, class... Rest, class... Args>
    typename std::enable_if<sizeof...(Rest) != 0, void>::type
    InvokeAspectsAfter(Args... args) {
        InvokeAspectsAfter<N, Aspect>(std::forward<Args>(args)...);
        InvokeAspectsAfter<N + 1, Rest...>(std::forward<Args>(args)...);
    }

    template<int N, class Aspect, class... Args>
    void InvokeAspectsAfter(Args... args) {
        //InvokeFunc(&Aspect::After, std::get<N>(aspects_), std::forward<Args>(args)...);
        Aspect* obj = &std::get<N, Aspects...>(aspects_);
        obj->After(std::forward<Args>(args)...);
    }

    template <class... Args>
    void After(Args... args) {
        InvokeAspectsAfter<0, Aspects...>(std::forward<Args>(args)...);
    }

    template <class Fn, class... Args>
    typename std::enable_if<std::tuple_size<typename function_helper<Fn>::arguments>::value != 0 &&
                            std::is_member_function_pointer<Fn>::value, void>::type
    InvokeFunc(Fn fn, typename function_helper<Fn>::class_type& obj, Args... args) {
        auto memfunc = std::mem_fn(fn);
        memfunc(&obj, std::forward<Args...>(args)...);
    }

    template <class Fn, class... Args>
    typename std::enable_if<std::tuple_size<typename function_helper<Fn>::arguments>::value == 0 &&
                            std::is_member_function_pointer<Fn>::value, void>::type
    InvokeFunc(Fn fn, typename function_helper<Fn>::class_type& obj, Args... args) {
        auto memfunc = std::mem_fn(fn);
        memfunc(&obj);
    }
};

struct function_aspect_helper {};

template <class FuncType, class... Aspects>
class FunctionAspectProxy : private AspectProxy<function_aspect_helper, Aspects...> {
public:
    FunctionAspectProxy(FuncType func): func_(func) {}

    template <class Aspect, class... Args>
    FunctionAspectProxy& SetParam(Args... args) {
        AspectProxy<function_aspect_helper, Aspects...>::template SetParam<Aspect>(std::forward<Args>(args)...);
        return *this;
    }

    template <class... Args>
    typename function_helper<FuncType>::return_type
    InvokeFunction(Args... args) {
        return AspectProxy<function_aspect_helper, Aspects...>::Invoke(func_, std::forward<Args>(args)...);
    }

    template <class... Args>
    typename function_helper<FuncType>::return_type
    operator()(Args... args) {
        return InvokeFunction(std::forward<Args>(args)...);
    }

private:
    std::function<FuncType> func_;
};

#define ENABLED_ASPECT ParameterPrinter, CodeProfiler, StackTrace, TimeLogger
#define _ASPECT_TO_STRING_HELPER_INNER(x) #x
#define _ASPECT_TO_STRING_HELPER(x) _ASPECT_TO_STRING_HELPER_INNER(x)
#define _ASPECT_SET_PARAM_TIME_LOGGER(func) SetParam<TimeLogger>(#func)
#define _ASPECT_SET_PARAM_CODE_PROFILER(func) SetParam<CodeProfiler>(#func "()", __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define _ASPECT_SET_PARAM(func) _ASPECT_SET_PARAM_TIME_LOGGER(func)._ASPECT_SET_PARAM_CODE_PROFILER(func)

/// @brief Disable aspect proxy, define DISABLE_ASPECT_PROXY=1 to disable aspect proxies
#if DISABLE_ASPECT_PROXY
//#define _DISABLE_ASPECT
#endif

#ifndef _DISABLE_ASPECT

/// @brief Define an aspect class for a non-member function, use with ASPECT_INVOKE()
#define DEFINE_ASPECT(func_name, ...) \
    FunctionAspectProxy<decltype(func_name), ENABLED_ASPECT, ##__VA_ARGS__> aspect_##func_name(func_name); \
    aspect_##func_name._ASPECT_SET_PARAM(func_name)

/// @brief Define an aspect proxy for any object, use with ASPECT_OBJECT_INVOKE()
#define DEFINE_ASPECT_OBJECT(object, ...) \
    using _aspect_##object##_type = typename std::decay<decltype(object)>::type;\
    AspectProxy<_aspect_##object##_type, ENABLED_ASPECT, ##__VA_ARGS__> aspect_##object(&(object))

/// @brief Define an aspect proxy for any variable with pointer type, use with ASPECT_OBJECT_INVOKE()
#define DEFINE_ASPECT_OBJECT_POINTER(pointer, ...) \
    using _aspect_##pointer##_type = typename std::decay<decltype(*(pointer))>::type;\
    AspectProxy<_aspect_##pointer##_type, ENABLED_ASPECT, ##__VA_ARGS__> aspect_##pointer(pointer)

/// @brief Define an aspect proxy for this class, use with ASPECT_THIS_INVOKE()
#define DEFINE_ASPECT_THIS(...) DEFINE_ASPECT_OBJECT_POINTER(this, ##__VA_ARGS__)

/// @brief Get variable name of aspect proxy
#define ASPECT(func) aspect_##func

/// @brief Invoke a non-member function with pre-defined aspect variable defined by DEFINE_ASPECT(func_name)
#define ASPECT_INVOKE(func, ...) ASPECT(func)(__VA_ARGS__)

/// @brief Invoke a non-member function
#define ASPECT_FUNC_INVOKE(func_name, ...) \
    FunctionAspectProxy<decltype(func_name), ENABLED_ASPECT>(func_name).\
            _ASPECT_SET_PARAM(func_name).\
            InvokeFunction(__VA_ARGS__)

/// @brief Invoke a class member function
#define ASPECT_OBJECT_INVOKE(object, func, ...)\
    (aspect_##object._ASPECT_SET_PARAM(func) \
                .Invoke(&_aspect_##object##_type::func, ##__VA_ARGS__) )

/// @brief Invoke a class member function with pointer type
#define ASPECT_OBJECT_POINTER_INVOKE(object, func, ...)  ASPECT_OBJECT_INVOKE(object, func, ##__VA_ARGS__)

// Invoke a class member function inside the same class member
#define ASPECT_THIS_INVOKE(func, ...) ASPECT_OBJECT_INVOKE(this, func, ##__VA_ARGS__)

#else
#define DEFINE_ASPECT(func_name, ...)
#define ASPECT(func) func
#define ASPECT_INVOKE(func, ...) func(__VA_ARGS__)
#define DEFINE_ASPECT_THIS(...)
#define DEFINE_ASPECT_OBJECT(...)
#define DEFINE_ASPECT_OBJECT_POINTER(...)
#define ASPECT_THIS_INVOKE(func, ...) func(__VA_ARGS__)
#define ASPECT_FUNC_INVOKE(func, ...) func(__VA_ARGS__)
#define ASPECT_OBJECT_INVOKE(object, func, ...) (object).func(__VA_ARGS__)
#define ASPECT_OBJECT_POINTER_INVOKE(object, func, ...) (object)->func(__VA_ARGS__)
#endif

#endif // _ACANE_ASPECT_H_
