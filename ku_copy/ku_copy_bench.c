#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ben");
MODULE_DESCRIPTION("Linux kernel module to microbenchmark memory copy between "
                   "user and kernel");

static __init int ku_copy_benchmark(void) {
  int retval = 0;
  char *kmem = NULL;
  char __user *umem = NULL;
  u64 start, end;
  unsigned int size;
  unsigned int iterations;
  unsigned int buf_size = 256 * 1024; /* 256KB buffer */
  kmem = kzalloc(buf_size, GFP_KERNEL);
  if (kmem == NULL) {
    pr_warn("Kernel buffer allocation failed.\n");
    retval = -ENOMEM;
    goto cleanup;
  }

  umem = (char __user *)vm_mmap(NULL, 0, buf_size, PROT_READ | PROT_WRITE,
                                MAP_ANONYMOUS | MAP_PRIVATE, 0);
  if (umem == NULL) {
    pr_warn("User buffer allocation failed.\n");
    retval = -ENOMEM;
    goto cleanup;
  }

  pr_info("Kernel-User memory copy microbenchmark starts.\n");
  for (size = 8; size <= buf_size; size = (size << 1)) {
    iterations = (1024UL * 1024 * 1024) / size;
    memset(kmem, 0x55, size);
    start = ktime_get_ns();
    for (unsigned int i = 0; i < iterations; i++) {
      if (copy_to_user(umem, kmem, size) != 0) {
        pr_warn("copy_to_user did partial copy\n");
        retval = -EINVAL;
        goto cleanup;
      }
    }
    end = ktime_get_ns();
    pr_info("copy_to_user   %8u bytes: %10llu ns (%10u iters) %14llu bytes/s\n",
            size, end - start, iterations,
            iterations * size * 1000000000UL / (end - start));

    start = ktime_get_ns();
    for (unsigned int i = 0; i < iterations; i++) {
      if (copy_from_user(kmem, umem, size) != 0) {
        pr_warn("copy_from_user did partial copy\n");
        retval = -EINVAL;
        goto cleanup;
      }
    }
    end = ktime_get_ns();
    pr_info("copy_from_user %8u bytes: %10llu ns (%10u iters) %14llu bytes/s\n",
            size, end - start, iterations,
            iterations * size * 1000000000UL / (end - start));
  }

  pr_info("Kernel-User memory copy microbenchmark ends.\n");

cleanup:
  if (kmem != NULL) {
    kfree(kmem);
  }
  if (umem != NULL) {
    vm_munmap((unsigned long)umem, buf_size);
  }
  return retval;
}

static int __init ku_copy_benchmark_module_init(void) {
  return ku_copy_benchmark();
}

static void __exit ku_copy_benchmark_module_exit(void) {}

module_init(ku_copy_benchmark_module_init);
module_exit(ku_copy_benchmark_module_exit);
