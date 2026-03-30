#define pr_fmt(fmt) "kernel_monster: " fmt

#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ECAM");
MODULE_DESCRIPTION("A virtual kernel monster (pocket size)");
MODULE_VERSION("1.0");

#define MONSTER_NAME_LEN 32
#define MONSTER_MOOD_LEN 16


/* ------------------------------------------------------------------------- */
/* Module parameters                                                         */
/* ------------------------------------------------------------------------- */

static char name[MONSTER_NAME_LEN] = "Poupoule";
module_param_string(name, name, sizeof(name), S_IRUGO);
MODULE_PARM_DESC(name, "Name of the monster");

static int hunger = 20;
module_param(hunger, int, S_IRUGO);
MODULE_PARM_DESC(hunger, "Initial hunger level (0-100)");

static int energy = 90;
module_param(energy, int, S_IRUGO);
MODULE_PARM_DESC(energy, "Initial energy level (0-100)");

/* ------------------------------------------------------------------------- */
/* Monster state                                                                 */
/* ------------------------------------------------------------------------- */

struct kernel_monster {
	char name[MONSTER_NAME_LEN];
	int hunger;
	int energy;
	int age;
	char mood[MONSTER_MOOD_LEN];
};

static struct kernel_monster monster;
static struct delayed_work monster_work;
static struct proc_dir_entry *monster_proc_entry;

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

static int clamp_value(int value, int min, int max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

static void update_mood(struct kernel_monster *p)
{
	if (p->energy < 20)
		strscpy(p->mood, "sleepy", sizeof(p->mood));
	else if (p->hunger > 80)
		strscpy(p->mood, "angry", sizeof(p->mood));
	else if (p->hunger > 50)
		strscpy(p->mood, "hungry", sizeof(p->mood));
	else
		strscpy(p->mood, "happy", sizeof(p->mood));
}

static void monster_tick(struct kernel_monster *p)
{
	p->age += 1;
	p->hunger += 10;
	p->energy -= 5;

	p->hunger = clamp_value(p->hunger, 0, 100);
	p->energy = clamp_value(p->energy, 0, 100);

	update_mood(p);

	pr_info("Tick %d -- name=%s hunger=%d energy=%d mood=%s\n",
		p->age, p->name, p->hunger, p->energy, p->mood);
}

/* ------------------------------------------------------------------------- */
/* Delayed work                                                              */
/* ------------------------------------------------------------------------- */

static void monster_work_handler(struct work_struct *work)
{
	monster_tick(&monster);

	/* Schedule again in 1000 ms */
	schedule_delayed_work(&monster_work, msecs_to_jiffies(1000));
}

/* ------------------------------------------------------------------------- */
/* /proc interface                                                           */
/* ------------------------------------------------------------------------- */

static int monster_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "name: %s\n", monster.name);
	seq_printf(m, "age: %d\n", monster.age);
	seq_printf(m, "hunger: %d\n", monster.hunger);
	seq_printf(m, "energy: %d\n", monster.energy);
	seq_printf(m, "mood: %s\n", monster.mood);

	return 0;
}

static int monster_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, monster_proc_show, NULL);
}

static const struct proc_ops monster_proc_ops = {
	.proc_open    = monster_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* ------------------------------------------------------------------------- */
/* Module init / exit                                                        */
/* ------------------------------------------------------------------------- */

static int __init kernel_monster_init(void)
{
	hunger = clamp_value(hunger, 0, 100);
	energy = clamp_value(energy, 0, 100);

	strscpy(monster.name, name, sizeof(monster.name));
	monster.hunger = hunger;
	monster.energy = energy;
	monster.age = 0;
	update_mood(&monster);

	pr_info("A new monster named %s was born!\n", monster.name);
	pr_info("Initial state -- hunger=%d energy=%d mood=%s\n",
		monster.hunger, monster.energy, monster.mood);

	/* Create /proc entry */
	monster_proc_entry = proc_create("kernel_monster", S_IRUGO, NULL, &monster_proc_ops);
	if (!monster_proc_entry) {
		pr_err("Failed to create /proc/kernel_monster\n");
		return -ENOMEM;
	}

	/* Start periodic updates */
	INIT_DELAYED_WORK(&monster_work, monster_work_handler);
	schedule_delayed_work(&monster_work, msecs_to_jiffies(1000));

	return 0;
}

static void __exit kernel_monster_exit(void)
{
	cancel_delayed_work_sync(&monster_work);

	if (monster_proc_entry)
		proc_remove(monster_proc_entry);

	pr_info("%s has left the kernel world.\n", monster.name);
}

module_init(kernel_monster_init);
module_exit(kernel_monster_exit);