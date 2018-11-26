#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "../lib/user/syscall.h"

void syscall_init (void);
void check_vaddr(void* esp);
int pibonacci(int n);
int sum_of_four_integers(int a,int b,int c,int d);

#endif /* userprog/syscall.h */
