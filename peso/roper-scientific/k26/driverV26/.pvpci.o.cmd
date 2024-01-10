cmd_/Photometrics/Release27/driverV26/pvpci.o := gcc -Wp,-MD,/Photometrics/Release27/driverV26/.pvpci.o.d -nostdinc -iwithprefix include -D__KERNEL__ -Iinclude -Iinclude2 -I/usr/src/linux-2.6.8-24/include  -I/Photometrics/Release27/driverV26 -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -O2 -fomit-frame-pointer -pipe -msoft-float -mpreferred-stack-boundary=2 -funit-at-a-time -fno-unit-at-a-time -march=i586 -mregparm=3 -I/usr/src/linux-2.6.8-24/include/asm-i386/mach-default -Iinclude/asm-i386/mach-default -DMODULE -DKBUILD_BASENAME=pvpci -DKBUILD_MODNAME=pvpci -c -o /Photometrics/Release27/driverV26/.tmp_pvpci.o /Photometrics/Release27/driverV26/pvpci.c

deps_/Photometrics/Release27/driverV26/pvpci.o := \
  /Photometrics/Release27/driverV26/pvpci.c \
    $(wildcard include/config/modernversions.h) \
  /usr/src/linux-2.6.8-24/include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  /usr/src/linux-2.6.8-24/include/linux/config.h \
    $(wildcard include/config/h.h) \
  /usr/src/linux-2.6.8-24/include/linux/compiler.h \
  /usr/src/linux-2.6.8-24/include/linux/compiler-gcc3.h \
  /usr/src/linux-2.6.8-24/include/linux/compiler-gcc.h \
  /usr/src/linux-2.6.8-24/include/linux/module.h \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/kallsyms.h) \
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
  /usr/src/linux-2.6.8-24/include/linux/pci.h \
    $(wildcard include/config/pci/names.h) \
    $(wildcard include/config/pci.h) \
    $(wildcard include/config/isa.h) \
    $(wildcard include/config/eisa.h) \
    $(wildcard include/config/pci/msi.h) \
    $(wildcard include/config/pci/domains.h) \
  /usr/src/linux-2.6.8-24/include/linux/mod_devicetable.h \
  /usr/src/linux-2.6.8-24/include/linux/pci_ids.h \
  /usr/src/linux-2.6.8-24/include/linux/ioport.h \
  /usr/src/linux-2.6.8-24/include/linux/device.h \
    $(wildcard include/config/evlog.h) \
  /usr/src/linux-2.6.8-24/include/linux/pm.h \
    $(wildcard include/config/pm.h) \
  /usr/src/linux-2.6.8-24/include/linux/dmapool.h \
  include2/asm/scatterlist.h \
  include2/asm/pci.h \
  /usr/src/linux-2.6.8-24/include/linux/mm.h \
    $(wildcard include/config/sysctl.h) \
    $(wildcard include/config/stack/growsup.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/shmem.h) \
    $(wildcard include/config/proc/fs.h) \
    $(wildcard include/config/debug/pagealloc.h) \
    $(wildcard include/config/arch/gate/area.h) \
  /usr/src/linux-2.6.8-24/include/linux/prio_tree.h \
  /usr/src/linux-2.6.8-24/include/linux/fs.h \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/auditsyscall.h) \
  /usr/src/linux-2.6.8-24/include/linux/limits.h \
  /usr/src/linux-2.6.8-24/include/linux/kdev_t.h \
  /usr/src/linux-2.6.8-24/include/linux/ioctl.h \
  include2/asm/ioctl.h \
  /usr/src/linux-2.6.8-24/include/linux/dcache.h \
  /usr/src/linux-2.6.8-24/include/linux/rcupdate.h \
  /usr/src/linux-2.6.8-24/include/linux/radix-tree.h \
  /usr/src/linux-2.6.8-24/include/linux/audit.h \
    $(wildcard include/config/audit.h) \
  /usr/src/linux-2.6.8-24/include/linux/quota.h \
  /usr/src/linux-2.6.8-24/include/linux/dqblk_xfs.h \
  /usr/src/linux-2.6.8-24/include/linux/dqblk_v1.h \
  /usr/src/linux-2.6.8-24/include/linux/dqblk_v2.h \
  /usr/src/linux-2.6.8-24/include/linux/nfs_fs_i.h \
  /usr/src/linux-2.6.8-24/include/linux/nfs.h \
  /usr/src/linux-2.6.8-24/include/linux/sunrpc/msg_prot.h \
  /usr/src/linux-2.6.8-24/include/linux/fcntl.h \
  include2/asm/fcntl.h \
  /usr/src/linux-2.6.8-24/include/linux/err.h \
  include2/asm/pgtable.h \
    $(wildcard include/config/highpte.h) \
  include2/asm/fixmap.h \
    $(wildcard include/config/x86/local/apic.h) \
    $(wildcard include/config/x86/io/apic.h) \
    $(wildcard include/config/x86/visws/apic.h) \
    $(wildcard include/config/x86/f00f/bug.h) \
    $(wildcard include/config/x86/cyclone/timer.h) \
    $(wildcard include/config/acpi/boot.h) \
    $(wildcard include/config/pci/mmconfig.h) \
  include2/asm/acpi.h \
    $(wildcard include/config/acpi/pci.h) \
    $(wildcard include/config/acpi/sleep.h) \
  include2/asm/apicdef.h \
  include2/asm/kmap_types.h \
    $(wildcard include/config/debug/highmem.h) \
  include2/asm/pgtable-2level-defs.h \
  include2/asm/pgtable-2level.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/pgtable.h \
  /usr/src/linux-2.6.8-24/include/linux/page-flags.h \
    $(wildcard include/config/swap.h) \
  /usr/src/linux-2.6.8-24/include/asm-generic/pci-dma-compat.h \
  /usr/src/linux-2.6.8-24/include/linux/dma-mapping.h \
  include2/asm/dma-mapping.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/pci.h \
  /usr/src/linux-2.6.8-24/include/linux/interrupt.h \
  /usr/src/linux-2.6.8-24/include/linux/hardirq.h \
    $(wildcard include/config/preept.h) \
  include2/asm/hardirq.h \
  /usr/src/linux-2.6.8-24/include/linux/irq.h \
    $(wildcard include/config/arch/s390.h) \
  include2/asm/irq.h \
  /usr/src/linux-2.6.8-24/include/asm-i386/mach-default/irq_vectors.h \
  /usr/src/linux-2.6.8-24/include/asm-i386/mach-default/irq_vectors_limits.h \
  include2/asm/hw_irq.h \
  /usr/src/linux-2.6.8-24/include/linux/profile.h \
    $(wildcard include/config/profiling.h) \
  include2/asm/sections.h \
  /usr/src/linux-2.6.8-24/include/asm-generic/sections.h \
  /usr/src/linux-2.6.8-24/include/linux/irq_cpustat.h \
  include2/asm/uaccess.h \
    $(wildcard include/config/x86/intel/usercopy.h) \
    $(wildcard include/config/x86/wp/works/ok.h) \
  /Photometrics/Release27/driverV26/pvpci.h \

/Photometrics/Release27/driverV26/pvpci.o: $(deps_/Photometrics/Release27/driverV26/pvpci.o)

$(deps_/Photometrics/Release27/driverV26/pvpci.o):
