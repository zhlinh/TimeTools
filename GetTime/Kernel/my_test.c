/**
 * Created Time: 2015-11-27
 * Last_modify:  2015-12-02
 * platform VMWare Linux
 * VMware virtual machines provide a set of pseudoperformance counters that
 * software running in the virtual machine can read with the rdpmc instruction
 * to obtain fine-grained time.
 * To enable this feature, use the following configuration file setting:
 *     monitor_control.pseudo_perfctr = TRUE
 * rdpmc 0x10000 : Physical host TSC
 * rdpmc 0x10001 : Elapsed real time in ns
 * rdpmc 0x10002 : Elapsed apparent time in ns
 * Kernal Timer - hrtimer Test
 * USAGE:
 * $ make
 * $ sudo insmod hrtimer_test.ko
 * $ dmesg | grep my_hrtimer
 * $ sudo rmmod hrtimer_test [To Remove Module]
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <linux/ioctl.h>

#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/file.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/errno.h>

#define IO_MAGIC '='
#define HOST_GET_PCIE_TIME _IOR(IO_MAGIC, 1, struct timespec)

#define  BEGIN_KMEM { mm_segment_t  old_fs  =  get_fs();  set_fs(get_ds());  
#define  END_KMEM  set_fs(old_fs); }

#define PCIE_DEV "/dev/pcietime0"

MODULE_LICENSE("GPL");

int get_time(struct file *filp, struct timespec *time)
{
   return filp->f_op->unlocked_ioctl(filp, HOST_GET_PCIE_TIME, time);
}

int init_module(void)
{
  long unsigned int ptime;
  struct file *filp;
  struct timespec timePCIE;
  int error = -ENOTTY;
  printk("my_test module installing\n");
  /**
  fd = open ("/dev/pcie", O_RDWR, 0);
  
  if (fd == -1) {
    printk ("Please check the PCIE card and try again. For Help input: sync -h \n");
    return -1;
  }
 
  ioctl (fd, HOST_GET_PCIE_TIME, &timePCIE);
  */

  filp = filp_open(PCIE_DEV, O_RDONLY, 0444);
  if (IS_ERR(filp)) { 
    printk("my_test ioctl failed\n");
    filp = NULL;  
  } 
  BEGIN_KMEM;   	
  //error = filp->f_op->unlocked_ioctl(filp, HOST_GET_PCIE_TIME, &timePCIE);
  error = get_time(filp, &timePCIE);
  if (error == -ENOIOCTLCMD){  
    error = -EINVAL;
    printk("my_test ioctl failed\n");
  }
  //ptime = timePCIE.tv_sec;
  //printk("my_test %ld\n", ptime);
  END_KMEM;
  filp_close(filp, NULL);   	
  return -1;
}

void cleanup_module( void )
{
  printk("my_test module uninstalling\n");
  return;
}
