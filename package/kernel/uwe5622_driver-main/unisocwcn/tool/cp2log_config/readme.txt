1. 新增unisoc_cp2log_config.txt配置文件，文件内容如下：

wcn_cp2_log_limit_size=500M;
wcn_cp2_file_max_num=2;
wcn_cp2_file_cover_old=true;
wcn_cp2_log_path="/data/unisoc_dbg";

若要使用自定义的cp2 log配置，请将以上文件push到/data路径下，文件权限修改为666或777。
程序运行以后，将检索/data目录下是否存在unisoc_cp2log_config.txt配置文件：
a. 若存在该配置文件，使用配置文件配置的逻辑属性抓取cp2 log；
b. 若不存在该配置文件，使用软件默认配置的逻辑行为抓取cp2 log。

2. unisoc_cp2log_config.txt配置文件的属性说明：
（1）wcn_cp2_log_limit_size
wcn_cp2_log_limit_size为每个log文件最大尺寸，默认为500M，同code中的默认值。
当某个文件达到最大尺寸（文件最终大小可能在wcn_cp2_log_limit_size上下浮动1KB），
则后边log需要存入到其他文件。

也可以用at指令配置log文件最大尺寸，重启失效：
echo "loglimitsize=xxx" > /proc/mdbg/at_cmd,
xxx为log文件最大尺寸，单位为MB，
例如echo "loglimitsize=500" > /proc/mdbg/at_cmd,即为配置log文件最大尺寸为500M。

（2）wcn_cp2_file_max_num
wcn_cp2_file_max_num为存储的log文件最大个数。
此项在wcn_cp2_file_cover_old为true时生效，即只有在允许覆盖旧的log文件时，
才判断存储的log文件最大个数属性。默认值为2，同code中相同。

也可以用at指令配置log文件最大个数，重启失效：
echo "logmaxnum=xxx" > /proc/mdbg/at_cmd,
xxx为log文件最大个数，
例如echo "logmaxnum=100" > /proc/mdbg/at_cmd,即为配置log文件最大个数为100。

（3）wcn_cp2_file_cover_old
wcn_cp2_file_cover_old代表是否覆盖已有文件，循环使用已经存在的log文件记录log。
默认值为true，同code中的默认值。

当wcn_cp2_file_cover_old为true时，log文件循环地在log文件中存储。
假设wcn_cp2_file_max_num为3的话，当2号文件（unisoc_cp2log_2.txt）文件达到
wcn_cp2_log_limit_size后，之后的log将记录于0号文件（unisoc_cp2log_0.txt），
当0号文件达到最大size后，log将被记录于1号文件，如此循环记录。
每次开机后，如果文件路径下log文件个数小于wcn_cp2_file_max_num，则新建文件记录log；
当文件个数已经达到最大值，则log将存储于0~wcn_cp2_file_max_num-1号文件中第一个小于
wcn_cp2_log_limit_size的文件。
若修改配置文件存储路径到u盘、sd卡等外接设备，可以合理设置文件个数及文件大小。

当wcn_cp2_file_cover_old为false的话log文件个数不受限制。
每次开机后的log文件从0号文件开始存储，若0号文件已经存在，先将其清空。
每当文件size达到wcn_cp2_log_limit_size后会新建log文件存储log，直到存储空间被用完。
由于每次开机后历史的0号文件会清空，请注意log的保存！

也可以用at指令配置是否覆盖旧的log文件，重启失效：
//不覆盖旧的log文件：
echo "logcoverold=0" > /proc/mdbg/at_cmd
//覆盖旧的log文件：
echo "logcoverold=1" > /proc/mdbg/at_cmd

（4）wcn_cp2_log_path
wcn_cp2_log_path为log存储位置，默认位置为"/data/unisoc_dbg"，同code中默认位置。
可通过修改该项来调整log存储路径。若新设置的路径不存在则默认还在"/data/unisoc_dbg"
中存储，新设置的文件路径长度请不要超过100！
也可以用at指令切换log存储路径，重启失效：echo "logpath=/xxx" > /proc/mdbg/at_cmd
增加配置文件后开机第一次可能会提示："new path [/data/unisoc_dbg] is invalid”
属于正常情况，初始化过程会新建路径/data/unisoc_dbg。
若log上报过程中log存储路径被手动删除，如手动删除了/data/unisoc_dbg，
或比如log存储路径为sd卡，但卡被拔出，这些情况下log上报后会不能打开当前存储路径，
于是log存储会切换到/data/unisoc_dbg路径，若该路径不存在则新建该路径。
dump memory生成的文件所存的路径始终与log文件存储路径相同，规则同上。

3. 假设config文件中某条设置不需要可以直接删除，其他设置内容依然生效。
建议修改config文件后将原来已经存在的log文件删除掉。
若文件系统中没有config文件，配置文件中的四项设置依据code中的默认值。

4. 若code中CONFIG_CPLOG_DEBUG关闭，需要打开cp2 log时，
请在芯片上电后执行echo "at+armlog=1\r" > /proc/mdbg/at_cmd
CONFIG_CPLOG_DEBUG宏不影响dump memory功能，该功能一直开启。
由于cp2侧log每100k上报一次，若出现问题时log不完整，可以执行：
echo "at+flushwcnlog\r" > /proc/mdbg/at_cmd将剩余log刷新到ap侧。

5. 如何确定最后一次存储的log文件？
a. 若wcn_cp2_file_cover_old=false，最后一个文件即为序号最大的文件。
b. 当开启wifi进行时间同步后，log文件修改时间也会同步更新，此时可以根据文件时间确定，
否则ls –al显示的文件时间为出厂时间；
c. 若wcn_cp2_file_cover_old=true，且文件个数小于wcn_cp2_file_max_num，
则最后一个文件为最后的log；
d. 若wcn_cp2_file_cover_old=true，且文件个数等于wcn_cp2_file_max_num，
由于log在所有文件中循环存储，文件尺寸小于wcn_cp2_log_limit_size的文件为最后的log；
e. 同时bsp driver的log中每次切换文件都会有打印：
“WCN: log_rx_callback cp2 log file is /data/unisoc_dbg/unisoc_cp2log_2.txt”。
