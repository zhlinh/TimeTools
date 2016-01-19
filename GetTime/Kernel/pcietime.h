#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/fcntl.h>

#define	TMR_CNT_L	0x24e1c
#define	TMR_CNT_H	0x24e18

#define KVM_HC_GET_PCIE_TIME 100
#define PCIE_DEV "/dev/pcie"

struct host_priv
{
	spinlock_t lock;
	unsigned long m_immrbar;
	unsigned long shm_mem;
	unsigned long  m_immrbar_phy_addr;
	int minor;
	wait_queue_head_t tsev_wq;
	
};

int pcie_open(struct file *filp, char *dev)
{
  filp = filp_open(dev, O_RDONLY, 0444);
  if (IS_ERR(filp)) { 
    filp = NULL;
    return -1; 
  }
  return 0;
}

int pcie_close(struct file *filp)
{
  return filp_close(filp, NULL);
}

int get_pcietime(struct timespec *pcie_time, struct file *filp)
{
  struct host_priv *priv;
  unsigned long flag;
  unsigned long long  tmr_cnt;
  u32 nano_value = 0;

  priv = (struct host_priv *)filp->private_data;
  local_irq_save (flag);
  tmr_cnt =*(u32 *)(priv->m_immrbar + TMR_CNT_L);
  local_irq_restore (flag);
  tmr_cnt = be32_to_cpu (tmr_cnt);
  tmr_cnt |= ((u64)be32_to_cpu (*(u32 *)(priv->m_immrbar + TMR_CNT_H))) << 32;
  if (tmr_cnt){
    pcie_time->tv_sec = div_u64_rem(tmr_cnt,1000000000,&nano_value) - 35 ;
    pcie_time->tv_nsec = nano_value;	            	          
  }
  return 0;
}
