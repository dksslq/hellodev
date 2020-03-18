#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

/*
 * Prototypes - this would normally go in a .h file
 */

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 16

static int major;
static int busy = 0;

static unsigned char BUF[BUF_LEN] = {0};
static int LOAD = 0;

static struct file_operations fops = {.owner = THIS_MODULE,
                                      .read = device_read,
                                      .write = device_write,
                                      .open = device_open,
                                      .release = device_release};

int init_module(void) {
  major = register_chrdev(0, DEVICE_NAME, &fops);

  if (major < 0) {
    return major;
  }

  printk(KERN_INFO "chardev registered, major: %d, minor: %d ", major, 0);
  return SUCCESS;
}

void cleanup_module(void) { unregister_chrdev(major, DEVICE_NAME); }

/*
 * Methods
 */

static int device_open(struct inode *inode, struct file *filp) {
  if (busy) return -EBUSY;

  busy++;
  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *filp) {
  busy--;
  return SUCCESS;
}

static ssize_t device_read(struct file *filp, char *buf, size_t len,
                           loff_t *off) {
  ssize_t ncopy;
  unsigned long ret;

  if (!LOAD) return 0;

  if (len < LOAD) {
    ret = copy_to_user(buf, BUF, len);
    ncopy = len - ret;
  } else {
    ret = copy_to_user(buf, BUF, LOAD);
    ncopy = LOAD - ret;
  }

  LOAD = 0;

  printk(KERN_INFO "ret: %lu, read: %d", ret, ncopy);
  return ncopy;
}

static ssize_t device_write(struct file *filp, const char *buf, size_t len,
                            loff_t *off) {
  ssize_t ncopy;
  unsigned long ret;

  if (LOAD) return -EBUSY;
  if (len > BUF_LEN) return -ENOSPC;

  ret = copy_from_user(BUF, buf, len);
  ncopy = len - ret;

  LOAD = ncopy;

  printk(KERN_INFO "ret: %lu, write: %d", ret, ncopy);
  return ncopy;
}
