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
#                    FILE 2 OF 2: driver.c                                     #
#                        VERSION: 1.0                                          #
#                                                                              #
################################################################################
	
################################################################################
#                                                                              #
#  DESCRIPTION:                                                                #
#                                                                              #
#                                                                              #
#  IMPLEMENTATION STRATEGY:                                                    #
#                                                                              #
#                                                                              #
#  USAGE:                                                                      #
#                                                                              #
#  root@localhost:~/insmod driver.ko                                           #
#                                                                              #
#  1. obtain the device's file descriptor: fd = open(/dev/inodes,,)            #
#  2. perform r/w operations: read(fd,buf,bytes) / write(fd,buf,bytes)         #
#  3. etc...                                                                   #
#                                                                              #
################################################################################
	
	
_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(
	
	
################################################################################
#                                                                              #
#                             1. INCLUDES & DEFINES                            #
#                                                                              #
##############################################################################*/

#include <linux/kvm_irqfd.h> /* copy from / to user - kernel spaces */
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/sched.h>	/* find_task_by_pid_ns */
#include <linux/semaphore.h>

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Jordi Bericat Ruz");
MODULE_DESCRIPTION ("implementació d'un driver que respon a una sèrie de crides\
al sistema determinades, les quals permeten consultar i modificar les\
proteccions d'un inode del sistema de fitxers");

// 1.1 - The device /dev/inodes already exists natively in our linux kernel 
//       build, therefore we don't have to assign a new MAJOR number (instead, 
//       we will use the current MAJOR designation for this device. More details
//       about this in the PDF memory (section 2.2.2).

#define DRIVER_MAJOR 127 
#define DRIVER_NAME "inodes"

/*##############################################################################
#                                                                              #	
#                     2. AUXILIAR DATA STRUCTURES & FUNCTIONS                  #
#                                                                              #
##############################################################################*/

// 2.1 - We set a semaphore type global variable, so later we can constrain the 
//       number of concurrent open devices to one (and only one).

static struct semaphore dev_sem;

// 2.2 - We will be using the next functions and data structures in order to  
//       obtain one inode's protection (Code obtained from - 5.BIBLIOGRAPHY [#x])

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

/*##############################################################################
#                                                                              #
#                       3. DEVICE OPERATIONS IMPLEMENTATION                    #
#                                                                              #
##############################################################################*/

// 3.1 - open() device dependent operation implementation 

int do_open (struct inode *inode, struct file *filp) { 
	
	int sem_status;

	// 3.1.1 - ERROR CHECK: The device must be open in Read / Write mode. If 
	//         don't, we return an EACCES errno (permission denied).
	
	if (filp->f_flags != O_RDWR) {
		return -EACCES;
	}
	
	// 3.1.2 - ERROR CHECK: If the device is already open, then we return the 
	//         EBUSY errno. We can stablish this behaviour by means of defining 
	//         a mechanism of mutual exclusion, which in linux can be  
	//         implemented using a binary semaphore (linux/semaphore.h).
	
	sem_status = down_trylock(&dev_sem);
	if (sem_status == 0)
		return 0;
	else
		return -EBUSY;
}

// 3.2 - read() device dependent operation implementation

ssize_t do_read (struct file * filp, char *buf, size_t count, loff_t * f_pos) {
	
	int k;
	struct inode* my_inode = inode_get(*f_pos);
	
	// 3.2.1 - ERROR CHECK: If we try to read more than one inode (or nothing at   
	//         all) on this read operation, then we return an EINVAL errno 
	//         (invalid argument)
	
	if (count != 1) {
		return -EINVAL;
	}
		
	// 3.2.2 - ERROR CHECK: If beyond end of file, returns 0
	//
	//         We don't have to perform this error check. From Bibliography [1]:
	//         
	//         "On files that support seeking, (...) if the current file offset 
	//         is at or past the end of file, no bytes are read, and read() 
	//         returns zero"
	
	/*if (*f_pos >= sizeof(?)) 
	return 0;*/
		
	// 3.2.3 - ERROR CHECK: If we are trying to retrieve an invalid inode, then 
	//         we return an ENOENT errno (no such file or directory).
	
	if (my_inode == NULL)
		return -ENOENT;

	// 3.2.4 - We transfer the inode's protection data to the user space. More 
	//         precisely, we do copy the amount of bytes corresponding to the 
	//         "mode_t" type (the inode's protection data size) into the buffer 
	//         allocated in user memory space.
	
	k = raw_copy_to_user(buf, &my_inode->i_mode, sizeof(mode_t)); 
	
	// 3.2.5 - ERROR CHECK: If we refer an invalid memory address (e.g. out of  
	//         the kernel memory area where the process stack is allocated), 
	//         then we returnan EFAULT errno (bad address). 
	
	if (k!=0)
	return -EFAULT;
	
	// 3.2.6 - As a side effect, the file R/W pointer needs to be updated before 
	//         exiting the system call (see comment 3.3.6 for further reference).
	
	*f_pos += count;
	
	// 3.2.7 - Finally, since the inode pointed by *f_pos is a valid one, and we 
	//         also have been able to read from kernel memory space succesfully, 
	//         then we can leave this read syscall returning a success code (1)
	
	return count; 
}

// 3.3 - write() device dependent operation implementation

ssize_t do_write (struct file * filp, const char *buf, size_t count, loff_t * f_pos) {
	
	int k;
	struct inode* my_inode = inode_get(*f_pos);
	
	// 3.3.1 - The third "do_write()" parameter must be 1. If not, this means
	//         that we're trying to modify more than one inode in the same 
	//         write() operation (in tht case we return an EINVAL errno (invalid 
	//         argument).
	
	if (count != 1) 
		return -EINVAL;
		
	// 3.3.2 - If we are trying to write on an invalid inode, then we return an
	//         ENOENT errno (no such file or directory)
	
	if (my_inode == NULL)
		return -ENOENT;
	
	// 3.3.3 - Caller must check the specified block with "access_ok" before 
	//         calling the "copy_from_user" function
	
	if (!access_ok (VERIFY_WRITE, buf, sizeof(my_inode))) 
		return -EFAULT;
	
	// 3.3.4 - Now we transfer the protection data from user space 
	
	k = copy_from_user(&my_inode->i_mode,buf,sizeof(mode_t));
	
	// 3.3.5 - If "copy_from_user" couldn't copy the whole set of requested 
	//         bytes into kernel space, then the function returns the amount of 
	//         bytes that were not transfered (so the "do_write" routine will 
    //         return the -1 error code). On the contrary, if the function was
	//         able to copy all the data retrieved from user space ("mode_t" 
    //         bytes), then its return value will zero. In that case, the 
	//         "do_write" routine will return the 1 success code.
	
	if (k != 0)
		return -1;
		
	// 3.3.6 - Before leaving though, we shoud take into consideration that as 
	//         a side effect of writing into the device, we will have to
	//         properly update its file descriptor R/W pointer, so it refers to 
	//         the next inode in the table. This way, we can perform lseek 
	//         "SEEK_CUR" operations -that is, retrieve the current pointer 
	//         position.
	
	*f_pos += count;
	return count;
}

// 3.4 - close() device dependent operation implementation

int do_release (struct inode *inode, struct file *filp) {
	
	// 3.4.1 - We must release the lock on the semaphore, so we can later safely
	//         unload the driver module without leaving any locked OS resource 
	//         (on this case, the /dev/inodes device).
	
	up(&dev_sem);
	return 0;
}

static long do_ioctl (struct file *filp, u_int cmd, u_long arg) {
	return 0;
}

// 3.5 - lseek() device dependent operation implementation

static loff_t do_llseek (struct file *file, loff_t offset, int orig)
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

/*##############################################################################
#                                                                              #
#                          4. MODULE INIT / EXIT FUNCTIONS                     #
#                                                                              #
##############################################################################*/

//  The code below is an adaptation from the ones found at - 5.BIBLIOGRAPHY [#x]   

//  4.1 - Module / Driver INIT Function

static int __init inodesDriver_init (void)
{
	int result;
	
	//  4.1.1 - Register (link) the already existing /dev/inodes device file (we 
	//          did already set the MAJOR # in the section #1).  
	
	result = register_chrdev (DRIVER_MAJOR, DRIVER_NAME, &driver_op);
	if (result < 0)
    {
		printk ("Unable to register device");
		return result;
	}
	
	//  4.1.2 - We set the binary semaphore that will restrict concurrent access
	//          to /dev/inodes, so we can track when the device is busy.

	sema_init(&dev_sem, 1);
	
	printk (KERN_INFO "Driver correctly installed\n Compiled at %s %s\n", __DATE__, __TIME__);
	return (0);
}

//  4.2 - Module / Driver EXIT Function

static void __exit inodesDriver_cleanup (void)
{
	
	//  4.2.1 - Unregister the /dev/inodes device, so we can unload the module 
	//          and the device becomes available to other OS processes.
	
	unregister_chrdev (DRIVER_MAJOR, DRIVER_NAME);
	
	printk (KERN_INFO "Cleanup successful\n");
}

module_init (inodesDriver_init);
module_exit (inodesDriver_cleanup);


/*##############################################################################
#                                                                              #
#                                 5. BIBLIOGRAPHY                              #
#                                                                              #
################################################################################

     1. Examples provided into the DSO Practical Assignment #2:
	 1.1. newsyscall.c (example 5)
	 1.2. newsyscall2.c
	 1.3. abc.c (example 6)
	 2. "man 2 read" -> https://linux.die.net/man/2/read
	 3. .........

*/