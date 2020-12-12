/*##############################################################################
#                                                                              #
#                       UOC - Open University of Catalonia                     #
#                                                                              #
################################################################################

################################################################################
#                                                                              #
#                         OPERATING SYSTEMS DESIGN (DSO)                       #
#                            PRACTICAL ASSIGNMENT #2                           #
#                                                                              #
#                        STUDENT: Jordi Bericat Ruz                            #
#                           TERM: Autumn 2020/21                               #
#                       GIT REPO: UOC-Assignments/uoc.dso.prac2                #
#                    FILE 1 OF 2: newsyscall2.c                                #
#                        VERSION: 1.0                                          #
#                                                                              #
################################################################################

################################################################################
#                                                                              #
#  DESCRIPTION:                                                                #
#                                                                              #
#  Linux system call implementation that allows us to retrieve the protection  #
#  data related to the inode specified as a parameter                          #
#                                                                              #
################################################################################


_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~*/


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

  // If the inode we got as a parameter is not valid, then we return an ENOENT 
  // errno (no such file or directory)
  if (parameter == 0) { 
	return -ENOENT; 
  }
  
  // If the inode exists, then we call the provided "inode_get" function in 
  // order to retrieve the inode's protection data, so we can return that vaule.
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
