#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <asm/irq.h>
#include <linux/spinlock.h>
//#include <asm-generic/uaccess.h>
//The different version linux kernel has different include head file name
//#include <asm/uaccess.h> 
//Linux version 2.6.27.19-5-default (geeko@buildhost) (gcc version 4.3.2 [gcc-4_3-branch revision 141291] (SUSE Linux) ) 
//#1 SMP 2009-02-28 04:40:21 +0100
// uaccess.h  at Linux version 2.6.27.19-5 is #include <asm/uaccess.h> 

#include <linux/syscalls.h>
//#include <asm-generic/div64.h>
//#include<math64.h>

//#if !defined (copy_to_user)
//#include <asm-generic/uaccess.h>
//#endif

#define	DEBUG
#ifdef DEBUG 
  #define DPRINTK(fmt, args...)   printk(KERN_ERR "%s: " fmt, __FUNCTION__ , ##args)
#else
  #define DPRINTK(fmt, args...)
#endif

#define	PCI_DRIVER_NAME	"PTP PCIExpress"
#define IMMRBAR_SIZE	0x00100000
#define MPC83xx_DMA_OFFSET      (0x08000)
#define NSEC            ((u32)1000000000)
#define	TMR_CNT_L	0x24e1c
#define	TMR_CNT_H	0x24e18
#define	MAGIC		'='
#define	GET_NS_TIME	_IOR (MAGIC,1,struct timespec)
#define GPIO_DATA_ADDRESS_OFFSET 0xc08
#define TAI_OFFSET 35000000000    //35秒
#define IO_MAGIC '='
#define HOST_GET_PCIE_TIME _IOR(IO_MAGIC,1, struct timespec )
#define HOST_GET_LOCAL_SYSTEM_TIME _IOR(IO_MAGIC,2, struct timespec )
#define	PEX_OMBCR	 0x9b20    //for clear the ready 
#define	PEX_OMB_DATA 0x9ba4    //close pcie host the interrupt.
#define PCIE_HOST_INTTRUPT_OMB_BIT         0x00000400
#define HOST_GET_OFFSET _IOR(IO_MAGIC,4, s64 )

struct base_addr_reg 
{
        uint32_t start;
        uint32_t end;
        uint32_t len;
        uint32_t flags;
};

#define MU_OM0I_MASK 0x00000001
#define MU_IM0I_MASK 0x00000001
#define MU_IM0I_CLR  0x00000001
#define MU_OM0I_CLR  0x00000001

#define HOST_NAME "pcie_host_device"
#define HOST_VERSION		"1.0"

#define SHARE_MEM_SIZE 4096
#define TX_BUF_SIZE 2048
#define RX_BUF_SIZE (SHARE_MEM_SIZE-TX_BUF_SIZE)

#define	PING_PANG_SIZE	2
#define ADJ_FREQ_MAX  512000
#define	COUNT_AMOUNT	4
#define	MAX_PTP_PCI	8
#define	PTP_PCI_MAJOR	87

#ifdef SYNC_FROM_SYSTEM_TIME
#define TMR_PERIOD 5
#endif


struct timespec get_sys_time ={0};

struct host_priv
{
	spinlock_t lock;
	unsigned long m_immrbar;
	unsigned long shm_mem;
	unsigned long  m_immrbar_phy_addr;
	int minor;
	wait_queue_head_t tsev_wq;
	
};

struct class *ptp_pci_class;

#define PCI_VENDOR_ID_FREESCALE 0x1957
#define PCI_DEVICE_ID_MPC8308	0xC006
static struct pci_device_id pci_cdev_id_table[] =
{
	{
		.vendor = PCI_VENDOR_ID_FREESCALE,
		.device = PCI_DEVICE_ID_MPC8308,
		.subvendor = PCI_ANY_ID,
		.subdevice = PCI_ANY_ID,
	},
        {0,},
};

MODULE_DEVICE_TABLE(pci, pci_cdev_id_table);

static struct host_priv *priv[MAX_PTP_PCI];

static ssize_t host_read(struct file *filp, char __user *buf, size_t size,
                loff_t *f_pos)
{
	if (size != sizeof (s64))
	{
		return -EINVAL;
	}
	
	if (copy_to_user (buf,&get_sys_time,sizeof(struct timespec)))
	{
		return -EFAULT;
	}
	return size;
}


/* character device operations */
//static int host_ioctl (struct file *filp, unsigned int cmd, char __user *buf, size_t size,loff_t *f_pos)
/* character device operations */
//static long host_ioctl(struct inode *node, struct file *filp,
static long host_ioctl(struct file *filp,
		      unsigned int cmd, unsigned long  arg)
{
	struct host_priv *priv;
	unsigned long flag;
	struct timespec  pcie_time;
	unsigned long long  tmr_cnt;
	int err = 0;
    u32 nano_value = 0;
	
	priv  = (struct host_priv *)filp->private_data;
	switch (cmd) 
	{
	    case HOST_GET_PCIE_TIME: 
		{
			local_irq_save (flag);
		    tmr_cnt =*(u32 *)(priv->m_immrbar + TMR_CNT_L);
			local_irq_restore (flag);
			tmr_cnt = be32_to_cpu (tmr_cnt);
	     	tmr_cnt |= ((u64)be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_H))) << 32;
	        if (tmr_cnt)
	        {
	            pcie_time.tv_sec = div_u64_rem(tmr_cnt,1000000000,&nano_value) - 35 ;
	            pcie_time.tv_nsec = nano_value;
	            
	            if (copy_to_user ((void __user *)arg, &pcie_time , sizeof (pcie_time)))
	            {
	                err = -EFAULT;
	            }
	        }
	        break;
	     }
	     
	    case HOST_GET_LOCAL_SYSTEM_TIME: 
	    {
		struct timespec sys_time={0};
		getnstimeofday (&sys_time);
		if (copy_to_user ((void __user *)arg,&sys_time,sizeof(sys_time)))
		{
			err = -EFAULT;
		}
		//printk(KERN_INFO "get sys time %ld.%ld \n",sys_time.tv_sec,sys_time.tv_nsec);
	        break;
	     }
		case HOST_GET_OFFSET:
		{
			u64 tmr_cnt;
			u64 cur_cnt;
			s64 offset;
			struct host_priv *priv=NULL;
			unsigned long flag = 0;
			struct timespec cur_time={0};
			priv  = (struct host_priv *)filp->private_data;
			local_irq_save (flag);
			getnstimeofday (&cur_time);
			tmr_cnt =*(u32 *)(priv->m_immrbar + TMR_CNT_L);
			local_irq_restore (flag);
			tmr_cnt = be32_to_cpu (tmr_cnt);
			tmr_cnt |= ((u64)be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_H))) << 32;
			tmr_cnt -=TAI_OFFSET;
			cur_cnt = (u64)cur_time.tv_sec * 1000000000 + cur_time.tv_nsec;
			if (tmr_cnt > cur_cnt)
			{
				offset = -(tmr_cnt - cur_cnt);
			}
			else
			{
				offset = cur_cnt - tmr_cnt;
			}
			
			if (copy_to_user ((void __user *)arg,&offset,sizeof(offset)))
			{
				err = -EFAULT;
			}
			break; 
		}
	    default:
		{
		    err = -ENOTTY;
		    break;
	    }
	}
	return err;
}

ssize_t host_write(struct file *filp, const char __user *buf, size_t size,
                loff_t *f_pos)
{
	return 0;
}

static loff_t host_llseek(struct file *filp, loff_t off, int whence)
{
	return 0;
}


static  int host_open (struct inode *inodep, struct file *filp)
{
  	int minor = iminor( inodep );
	if (priv[minor] == NULL)
	{
		printk (KERN_ERR "keywie priv[minor] ==NULL ---host_open---Error in 1\n");
		return -ENODEV;
	}
	filp->private_data = priv[minor];
	return 0;
}

static int host_release (struct inode *inodep, struct file *filp)
{
	return 0;
}


static int host_mmap( struct file * filp, struct vm_area_struct *vma )
{
  unsigned long addr;
  struct host_priv *priv = filp->private_data; 
  int rc;

  addr = priv->m_immrbar_phy_addr;

  if ( ( ( vma->vm_end - vma->vm_start ) != 0x100000 ) )
  {
  	printk (KERN_ERR "keywie vma mapping size !=0x100000---host_mmap---Error in 2\n");
    //printk( KERN_ERR "%s: vm_end (0x%08lX) - vm_start (0x%08lX) doesn't match PAGE_SIZE (0x%08lX)\n",
    //       PCI_DRIVER_NAME, vma->vm_end, vma->vm_start, (unsigned long)0x100000);
    rc = -EINVAL;
    goto out;
  }

   // printk ( KERN_DEBUG "%s: vm_end (0x%08lX) - vm_start (0x%08lX) == PAGE_SIZE (0x%08lX), pg_off: 0x%08lX",
   //              PCI_DRIVER_NAME, vma->vm_end, vma->vm_start, (unsigned long) PAGE_SIZE, vma->vm_pgoff );
  vma->vm_flags |= VM_IO;
  rc = io_remap_pfn_range( vma, vma->vm_start, addr >> PAGE_SHIFT, 0x100000, vma->vm_page_prot );
  if ( rc )
  {
  	printk (KERN_ERR "keywie io_remap_pfn_range failed---host_mmap---Error in  3\n");
    rc = -EAGAIN;
    goto out;
  }
  rc = 0;
out:
  return rc;

}

struct file_operations host_fops = {
	.owner =		     THIS_MODULE,
	.llseek =			 host_llseek,
	.unlocked_ioctl= 	 host_ioctl,
	.read =     		 host_read,
	.write =    		 host_write,
	.open =     		 host_open,	//Ã“Ã€Ã”Â¶Â´Ã²Â¿Âª
	.release =  		 host_release,	//Ã“Ã€Ã”Â¶Â´Ã²Â¿Âª
	.mmap = 			 host_mmap,
};

static void host_board_cleanup(struct pci_dev *pdev)
{
	struct host_priv *priv = pci_get_drvdata(pdev);
	device_destroy (ptp_pci_class,MKDEV (PTP_PCI_MAJOR,priv->minor));

	/* Unmap the space address */
	iounmap((void *)(priv->m_immrbar));
	//iounmap((void *)(priv->shm_mem));
	/* Release region */
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

//For NeoKylin
static   void host_remove(struct pci_dev *pdev)
//static __devexit void host_remove(struct pci_dev *pdev)
{
	struct host_priv *priv = pci_get_drvdata(pdev);
	
	host_board_cleanup(pdev);
	pci_set_drvdata(pdev, NULL);
	kfree(priv);
}

static int host_board_init(struct pci_dev *pdev,int minor)
{
	struct host_priv *priv = (struct host_priv *)pci_get_drvdata(pdev);
	struct base_addr_reg immrbar;
	void *mapped_immrbar;
        u32 bar1;
        u16 vendor,device;
	int retval = 0;
#ifdef SYNC_FROM_SYSTEM_TIME
	//2013.1.7 add the fuction sync the PC system time to PTP Board time .
//------------------------------------------------------
	u32 hi;
	u32 lo;
    u32 remainder; 
	u64 cur_cnt;
   struct timespec sync_sys_time;
//-------------------------------------------------------------
#endif
	
	if (pdev->vendor == PCI_VENDOR_ID_FREESCALE && 
		pdev->device == PCI_DEVICE_ID_MPC8308) {
		printk(KERN_INFO "Finded The PCIE Device Keywie PTPExpress1500\n");
	}
	else {
		printk(KERN_INFO "Can not find any PTPExpress1500 device\n");
	}
	/* Enable device */
	retval = pci_enable_device(pdev);
	if (retval) {
		printk(KERN_ERR "keywie Cannot enable device---host_mmap---Error in 16\n");
		goto out;
	}
        pci_set_master(pdev);
        pci_read_config_word(pdev,PCI_VENDOR_ID_FREESCALE,&vendor);
        pci_read_config_word(pdev,PCI_DEVICE_ID_MPC8308,&device);
        pci_read_config_dword(pdev,0x14,&bar1);
        if (bar1 == 0) {
				printk (KERN_ERR "keywie No I/O-Address for card detected---host_mmap---Error in 4\n");
                goto err_to_pci_disable_device;
        }
	/* Read information from base register */
	immrbar.start	= pci_resource_start(pdev, 1);
	immrbar.end	= pci_resource_end(pdev, 1);
	immrbar.flags	= pci_resource_flags(pdev, 1);
	immrbar.len	= pci_resource_len(pdev, 1);
	/* Make sure region #0 is memory space */
	if (!(immrbar.flags & IORESOURCE_MEM))
	{
		//printk(KERN_ERR "Region #0 is not memory space\n");
		printk (KERN_ERR "keywie Region #0 is not memory space---host_mmap---Error in 5\n");
		retval = -ENODEV;
		goto err_to_iounmap_immrbar;
         	//goto err_to_pci_disable_device;
	}
	/* Make sure region #1 size is 1MB */
	if (immrbar.len < IMMRBAR_SIZE)
	{
		//printk(KERN_ERR "%s: Invalid PCI mem region size(s)  aborting\n",pci_name(pdev));
		printk (KERN_ERR "keywie Invalid PCI mem region size---host_mmap---Error in 6");
		retval = -ENODEV;
		goto err_to_pci_disable_device;
	}
	

	/* Mark the memory region used by Methernet */
	retval = pci_request_regions(pdev, HOST_NAME);
	if (retval) 
	{
		//printk(KERN_ERR "%s: Cannot reserve region, aborting\n", pci_name(pdev));
		printk (KERN_ERR "keywie Cannot reserve region---host_mmap---Error in 7\n");
		retval = -ENODEV;	/* tell me what is the proper return error number */
		goto err_to_pci_disable_device;
		
	}

	/* Enable PCI bus mastering  */
	pci_set_master(pdev);
	/* ioremap address */
	mapped_immrbar = ioremap(immrbar.start, immrbar.len);
	if (mapped_immrbar == NULL) 
	{
		//printk(KERN_ERR "%s: Cannot remap memory, aborting\n", pci_name(pdev));
		printk (KERN_ERR "keywie Cannot remap memory---host_mmap---Error in 8\n");
		retval = -EIO;
		goto err_to_pci_release_regions;
	}

	//printk(KERN_INFO "keywie PTPExpress1500 device immrbar [BAR1 Start:%0X length:(%0X))]\n",bar1,immrbar.len);  
    //   printk(KERN_INFO
     //    "MPC8308 PCIe driver 0.10 [%0X:%0X - BAR: %0X->%0X(%0X))]\n",
     //                           vendor,device,bar1,mapped_immrbar,immrbar.len);

	/* Init private data structure */
	priv->m_immrbar = (unsigned long)mapped_immrbar;
	priv->m_immrbar_phy_addr = immrbar.start;
	/* Read information from base register */
	immrbar.start	= pci_resource_start(pdev, 1);
	immrbar.end	= pci_resource_end(pdev, 1);
	immrbar.flags	= pci_resource_flags(pdev, 1);
	immrbar.len	= pci_resource_len(pdev, 1);

	/* Make sure region #1 is memory space */
	if (!(immrbar.flags & IORESOURCE_MEM))
	{
		printk(KERN_ERR "Region #0 is not memory space\n");
		retval = -ENODEV;
		goto err_to_iounmap_immrbar;
	}
	
	mapped_immrbar = ioremap (immrbar.start,immrbar.len);
	if (mapped_immrbar == NULL) 
	{
		printk(KERN_ERR "keywie %s: Cannot remap memory, -host_mmap---Error in 9\n", pci_name(pdev));
		retval = -EIO;
		goto err_to_iounmap_immrbar;
	}
	priv->shm_mem = (unsigned long)mapped_immrbar;

	//printk (KERN_INFO "pci agent info %s\n",(char *)mapped_immrbar);
	priv->minor = minor;
	device_create (ptp_pci_class,NULL,MKDEV (PTP_PCI_MAJOR,minor),NULL,"%s%d",(char *)priv->shm_mem,minor);
#ifdef SYNC_FROM_SYSTEM_TIME
	getnstimeofday (&sync_sys_time);
	cur_cnt = (u64)sync_sys_time.tv_sec * 1000000000 + sync_sys_time.tv_nsec;
	printk (KERN_INFO "-------------------------------- %lld  \n",cur_cnt);
	div_u64_rem (cur_cnt,TMR_PERIOD,&remainder);
    printk (KERN_INFO "------------------------------------remainder %d\n",remainder);
    cur_cnt = cur_cnt - remainder;
    hi = cur_cnt >> 32;
    lo = cur_cnt & 0xffffffff;	
    hi = htonl(hi);
    lo = htonl(lo);
	printk (KERN_INFO "1--tmr_cnt_l %#x\n",be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_L)));
	printk (KERN_INFO "1--tmr_cnt_H %#x\n",be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_H)));
	*(u32 *)(priv->m_immrbar + TMR_CNT_L) = lo;
	*(u32 *)(priv->m_immrbar + TMR_CNT_H) = hi;
	printk (KERN_INFO "2--tmr_cnt_l %#x\n",be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_L)));
	printk (KERN_INFO "2--tmr_cnt_H %#x\n",be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_H)));
#endif
	init_waitqueue_head(&priv->tsev_wq);

	return 0;

err_to_iounmap_immrbar:
	iounmap ((void *)priv->m_immrbar);
err_to_pci_release_regions:
	pci_release_regions(pdev);
err_to_pci_disable_device:
	pci_disable_device(pdev);
out:
	return retval;
}

static int host_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int retval = 0;
	int i;

//DPRINTK("host_probe\n");
	for (i = 0;i < MAX_PTP_PCI;i++)
	{
		if (priv[i] == NULL)
		{
			break;
		}
	}

	if (i == MAX_PTP_PCI)
	{
		printk (KERN_ERR "keywie Too more PCIE device---host_probe---Error in 10\n");
		//printk (KERN_ERR "Too more PTP PCIEXpress here !!\n");
		return -EBUSY;
	}

	priv[i] = kmalloc(sizeof(struct host_priv), GFP_KERNEL);
	if(priv[i] == NULL)
	{
		printk (KERN_ERR "keywie kmalloc host_priv failed---host_probe---Error in 11\n");
		goto fail;
	}
	spin_lock_init(&priv[i]->lock);
	memset(priv[i], 0, sizeof(struct host_priv));
	pci_set_drvdata(pdev, priv[i]);
	retval = host_board_init (pdev,i);
	if(retval) 
	{
		printk (KERN_ERR "keywie host_board_init failed---host_probe---Error in 12\n");
		goto fail2;
	}
	priv[i]->minor = i;
	return 0;

fail2:
	pci_set_drvdata(pdev, NULL);
	kfree(priv[i]);
fail:
	return retval;
}

static int host_suspend(struct pci_dev *pdev, pm_message_t state)
{
	return 0;
}

static int host_resume(struct pci_dev *pdev)
{
	return 0;
}

static struct pci_driver pcie_host_device = 
{
	.name		= HOST_NAME,
	.id_table	= pci_cdev_id_table,
	.probe		= host_probe,
	.remove		= host_remove,
	.suspend	= host_suspend,
	.resume		= host_resume
};


static struct cdev cdev_host;
static int  host_module_init(void)
{
	int retval;
	dev_t dev;

//	DPRINTK("1\n");
	ptp_pci_class = class_create (THIS_MODULE,"pcie_host_device");
	if (IS_ERR (ptp_pci_class))
	{
		//printk (KERN_ERR "Failed to creating class\n");
		printk (KERN_ERR "keywie Failed to creating class---host_module_init---Error in 13\n");
		return IS_ERR (ptp_pci_class);
	}

	retval = pci_register_driver(&pcie_host_device);
	if(retval) 
	{
		printk (KERN_ERR "keywie Pci_register_driver failed---host_module_init---Error in 14\n");
		//printk(KERN_ERR "%s init fail. retval = %d\n", HOST_NAME, retval);
		return retval;
	}

	dev = MKDEV (PTP_PCI_MAJOR,0);
	retval = register_chrdev_region(dev, MAX_PTP_PCI, HOST_NAME);
	//DPRINTK("3 retval =%d \n",retval);
	if (retval < 0)
	{	
		printk (KERN_ERR "keywie Register_chrdev_region failed---host_module_init---Error in 15\n");
		goto fail; 
	}

	cdev_init(&cdev_host, &host_fops);
	cdev_host.owner = THIS_MODULE;
	cdev_host.ops = &host_fops;
	cdev_add(&cdev_host, dev, MAX_PTP_PCI);
	printk(KERN_INFO "%s init succeed\n", HOST_NAME);
	return 0;

fail:
	pci_unregister_driver(&pcie_host_device);
	return retval;
}


static void __exit host_module_exit(void)
{
	cdev_del(&cdev_host);
	unregister_chrdev_region(MKDEV (PTP_PCI_MAJOR,0),MAX_PTP_PCI);
	pci_unregister_driver(&pcie_host_device);
	class_destroy (ptp_pci_class);
	printk(KERN_INFO "%s del succeed\n", HOST_NAME);
	return;
}

MODULE_AUTHOR("LEMON");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("PTPExpress1500 Divice Host Driver");

module_init(host_module_init);
module_exit(host_module_exit);
