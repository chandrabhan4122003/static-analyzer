#include <stdexcept>

// HSCAA.1.2 - unused return value
int compute() { return 42; }

void test1() {
    compute();        // violation
    int x = compute(); // ok
    (void)compute();   // ok
}

// HSCAJ.4.1 - missing else
void test2(int num) {
    if (num > 0) {
        // positive
    } else if (num < 0) {
        // negative
    }
    // violation - no final else
}

// HSCAS.1.1 - throw pointer
void test3() {
    throw new std::runtime_error("error"); // violation
}
