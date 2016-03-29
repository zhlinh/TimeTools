# GetTime/

仅用于测试用IO的方式和Hypercall的方式来获取时间。

> 环境准备

gcc编译环境

> 安装和使用

0. 注意需先获取root权限。Ubuntu下获取root权限的方式为：

        sudo -i

1. 测试用IO的方式获取时间：

        gcc -o io_test io_test.c
        ./io_test

1. 测试用Hypercall的方式获取时间：

        gcc -o hypercall_test hypercall_test.c
        ./hypercall_test
