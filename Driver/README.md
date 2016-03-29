# Driver/

PTP板卡驱动。

> 环境准备

Linux系统环境是否安装好gcc编译环境与相关lib开发库，kernel-devel-xxxxxx.rpm也需要安装，
主要是为了编译内核驱动使用， 如果上述环境未安装，需要从操作系统光盘Package目录下安装。

> 驱动安装步骤

1. 确定是否取得root权限。
2. 修改文件运行权限，操作命令如下：

        chmod 0555 install.sh

3. 在当前目录下运行`sh install.sh`，即完成安装。

> 注意事项

每次重新启动电脑，需要按照上述步骤重新执行一遍，如果想配置一次，每次启动自动加载，

可以将./install.sh脚本添加到init.d启动配置项中。

> 用户自定义内容

运行install.sh后，默认会安装驱动程序并运行时间同步程序。

如果不想运行时间同步程序，可以在运行`sh install.sh`之前，注释掉install.sh文件的最后两行。
