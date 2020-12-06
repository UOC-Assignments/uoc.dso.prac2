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


#define DRIVER_MAJOR 231
#define DRIVER_NAME "abc"

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("Example driver");
MODULE_AUTHOR ("Enric Morancho");

char alphabet[] =
  { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

int
do_open (struct inode *inode, struct file *filp)
{
  /* Just O_RDONLY mode */
  if (filp->f_flags != O_RDONLY) 
    return -EACCES;

  return 0;
}

ssize_t
do_read (struct file * filp, char *buf, size_t count, loff_t * f_pos)
{
  loff_t real_read;
  int k;

  /* The number of bytes to be read must be >= 0 */
  if (count < 0)
    return -EINVAL;

  /* If beyond end of file, returns 0 */
  if (*f_pos >= sizeof (alphabet))
    return 0;

  /* Driver limits the maximum number of characters to be read simultaneously */
  if (count > 3)
    count = 3;

  /* Computes the real number of characters that can be read */
  if ((*f_pos + count) < sizeof (alphabet))
    real_read = count;
  else
    real_read = sizeof (alphabet) - *f_pos;

  /* Checks access rights over the range of memory addresses that will be modified */
  /* This check has already been performed by vfs_read() from buff to buff+count: access_ok(VERIFY_WRITE, buf, count) */
  /* Consequently, in abc-driver, this check is unnecessary */
  if (!access_ok (VERIFY_WRITE, buf, real_read))
    return -EFAULT;

  /* Transfers data to user space */
  k = raw_copy_to_user (buf, alphabet + *f_pos, real_read);
  if (k!=0)
    return -EFAULT;
  
  /* Updates file pointer */
  *f_pos += real_read;

  /* Returns number of read characters */
  return real_read;
}

ssize_t
do_write (struct file * filp, const char *buf, size_t count, loff_t * f_pos)
{
  return 0;
}

/* close system call */
int
do_release (struct inode *inode, struct file *filp)
{
  return 0;
}

static long
do_ioctl (struct file *filp, u_int cmd, u_long arg)
{
  return 0;
}

static loff_t
do_llseek (struct file *file, loff_t offset, int orig)
{
  loff_t ret;

  switch (orig)
    {
    case SEEK_SET:
      ret = offset;
      break;
    case SEEK_CUR:
      ret = file->f_pos + offset;
      break;
    case SEEK_END:
      ret = sizeof (alphabet) + offset;
      break;
    default:
      ret = -EINVAL;
    }

  if (ret >= 0)
    file->f_pos = ret;
  else
    ret = -EINVAL;

  return ret;
}

struct file_operations abc_op = {
open:do_open,
read:do_read,
write:do_write,
release:do_release,            /* close system call */
unlocked_ioctl:do_ioctl,
llseek:do_llseek
};

static int __init
abc_init (void)
{
  int result;

  result = register_chrdev (DRIVER_MAJOR, DRIVER_NAME, &abc_op);
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
