
### compile

example:

1. buildroot select elfutils

```
│ Symbol: BR2_PACKAGE_ELFUTILS [=y]
│ Type  : bool
│ Prompt: elfutils
│   Location:
│     -> Target packages
│       -> Libraries
│         -> Other
│   Defined at package/elfutils/Config.in:5
```


2. compile perf

add EXTRA_CFLAGS --sysroot to buildroot staging dirs.

```
make ARCH=mips CROSS_COMPILE=mips-linux-gnu- EXTRA_CFLAGS='--sysroot=/data/home/pzqi/work/halley5/buildroot/output/staging'
```


3. run on board

usr/lib/libelf.so.1
perf



=====
4. using compiled binaries.


adb push usr/lib/libelf.so.1 /usr/lib
adb push perf-xburst2 /usr/bin/perf



