# Driver/

PTP板卡(Keywie KW-800)驱动。

> 环境准备

Linux系统环境是否安装好gcc编译环境与相关lib开发库，kernel-devel-xxxxxx.rpm也需要安装，
主要是为了编译内核驱动使用， 如果上述环境未安装，需要从操作系统光盘Package目录下安装。

> 驱动安装步骤

0. 注意需先获取root权限。Ubuntu下获取root权限的方式为：

        sudo su root

1. cd到本目录下。

2. 在当前目录下运行`sh install.sh`，即完成安装。

> 注意事项

每次重新启动电脑，需要按照上述步骤重新执行一遍，如果想配置一次，每次启动自动加载，

可以将`./install.sh`脚本添加到`init.d`启动配置项中。

> 用户自定义内容

运行`sh install.sh`后，默认只会安装PTP板卡驱动程序，但并未进行时间同步。

如果想在安装驱动的同时执行时间同步程序，可以在运行`sh install.sh`之前，取消注释`install.sh`文件的最后两行。

当然也可在安装驱动后进入上层目录的`Sync/`文件夹，再进行时间同步。详细操作见其目录下的README.md文件。
