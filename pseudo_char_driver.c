#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/err.h>

#define MAX_SIZE 256

/*Created a small memory area */
char dev_buf[MAX_SIZE];

/* Holds device number */
dev_t dev_num;

/* Cdev variable */
struct cdev char_cdev;

loff_t char_lseek(struct file* filp, loff_t off, int whence)
{
	loff_t temp;
	printk(KERN_INFO"%s: Current file position = %lld\n",__func__,filp->f_pos);
	
	switch(whence)
	{
		case SEEK_SET:
						if((off > MAX_SIZE) || (off < 0) )
							return -EINVAL;
						filp->f_pos = off;
						break;
		case SEEK_CUR:
						temp = filp->f_pos + off;
						if(temp > MAX_SIZE || temp < 0)
							return -EINVAL;
						filp->f_pos = temp;
						break;
		case SEEK_END:
						temp = MAX_SIZE + off;
						if((temp > MAX_SIZE) || (temp < 0))
							return -EINVAL;
						filp->f_pos = temp;
						break;
		default:
				return -EINVAL;
	}
	printk(KERN_INFO"%s: Updated position: %lld\n",__func__,filp->f_pos); 
	return filp->f_pos;
}
	

ssize_t char_read(struct file* filp, char __user* buff, size_t count, loff_t *f_pos)
{
	/* 1. Check the user requested count value with the MAX_SIZE */
	if((*f_pos + count) > MAX_SIZE)
		count = MAX_SIZE - *f_pos;

	/* 2. Copy count number of bytes from device memory buffer to user space buffer */
	if(copy_to_user(buff,&dev_buf[*f_pos],count))
	{
		return -EFAULT;
	}
	
	/* 3. Update the f_pos */
	*f_pos += count;
	
	printk(KERN_INFO"%s: Read bytes: %ld\n",__func__,count);
	printk(KERN_INFO"%s: Read buffer: %s\n",__func__,dev_buf);
	printk(KERN_INFO"%s: User buffer: %s\n",__func__,buff);
	/* 4. Return number of bytes successfully read */
	return count;
}

ssize_t char_write(struct file* filp, const char __user* buff, size_t count, loff_t* f_pos)
{
	/* 1. Check the user requested count value with the MAX_SIZE */
	if((*f_pos + count) > MAX_SIZE)
		count = MAX_SIZE - *f_pos;
	
	/* If count is 0, then return ENOMEM */
	if(!count)
		return -ENOMEM;

	/* 2. Copy count number of bytes from userspace buffer to device memory buffer */
	if(copy_from_user(&dev_buf[*f_pos],buff,count))
	{
		return -EFAULT;
	}
	
	/* 3. Update the f_pos */
	*f_pos += count;

	printk(KERN_INFO"%s: Written bytes: %ld\n",__func__,count);
	printk(KERN_INFO"%s: buffer: %s\n",__func__,dev_buf);
	/* 4. Return number of bytes successfully written */
	return count;
}

int char_open(struct inode* inode, struct file* filp)
{
	printk(KERN_INFO"%s: Device file opened\n",__func__);
	return 0;
}

int char_release(struct inode* inode, struct file* filp)
{
	printk(KERN_INFO"%s: Device file closed\n",__func__);
	return 0;
}

/* file_operations structure */
struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = char_open,
	.release = char_release,
	.write = char_write,
	.read = char_read,
	.llseek = char_lseek,
};

struct class* char_class;
struct device* char_device;

static int __init char_driver_init(void)
{
	/* Dynamically allocate a device number */
	if((alloc_chrdev_region(&dev_num,0,1,"pseudo_char_dev"))<0)
	{
		printk(KERN_INFO"%s: Cannot allocate Major number\n",__func__);
		return -1;
	}
	printk(KERN_INFO"%s: MAJOR = %d, MINOR = %d\n",__func__,MAJOR(dev_num),MINOR(dev_num));
	
	/* Initialize the cdev structure with fops */
	cdev_init(&char_cdev,&fops);
	char_cdev.owner = THIS_MODULE;

	/* Register cdev structre with VFS */
	if((cdev_add(&char_cdev,dev_num,1))<0)
	{
		printk(KERN_INFO"%s: Cannot register cdev structure with VFS\n",__func__);
		goto unreg_chrdev;
	}
	
	/* create device class under /sys/class/ */
	if(IS_ERR(char_class = class_create(THIS_MODULE,"char_class")))
	{
		printk(KERN_INFO"%s: Cannot create device class under /sys/class\n",__func__);
		goto cdev_del;
	}
	
	/* device file creation */
	if(IS_ERR(device_create(char_class,NULL,dev_num,NULL,"char_dev")))
	{
		printk(KERN_INFO"%s: device file creation failed\n",__func__);
		goto destroy_class;
	}
	
	printk(KERN_INFO"%s: Module init was successful",__func__);
	return 0;

destroy_class:
	class_destroy(char_class);
cdev_del:
	cdev_del(&char_cdev);
unreg_chrdev:
	unregister_chrdev_region(dev_num,1);
	return -1;
}

static void __exit char_driver_cleanup(void)
{
	device_destroy(char_class,dev_num);
	class_destroy(char_class);
	cdev_del(&char_cdev);
	unregister_chrdev_region(dev_num,1);
	printk(KERN_INFO"%s: Module unloaded\n",__func__);
}

module_init(char_driver_init);
module_exit(char_driver_cleanup);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pseudo Character Driver");
