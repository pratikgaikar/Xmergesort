#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h> 
#include <asm/uaccess.h>i
#include <linux/string.h>
asmlinkage extern long (*sysptr)(char *arg, char *arg1);
int read_file(struct file* file, char* data, int size);
int write_file(struct file* file, char* data, int size);
asmlinkage long xcrypt(char *arg, char *arg1)
{
	/* dummy syscall: returns 0 for non null, -EINVAL for NULL */ 
        struct file *filp1=NULL, *filp2=NULL, *output_filp=NULL;	
       
        struct filename *filename1=NULL,*filename2=NULL;

        int err = 0;

        int ret;

        char buf1[3], buf2[3];
    
        unsigned long long offset =0;

        int size = 3;

        mm_segment_t oldfs;

        oldfs = get_fs();

        set_fs(get_ds());

	filename1 =  getname(arg);
       	if(filename1 == NULL)
        {
               printk("\nError in fisrt argument--->");
	       goto out;      
	}
	
	filename2 = getname(arg1);
	if(filename2 == NULL)
	{
               printk("\nError in 1st argument");
               goto out;
	}
                  
             
        filp1 = filp_open(filename1->name, O_RDONLY, 0);

        printk("\nFile Path-------------> %s", filename1->name);
       
        if(IS_ERR(filp1)) {
	  	// err = PTR_ERR(filp1);
        	printk("\nError in file open %s",filename1->name);
		goto out;
        }
	
          
	filp2 = filp_open(filename2->name, O_RDONLY, 0);

        printk("\nFile Path-------------> %s", filename2->name);

        if(IS_ERR(filp2)) {
                 //err = PTR_ERR(filp2);
                printk("\nError in file open %s",filename2->name);
		goto out;
        }


        output_filp = filp_open("merge.txt", O_WRONLY|O_CREAT, 0644);
        
        if(IS_ERR(output_filp)) {

                 err = PTR_ERR(output_filp);
                 printk("\nError in file open %d",err);
        }
	
	read_file(filp1, buf1, size);
	buf1[3]='\0';
 	read_file(filp2, buf2, size);
	buf2[3]='\0';      
//while(1)
	{
         	if(strcmp(buf1,buf2)==-1)
		{
			write_file(output_filp, buf1 , size);
		}
		else
		{
			write_file(output_filp, buf2,size);
		}
	}

	printk("\nWRITING FILE 1 -----------------------------------------> %s \n", buf1);
       
        printk("\nWRITING FILE 2 -----------------------------------------> %s \n", buf2);

        set_fs(oldfs);  
 	
        filp_close(filp1,NULL);

        filp_close(filp2,NULL); 

        filp_close(output_filp,NULL); 

        out:

	 if(!filename1)
	     return -EINVAL;
         if(!filename2)
             return -EINVAL;
	 if(!filp1)
             return -EINVAL;
         if(!filp2)
             return -EINVAL;

	 return 0;
}

int read_file(struct file* file, char* data, int size) {
    int ret;

    ret = vfs_read(file, data, size, &file->f_pos);
    printk("READING FILE  -----------------------------------------> %s \n", data);

    return ret;
} 

int write_file(struct file* file, char* data, int size) {
    int ret;

    ret = vfs_write(file, data, size, &file->f_pos);
  
    return ret;
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
