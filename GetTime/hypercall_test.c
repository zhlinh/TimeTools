/**
 * This program is to read HyperCallTime(just for test).
 * HyperCallTime: pcie ptp-card time from hypercall
*/

#include <stdio.h>
#include <time.h>
#include <linux/kvm_para.h>

#define KVM_HC_GET_PCIE_TIME 100
#define KVM_HC_GET_SYS_TIME 101
#define KVM_HYPERCALL ".byte 0x0f,0x01,0xc1"
#define NSEC 1000000000

int main()
{
    printf("Hello HyperCall!\n");
    unsigned long ret = 0;
    unsigned long rete = 0;
    unsigned  nr_p = KVM_HC_GET_PCIE_TIME;
    unsigned  nr_s = KVM_HC_GET_SYS_TIME;
    struct timespec time0, time1;

    clock_gettime(CLOCK_REALTIME, &time0);
    asm volatile(KVM_HYPERCALL
            : "=a"(ret),"=b"(rete)
            : "a"(nr_p)
            :"memory");
    clock_gettime(CLOCK_REALTIME, &time1);
    printf("pcietime: %ld(s)\n", ret);
    printf("pcietime: %ld(ns)\n", rete);
    printf("hypercall takes %ld\n", (time1.tv_sec * NSEC \
                + time1.tv_nsec) - (time0.tv_sec * NSEC + time0.tv_nsec));

    ret = 0;
    rete = 0;
    asm volatile(KVM_HYPERCALL
            : "=a"(ret),"=b"(rete)
            : "a"(nr_s)
            :"memory");
    printf("hostsystime: %ld(n)\n", ret);
    printf("hostsystime: %ld(ns)\n", rete);
    return 0;
}

