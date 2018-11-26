#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
//char* parsed_word(char* input);//input을 parsing해준다.
void push_stack(char* word,void** stack);//stack(esp)에 word를 쌓아준다.
#endif /* userprog/process.h */
