/*
 * Example driver
 *
 */

#include <linux/kvm_irqfd.h> /* copy_to_user */
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>	/* find_task_by_pid_ns */


/* /dev/inodes ja està registrat com a dispositiu*/
#define DRIVER_MAJOR 127
#define DRIVER_NAME "inodes"

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("implementació d'un driver que respon a una sèrie de crides al sistema determinades que permeten obtenir les proteccions d'un inode del sistema de fitxers");
MODULE_AUTHOR ("Jordi Bericat Ruz");

int do_open (struct inode *inode, struct file *filp) {
  /* Just O_RDONLY mode */
  if (filp->f_flags != O_RDONLY) 
    return -EACCES;

  return 0;
}

ssize_t do_read (struct file * filp, char *buf, size_t count, loff_t * f_pos) {
  return 0;
}

ssize_t do_write (struct file * filp, const char *buf, size_t count, loff_t * f_pos) {
  return 0;
}

int do_release (struct inode *inode, struct file *filp) {
  return 0;
}

static long do_ioctl (struct file *filp, u_int cmd, u_long arg) {
  return 0;
}

static loff_t do_llseek (struct file *file, loff_t offset, int orig) {
  return 0;
}

struct file_operations driver_op = {
open:do_open,
read:do_read,
write:do_write,
release:do_release,           
unlocked_ioctl:do_ioctl,
llseek:do_llseek
};

static int __init
abc_init (void)
{
  int result;

  result = register_chrdev (DRIVER_MAJOR, DRIVER_NAME, &driver_op);
  if (result < 0)
    {
      printk ("Unable to register device");
      return result;
    }

  printk (KERN_INFO "Correctly installed\n Compiled at %s %s\n", __DATE__,
          __TIME__);
  return (0);
}

static void __exit
abc_cleanup (void)
{
  unregister_chrdev (DRIVER_MAJOR, DRIVER_NAME);

  printk (KERN_INFO "Cleanup successful\n");
}

module_init (abc_init);
module_exit (abc_cleanup);
