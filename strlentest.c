#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/random.h>

#define BUF_SIZE 1024 * 1024

static int test_count = 20;
module_param(test_count, int, 0660);
static int test_per_size = 100;
module_param(test_per_size, int, 0660);
static int delta_percent = 10;
module_param(delta_percent, int, 0660);

static char *buffer, *buffer2;

static void __exit mod_exit(void)
{
	kvfree(buffer);
	kvfree(buffer2);
}

static void test(u32 current_size)
{
	u32 i;
	u32 offset, size, delta = 0;
	u32 delta_max = current_size * delta_percent / 100;
	size_t res_old, res_new;
	ktime_t start, end;
	u64 mean_old = 0, mean_new = 0;

	for (i = 0; i < test_per_size; i++) {
		if (delta_max > 0) {
			get_random_bytes(&delta, sizeof(delta));
			delta %= delta_max;
		}
		get_random_bytes(&offset, sizeof(offset));
		offset %= 8;
		if (offset > size)
			offset = 0;
		size = current_size - delta;
		memset(buffer, 'A', size);
		buffer[size + 1] = '\0';
		memset(buffer2, 'B', size);
		buffer2[size + 1] = '\0';
		
		start = ktime_get();
		res_new = strlen2(buffer2 + offset);
		end = ktime_get();

		mean_new += end - start;

		start = ktime_get();
		res_old = strlen3(buffer + offset);
		end = ktime_get();

		mean_old += end - start;

		if (res_old != res_new) {
			pr_err("AHTUNG! Size: %u, res_old: %lu, res_new: %lu, offset: %u\n",
			       size, res_old, res_new, offset);
		}
	}
	mean_old /= test_per_size;
	mean_new /= test_per_size;

	pr_info("Size: %u (+-%u), mean_old: %llu, mean_new: %llu\n", current_size, delta, mean_old, mean_new);
}

static int __init mod_init(void)
{
	u32 current_size = 1;
	u32 i;

	buffer = kvmalloc(BUF_SIZE, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;
	buffer2 = kvmalloc(BUF_SIZE, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	for (i = 0; i < test_count; i++) {
		test(current_size);
		current_size *= 2;
	}

	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Orlov");
module_init(mod_init);
module_exit(mod_exit);
