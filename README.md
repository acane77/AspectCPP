# Aspect-Oriented Programming

Aspect with a function with time log, stack traces, printing parameter and return value, and more.

## Sample 

```cpp
//#define DISABLE_ASPECT_PROXY 1
#include "aspect.h"

class AOTSample {
public:
    int FibNacii(int x) {
        if (x <= 2) return 0;
        return x+1;
    }

    int TestThisFunction(int x, int b, int c) {
        DEFINE_ASPECT_THIS();
        return ASPECT_THIS_INVOKE(FibNacii, 3) + b + c;
    }
    int Test() {
        DEFINE_ASPECT_THIS();
        int x = ASPECT_THIS_INVOKE(TestThisFunction, 1, 2, 3);
        SMART_LOGD("result = %d", x);
        return 0;
    }
};

int main() {
    DEFINE_ASPECT(strlen);
    size_t len = ASPECT(strlen)("hello world");
    SMART_LOGD("len = %zd", len);
    len = ASPECT_FUNC_INVOKE(strlen, "hello aspect");
    SMART_LOGD("len = %zd", len);
    AOTSample tester;
    DEFINE_ASPECT_OBJECT(tester);
    return ASPECT_OBJECT_INVOKE(tester, Test);
}
```

## Sample Output

```
smart debug: [============] =======================================
smart debug: [function    ] strlen()
smart debug: [file        ] /Users/acane/projects/aspect/main.cpp:24
smart debug: [call from   ] int main()
smart debug: The function have 1 parameters
smart debug: [Parameter #0] (char const*) hello world
smart debug: [Stack Trace ] =======================================
smart debug: [Stack Trace ] [#1  ] main
smart debug: [Stack Trace ] [#0  ] start
smart debug: [Return value] 11
smart debug: strlen time: 0.000003 ms
smart debug: len = 11
smart debug: [============] =======================================
smart debug: [function    ] strlen()
smart debug: [file        ] /Users/acane/projects/aspect/main.cpp:27
smart debug: [call from   ] int main()
smart debug: The function have 1 parameters
smart debug: [Parameter #0] (char const*) hello aspect
smart debug: [Stack Trace ] =======================================
smart debug: [Stack Trace ] [#1  ] main
smart debug: [Stack Trace ] [#0  ] start
smart debug: [Return value] 12
smart debug: strlen time: 0.000001 ms
smart debug: len = 12
smart debug: [============] =======================================
smart debug: [function    ] Test()
smart debug: [file        ] /Users/acane/projects/aspect/main.cpp:31
smart debug: [call from   ] int main()
smart debug: The function have 0 parameters
smart debug: [Stack Trace ] =======================================
smart debug: [Stack Trace ] [#1  ] main
smart debug: [Stack Trace ] [#0  ] start
smart debug: [============] =======================================
smart debug: [function    ] TestThisFunction()
smart debug: [file        ] /Users/acane/projects/aspect/main.cpp:17
smart debug: [call from   ] int AOTSample::Test()
smart debug: The function have 3 parameters
smart debug: [Parameter #0] (int) 1
smart debug: [Parameter #1] (int) 2
smart debug: [Parameter #2] (int) 3
smart debug: [Stack Trace ] =======================================
smart debug: [Stack Trace ] [#6  ] AOTSample::Test()
smart debug: [Stack Trace ] [#5  ] decltype(*static_cast<AOTSample*&>(fp0).*fp()) std::__1::__invoke<int (AOTSample::* const&)(), AOTSample*&, void>(int (AOTSample::* const&)(), AOTSample*&)
smart debug: [Stack Trace ] [#4  ] std::__1::__invoke_return<int (AOTSample::*)(), AOTSample*&>::type std::__1::__mem_fn<int (AOTSample::*)()>::operator()<AOTSample*&>(AOTSample*&) const
smart debug: [Stack Trace ] [#1  ] main
smart debug: [Stack Trace ] [#0  ] start
smart debug: [============] =======================================
smart debug: [function    ] FibNacii()
smart debug: [file        ] /Users/acane/projects/aspect/main.cpp:13
smart debug: [call from   ] int AOTSample::TestThisFunction(int, int, int)
smart debug: The function have 1 parameters
smart debug: [Parameter #0] (int) 3
smart debug: [Stack Trace ] =======================================
smart debug: [Stack Trace ] [#11 ] AOTSample::TestThisFunction(int, int, int)
smart debug: [Stack Trace ] [#10 ] decltype(*static_cast<AOTSample*&>(fp0).*fp(static_cast<int>(fp1), static_cast<int>(fp1), static_cast<int>(fp1))) std::__1::__invoke<int (AOTSample::* const&)(int, int, int), AOTSample*&, int, int, int, void>(int (AOTSample::* const&)(int, int, int), AOTSample*&, int&&, int&&, int&&)
smart debug: [Stack Trace ] [#9  ] std::__1::__invoke_return<int (AOTSample::*)(int, int, int), AOTSample*&, int, int, int>::type std::__1::__mem_fn<int (AOTSample::*)(int, int, int)>::operator()<AOTSample*&, int, int, int>(AOTSample*&, int&&, int&&, int&&) const
smart debug: [Stack Trace ] [#6  ] AOTSample::Test()
smart debug: [Stack Trace ] [#5  ] decltype(*static_cast<AOTSample*&>(fp0).*fp()) std::__1::__invoke<int (AOTSample::* const&)(), AOTSample*&, void>(int (AOTSample::* const&)(), AOTSample*&)
smart debug: [Stack Trace ] [#4  ] std::__1::__invoke_return<int (AOTSample::*)(), AOTSample*&>::type std::__1::__mem_fn<int (AOTSample::*)()>::operator()<AOTSample*&>(AOTSample*&) const
smart debug: [Stack Trace ] [#1  ] main
smart debug: [Stack Trace ] [#0  ] start
smart debug: [Return value] 4
smart debug: FibNacii time: 0.000003 ms
smart debug: [Return value] 9
smart debug: TestThisFunction time: 0.000175 ms
smart debug: result = 9
smart debug: [Return value] 0
smart debug: Test time: 0.000263 ms
```