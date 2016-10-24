# TimeTools

[![Build Status](https://travis-ci.org/zhlinh/TimeTools.svg?branch=master)](https://travis-ci.org/zhlinh/TimeTools)

PTP 1588虚拟机同步系统的相关时间工具集，使用的PTP板卡为Keywie KW-800。

---

> 目录

1. `Driver/`: PTP板卡驱动。
2. `GetTime/`: 获取PTP板卡时间的测试。
3. `Ptpd/`: PTP 1588协议的软件同步实现。
4. `Sync/`: PTP 1588协议的硬件同步实现。
5. `Timer/`: 相关的定时器，包括时间同步测量、增加CPU负载等。

> 安装和使用

见各目录下的README.md文件。

> 其他

GNU/Linux 3.13.0, gcc 4.8.4

zhlinh

Email: zhlinhng@gmail.com

2016-10-24
