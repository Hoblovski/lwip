lwIP 的 linux kernel 移植
==============================================================================
这里的 lwIP 移植只是在 x86-64 上, 而且只有 loopback 网络接口的支持.

而且 `recv`, `send`, `close` 等等需要满足一定的顺序才能正确运行.
可以说它只是测试了 TCP/IP 的功能, 但是和内核联系并不好.

修改 kernel
==============================================================================
## 复制源文件
将 `lwipport` 复制到 kernel 的主干目录下

## 复制头文件
将 `include/linux/lwip` 复制到 kernel 对应目录下

## 修改 Kbuild 文件
1. `arch/x86/entry/syscalls/sycall_64.tbl`
中增加系统调用 `lwip_closesock`, `send`, `recv`. 如
~~~ diff
--- linux-4.17.3/arch/x86/entry/syscalls/syscall_64.tbl	2018-06-26 07:51:32.000000000 +0800
+++ linux-4.17.3-lwipport/arch/x86/entry/syscalls/syscall_64.tbl	2018-07-04 19:56:04.181502419 +0800
@@ -341,6 +341,9 @@
 330	common	pkey_alloc		__x64_sys_pkey_alloc
 331	common	pkey_free		__x64_sys_pkey_free
 332	common	statx			__x64_sys_statx
+400	common	lwip_closesock		__x64_sys_lwip_closesock
+401	common	send			__x64_sys_send
+402	common	recv			__x64_sys_recv

 #
 # x32-specific system call numbers start at 512 to avoid cache impact
~~~

2. `arch/x86/Kconfig`
中增加构建的时候的选项 `CONFIG_LWIP`
~~~ diff
--- linux-4.17.3/arch/x86/Kconfig	2018-06-26 07:51:32.000000000 +0800
+++ linux-4.17.3-lwipport/arch/x86/Kconfig	2018-07-01 14:00:06.919143683 +0800
@@ -2945,6 +2945,8 @@
 config HAVE_GENERIC_GUP
 	def_bool y

+source "lwipport/Kconfig"
+
 source "net/Kconfig"

 source "drivers/Kconfig"
~~~

3. `kernel/sys_ni.c`
中增加我们新的 syscall 的 weak symbol
(不是必须的, 只是能保证当我们构建的 config 里面没选择 lwip 的时候,
执行 lwip 新增的系统调用返回 `-ENOSYS`)
~~~ diff
--- linux-4.17.3/kernel/sys_ni.c	2018-06-26 07:51:32.000000000 +0800
+++ linux-4.17.3-lwipport/kernel/sys_ni.c	2018-07-02 13:21:02.512005730 +0800
@@ -253,6 +253,9 @@

 /* arch/example/kernel/sys_example.c */

+/* lwipport/core/syscall.c */
+COND_SYSCALL(lwip_closesock);
+/* send/recv has already been defined in __ARCH_WANT_SYSCALL_DEPRECATED */
+
 /* mm/fadvise.c */
 COND_SYSCALL(fadvise64_64);
~~~

4. `Makefile`
~~~ diff
--- linux-4.17.3/Makefile	2018-06-26 07:51:32.000000000 +0800
+++ linux-4.17.3-lwipport/Makefile	2018-07-03 21:15:01.811269731 +0800
@@ -575,6 +575,7 @@
 init-y		:= init/
 drivers-y	:= drivers/ sound/ firmware/
 net-y		:= net/
+lwipport-y	:= lwipport/api/ lwipport/core/ lwipport/core/ipv4/ lwipport/netif/ lwipport/arch/linux/ # terrible hack
 libs-y		:= lib/
 core-y		:= usr/
 virt-y		:= virt/
@@ -982,7 +983,7 @@

 vmlinux-dirs	:= $(patsubst %/,%,$(filter %/, $(init-y) $(init-m) \
 		     $(core-y) $(core-m) $(drivers-y) $(drivers-m) \
-		     $(net-y) $(net-m) $(libs-y) $(libs-m) $(virt-y)))
+		     $(net-y) $(net-m) $(libs-y) $(libs-m) $(virt-y) $(lwipport-y)))

 vmlinux-alldirs	:= $(sort $(vmlinux-dirs) $(patsubst %/,%,$(filter %/, \
 		     $(init-) $(core-) $(drivers-) $(net-) $(libs-) $(virt-))))
@@ -991,13 +992,14 @@
 core-y		:= $(patsubst %/, %/built-in.a, $(core-y))
 drivers-y	:= $(patsubst %/, %/built-in.a, $(drivers-y))
 net-y		:= $(patsubst %/, %/built-in.a, $(net-y))
+lwipport-y	:= $(patsubst %/, %/built-in.a, $(lwipport-y))
 libs-y1		:= $(patsubst %/, %/lib.a, $(libs-y))
 libs-y2		:= $(patsubst %/, %/built-in.a, $(filter-out %.a, $(libs-y)))
 virt-y		:= $(patsubst %/, %/built-in.a, $(virt-y))

 # Externally visible symbols (used by link-vmlinux.sh)
 export KBUILD_VMLINUX_INIT := $(head-y) $(init-y)
-export KBUILD_VMLINUX_MAIN := $(core-y) $(libs-y2) $(drivers-y) $(net-y) $(virt-y)
+export KBUILD_VMLINUX_MAIN := $(core-y) $(libs-y2) $(drivers-y) $(net-y) $(virt-y) $(lwipport-y)
 export KBUILD_VMLINUX_LIBS := $(libs-y1)
 export KBUILD_LDS          := arch/$(SRCARCH)/kernel/vmlinux.lds
 export LDFLAGS_vmlinux
~~~

> *TODO: 写清楚怎么改, 最好做成一个 patch 然后可以自动脚本打上去*

之后构建 kernel 参见后文

测试用的用户程序
==============================================================================
测试程序是 `brownlazy`.
这个程序中有两个线程, 一个是 server 一个是 client (当时手滑打成了 guest).

server 执行 `socket` -> `bind` -> `listen` -> `accept` -> `send` -> `close`
client 执行 `socket` -> `connect` -> `recv` -> `close`.
server 监听 `127.0.0.1:19`, 发送给 client 一条消息 "brown lazy"..

我们在 host OS 上构建测试程序然后放到 guest OS 的 initrd 里面. 构建如下

~~~ bash
$ git clone https://github.com/Hoblovski/lwip.git -b linux-port
$ cd lwip/contrib
$ make
$ ls brownlazy
brownlazy
~~~

> *TODO: 把 brownlazy.c 弄干净一点*

运行
==============================================================================
## 构建带 lwIP 的 kernel
需要在 config 里面带上 lwIP 和去掉自带的 networking
~~~ bash
$ make x86_64_defconfig
$ make menuconfig
~~~
然后在 menuconfig 首页去掉 `Networking support`, 加上 `a test LWIP port`.

之后正常 `make -j8` 即可. 推荐并行度是核数的两倍.

## 下载和构建 busybox
我们需要 busybox 提供一些用户程序.

在 kernel 目录之外, 下载 busybox
~~~ bash
$ curl https://busybox.net/downloads/busybox-1.26.2.tar.bz2 | tar xjf -
~~~

然后
~~~ bash
$ cd busybox-1.26.2
$ mkdir -pv obj/busybox-x86
$ make O=obj/busybox-x86 defconfig
$ make O=obj/busybox-x86 menuconfig
~~~

然后设置 busybox 静态链接:
`Busybox Settings -> Build Options -> [ ] Build BusyBox as a static binary (no shared libs)`

之后 (这一步花费一分钟)
~~~ bash
$ cd obj/busybox-x86
$ make -j8
$ make install
$ cd ../../
~~~

## 构建 initrd
在 busybox 目录 `busybox-1.26.2/` 内,
~~~ bash
$ mkdir -pv initramfs/x86-busybox
$ cd initramfs/x86-busybox
$ mkdir -pv {bin,sbin,etc,proc,sys,usr/{bin,sbin}}
$ cp -av ../../obj/busybox-x86/_install/* .
~~~

之后创建 `init` 文件, 内容为
~~~ bash
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
echo -e "\nBoot took $(cut -d' ' -f1 /proc/uptime) seconds\n"
exec /bin/sh
~~~

然后 `chmod +x init` (这一步我不知道是不是必要的)
此时当前目录 (`initramfs/x86-busybox`) 里面就应该有 linux 的目录结构了,
执行 `ls` 结果如下
~~~ bash
$ ls
bin  etc  init  linuxrc  proc  sbin  sys  usr
~~~

然后将我们的测试程序 `brownlazy` 复制到当前目录 (`initramfs/x86-busybox`),
~~~ bash
$ cp path/to/brownlazy/executable .
~~~
构建 initrd
~~~ bash
$ find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../../initramfs-busybox-x86.cpio.gz
~~~

## 模拟器中运行
在 x86-64 的 qemu 上运行.
在 kernel 主目录下执行
~~~ bash
$ qemu-system-x86_64 -kernel arch/x86/boot/bzImage -initrd path/to/busybox/initramfs-busybox-x86.cpio.gz -serial stdio -append "root=/dev/ram0 console=ttyAMA0 console=ttyS0"
~~~

如果一切正常, 输出应当类似
```
/ # ./brownlazy
brownlazy_main(): hello
[   40.744696] you called socket!
[   40.749267] you called bind!
[   40.749787] you called listen!
[   40.750655] you called accept!
brownlazy_guest(): hello
[   40.754216] you called socket!
[   40.754683] you called connect!
brownlazy_guest(): connected to server
brownlazy_main(): got conn from port 49153
[   40.764653] you called send!
[   40.764879] send: delegate to lwip_send
[   40.765241] lwip_send: hello
[   40.765465] lwip_send: delegating to netconn_write
[   40.766652] lwip_send: netconn_write got 0
[   40.770760] you called recv!
brownlazy_guest(): got: brown lazy
brownlazy_guest(): guest bye
[   40.776895] you called closesock!
[   40.777420] you called closesock!
```

其中 `brownlazy_guest(): got: brown lazy` 是最重要的, 必须有这一行才算成功.

参考资料
==============================================================================
* [How to Build A Custom Linux Kernel For Qemu (2015 Edition)](http://mgalgs.github.io/2015/05/16/how-to-build-a-custom-linux-kernel-for-qemu-2015-edition.html)
