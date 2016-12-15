#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h> 
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/namei.h>
#include "xmergesort.h"

asmlinkage extern long (*sysptr)(void *arg);

int read_file(struct file* file, char* data, int size, int *file_seek_position);
int write_file(struct file* file, char* data, int size);
int remove_garbage_value(char *data, int pagesize);
int check_value(char *src, char *des, int flag_i);
void write_buffer(struct file *filp, char *output_buf, int *size_of_output);
void add_to_output_buffer(char **parse, char **prev_of_parse, char **output_buf, char **buf, int *size_of_output, int *size_f,int *records_merge);
int  rename_file(struct file *temp_file, struct file *output_file);
void getprev_file_name(int file_index, char **file_prev_name);
void getnext_file_name(int file_index, char **file_next_name);
char *nl = "\n";
asmlinkage long mergesort(void *arg)
{
	int ret = 0;
	
	printk("%s",(char *)arg);	
	struct input *data= NULL;
	struct file *filp1 = NULL, *filp2 = NULL, *output_filp = NULL, *temp_filp = NULL;	
	struct filename *file_name1=NULL,*file_name2=NULL, *output_file_name=NULL;
	char *buf1 = NULL, *buf2 =NULL, *b1 = NULL, *b2 =NULL, *output_buf = NULL, *parse1 = NULL ,*parse2 =NULL, *prev_of_parse1 = NULL, *prev_of_parse2 =NULL, *file_prev_name = NULL, *file_next_name= NULL;
	int  page_size = 4096,records_merge = 0, file1_seek_position = 0,file2_seek_position = 0, len =0, size_f1=0 ,size_f2=0 , size_of_output = 0,cmp_value = 0, val = 0, parse1_set = 0, parse2_set = 0, flagt_is_set=0, file_index=0;
	struct inode *inode_file1, *inode_file2, *inode_file3;
	if(arg == NULL)
		goto free;
	else
	{
		data = (struct input *)kmalloc(sizeof(struct input),GFP_KERNEL);
		if(copy_from_user(data, arg, sizeof(struct input)))
		{
			ret = -EFAULT;
			goto free;
		}

	}

	printk("\n*********************************************************************************************");
	buf1 = (char *)kmalloc(page_size,GFP_KERNEL);
	if(buf1 == NULL)
	{
		ret = -ENOMEM;
		goto free;
	}

	buf2 = (char *)kmalloc(page_size,GFP_KERNEL);
	if(buf2 == NULL)
	{
		ret = -ENOMEM;
		goto free;
	}

	output_buf = (char *)kmalloc(page_size, GFP_KERNEL);
	if(output_buf == NULL)
	{
		ret = -ENOMEM;          
		goto free;
	}

	prev_of_parse1 =(char *)kmalloc(page_size, GFP_KERNEL);
	if(prev_of_parse1 == NULL)
	{
		ret = -ENOMEM;
		goto free;
	}

	prev_of_parse2 =(char *)kmalloc(page_size, GFP_KERNEL);
	if(prev_of_parse2 == NULL)
	{
		ret = -ENOMEM;
		goto free;
	}

	b1 = buf1;
	b2 = buf2;

	file_prev_name = (char *)kmalloc(page_size, GFP_KERNEL);
	if( file_prev_name == NULL)
        {
                ret = -ENOMEM;
                goto free;
        }

	file_next_name = (char *)kmalloc(page_size, GFP_KERNEL);
	if( file_next_name == NULL)
        {
                ret = -ENOMEM;
                goto free;
        }

	while(data->input_files[file_index]!=NULL)
	{		
		buf1 = b1;
		buf2 = b2;
		output_buf[0]='\0';
        	buf1[0]='\0';
        	buf2[0]='\0';
		size_of_output = 0;
		parse1_set = 0; parse2_set = 0;     
		records_merge = 0; 
		file1_seek_position = 0;
		file2_seek_position = 0;
		
		file_name1 = getname(data->input_files[file_index]);
		if(!file_name1)
		{
			printk("\nError in filename");
			ret = -ENOENT;
			goto free;      
		}
	
		// Opening file using filp_open.
		filp1 = filp_open(file_name1->name, O_RDONLY, 0);
		if(IS_ERR(filp1)) 
		{
			printk("\nError in file open %s",file_name1->name);
			goto free;
		}

		if(file_index!=0)
		{
			// get previous temporary file name.
			getprev_file_name(file_index-1, &file_prev_name);
			filp2 = filp_open(file_prev_name, O_RDONLY, 0);
			if(IS_ERR(filp2)) {
				printk("\nError in file open %s",file_name2->name);
				goto free;
			}
		}
		else
			size_f2=0;
		//generate next file name.
		getnext_file_name(file_index,&file_next_name);
		
		//creating tempory output file.
		temp_filp = filp_open(file_next_name, O_WRONLY|O_CREAT, 0644);
		if(IS_ERR(temp_filp)) {
			ret = PTR_ERR(temp_filp);
			goto free;
		}

		//get the totsl size of file using inode of a file.
		size_f1 = i_size_read(file_inode(filp1));
		printk("\nFile1 size -->%d ", size_f1);
		if(size_f1 < 0)
		{
			goto free;
		}

		if(filp2!= NULL)
		{
			size_f2 = i_size_read(file_inode(filp2));
			if(size_f2 < 0)
			{
				goto free;
			}
		}

		printk("\nFile2 size -->%d ", size_f2);

		inode_file1 = file_inode(filp1);
		  inode_file2 = file_inode(filp2);
		  inode_file3 = file_inode(output_filp);

		  if(inode_file1->i_ino == inode_file2->i_ino || inode_file2->i_ino == inode_file3->i_ino || inode_file1->i_ino == inode_file3->i_ino ) {
		  if(inode_file1->i_sb->s_dev == inode_file2->i_sb->s_dev || inode_file1->i_sb->s_dev == inode_file3->i_sb->s_dev || inode_file2->i_sb->s_dev == inode_file3->i_sb->s_dev  )
		  {
		  printk(KERN_INFO "Same file passed mounted on same file system.");
		  ret=-EINVAL;
		  goto free;
		  }
		  }

		while(size_f2>0 && size_f1>0)
		{
			if(parse1 == NULL)
			{
				buf1 = b1;
				read_file(filp1, buf1, page_size, &file1_seek_position);
				//printk("\nbuf1 %s", buf1);
				parse1 = strsep(&buf1, "\n");	
			}

			if(parse2 == NULL)
			{
				buf2 = b2;
				read_file(filp2, buf2,  page_size, &file2_seek_position);
				//printk("\nbuf2 %s", buf2);
				parse2 = strsep(&buf2, "\n");
			}

			while(parse1!=NULL && parse2!=NULL)
			{			
				cmp_value = check_value(parse1,parse2,data->flag & 0x4);
				switch(cmp_value)
				{
					case -1: 
						if(parse1_set != 0)
						{
							val = check_value(prev_of_parse1, parse1,data->flag & 0x4);
							switch(val)
							{
								case 1: 
									if((data->flag & 0x10))
									{	
										flagt_is_set=1;
										goto free;
									}
									else
									{
										size_f1 = size_f1 - (strlen(parse1)+1);
										parse1 = strsep(&buf1,"\n");
									}
									break;

								case -1:
									if(strlen(parse1) < ( page_size - size_of_output) )
										add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output, &size_f1, &records_merge);
									else
										write_buffer(temp_filp , output_buf, &size_of_output);
									break;
								case 0:
									if((data->flag & 0x01))
									{
										size_f1 = size_f1 - (strlen(parse1)+1);
										parse1 = strsep(&buf1,"\n");
									}
									else
									{
										if(strlen(parse1)< ( page_size- size_of_output) )
											add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output, &size_f1, &records_merge);
										else
											write_buffer(temp_filp , output_buf, &size_of_output);
									}
									break;
							}
						}
						else
						{	parse1_set = 1;
							if(strlen(parse1) < ( page_size - size_of_output) )
								add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf,&buf1, &size_of_output, &size_f1, &records_merge);
							else
								write_buffer(temp_filp,output_buf, &size_of_output);
						}
						break;

					case 1: 
						if(parse2_set !=0)
						{
							val = check_value(prev_of_parse2, parse2, data->flag & 0x4);
							switch(val)
							{
								case 1:
									if((data->flag & 0x10))
									{
										flagt_is_set=1;
										goto free;
									}
									else
									{
										size_f2 = size_f2 - (strlen(parse2)+1);
										parse2 = strsep(&buf2,"\n");
									}
									break;

								case -1:
									if(strlen(parse2) < (page_size - size_of_output) )
										add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);
									else
										write_buffer(temp_filp , output_buf, &size_of_output);
									break;

								case 0: 
									if((data->flag & 0x01))
									{
										size_f2 =size_f2 - (strlen(parse2)+1);
										if(buf2 != NULL && strlen(buf2) != 0)
											parse2 = strsep(&buf2,"\n");
									}
									else
									{

										if(strlen(parse2) < (page_size - size_of_output) )
											add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);
										else
											write_buffer(temp_filp , output_buf, &size_of_output);

									}
									break;
							}
						}
						else
						{
							parse2_set=1;
							if(strlen(parse2) < (page_size - size_of_output) )
								add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);
							else
								write_buffer(temp_filp , output_buf, &size_of_output);
						}
						break;
					case 0: 	
						if((data->flag & 0x10))
						{
							if(parse1_set!=0)
							{
								if(check_value(prev_of_parse1, parse1, data->flag & 0x04)==1)
								{
									flagt_is_set=1;
									goto free;
								}
							}
							if(parse2_set!=0)
							{
								if(check_value(prev_of_parse2, parse2 , data->flag & 0x04)==1)
								{
									flagt_is_set=1;
									goto free;
								}
							}						
						}
						len = strlen(parse1);
						if((data->flag & 0x01)) 
						{
							int flag_1 =0;
							int flag_2 =0;	
							if(parse1_set!=0)
							{
								if(check_value(prev_of_parse1, parse1, data->flag & 0x04)==0)
									flag_1=1;
							}
							if(parse2_set!=0)
							{
								if(check_value(prev_of_parse2, parse2, data->flag & 0x04)==0)
									flag_2=1;
							}
							if(flag_1 || flag_2)
							{
								size_f1 = size_f1 - len -1;
								size_f2 = size_f2 - len -1;
								parse1 = strsep(&buf1,"\n");
								parse2 = strsep(&buf2,"\n");
							}
							else
							{
								if(len < (page_size - size_of_output -1) )
								{
									parse1_set = 1;
									parse2_set = 1;
									add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output,&size_f1, &records_merge);
									size_f2 = size_f2 - ((len)+1);
									strcpy(prev_of_parse2,parse2);
									parse2 = strsep(&buf2,"\n");
								}
								else
									write_buffer(temp_filp , output_buf, &size_of_output);
							}
						}
						else
						{
							if((len+len) < (page_size - size_of_output) )
							{
								parse1_set = 1;
								parse2_set = 1;
								add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output,&size_f1, &records_merge);
								add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);
							}
							else
								write_buffer(temp_filp , output_buf, &size_of_output);
						}

						break;
				}
				if(parse1 == NULL || strlen(parse1) == 0 ){
					if(buf1 != NULL && strlen(buf1) > 0)
						parse1 = nl;	
					else
						parse1 = NULL;
				}
				if( (parse2 == NULL || strlen(parse2) == 0 )){
					if( buf2 != NULL && strlen(buf2) > 0)
						parse2 = nl;
					else
						parse2 = NULL;
				}
			}
		}
		while(size_f1 > 0 )
		{
			if(parse1 == NULL)
			{
				buf1 = b1;
				read_file(filp1, buf1, page_size, &file1_seek_position);
				parse1 = strsep(&buf1, "\n");        
			}

			while(parse1!=NULL)
			{
				if(parse1_set != 0)
				{
					val = check_value(prev_of_parse1, parse1, data->flag & 0x4);
					switch(val)
					{
						case 1:
							if((data->flag & 0x10))
							{
								flagt_is_set=1;
								goto free;
							}
							else
							{
								size_f1 = size_f1 - (strlen(parse1)+1);
								parse1 = strsep(&buf1,"\n");
							}
							break;
						case -1:
							if(strlen(parse1) < (page_size - size_of_output) )
								add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output, &size_f1, &records_merge);

							else
								write_buffer(temp_filp , output_buf, &size_of_output);
							break;
						case 0:
							if((data->flag & 0x01))
							{
								size_f1 = size_f1 - (strlen(parse1)+1);
								parse1 = strsep(&buf1,"\n");
							}
							else
							{
								if(strlen(parse1) < (page_size - size_of_output) )
									add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output, &size_f1, &records_merge);
								else
									write_buffer(temp_filp , output_buf, &size_of_output);
							}
							break;
					}
				}
				else
				{
					parse1_set = 1;
					if(strlen(parse1) < (page_size - size_of_output) )
						add_to_output_buffer(&parse1, &prev_of_parse1, &output_buf, &buf1, &size_of_output, &size_f1, &records_merge);
					else
						write_buffer(temp_filp , output_buf, &size_of_output);

				}

				if(parse1 == NULL || strlen(parse1) == 0 ){
					if(buf1 != NULL && strlen(buf1) > 0)
						parse1 = nl;
					else

						parse1 = NULL;
				}
			}

		}
		while(size_f2 > 0 )
		{
			if(parse2 == NULL)
			{
				buf2 = b2;
				read_file(filp2, buf2, page_size, &file2_seek_position);
				parse2 = strsep(&buf2, "\n");
			}
			while(parse2!=NULL)
			{
				if(parse2_set!= 0)
				{
					val = check_value(prev_of_parse2, parse2, data->flag & 0x4);
					switch(val)
					{
						case 1:// printk("\nUnsorted data in file 2 flag t is not set.........");
							if((data->flag & 0x10))
							{
								flagt_is_set=1;
								goto free;
							}
							else
							{
								size_f2 =size_f2 - (strlen(parse2)+1);
								parse2 = strsep(&buf2,"\n");
							}
							break;

						case -1:if(strlen(parse2) < (page_size - size_of_output) )

								add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);            
							else
								write_buffer(temp_filp , output_buf, &size_of_output);

							break;
						case 0:
							if((data->flag & 0x01))
							{
								size_f2 =size_f2 - (strlen(parse2)+1);
								parse2 = strsep(&buf2,"\n");
							}
							else
							{
								if(strlen(parse2) < (page_size - size_of_output) )
									add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);
								else
									write_buffer(temp_filp , output_buf, &size_of_output);

							}
							break;
					}

				}
				else
				{
					parse2_set=1;
					if(strlen(parse2) < (page_size - size_of_output) )
						add_to_output_buffer(&parse2, &prev_of_parse2, &output_buf, &buf2, &size_of_output,&size_f2, &records_merge);
					else
						write_buffer(temp_filp , output_buf, &size_of_output);					

				}

				if( (parse2 == NULL || strlen(parse2) == 0 )){
					if( buf2 != NULL && strlen(buf2) > 0)
						parse2 = nl;
					else
						parse2 = NULL;
				}

			}

		}

		if(output_buf != NULL && (strlen(output_buf))!= 0)
		{
			//printk("\nOutput Buf %s", output_buf);
			//printk("\nWriting to the file %s", file_next_name);
			write_file(temp_filp, output_buf, size_of_output);
		}
		if(filp2)
			vfs_unlink(d_inode(file_dentry(filp2)->d_parent), file_dentry(filp2), NULL);

		if(filp1)
			filp_close(filp1,NULL);

		if(filp2)
			filp_close(filp2,NULL);

		if(temp_filp)
			filp_close(temp_filp,NULL);

		file_index++;

	}
	
	// to check output file name.
	output_file_name = getname(data->output_file_name);

        if(!output_file_name)
        {
                printk("\nError in output filename");
                ret = -ENOENT;
                goto free;
        }

	//to open output file.
        output_filp = filp_open(output_file_name->name, O_WRONLY|O_CREAT, 0644);
        if(IS_ERR(output_filp)) {
                  ret = PTR_ERR(output_filp);
                  printk("\nError in file open %s",output_file_name->name);
                  goto free;
        }


	ret = rename_file(temp_filp,output_filp);


free:
	if(flagt_is_set)
	{
		vfs_unlink(d_inode(file_dentry(temp_filp)->d_parent), file_dentry(temp_filp), NULL);
		vfs_unlink(d_inode(file_dentry(output_filp)->d_parent), file_dentry(output_filp), NULL);	
	}

	if(file_name1)
		putname(file_name1);
	if(file_name2)
	  putname(file_name2);
	
	if(data)
		kfree(data);	
	buf1=b1;
	if(buf1)
		kfree(buf1);
	buf2=b2;
	if(buf2)
		kfree(buf2);
	if(output_buf)
		kfree(output_buf);
	if(prev_of_parse1)
		kfree(prev_of_parse1);
	if(prev_of_parse2)
		kfree(prev_of_parse2);

	if(filp1)
		filp_close(filp1,NULL);

	if(filp2)
		filp_close(filp2,NULL);

	if(temp_filp)
		filp_close(temp_filp,NULL);

	if(data->flag & 0x20 && !flagt_is_set)
		return records_merge;

	return ret;
}


int read_file(struct file* file, char* data, int size,int *file_seek_position ) {
	int ret;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_read(file, data, size , &file->f_pos);
	set_fs(oldfs);
	*file_seek_position += remove_garbage_value(data, size);
	file->f_pos = *file_seek_position;
	return ret;
} 

int remove_garbage_value(char *data, int pagesize)
{
	int i= strlen(data)-1;
	if(data[i]!='\n' || data[i]!='\0')
	{
		while(i>0)
		{
			if(data[i]=='\n' || data[i] == '\0')
				break;
			i--;
		}
	}
	if(i < pagesize && data[i]!='\0')
		data[++i]='\0';
	return strlen(data);
}

void add_to_output_buffer(char **parse, char **prev_of_parse, char **output_buf, char **buf, int *size_of_output, int *size_f, int *mergs_records)
{
	(*mergs_records)++;
	strcat(*output_buf,*parse);
	int l = 0;
	if(*parse != nl){
		strcat(*output_buf,"\n");
		l = 1; 
	}
	*size_of_output = strlen(*parse) + *size_of_output + l;
	*size_f =*size_f - (strlen(*parse)+l);
	*prev_of_parse[0]='\0';
	strcpy(*prev_of_parse,*parse);
	*parse = strsep(buf, "\n");
}

void write_buffer(struct file  *filp, char *output_buf, int *size_of_output)
{
	write_file(filp , output_buf,*size_of_output);
	*size_of_output = 0;
	output_buf[0]='\0';	
}

int check_value(char *src, char *des, int flag_i)
{
	int ret = 0;
	if(flag_i)
	{
		ret = strcasecmp(src,des);
		if(ret<0)
			return -1;
		if(ret>0)
			return 1;
	}
	else
	{
		ret = strcmp(src,des);
		if(ret<0)
			return -1;
		if(ret>0)
			return 1;
	}
	return ret;
}

void getprev_file_name(int file_index, char **file_prev_name)
{
	char *str = (char *)kmalloc(3,GFP_KERNEL);;
	sprintf(str, "%d", file_index);
	strcpy(*file_prev_name,"output_test_");
	strcat(*file_prev_name,str);
	kfree(str);
}

void getnext_file_name(int file_index, char **file_next_name)
{
	char *str = (char *)kmalloc(3,GFP_KERNEL);;
	sprintf(str, "%d", file_index);
	strcpy(*file_next_name,"output_test_");
	strcat(*file_next_name,str);
	kfree(str);
}

int rename_file(struct file *temp_file, struct file *output_file)
{
	int ret=vfs_rename(d_inode(file_dentry(temp_file)->d_parent),file_dentry(temp_file), d_inode(file_dentry(output_file)->d_parent),file_dentry(output_file), NULL, 0);
	vfs_unlink(d_inode(file_dentry(output_file)->d_parent), file_dentry(output_file), NULL);
	return ret;
}


int write_file(struct file* file, char* data, int size)
{
	int ret;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_write(file, data, strlen(data), &file->f_pos);
	set_fs(oldfs);
	return ret;
}

static int __init init_sys_mergesort(void)
{
	printk("Installed new sys_mergesort module\n");
	if (sysptr == NULL)
		sysptr = mergesort;
	return 0;
}

static void  __exit exit_sys_mergesort(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk("Removed sys_mergesort module\n");
}
module_init(init_sys_mergesort);
module_exit(exit_sys_mergesort);
MODULE_LICENSE("GPL");
