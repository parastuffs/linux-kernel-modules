#define pr_fmt(fmt) "kmalloc_vs_vmalloc: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/ktime.h>
#include <linux/moduleparam.h>

#define MAX_ORDER_PRINT 11  // typically enough (0..10)

/* ---------- Parameters ---------- */

/*
 * Number of higher-order blocks to allocate during fragmentation.
 * Larger values increase memory pressure and make kmalloc() failure
 * more likely at lower orders.
 */
static int frag_blocks = 4000;
module_param(frag_blocks, int, 0644);
MODULE_PARM_DESC(frag_blocks,
    "Number of blocks allocated to fragment memory (higher = more fragmentation)");

/*
 * Order of the blocks used to fragment memory.
 * Each block has size: (2^frag_order * PAGE_SIZE).
 * Using higher orders destroys large contiguous regions more effectively.
 */
static int frag_order = 8;
module_param(frag_order, int, 0644);
MODULE_PARM_DESC(frag_order,
    "Order of allocated blocks for fragmentation (block size = 2^order * PAGE_SIZE)");

/*
 * Order of the test allocation attempted with kmalloc()/vmalloc().
 * The requested size is: (2^test_order * PAGE_SIZE).
 */
static int test_order = 9;
module_param(test_order, int, 0644);
MODULE_PARM_DESC(test_order,
    "Order of test allocation (size = 2^order * PAGE_SIZE)");

/*
 * Number of times to repeat the allocation benchmark.
 * Useful to observe variability and timing differences.
 */
static int repeat = 1;
module_param(repeat, int, 0644);
MODULE_PARM_DESC(repeat,
    "Number of times to repeat kmalloc/vmalloc timing test");

/* ---------- Globals ---------- */

static void **blocks;

/* ---------- Buddy info helpers ---------- */

static void print_all_zones(const char *tag)
{
    pg_data_t *pgdat;
    struct zone *zone;
    int nid;
    int order;

    pr_info("==== Buddy info (%s) ====\n", tag);

    for_each_online_node(nid) {
        pgdat = NODE_DATA(nid);

        for (zone = pgdat->node_zones;
             zone < pgdat->node_zones + MAX_NR_ZONES;
             zone++) {

            if (!populated_zone(zone))
                continue;

            pr_info("Node %d, zone %s\n", nid, zone->name);

            pr_info("orders: ");
            for (order = 0; order < MAX_ORDER_PRINT; order++)
                pr_cont("%lu ", zone->free_area[order].nr_free);
            pr_cont("\n");
        }
    }
}

/* ---------- Fragmentation ---------- */

static int fragment_memory(void)
{
    int i;

    blocks = kmalloc_array(frag_blocks, sizeof(void *), GFP_KERNEL);
    if (!blocks)
        return -ENOMEM;

    /* Step 1: allocate higher-order blocks */
    for (i = 0; i < frag_blocks; i++) {
        blocks[i] = (void *)__get_free_pages(GFP_KERNEL, frag_order);
        if (!blocks[i])
            break;
    }

    pr_info("Allocated %d/%d blocks of order %d\n", i, frag_blocks, frag_order);

    /* Step 2: partially free them → prevent coalescing */
    for (int j = 0; j < i; j++) {
        if (!blocks[j])
            continue;

        /* free half: split into buddies */
        free_pages((unsigned long)blocks[j], frag_order - 1);

        /* keep half allocated → blocks merging */
    }

    return 0;
}

/* ---------- Test ---------- */

static void run_test(void)
{
    int i;
    size_t size = (1 << test_order) * PAGE_SIZE;
    void *kptr, *vptr;
    u64 t1, t2;

    pr_info("Test size: %zu bytes (%d pages)\n", size, 1 << test_order);

    for (i = 0; i < repeat; i++) {

        /* kmalloc */
        t1 = ktime_get_ns();
        kptr = kmalloc(size, GFP_KERNEL | __GFP_NORETRY);
        t2 = ktime_get_ns();

        if (!kptr)
            pr_info("[run %d] kmalloc FAIL (%llu ns)\n", i, t2 - t1);
        else {
            pr_info("[run %d] kmalloc OK (%llu ns)\n", i, t2 - t1);
            kfree(kptr);
        }

        /* vmalloc */
        t1 = ktime_get_ns();
        vptr = vmalloc(size);
        t2 = ktime_get_ns();

        if (!vptr)
            pr_err("[run %d] vmalloc FAIL (%llu ns)\n", i, t2 - t1);
        else {
            pr_info("[run %d] vmalloc OK (%llu ns)\n", i, t2 - t1);
            vfree(vptr);
        }
    }
}

/* ---------- Init / Exit ---------- */

static int __init demo_init(void)
{
    pr_info("=== kmalloc vs vmalloc + buddy histogram demo ===\n");

    print_all_zones("BEFORE");

    if (fragment_memory()) {
        pr_err("Fragmentation failed\n");
        return -ENOMEM;
    }

    print_all_zones("AFTER_FRAG");

    run_test();

    print_all_zones("AFTER_TEST");

    return 0;
}

static void __exit demo_exit(void)
{
    int i;

    if (blocks) {
        for (i = 0; i < frag_blocks; i++) {
            if (blocks[i])
                free_pages((unsigned long)blocks[i], frag_order);
        }
        kfree(blocks);
    }

    pr_info("Demo exit\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("kmalloc vs vmalloc with buddy histograms");