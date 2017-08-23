DNS-Forwarder for OpenWrt
===

 [![Download][B]][2]

���
---

 ����Ŀ�� [DNS-Forwarder][1] �� OpenWrt �ϵ���ֲ  

����
---

 - �� OpenWrt �� [SDK][S] ����  

   ```bash
   # �� ar71xx ƽ̨Ϊ��
   tar xjf OpenWrt-SDK-ar71xx-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2
   cd OpenWrt-SDK-ar71xx-*
   # ��ȡ Makefile
   git clone https://github.com/aa65535/openwrt-dns-forwarder.git package/dns-forwarder
   # ѡ��Ҫ����İ� Network -> dns-forwarder
   make menuconfig
   # ��ʼ����
   make package/dns-forwarder/compile V=99
   ```

����
---

 - Ĭ�� DNS �������˿�Ϊ `5300`, ��ʹ�� [LuCI][L] ��������  

 - ������Ϊ [ChinaDNS][3] �����η�����ʹ��, ���÷����ο� [Wiki][W]  


 [1]: https://github.com/aa65535/hev-dns-forwarder
 [2]: https://github.com/aa65535/openwrt-dns-forwarder/releases/latest
 [3]: https://github.com/aa65535/openwrt-chinadns
 [B]: https://img.shields.io/github/release/aa65535/openwrt-dns-forwarder.svg
 [S]: https://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
 [L]: https://github.com/aa65535/openwrt-dist-luci
 [W]: https://github.com/aa65535/openwrt-chinadns/wiki/Use-DNS-Forwarder
