#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h> 
#include <asm/uaccess.h> 
asmlinkage extern long (*sysptr)(char *arg);

asmlinkage long xcrypt(char *arg)
{
	/* dummy syscall: returns 0 for non null, -EINVAL for NULL */ 
	
        struct file *filp;
       
        mm_segment_t oldfs;
       
        int err = 0;

        int ret;
        
        char buf[128];

        oldfs = get_fs();
     
        set_fs(get_ds());
      
        filp = filp_open(arg, O_RDONLY, 0);
   	
        //set_fs(oldfs);
   	
        printk("FIlePath------> %s", arg);

        if(IS_ERR(filp)) {
       
	  	 err = PTR_ERR(filp);
        	 printk("Error in file open %d",err);
        }

         
        ret = vfs_read(filp, buf, 128, &filp->f_pos);

        set_fs(oldfs);  

        printk("\nBuffer ---->%s", buf);
      
        printk("\nAfter file read");
 	
        filp_close(filp,NULL);
   
        return 0;
}

struct file* file_open(const char* path, int flags, int rights) {
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

static int __init init_sys_xcrypt(void)
{
	printk("installed new sys_xcrypt module\n");
	if (sysptr == NULL)
		sysptr = xcrypt;
	return 0;
}
static void  __exit exit_sys_xcrypt(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_xcrypt module\n");
}
module_init(init_sys_xcrypt);
module_exit(exit_sys_xcrypt);
MODULE_LICENSE("GPL");
