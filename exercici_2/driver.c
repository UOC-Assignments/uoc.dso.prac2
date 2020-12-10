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


#define DRIVER_MAJOR 127 /* "/dev/inodes" ja està registrat com a dispositiu al SO */
#define DRIVER_NAME "inodes"
 

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("implementació d'un driver que respon a una sèrie de crides al sistema determinades que permeten obtenir les proteccions d'un inode del sistema de fitxers");
MODULE_AUTHOR ("Jordi Bericat Ruz");

/* ESTABLIM LA VARIABLE GLOBAL ON DESAREM EL SEMÂFOR QUE ENS PERMETRA RESTRINGIR LA CONCURRENCIA DE DISPOSITIUS /dev/inodes OBERTS (NOMÉS 1) */

static struct semaphore dev_sem;

/* DEFINIM VARIABLES I FUNCIONS AUXILIARS PER A OBTENIR LES PROTECCIONS D'UN INODE CONCRET (CODI EXTRET DE "NEWSYSCALL2.c") */

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

/* IMPLEMENTEM LES OPERACIONS D'ACCÉS AL DISPOSITIU */

int do_open (struct inode *inode, struct file *filp) { //AQUI EL paramatre inode no el fem servir per res........

  int sem_status;

  /* Si no s'ha obert el dispositiu en mode R/W, aleshores retornem error "EACCES" */
  if (filp->f_flags != O_RDWR) {
    return -EACCES;
  }

  /* Si el dispositiu ja està obert, aleshores retornem error "EBUSY". Implementem 
   * la comprovació mitjançant el semàfor "dev_sem" (veuere memòria PDF, exercici 2, 
   * secció 2.2 (detalls do_open) */ 
  sem_status = down_trylock(&dev_sem);
  if (sem_status == 0) {
		//THE DEVICE FILE IS FREE SO WE CAN OPEN IT 
		return 0;		
  } else {
		//TRYING TO OPEN THE DEVICE FILE FOR SECOND TIME 
		return -EBUSY; 
  }
}

ssize_t do_read (struct file * filp, char *buf, size_t count, loff_t * f_pos) {

  loff_t real_read;
  int k;

  printk("BREAKPOINT 1\n");

  /*Si intentem llegir una quantitat diferent a un byte, retornem error EINVAL */
  if (count != 1) {
	return -EINVAL;
  }

  printk("BREAKPOINT 2\n");

  /* If beyond end of file, returns 0 */
  /* if (*f_pos >= sizeof (?????))
    return 0; */

  printk("BREAKPOINT 3\n");

  /* Computes the real number of characters that can be read */
  /*if ((*f_pos + count) < sizeof (??????))
    real_read = count;
  else */
    real_read = count; // o bé = sizeof (*f_pos) .......

  printk("BREAKPOINT 4\n");

  struct inode* inode = inode_get(*f_pos);
  printk("******* INODE f_pos = %d\n",*f_pos);
  printk("******* INODE i_mode = %o\n",inode->i_mode);

  /* Transfers data to user space */
  k = raw_copy_to_user(buf, &inode->i_mode, sizeof(mode_t)); //Copiem al bufer la quantitat de bytes corresponents al tipus mode_t
  if (k!=0)
    return -EFAULT;
  
  /* Updates file pointer */ //AIXÒ REALMENT CREC QUE NO CAL, ES FEIA SERVIR A ABC PER A FER REOCRREGUTS (CREC)
  *f_pos += real_read;

  return real_read; 
}

ssize_t do_write (struct file * filp, const char *buf, size_t count, loff_t * f_pos) {
  return 0;
}

int do_release (struct inode *inode, struct file *filp) {
  /* Alliberem el semàfor */
  up(&dev_sem);
  return 0;
}

static long do_ioctl (struct file *filp, u_int cmd, u_long arg) {
  return 0;
}

static loff_t do_llseek (struct file *file, loff_t offset, int orig)
{
  loff_t ret;

  switch (orig)
    {
    case SEEK_SET: //AIXÒ ÉS CORRECTE
      ret = offset;
      break;
    case SEEK_CUR: //AIXÒ S'HA DE PROVAR ENCARA
      ret = file->f_pos + offset;
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

struct file_operations driver_op = {
	open:do_open,
	read:do_read,
	write:do_write,
	release:do_release,           
	unlocked_ioctl:do_ioctl,
	llseek:do_llseek
};

/* RUTINES INIT I EXIT DE CÀRREGA / DESCARREGA DEL MÒDUL */

static int __init inodesDriver_init (void)
{
  int result;

  result = register_chrdev (DRIVER_MAJOR, DRIVER_NAME, &driver_op);
  if (result < 0)
    {
      printk ("Unable to register device");
      return result;
    }
  /* Inicialitzem el semàfor que ens permetrà controlar quan el dispositiu /dev/inodes està ocupat */
  sema_init(&dev_sem, 1);
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
