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
#include <linux/fcntl.h>
#include <linux/semaphore.h>
#include <linux/slab.h>


/* /dev/inodes ja està registrat com a dispositiu*/
#define DRIVER_MAJOR 127
#define DRIVER_NAME "inodes"
//#define DEFINE_SEMAPHORE(dev_sem)  

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("implementació d'un driver que respon a una sèrie de crides al sistema determinades que permeten obtenir les proteccions d'un inode del sistema de fitxers");
MODULE_AUTHOR ("Jordi Bericat Ruz");

struct semaphore *dev_sem;

int do_open (struct inode *inode, struct file *filp) {

  int sem_status;

  /* Si no s'ha obert el dispositiu en mode R/W, aleshores retornem error "EACCES" */
  if (filp->f_flags != O_RDWR) {
    return -EACCES;
  }

  /* Si el dispositiu ja està obert, aleshores retornem error "EBUSY". Implementem 
   * la comprovació mitjançant el semàfor "dev_sem" (veuere memòria PDF, exercici 2, 
   * secció 2.2 (detalls do_open) */ 
  sem_status = down_trylock(dev_sem);
  if (sem_status == 0) {
		//printk("THE DEVICE FILE IS FREE SO WE CAN OPEN IT [ sem_status -> %d ]\n",sem_status);
		return 0;		
  } else {
		//printk("TRYING TO OPEN THE DEVICE FILE FOR SECOND TIME ->  [ sem_status -> %d ]\n",sem_status);
		return -EBUSY; 
  }
}

ssize_t do_read (struct file * filp, char *buf, int num) {
  if (num != 1) {
	return -EINVAL;
  }
  return 0;
}

ssize_t do_write (struct file * filp, const char *buf, size_t count, loff_t * f_pos) {
  return 0;
}

int do_release (struct inode *inode, struct file *filp) {
  /* Alliberem el semàfor / mutex */
  up(dev_sem);
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

static int __init inodesDriver_init (void)
{
  int result;

  result = register_chrdev (DRIVER_MAJOR, DRIVER_NAME, &driver_op);
  if (result < 0)
    {
      printk ("Unable to register device");
      return result;
    }
  /* reservem memòria a l'espai del kernel per al semàfor */
  dev_sem = (struct semaphore *)kzalloc(sizeof(struct semaphore), GFP_KERNEL);
  /* Inicialitzem el semàfor que ens permetrà controlar quan el dispositiu /dev/inodes està ocupat */
  sema_init(dev_sem, 1);
  //DEFINE_SEMAPHORE(dev_sem); /* NO SE PERQUÈ AMB EL SEMÀFOR ESTàTIC (BINARI) NO ACONSEGUEIXO FER EL LOCK, PERÒ AMB EL DINÀMIC SÍ...*/
  printk (KERN_INFO "Correctly installed\n Compiled at %s %s\n", __DATE__, __TIME__);
  return (0);
}

static void __exit inodesDriver_cleanup (void)
{
  unregister_chrdev (DRIVER_MAJOR, DRIVER_NAME);

  printk (KERN_INFO "Cleanup successful\n");
}

module_init (inodesDriver_init);
module_exit (inodesDriver_cleanup);
