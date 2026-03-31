#include <stdexcept>
#include <memory>
#include <string>
#include <utility>

// HSCAA.1.2 - unused return value
int compute() { return 42; }
void test_hscaa12() {
    compute();           // violation
    int x = compute();   // ok
    (void)compute();     // ok
    (void)x;
}

// HSCAA.2.1 - unused variable
void test_hscaa21() {
    int unused = 10;     // violation
    int used = 20;
    (void)used;
}

// HSCAA.2.2 - unused parameter
void test_hscaa22(int used, int unused) {  // violation: unused
    (void)used;
}

// HSCAJ.4.1 - missing else
void test_hscaj41(int num) {
    if (num > 0) {
        // positive
    } else if (num < 0) {
        // negative
    }                    // violation - no final else
}

// HSCAS.1.1 - throw pointer
void test_hscas11() {
    throw new std::runtime_error("error");  // violation
}

// HSCAP.1.3 - implicit constructor
class ImplicitCtor {
public:
    ImplicitCtor(int x) {}       // violation - not explicit
    explicit ImplicitCtor(double x) {}  // ok
};

// HSCAI.2.2 - c-style cast
void test_hscai22() {
    int x = 10;
    double d = (double)x;   // violation
}

// HSCAI.2.3 - const cast
void test_hscai23() {
    const int x = 10;
    int* p = const_cast<int*>(&x);  // violation
    (void)p;
}

// HSCAV.6.2 - raw new/delete
void test_hscav62() {
    int* p = new int(42);   // violation
    delete p;               // violation
}

// HSCAS.4.1 - noexcept
class NoexceptTest {
public:
    NoexceptTest(NoexceptTest&& other) {}           // violation - move ctor
    NoexceptTest& operator=(NoexceptTest&& other) { // violation - move assign
        return *this;
    }
    ~NoexceptTest() {}                              // ok - noexcept by default
};

// HSCAN.3.1 - virtual specifier
class Base {
public:
    virtual void method1() {}
    virtual void method2() {}
};
class Derived : public Base {
public:
    virtual void method1() override {}  // violation - virtual+override
    void method2() {}                   // violation - missing override
};

int main() { return 0; }
