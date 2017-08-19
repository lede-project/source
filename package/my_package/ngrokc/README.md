ngrok-c for OpenWrt
===
版本 20160829

为编译[此固件][N]所需依赖包而写的Makefile

简介
---

 软件包只包含 [ngrok-c][1] 的可执行文件,可配合[luci-app-ngrokc][M]使用
 
 本项目是 [ngrok-c][1] 在 OpenWrt 上的移植  
 
 当前版本: 2016.8.29最后一次commit  
 
 可以修改Makefile中PKG_SOURCE_VERSION为你需要编译的commit id
 
依赖
---
显式依赖 `libc` `libpthread` `libopenssl` `libstdcpp`
 
安装位置
---
  ```
   客户端/
   └── usr/
       └── bin/
           └── ngrokc      // 提供内网穿透服务
   ```
 
 编译
---

 - 从 OpenWrt 的 [SDK][S] 编译  

   ```bash
   # 以 ar71xx 平台为例
   tar xjf OpenWrt-SDK-ar71xx-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2
   cd OpenWrt-SDK-ar71xx-*
   # 获取 Makefile
   git clone https://github.com/AlexZhuo/openwrt-ngrokc.git package/ngrokc
   # 选择要编译的包 Network -> ngrokc
   make menuconfig
   # 开始编译
   make package/ngrokc/compile V=99
   ```

---

[1]: https://github.com/dosgo/ngrok-c
[S]: http://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
[N]: http://www.right.com.cn/forum/thread-198649-1-1.html
[M]: https://github.com/AlexZhuo/luci-app-ngrokc
