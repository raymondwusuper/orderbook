#include <cstdio>
#include <cstdint>
#include <thread>
#include <chrono>

uint64_t cntvct() {
    asm volatile("isb" ::: "memory");
    uint64_t t;
    asm volatile("mrs %0, cntvct_el0" : "=r"(t));
    return t;
}

uint64_t cntfrq() {
    uint64_t f;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
    return f;
}

int main() {
    printf("cntfrq_el0 = %llu Hz\n", (unsigned long long)cntfrq());

    uint64_t t0 = cntvct();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint64_t t1 = cntvct();
    printf("ticks over 100ms = %llu\n", (unsigned long long)(t1 - t0));
    printf("implied freq = %llu Hz\n", (unsigned long long)((t1 - t0) * 10));
}