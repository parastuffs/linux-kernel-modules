# Purpose
Illustrate the difference in memory allocation between `kmalloc` and `vmalloc`.
This kernel modules first fragments the memory to empty high order page groups by allocating lots of smaller chunks, then tries to allocate memory that would required said pages.
If none are available, the allocation might fail.

To illustrate the fragmentation, the module print some buddyinfo before and after fragmentation.

`kmalloc` and `vmalloc` are also timed to illustrate how `vmalloc` is usually slower than `kmalloc`.

Play around with the module parameters to observe their effect and grasp how memory allocation might fail, especially when large chunks are requested or the system has been running for a long time and memory is heavily fragmented.

# Usage
```sh
sudo insmod kmalloc_vs_vmalloc.ko
```

Check the kernel log:
```sh
sudo dmesg | tail -n 100
```

# Example
Parameters used:
```
frag_blocks = 4000
int frag_order = 8
int test_order = 9
int repeat = 1
```

We see that memory fragmentation fails at some point, as no high-enough order pages are available anymore.
```
[ 1186.919851] kmalloc_vs_vmalloc: === kmalloc vs vmalloc + buddy histogram demo ===
[ 1186.919952] kmalloc_vs_vmalloc: ==== Buddy info (BEFORE) ====
[ 1186.920128] kmalloc_vs_vmalloc: Node 0, zone DMA
[ 1186.920151] kmalloc_vs_vmalloc: orders: 0 0 0 0 1 1 0 1 0 1 3 
[ 1186.920245] kmalloc_vs_vmalloc: Node 0, zone DMA32
[ 1186.920254] kmalloc_vs_vmalloc: orders: 8293 7086 5811 858 432 129 51 23 20 7 251 
[ 1191.286532] insmod: page allocation failure: order:8, mode:0xcc0(GFP_KERNEL), nodemask=(null),cpuset=/,mems_allo0
[ 1191.286834] CPU: 0 PID: 3256 Comm: insmod Tainted: G    B      OE      6.8.0-106-generic #106-Ubuntu
[ 1191.286853] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS Arch Linux 1.17.0-2-2 04/01/2014
[ 1191.286863] Call Trace:
[ 1191.286876]  <TASK>
[ 1191.286897]  dump_stack_lvl+0x76/0xa0
[ 1191.286923]  dump_stack+0x10/0x20
[ 1191.286934]  warn_alloc+0x174/0x1f0
[ 1191.286952]  ? __alloc_pages_direct_compact+0x20b/0x240
[ 1191.286971]  __alloc_pages_slowpath.constprop.0+0x992/0xa50
[ 1191.286992]  __alloc_pages+0x31f/0x350
[ 1191.286998]  alloc_pages_mpol+0x91/0x210
[ 1191.286998]  alloc_pages+0x5b/0xd0
[ 1191.286998]  __get_free_pages+0x11/0x50
[ 1191.286998]  demo_init+0x9f/0xff0 [kmalloc_vs_vmalloc]
[ 1191.286998]  ? __pfx_demo_init+0x10/0x10 [kmalloc_vs_vmalloc]
[ 1191.286998]  do_one_initcall+0x5e/0x340
[ 1191.286998]  do_init_module+0x97/0x290
[ 1191.286998]  load_module+0xb5f/0xca0
[ 1191.286998]  ? security_kernel_post_read_file+0x75/0x90
[ 1191.286998]  init_module_from_file+0x96/0x100
[ 1191.286998]  ? init_module_from_file+0x96/0x100
[ 1191.286998]  idempotent_init_module+0x11c/0x310
[ 1191.286998]  __x64_sys_finit_module+0x64/0xd0
[ 1191.286998]  x64_sys_call+0x2019/0x25a0
[ 1191.286998]  do_syscall_64+0x7f/0x180
[ 1191.286998]  ? arch_exit_to_user_mode_prepare.isra.0+0x95/0xe0
[ 1191.286998]  ? syscall_exit_to_user_mode+0x43/0x1e0
[ 1191.286998]  ? do_syscall_64+0x8c/0x180
[ 1191.286998]  ? rseq_get_rseq_cs+0x22/0x280
[ 1191.286998]  ? rseq_ip_fixup+0x90/0x170
[ 1191.286998]  ? generic_file_llseek+0x24/0x40
[ 1191.286998]  ? ksys_lseek+0x80/0xd0
[ 1191.286998]  ? arch_exit_to_user_mode_prepare.isra.0+0x1a/0xe0
[ 1191.286998]  ? syscall_exit_to_user_mode+0x43/0x1e0
[ 1191.286998]  ? do_syscall_64+0x8c/0x180
[ 1191.286998]  ? do_syscall_64+0x8c/0x180
[ 1191.286998]  ? __count_memcg_events+0x6b/0x120
[ 1191.286998]  ? count_memcg_events.constprop.0+0x2a/0x50
[ 1191.286998]  ? handle_mm_fault+0xad/0x380
[ 1191.286998]  ? arch_exit_to_user_mode_prepare.isra.0+0x1a/0xe0
[ 1191.286998]  ? irqentry_exit_to_user_mode+0x38/0x1e0
[ 1191.286998]  ? irqentry_exit+0x43/0x50
[ 1191.286998]  ? exc_page_fault+0x94/0x1b0
[ 1191.286998]  entry_SYSCALL_64_after_hwframe+0x78/0x80
[ 1191.286998] RIP: 0033:0x72795652728d
[ 1191.286998] Code: ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa 48 89 f8 48 89 f7 48 89 d6 48 89 ca 4d 89 c8
[ 1191.286998] RSP: 002b:00007ffd1554f088 EFLAGS: 00000246 ORIG_RAX: 0000000000000139
[ 1191.286998] RAX: ffffffffffffffda RBX: 00005bede13a9740 RCX: 000072795652728d
[ 1191.286998] RDX: 0000000000000000 RSI: 00005beda3ba9e52 RDI: 0000000000000003
[ 1191.286998] RBP: 00007ffd1554f140 R08: 0000000000000040 R09: 0000000000004100
[ 1191.286998] R10: 0000727956603b20 R11: 0000000000000246 R12: 00005beda3ba9e52
[ 1191.286998] R13: 0000000000000000 R14: 00005bede13a9700 R15: 0000000000000000
[ 1191.286998]  </TASK>
[ 1191.287824] Mem-Info:
[ 1191.288136] active_anon:7284 inactive_anon:19 isolated_anon:0
                active_file:19402 inactive_file:25996 isolated_file:0
                unevictable:3650 dirty:768 writeback:0
                slab_reclaimable:4529 slab_unreclaimable:8836
                mapped:10100 shmem:151 pagetables:381
                sec_pagetables:0 bounce:0
                kernel_misc_reclaimable:0
                free:78183 free_pcp:74 free_cma:0
[ 1191.288341] Node 0 active_anon:29136kB inactive_anon:76kB active_file:77608kB inactive_file:103984kB unevictableo
[ 1191.288450] Node 0 DMA free:8896kB boost:0kB min:344kB low:428kB high:512kB reserved_highatomic:0KB active_anon:B
[ 1191.288566] lowmem_reserve[]: 0 1929 1929 1929 1929
[ 1191.288629] Node 0 DMA32 free:303836kB boost:4096kB min:48804kB low:59980kB high:71156kB reserved_highatomic:0KBB
[ 1191.288653] lowmem_reserve[]: 0 0 0 0 0
[ 1191.288755] Node 0 DMA: 0*4kB 0*8kB 0*16kB 0*32kB 1*64kB (U) 1*128kB (U) 0*256kB 1*512kB (U) 0*1024kB 2*2048kB (B
[ 1191.289114] Node 0 DMA32: 6478*4kB (UME) 5837*8kB (UME) 5517*16kB (UME) 1221*32kB (UME) 1035*64kB (UME) 227*128kB
[ 1191.289316] Node 0 hugepages_total=0 hugepages_free=0 hugepages_surp=0 hugepages_size=2048kB
[ 1191.289374] 49157 total pagecache pages
[ 1191.289620] 0 pages in swap cache
[ 1191.289642] Free swap  = 0kB
[ 1191.289655] Total swap = 0kB
[ 1191.289724] 524155 pages RAM
[ 1191.289736] 0 pages HighMem/MovableOnly
[ 1191.289753] 20434 pages reserved
[ 1191.289767] 0 pages hwpoisoned
[ 1191.289817] kmalloc_vs_vmalloc: Allocated 1234/4000 blocks of order 8
[ 1191.292940] kmalloc_vs_vmalloc: ==== Buddy info (AFTER_FRAG) ====
[ 1191.292961] kmalloc_vs_vmalloc: Node 0, zone DMA
[ 1191.292970] kmalloc_vs_vmalloc: orders: 0 0 0 0 1 1 0 7 0 2 1 
[ 1191.293067] kmalloc_vs_vmalloc: Node 0, zone DMA32
[ 1191.293075] kmalloc_vs_vmalloc: orders: 6476 5837 5516 1223 1034 227 22 1236 1 0 0 
[ 1191.293160] kmalloc_vs_vmalloc: Test size: 2097152 bytes (512 pages)
[ 1191.293674] kmalloc_vs_vmalloc: [run 0] kmalloc OK (481639 ns)
[ 1191.294482] kmalloc_vs_vmalloc: [run 0] vmalloc OK (742638 ns)
[ 1191.294726] kmalloc_vs_vmalloc: ==== Buddy info (AFTER_TEST) ====
[ 1191.294736] kmalloc_vs_vmalloc: Node 0, zone DMA
[ 1191.294743] kmalloc_vs_vmalloc: orders: 0 0 0 0 1 1 0 7 0 2 1 
[ 1191.294799] kmalloc_vs_vmalloc: Node 0, zone DMA32
[ 1191.294806] kmalloc_vs_vmalloc: orders: 5531 5837 5516 1223 1034 227 22 1236 1 0 0 
```