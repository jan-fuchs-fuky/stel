cmd_/Photometrics/Release27/driverV26/pvpci.mod.o := gcc -Wp,-MD,/Photometrics/Release27/driverV26/.pvpci.mod.o.d -nostdinc -iwithprefix include -D__KERNEL__ -Iinclude -Iinclude2 -I/usr/src/linux-2.6.8-24/include -I/usr/src/linux-2.6.8-24/ -I -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -O2 -fomit-frame-pointer -pipe -msoft-float -mpreferred-stack-boundary=2 -funit-at-a-time -fno-unit-at-a-time -march=i586 -mregparm=3 -I/usr/src/linux-2.6.8-24/include/asm-i386/mach-default -Iinclude/asm-i386/mach-default  -DKBUILD_BASENAME=pvpci -DKBUILD_MODNAME=pvpci -DMODULE -c -o /Photometrics/Release27/driverV26/pvpci.mod.o /Photometrics/Release27/driverV26/pvpci.mod.c

deps_/Photometrics/Release27/driverV26/pvpci.mod.o := \
  /Photometrics/Release27/driverV26/pvpci.mod.c \
    $(wildcard include/config/module/unload.h) \
  /usr/src/linux-2.6.8-24/include/linux/module.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/kallsyms.h) \
  /usr/src/linux-2.6.8-24/include/linux/config.h \
    $(wildcard include/config/h.h) \
  /usr/src/linux-2.6.8-24/include/linux/sched.h \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/preempt.h) \
  include2/asm/param.h \
  /usr/src/linux-2.6.8-24/include/linux/capability.h \
  /usr/src/linux-2.6.8-24/include/linux/types.h \
    $(wildcard include/config/uid16.h) \
  /usr/src/linux-2.6.8-24/include/linux/posix_types.h \
  /usr/src/linux-2.6.8-24/include/linux/stddef.h \
  /usr/src/linux-2.6.8-24/include/linux/compiler.h \
  /usr/src/linux-2.6.8-24/include/linux/compiler-gcc3.h \
  /usr/src/linux-2.6.8-24/include/linux/compiler-gcc.h \
  include2/asm/posix_types.h \
  include2/asm/types.h \
    $(wildcard include/config/highmem64g.h) \
    $(wildcard include/config/lbd.h) \
  /usr/src/linux-2.6.8-24/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/lockmeter.h) \
  /usr/src/linux-2.6.8-24/include/linux/preempt.h \
  /usr/src/linux-2.6.8-24/include/linux/linkage.h \
  include2/asm/linkage.h \
    $(wildcard include/config/x86/alignment/16.h) \
  /usr/src/linux-2.6.8-24/include/linux/thread_info.h \
  /usr/src/linux-2.6.8-24/include/linux/bitops.h \
  include2/asm/bitops.h \
  include2/asm/thread_info.h \
    $(wildcard include/config/4kstacks.h) \
    $(wildcard include/config/debug/stack/usage.h) \
  include2/asm/page.h \
    $(wildcard include/config/x86/use/3dnow.h) \
    $(wildcard include/config/x86/pae.h) \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/highmem4g.h) \
    $(wildcard include/config/discontigmem.h) \
  include2/asm/processor.h \
    $(wildcard include/config/mk8.h) \
    $(wildcard include/config/mk7.h) \
  include2/asm/vm86.h \
  include2/asm/math_emu.h \
  include2/asm/sigcontext.h \
  include2/asm/segment.h \
  include2/asm/cpufeature.h \
  include2/asm/msr.h \
  include2/asm/system.h \
    $(wildcard include/config/x86/cmpxchg.h) \
    $(wildcard include/config/x86/oostore.h) \
  /usr/src/linux-2.6.8-24/include/linux/kernel.h \
    $(wildcard include/config/debug/spinlock/sleep.h) \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.4/include/stdarg.h \
  include2/asm/byteorder.h \
    $(wildcard include/config/x86/bswap.h) \
  /usr/src/linux-2.6.8-24/include/linux/byteorder/little_endian.h \
  /usr/src/linux-2.6.8-24/include/linux/byteorder/swab.h \
  /usr/src/linux-2.6.8-24/include/linux/byteorder/generic.h \
  include2/asm/bug.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/bug.h \
  /usr/src/linux-2.6.8-24/include/linux/cache.h \
  include2/asm/cache.h \
    $(wildcard include/config/x86/l1/cache/shift.h) \
  /usr/src/linux-2.6.8-24/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
  include2/asm/percpu.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/percpu.h \
  /usr/src/linux-2.6.8-24/include/linux/stringify.h \
  /usr/src/linux-2.6.8-24/include/linux/timex.h \
    $(wildcard include/config/time/interpolation.h) \
  /usr/src/linux-2.6.8-24/include/linux/jiffies.h \
  /usr/src/linux-2.6.8-24/include/linux/seqlock.h \
  include2/asm/io.h \
    $(wildcard include/config/x86/ppro/fence.h) \
    $(wildcard include/config/x86/numaq.h) \
  /usr/src/linux-2.6.8-24/include/linux/string.h \
  include2/asm/string.h \
  /usr/src/linux-2.6.8-24/include/linux/vmalloc.h \
  include2/asm/timex.h \
    $(wildcard include/config/x86/elan.h) \
    $(wildcard include/config/x86/tsc.h) \
    $(wildcard include/config/x86/generic.h) \
  /usr/src/linux-2.6.8-24/include/linux/time.h \
  include2/asm/div64.h \
  /usr/src/linux-2.6.8-24/include/linux/rbtree.h \
  /usr/src/linux-2.6.8-24/include/linux/cpumask.h \
    $(wildcard include/config/hotplug/cpu.h) \
  /usr/src/linux-2.6.8-24/include/linux/bitmap.h \
  include2/asm/semaphore.h \
  include2/asm/atomic.h \
  /usr/src/linux-2.6.8-24/include/linux/wait.h \
  /usr/src/linux-2.6.8-24/include/linux/list.h \
  /usr/src/linux-2.6.8-24/include/linux/prefetch.h \
  /usr/src/linux-2.6.8-24/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include2/asm/rwsem.h \
  include2/asm/ptrace.h \
    $(wildcard include/config/frame/pointer.h) \
  include2/asm/mmu.h \
  /usr/src/linux-2.6.8-24/include/linux/smp.h \
  /usr/src/linux-2.6.8-24/include/linux/sem.h \
    $(wildcard include/config/sysvipc.h) \
  /usr/src/linux-2.6.8-24/include/linux/ipc.h \
  include2/asm/ipcbuf.h \
  include2/asm/sembuf.h \
  /usr/src/linux-2.6.8-24/include/linux/signal.h \
  include2/asm/signal.h \
  include2/asm/siginfo.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/siginfo.h \
  /usr/src/linux-2.6.8-24/include/linux/resource.h \
  include2/asm/resource.h \
  /usr/src/linux-2.6.8-24/include/linux/securebits.h \
  /usr/src/linux-2.6.8-24/include/linux/fs_struct.h \
  /usr/src/linux-2.6.8-24/include/linux/completion.h \
  /usr/src/linux-2.6.8-24/include/linux/pid.h \
  /usr/src/linux-2.6.8-24/include/linux/percpu.h \
  /usr/src/linux-2.6.8-24/include/linux/slab.h \
    $(wildcard include/config/.h) \
  /usr/src/linux-2.6.8-24/include/linux/gfp.h \
  /usr/src/linux-2.6.8-24/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
  /usr/src/linux-2.6.8-24/include/linux/numa.h \
  /usr/src/linux-2.6.8-24/include/linux/topology.h \
  include2/asm/topology.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/topology.h \
  /usr/src/linux-2.6.8-24/include/linux/kmalloc_sizes.h \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/large/allocs.h) \
  /usr/src/linux-2.6.8-24/include/linux/param.h \
  /usr/src/linux-2.6.8-24/include/linux/timer.h \
  /usr/src/linux-2.6.8-24/include/linux/aio.h \
  /usr/src/linux-2.6.8-24/include/linux/workqueue.h \
  /usr/src/linux-2.6.8-24/include/linux/aio_abi.h \
  include2/asm/current.h \
  /usr/src/linux-2.6.8-24/include/linux/stat.h \
  include2/asm/stat.h \
  /usr/src/linux-2.6.8-24/include/linux/kmod.h \
    $(wildcard include/config/kmod.h) \
    $(wildcard include/config/hotplug.h) \
  /usr/src/linux-2.6.8-24/include/linux/errno.h \
  include2/asm/errno.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/errno.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/errno-base.h \
  /usr/src/linux-2.6.8-24/include/linux/elf.h \
  include2/asm/elf.h \
  include2/asm/user.h \
  /usr/src/linux-2.6.8-24/include/linux/utsname.h \
  /usr/src/linux-2.6.8-24/include/linux/kobject.h \
  /usr/src/linux-2.6.8-24/include/linux/sysfs.h \
    $(wildcard include/config/sysfs.h) \
  /usr/src/linux-2.6.8-24/include/linux/kref.h \
  /usr/src/linux-2.6.8-24/include/linux/moduleparam.h \
  /usr/src/linux-2.6.8-24/include/linux/init.h \
  include2/asm/local.h \
  include2/asm/module.h \
    $(wildcard include/config/m386.h) \
    $(wildcard include/config/m486.h) \
    $(wildcard include/config/m586.h) \
    $(wildcard include/config/m586tsc.h) \
    $(wildcard include/config/m586mmx.h) \
    $(wildcard include/config/m686.h) \
    $(wildcard include/config/mpentiumii.h) \
    $(wildcard include/config/mpentiumiii.h) \
    $(wildcard include/config/mpentiumm.h) \
    $(wildcard include/config/mpentium4.h) \
    $(wildcard include/config/mk6.h) \
    $(wildcard include/config/mcrusoe.h) \
    $(wildcard include/config/mwinchipc6.h) \
    $(wildcard include/config/mwinchip2.h) \
    $(wildcard include/config/mwinchip3d.h) \
    $(wildcard include/config/mcyrixiii.h) \
    $(wildcard include/config/mviac3/2.h) \
    $(wildcard include/config/regparm.h) \
  /usr/src/linux-2.6.8-24/include/linux/vermagic.h \
  include/linux/version.h \

/Photometrics/Release27/driverV26/pvpci.mod.o: $(deps_/Photometrics/Release27/driverV26/pvpci.mod.o)

$(deps_/Photometrics/Release27/driverV26/pvpci.mod.o):
