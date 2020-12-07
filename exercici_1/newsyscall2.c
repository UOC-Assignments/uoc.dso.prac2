/*
 * New system call
 */

#include <asm/unistd.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/thread_info.h>
#include <asm/asm-offsets.h>   /* NR_syscalls */
#include <linux/errno.h> 

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("New system call");

unsigned int free_ident = -1;

extern unsigned sys_call_table[];
extern struct inode *ext2_iget (struct super_block *, unsigned long);

struct pack{
	int num_inode;
	struct inode *inode;
};

void iterate_function(struct super_block *sb, void *void_ptr)
{
  struct pack *info_ptr = (struct pack*) void_ptr;

  if (info_ptr->inode != NULL) return;
  if (strcmp ("ext2", sb->s_type->name) != 0) return;

  info_ptr->inode = ext2_iget(sb, info_ptr->num_inode);
}

struct inode *inode_get (int num_inode)
{
  struct pack info = {.num_inode = num_inode, .inode = NULL};

  iterate_supers(iterate_function, (void*)&info);

  if ((info.inode == NULL) || (IS_ERR (info.inode)))
    return (NULL);
  else
    return (info.inode);
}

asmlinkage long sys_newsyscall (int parameter)
{
  /* Si l'inode rebut com a paràmetre és incorrecte, aleshores retornem error ENOENT (No such file or directory) */
  if (parameter == 0) { 
	return -ENOENT; 
  }
  /* En cas que el inode correspongui a un fitxer o directori existeixi al sistema de fitxers, 
   * aleshores retornem la seva configuració de restriccions (rwx)*/
  else { 
	return inode_get(parameter)->i_mode;
  }
}

#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) /* Disable RO protection */
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) /* Enable RO protection */

static int __init newsyscall_init (void)
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

static void __exit newsyscall_exit (void)
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
