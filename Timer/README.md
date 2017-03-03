# Timer/

基于定时器的各种时间工具，使用的PTP板卡为Keywie KW-800。

> 环境准备

gcc编译环境。

> 安装和使用

1. 注意需先获取root权限。Ubuntu下获取root权限的方式为：

        sudo su root

2. cd到本目录下，并编译本目录下所有C源文件。

        make

3. 主要的定时器[**main_user_timer**]，默认每隔5秒读取一次时间，并将数据保存在本目录下`timer.log`(无此文件时会自动创建)中。

   Usuage:
   ```
   ./main_user_timer [OPTION]
   -h    show help information.
   -t    set the timer start point.(default next secs(secs%%5==0))
   -i    set the timer interval.(default 5s)
   -m    set the mode to get time(default 0) :
           m : 0 means system time.
           m : 1 means pcie time from driver directly.
           m : 2 means pcie time from hypercall.
   Example: ./main_user_timer -t 8:30:0 -m 2
   ```
   **注意: 至少需要用-t和-m并添加相应参数选择定时器开始时间和获取时间的方式。**

   两个或多个PC机/VM所选择的定时器开始时间和获取时间的方式应保持一致，这样才比较方便比较其同步效果。

3. posix接口的定时器[**posix_timer**]，使用了posix接口，默认每隔5秒读取一次时间，
   并将数据保存在本目录下的`posix-timer.log`(无此文件时则自动创建)中。

   使用方法与`main_user_timer`相同，但经测试定时器效果比`main_user_timer`稍差，故建议使用`main_user_timer`。

4. 测量获取时间所需时间的工具[**rdtime_cost**]。
   默认每隔5秒得到一次获取时间的过程所耗费的时间。并将数据保存在本目录`cost.log`(无此文件时会自动创建)中。

   获取时间的过程也是需要一定时间的，其稳定性将影响测量精度。

   Usuage:
   ```
   ./rdtime_cost [OPTION]
   -h    show help information.
   -t    set the timer start point.(default next secs(secs%%5==0))
   -i    set the timer interval.(default 5s)
   -m    set the mode to get time(default 0) :
           m : 0 means system time.
           m : 1 means pcie time from driver directly.
           m : 2 means pcie time from hypercall.
           m : 3 means hypercall but do nothing.
   Example: ./main_user_timer -t 8:30:0 -m 2
   ```
   **注意: 至少需要用-t和-m并添加相应参数选择定时器开始时间和获取时间的方式。**
   其使用方法与`main_user_timer`基本相同。
   只是-m多了一个3的选择，表示进入hypercall但并不执行获取时间操作立即返回的所需时间。

5. 基于TCP定时传输所获取时间的工具[**tcp_client, tcp_server**]。定时器默认间隔为5s。
   其中tcp_server仅用于在本地环路测试tcp_client所传输的数据是否正确。
   而tcp_client可配合目前用C#开发的Windows平台上的可视化工具，可以显示实时同步效果。

   Usuage:
   ```
   ./tcp_client [OPTION]
   -h   show help information.
   -t   set the timer start point.(default next secs(secs%%5==0))
   -i   set the timer interval.(default 5s)
   -a   set the tcp server address.
   -p   set the tcp server port.(default 8888)
   -m   set the mode to get time(default 0) :
        m : 0 means system time.
        m : 1 means pcie time from driver directly.
        m : 2 means pcie time from hypercall.
        m : 3 means reference time from timer, only in integer interval.
   Example: ./tcp_client -m 2 -a 192.168.1.153 -p 8885
   ```
   只需在两台机子上运行`tcp_client`将数据传给第三台机子上的可视化软件，
   即可实时查看同步效果。

   **注意: 至少需要用-m、-a、-p和添加相应参数选择获取时间的方式、TCP服务器的IP地址和端口号。**

   如果使用-t参数请保证两个tcp_client所跟的定时器开始时间参数一致或相隔5的倍数秒，这样才能保证两个tcp_client想要获取的是同一时间的数据。


> 其他

make生成可执行文件之后，运行`./[文件名] -h`均可获取每个可执行文件的使用方法。

