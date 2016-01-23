#!/bin/sh

etc_dir="/etc"
target="ptpd2-master.conf"
fmode="664"

chmod $fmode ./${target}
cp -af ./${target} ${etc_dir}/ptpd2.conf
