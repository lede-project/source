/*
Copyright (c) 2013 Martin Knoll
Copyright (C) 2015 GuoGuo<gch981213@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>

int G_CLK,G_DIN,G_DC,G_CE,G_RST;

int gpio_exported=0;

const char this_driver_name[] = "pcd8544_gpio";

static struct proc_dir_entry *proc_getgpio = NULL;
static struct proc_dir_entry *proc_getstr = NULL;

struct mutex cmd_lock;

void pcd8544_write_byte(const unsigned char dat_cnst,bool cmd)
{
	int i;
	char data=dat_cnst;

	mutex_lock(&cmd_lock);
	gpio_set_value(G_CE, 0);
	gpio_set_value(G_DC, cmd);
	for(i=0;i<8;i++)
	{
		gpio_set_value(G_DIN, (data&0x80)?1:0);
		gpio_set_value(G_CLK, 0);
		data<<=1;
		gpio_set_value(G_CLK, 1);
	}
	gpio_set_value(G_CE, 1);
	mutex_unlock(&cmd_lock);
}
//GPIO PROCFG
static int proc_pcd8544_gpio_read(struct seq_file *seq, void *v)
{
	seq_printf(seq,"A GPIO Based PCD8544 Driver.\n");
	seq_printf(seq,"Copyright (C) 2015 GuoGuo<gch981213@gmail.com>\n");
	seq_printf(seq,"Please write the GPIOs of CLK DIN DC CE RST to this file in order.\n");
	return 0;
}

static ssize_t proc_pcd8544_gpio_write(struct file *file, const char __user *buffer, 
		      size_t count, loff_t *data)
{
	char buf[17];
	int len;

	mutex_lock(&cmd_lock);
	len=count>sizeof(buf)?sizeof(buf):count;
	if (copy_from_user(buf, buffer, len)){
		mutex_unlock(&cmd_lock);
		return -EFAULT;
	}
	if(sscanf(buf,"%d%d%d%d%d",&G_CLK,&G_DIN,&G_DC,&G_CE,&G_RST)==5)
	{
		if(gpio_exported)
		{
			gpio_free(G_CLK);
			gpio_free(G_DIN);
			gpio_free(G_DC);
			gpio_free(G_CE);
			gpio_free(G_RST);
		} else {
			gpio_exported=1;
		}
		gpio_request(G_CLK, "PCD8544 SPI Clock");
		gpio_direction_output(G_CLK, 0);
		gpio_request(G_DIN, "PCD8544 SPI Data in");
		gpio_direction_output(G_DIN, 0);
		gpio_request(G_DC, "PCD8544 Data/Command");
		gpio_direction_output(G_DC, 0);
		gpio_request(G_CE, "PCD8544 SPI CE");
		gpio_direction_output(G_CE, 0);
		gpio_request(G_RST, "PCD8544 Reset Pin");
		gpio_direction_output(G_RST, 0);
		gpio_set_value(G_RST, 0);
		udelay(20);
		gpio_set_value(G_RST, 1);
		gpio_set_value(G_DC, 0);
		gpio_set_value(G_CLK, 1);
		gpio_set_value(G_CE, 1);
	} else {
		printk("Please give the GPIOs in correct format.\n");
	}
	mutex_unlock(&cmd_lock);
	return count;
}

static int proc_pcd8544_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_pcd8544_gpio_read, NULL);
}

static const struct file_operations proc_pcd8544_gpio_operations = {
	.open		= proc_pcd8544_gpio_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write 		= proc_pcd8544_gpio_write,
	.release	= single_release,
};
//Command PROCFS
static int proc_pcd8544_str_read(struct seq_file *seq, void *v)
{
	seq_printf(seq,"A GPIO Based PCD8544 Driver.\n");
	seq_printf(seq,"Copyright (C) 2015 GuoGuo<gch981213@gmail.com>\n");
	seq_printf(seq,"Please write command to this file in this format:value dc\nExample:0x00 0\n");
	return 0;
}

static ssize_t proc_pcd8544_str_write(struct file *file, const char __user *buffer, 
		      size_t count, loff_t *data)
{
	char buf[7];
	int len;
	int cmd,dcval;

	if(!gpio_exported)
	{
		printk("pcd8544-gpio:Please specify gpios first!\n");
		return count;
	}
	len=count>sizeof(buf)?sizeof(buf):count;
	if (copy_from_user(buf, buffer, len)){
		return -EFAULT;
	}
	sscanf(buf,"0x%X %d",&cmd,&dcval);
	pcd8544_write_byte((unsigned char)cmd,dcval);
	return len;
}

static int proc_pcd8544_str_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_pcd8544_str_read, NULL);
}

static const struct file_operations proc_pcd8544_str_operations = {
	.open		= proc_pcd8544_str_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write 		= proc_pcd8544_str_write,
	.release	= single_release,
};
//procfs end
static int __init pcd8544_init(void)
{
	mutex_init(&cmd_lock);
	proc_getgpio = proc_create("pcd8544_gpio", 0, NULL, &proc_pcd8544_gpio_operations);
	if(proc_getgpio == NULL){
		printk("Failed to create proc entry.\n");
		return -1;
	}
	proc_getstr = proc_create("pcd8544_cmd", 0, NULL, &proc_pcd8544_str_operations);
	if(proc_getstr == NULL){
		printk("Failed to create proc entry.\n");
		return -1;
	}
	printk("PCD8544 LCD Controller driver.Copyright (C) 2015 GuoGuo<gch981213@gmail.com>\n");
	return 0;
}

static void __exit pcd8544_exit(void)
{
	remove_proc_entry("pcd8544_cmd",NULL);
	remove_proc_entry("pcd8544_gpio",NULL);
	if(gpio_exported)
	{
		gpio_free(G_CLK);
		gpio_free(G_DIN);
		gpio_free(G_DC);
		gpio_free(G_CE);
		gpio_free(G_RST);
	}
	printk("PCD8544 LCD Controller driver removed.\n");
}

module_exit(pcd8544_exit);
module_init(pcd8544_init);

MODULE_AUTHOR("GuoGuo");
MODULE_DESCRIPTION("GPIO-based PCD8544 driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");

