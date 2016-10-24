# Ptpd/

基于Ptpd的软件同步方式。

> 环境准备

gcc编译环境。

> 安装和使用

0. 注意需先获取root权限。Ubuntu下获取root权限的方式为：

        sudo su root

1. cd到本目录下。

2. Ptpd软件的安装：

        tar -vxzf ptpd-2.3.1.tar.gz
        cd ptpd-2.3.1/
        ./configure
        make
        make install

3. 如果本机作为时间服务器master，使用的网络接口为eth0(请根据实际情况更改)，则运行：

        sh master-install.sh eth0

3. 如果本机作为客户端slave，使用的网络接口为eth0(请根据实际情况更改)，则运行：

        sh slave-install.sh eth0

4. 取消同步(一般用不到，重启也可达到相同效果)，则可使用：

        sh uninstall.sh

> 其他

如果使用非源码的方式安装ptpd。例如`apt-get`或`yum`等其他方式。

此时，若运行`ptpd --help`后显示帮助信息。则需要将*.sh文件中的最后一行`ptpd2 -c /etc/ptpd2.conf`改为`ptpd -c /etc/ptpd2.conf`。

若运行`ptpd2 --help`后显示帮助信息，则无需作任何改动。
