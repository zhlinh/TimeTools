# Sync/

基于PTP板卡的硬件同步方式。

> 环境准备

gcc编译环境

> 安装和使用

0. 注意需先获取root权限。Ubuntu下获取root权限的方式为：

        sudo -i

1. 如果是在Host中，那就需要用IO的方式获取板卡时间。运行：

        sh host-install.sh

1. 如果是在Guest中，那就需要用Hypercall的方式获取板卡时间，运行：

        sh guest-install.sh

2. 取消同步(一般用不到，重启也可达到相同效果)，则可使用：

        sh uninstall.sh

> 其他

同步间隔默认为10ms，不建议调节。

如果想调节同步间隔，比如调节为20ms，Host中用IO可运行：

    sh host-install.sh 20000

或Guest用Hypercall可运行：

    sh guest-install.sh 20000

同步间隔参数的单位为us，且必须大于10ms，即参数必须大于10000。
