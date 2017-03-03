# Condition/

用于为测试添加各种条件，如cpu负载，网络负载等。

> 环境准备

gcc编译环境。

> 安装和使用

1. 注意需先获取root权限。Ubuntu下获取root权限的方式为：

        sudo su root

2. cd到本目录下，并编译本目录下所有C源文件。

        make

3. 增加CPU负载的工具[**load**]。如需增加CPU 6(请根据需要自行调整)个核的负载：

        sh cpu-stress-add.sh 6

   如果想取消增加CPU负载，则可运行

        sh cpu-stress-remove.sh

4. 增加网络负载的工具[**iperf3**]。其安装方法为`sudo apt-get install iperf3`。使用方法如下：

      
   如果是服务器(接收)端，则运行
        
        sh server-traffic-add.sh <using bidirection(true/false)>
        
   其中，`<using bidirection(true/false)>`表示是否使用双向传输，如果不使用，则运行`sh server-traffic-add.sh false`，
   否则，执行`sh server-traffic-add.sh true`。
   
   注意，是否使用双向传输这一选项在服务器端与客户端应保持一致。
        
   如果是客户端(发送)端，则运行
        
        sh client-traffic-add.sh <server address> <network traffic(b/s)> <running time(s)> <using bidirection(true/false)>
    
   其中，`<server address>`为服务器端的IP地址，`<network traffic(b/s)>`为使用的传输速率（单位为比特/秒），
   
   `<running time(s)>`为传输的持续时间（单位为秒），`<using bidirection(true/false)>`表示是否使用双向传输。
   
   各参数需要按固定顺序排列，举例： `sh client-traffic-add.sh 192.168.1.18 100M 10 false`
   
   注意，是否使用双向传输这一选项在服务器端与客户端应保持一致。
   
   服务器端或客户端如果想取消增加网络负载，则可运行
   
        sh traffic-remove.sh


