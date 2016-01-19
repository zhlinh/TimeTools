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
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/cpu.h>

MODULE_LICENSE("GPL");

#define MS_TO_NS(x)	(x * 1E6L)
#define MY_HOUR 11
#define MY_MIN 12
#define MY_SEC 0

static struct hrtimer hr_timer;
  
enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
  struct timex txc; 
  struct rtc_time tm;
  int result = 0;
  char cmd_path[] = "/home/ptp/settimer/samplecode";
  char* cmd_argv[] = {cmd_path,"no",NULL};
  char* cmd_envp[] = {"HOME=/", "PATH=/sbin:/bin:/usr/bin", NULL};
  
  do_gettimeofday(&(txc.time));
  rtc_time_to_tm(txc.time.tv_sec, &tm);

  // printk("my_hrtimer_callback called (%lu).\n", jiffies);
  // notice UTC, year+1900, mon+1, hour+8(to BeiJing timezone)
  // TODO: hour+8 may cause bugs.
  printk("my_hrtimer_callback at %02d-%02d-%02d %02d:%02d:%02d\n", \
          tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, \
          tm.tm_hour + 8, tm.tm_min, tm.tm_sec);
  // tsc to ns, to ABS time
  result = call_usermodehelper(cmd_path, cmd_argv, cmd_envp, UMH_WAIT_PROC);
  printk("my_hrtimer_callback get_pcie_time result is %d\n", result);
  return HRTIMER_NORESTART;
}

int init_module( void )
{
  ktime_t ktime;
//  unsigned long long delay_in_ms = 200L;
  struct timex txc; 
  struct rtc_time tm;
  struct timespec val;
  unsigned long long timep = 0;
  
  do_gettimeofday(&(txc.time));
  rtc_time_to_tm(txc.time.tv_sec, &tm);

  tm.tm_sec = MY_SEC;   // second of a minute
  tm.tm_min = MY_MIN;  // minute of an hour
  tm.tm_hour = MY_HOUR;  // hour of a day
  printk("my_hrtimer set time at %02d-%02d-%02d %02d:%02d:%02d\n", \
          tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, \
	  tm.tm_hour, tm.tm_min, tm.tm_sec);
  // notice mktime should use UTC, year+1900, mon+1, hour-8
  // TODO: hour-8 may cause bugs.
  timep = mktime(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, \
	  tm.tm_hour-8, tm.tm_min, tm.tm_sec);
  val.tv_sec = timep;
  val.tv_nsec = 0;
  
//  delay_in_ms = (unsigned long)timep * 1E3L;

  printk("my_hrtimer module installing\n");
 
//  ktime = ktime_set(0, MS_TO_NS(delay_in_ms));
  ktime = timespec_to_ktime(val);

  // second parm: CLOCK_MONOTONIC and CLOCK_REALTIME
  // third parm: HRTIMER_MODE_REL and HRTIMER_MODE_ABS 
  hrtimer_init(&hr_timer, CLOCK_REALTIME, HRTIMER_MODE_ABS);
  
  hr_timer.function = &my_hrtimer_callback;

  printk( "Starting my_hrtimer to fire in %lluns ktime (%lu)\n", \
	 ktime_to_ns(ktime), jiffies );

  printk( "Starting my_hrtimer to fire from %lluns now (%lu)\n", \
         ktime_to_ns(hr_timer.base->get_time()), jiffies );

  // third parm: HRTIMER_MODE_REL and HRTIMER_MODE_ABS 
  hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_ABS );

  return 0;
}

void cleanup_module( void )
{
  int ret;

  ret = hrtimer_cancel( &hr_timer );
  if (ret) printk("The timer was still in use...\n");

  printk("my_hrtimer module uninstalling\n");

  return;
}
