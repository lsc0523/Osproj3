#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

struct lock sys_lock;

static void syscall_handler (struct intr_frame *);
struct file
{
	struct inode *inode;
	off_t pos;
	bool deny_write;
};
	
void syscall_init (void) 
{
	lock_init(&sys_lock);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
void check_vaddr(void* esp)
{
	if(is_kernel_vaddr(esp))
	{
		lock_release(&thr_lock);
		exit(-1);
	}
}

	static void
syscall_handler (struct intr_frame *f UNUSED) 
{
//	printf("syscall : %d\n",*(uint32_t *)(f->esp));
/*	printf("address : %10X\n\n",f->esp);
	printf("f->esp+4 is %d\n\n",*(uint32_t*)(f->esp +4));
	printf("f->esp+8 is %d\n\n",*(uint32_t*)(f->esp+8));
	printf("f->esp+12 is %d\n\n",*(uint32_t*)(f->esp+12));
	printf("f->esp+12 is %d\n\n",*(uint32_t*)(f->esp+16));
	printf("f->esp+12 is %d\n\n",*(uint32_t*)(f->esp+20));
*/
	if(is_kernel_vaddr(f->esp))
		exit(-1);
	//hex_dump(f->esp,f->esp,100,1);
	int *temp=f->esp;
	struct thread* now=thread_current();
	//printf("syscall : %d\n",*(uint32_t *)(f->esp));
	switch(*(uint32_t *)(f->esp))
	{
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			lock_acquire(&thr_lock);
			check_vaddr(f->esp+4);
			lock_release(&thr_lock);
			exit(*(uint32_t *)(f->esp +4));
			break;
		case SYS_EXEC://2
			check_vaddr(f->esp+4);
			f->eax=exec((const char*)*(uint32_t*)(f->esp +4));
			break;
		case SYS_WAIT:
			check_vaddr(f->esp+4);
			f->eax=wait( (pid_t)*(uint32_t *)(f->esp + 4));
			break;
		case SYS_CREATE:
			check_vaddr(f->esp+4);
			check_vaddr(f->esp+8);
			f->eax=create((const char*)*(uint32_t*)(f->esp+4),(unsigned)*(uint32_t*)(f->esp+8));
			break;
		case SYS_REMOVE:
			check_vaddr(f->esp+4);
			f->eax=remove((const char*)*(uint32_t*)(f->esp+4));
			break;
		case SYS_OPEN:
			check_vaddr(f->esp+4);
			lock_acquire(&thr_lock);
			f->eax=open((const char*)*(uint32_t*)(f->esp+4));
			lock_release(&thr_lock);
			break;
		case SYS_FILESIZE:
			check_vaddr(f->esp+4);
			f->eax=filesize((int)*(uint32_t*)(f->esp+4));
			break;
		case SYS_READ:
			check_vaddr(f->esp+4);
			check_vaddr(f->esp+8);
			check_vaddr(f->esp+12);
			lock_acquire(&thr_lock);
			f->eax=read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
			lock_release(&thr_lock);
			break;
		case SYS_WRITE://9
			check_vaddr(f->esp+4);
			check_vaddr(f->esp+8);
			check_vaddr(f->esp+12);
			lock_acquire(&thr_lock);
			f->eax=write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8 ), (unsigned)*((uint32_t *)(f->esp +12)));
			lock_release(&thr_lock);
			break;
		case SYS_SEEK:
			check_vaddr(f->esp+4);
			check_vaddr(f->esp+8);
			seek((int)*(uint32_t*)(f->esp+4),(unsigned)*(uint32_t*)(f->esp+8));
			break;
		case SYS_TELL:
			check_vaddr(f->esp+4);
			f->eax=tell((int)*(uint32_t*)(f->esp+4));
			break;
		case SYS_CLOSE://12
			check_vaddr(f->esp+4);
			close((int)*(uint32_t*)(f->esp+4));
			break;
		case SYS_PIBO:
			check_vaddr(f->esp+4);
			f->eax=pibonacci((int)*(uint32_t*)(f->esp+4));
			break;
		case SYS_SUM:
			check_vaddr(f->esp+4);
			check_vaddr(f->esp+8);
			check_vaddr(f->esp+12);
			check_vaddr(f->esp+16);
			//f->eax=pibonacci((int)*(uint32_t *)(f->esp+4));
			f->eax=sum_of_four_integers((int)*(uint32_t *)(f->esp+4),(int)*(uint32_t *)(f->esp+8),(int)*(uint32_t *)(f->esp+12),(int)*(uint32_t *)(f->esp+16));
			break;

	}
	//printf ("system call!\n");
	//thread_exit ();
}

void halt(void)
{
	shutdown_power_off();
}

void exit(int status)
{
	struct thread* now=thread_current();
	//list_remove(&(now->child_elem));
	/*
	printf("%s: exit(%d)\n", thread_name(),status);
	now->parent->child_status=THREAD_DYING;
	thread_exit();*/
	/*if(now->parent!=NULL)
	{
		now->parent->child_status=THREAD_DYING;
		now->parent->waiting=false;
		now->parent->exit_flag=status;
	}*/
	printf("%s: exit(%d)\n",thread_name(),status);
	now->exit_status=status;
	thread_exit();

}

pid_t exec(const char *cmd)
{
//	printf("********* syscall : %s *******\n",cmd);
	return process_execute(cmd);
}

int wait(pid_t pid)
{
	int result= process_wait(pid);
	return result;

}

int read(int fd, void* buffer, unsigned size)
{
	int i=0;
	int r;
	check_vaddr(buffer);
	//lock_acquire(&sys_lock);
	if(fd==0){
		for (i=0;i<size;i++){
			if(*(uint8_t *)(buffer+i) = input_getc()){
					break;
			}		
		}
		if(i!=size){
			//lock_release(&sys_lock);
			return -1;
		}
	}
	else if(fd>2){	
		struct thread* now_t=thread_current();
		if(now_t->FD[fd]==NULL){
			//lock_release(&sys_lock);
			exit(-1);
		}
		
		r= file_read(now_t->FD[fd],buffer,size);
		//lock_release(&sys_lock);
		return r;
	}
	//lock_release(&sys_lock);
	return i;

}

int write(int fd,const void *buffer, unsigned size)
{
	//printf("fd is %d \nbuffer is [%s]\n",fd,buffer);
	check_vaddr(buffer);
	//lock_acquire(&sys_lock);
	struct thread* now_t=thread_current();
	if(fd==1){
		putbuf(buffer,size);
	//	lock_release(&sys_lock);
		return size;
	}
	else if(fd>2){
		if(now_t->FD[fd]==NULL){
			//lock_release(&sys_lock);
			exit(-1);
		}
		if(thread_current()->FD[fd]->deny_write){
			file_deny_write(thread_current()->FD[fd]);
		}
		/*if(thread_current()->FD[fd]->deny_write)
			printf("%d 는 deny당해버렸어요~~\n",fd);*/
		//lock_release(&sys_lock);
		int ret= file_write(now_t->FD[fd],buffer,size);
		//lock_release(&sys_lock);
		return ret;
	}
	//lock_release(&sys_lock);
	return -1;
}

int pibonacci(int n){
	if(n==0)
		return 0;
	if(n==1)
		return 1;
	if(n==2)
		return 1;
	
	int i,n1=1,n2=1,temp;
	for(i=0;i<n-2;i++){
		temp = n1+n2;
		n1 = n2;
		n2 = temp;
	}

	return temp;
}

int sum_of_four_integers(int a,int b,int c,int d){
	return a+b+c+d;
}

bool create(const char *file, unsigned initial_size)
{
	if(file==NULL)
		exit(-1);
	check_vaddr(file);
	if(filesys_create(file,initial_size))
		return true;
	else
		return false;
}
bool remove(const char *file)
{
	if(file==NULL)
		exit(-1);
	if(filesys_remove(file))
		return true;
	else
		return false;
}
int open(const char* file)
{
	if(file==NULL)
		exit(-1);
	check_vaddr(file);
	lock_acquire(&sys_lock);
	struct file* ret=filesys_open(file);
	lock_release(&sys_lock);
	if(ret==NULL){//could not open
		//printf("없어ㅠㅠ\n");
		//lock_release(&sys_lock);
		return -1;
	}

	//file_deny_write(file);

	struct thread* now_t=thread_current();
	int i;
	for(i=3;i<128;i++)
	{
		if(now_t->FD[i]==NULL){
			if(strcmp(now_t->name,file)==0)
			{
				file_deny_write(ret);
			}
			now_t->FD[i]=ret;
			//lock_release(&sys_lock);
			return i;
		}
	}
	//lock_release(&sys_lock);
	return -1;

}
int filesize(int fd)
{
	struct thread* now_t=thread_current();
	if(now_t->FD[fd]==NULL)
		exit(-1);
      	return file_length(now_t->FD[fd]);
}
void seek(int fd, unsigned position)
{
	struct thread* now_t=thread_current();
	if(now_t->FD[fd]==NULL)
		exit(-1);
	file_seek(now_t->FD[fd],position);

}
unsigned tell(int fd)
{
	struct thread* now_t=thread_current();
	if(now_t->FD[fd]==NULL)
		exit(-1);
	return file_tell(now_t->FD[fd]);
}
void close(int fd)
{
	struct thread* now_t=thread_current();
	if(now_t->FD[fd]==NULL)
	{
		exit(-1);
	}
	struct file* p;
	p=now_t->FD[fd];
	file_allow_write(p);
	p=NULL;
	return file_close(p);
	//now_t->FD[fd]=NULL;
}

