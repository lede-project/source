#!/bin/sh
cd /tmp
#�ȴ��������ӳɹ�
wget_ok="0"
rm -f 7620n.tar.gz
#rm -f ar71xx.tar.gz

while [ "$wget_ok" = "0" ] 
do 
wget http://update.adbyby.com/download/7620n.tar.gz
#wget http://update.adbyby.com/download/ar71xx.tar.gz
if [ "$?" == "0" ]; then
wget_ok="1"
else
sleep 30
fi
done
#��ѹ������adbyby
mkdir -p adbyby 
tar -zvxf 7620n.tar.gz -C ./adbyby 
#tar -zvxf ar71xx.tar.gz -C ./adbyby 

#����һ�ι���
murl=`cat /tmp/adbyby/bin/update.info|grep lazy.txt|awk '{print $1}'`

if [ ${#murl} -gt 5 ] ;then
murl2=`cat /tmp/adbyby/bin/update.info|grep lazy.txt|awk '{print $2}'`
mfile="/tmp/adbyby/bin"${murl2//\\//}
wget $murl -O $mfile
fi

murl=`cat /tmp/adbyby/bin/update.info|grep video.txt|awk '{print $1}'`
if [ ${#murl} -gt 5 ] ;then
murl2=`cat /tmp/adbyby/bin/update.info|grep video.txt|awk '{print $2}'`
mfile="/tmp/adbyby/bin"${murl2//\\//}
wget $murl -O $mfile
fi

murl=`cat /tmp/adbyby/bin/update.info|grep user.action|awk '{print $1}'`
if [ ${#murl} -gt 5 ] ;then
murl2=`cat /tmp/adbyby/bin/update.info|grep user.action|awk '{print $2}'`
mfile="/tmp/adbyby/bin"${murl2//\\//}
wget $murl -O $mfile
fi

cd ./adbyby/bin
./startadbb

