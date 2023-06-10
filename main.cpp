//#define DISABLE_ASPECT_PROXY 1
#include "aspect.h"
#include <cstring>

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

void test1(const std::vector<int>& vec, int x) {

}

int main() {
    size_t len = ASPECT_FUNC_INVOKE(strlen, "hello aspect");
    SMART_LOGD("len = %zd", len);

    AOTSample tester;
    DEFINE_ASPECT_OBJECT(tester);
    ASPECT_FUNC_INVOKE(test1, std::vector<int>{}, 1);

    return ASPECT_OBJECT_INVOKE(tester, Test);
}