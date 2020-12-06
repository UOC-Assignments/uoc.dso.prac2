/*
 * New system call
 */

#include <asm/unistd.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/thread_info.h>
#include <asm/asm-offsets.h>   /* NR_syscalls */

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("New system call");

unsigned int free_ident = -1;

extern unsigned sys_call_table[];

asmlinkage long
sys_newsyscall (int parameter)
{
  printk ("Hello world, parameter %d\n", parameter);
  return (29);
}

#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) /* Disable RO protection */
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) /* Enable RO protection */

static int __init
newsyscall_init (void)
{
  unsigned int i;

  /* Look for an unused sys_call_table entry */
  for (i = 0; i < NR_syscalls; i++)
    if (sys_call_table[i] == (unsigned) sys_ni_syscall)
      break;

  /* Found? */
  if (i == NR_syscalls)
    {
      printk ("No free entry available");
      return (-1);
    }

  /* Use this sys_call_entry */
  free_ident = i;

  GPF_DISABLE; /* Disable RO protection (sys_call_table is on a RO page )*/
  sys_call_table[free_ident] = (unsigned) sys_newsyscall;
  GPF_ENABLE; /* Enable RO protection */

  printk (KERN_INFO "New syscall installed. Identifier = %d\n", free_ident);
  return (0);
}

static void __exit
newsyscall_exit (void)
{
  /* Restore previous state */
  if (free_ident != -1) {
    GPF_DISABLE; /* Disable RO protection (sys_call_table is on a RO page )*/
    sys_call_table[free_ident] = (unsigned) sys_ni_syscall;
    GPF_ENABLE; /* Enable RO protection */

    free_ident = -1;
    printk (KERN_INFO "New syscall removed\n");
  }
  else 
    printk (KERN_INFO "Unexpected error\n");
}

module_init (newsyscall_init);
module_exit (newsyscall_exit);
