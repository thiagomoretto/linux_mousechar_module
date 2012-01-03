#include <linux/init.h>
// #include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */

// Favor 
// Usar com cautela.

MODULE_AUTHOR("Thiago Moretto <thiago@moretto.eng.br>");
MODULE_DESCRIPTION("eyeMouse/Joystick Kernel Linux interface.");
MODULE_LICENSE("GPL");

// http://www.xml.com/ldd/chapter/book/ch03.html

// Isto eu nao preciso
// Coloquei aqui para nao esquecer
// include <asm/irq.h>
// include <asm/io.h>
int joystick_open(struct inode *inode, struct file *filp);
int joystick_release(struct inode *inode, struct file *filp);
ssize_t joystick_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t joystick_write( struct file *filp, char *buf, size_t count, loff_t *f_pos);

// static DECLARE_MUTEX(fakejoy_sem);

struct joystick_device {
  struct input_dev *dev;
};

static struct joystick_device *jdev;

struct file_operations joystick_fops = {
read: joystick_read,
      write: joystick_write,
      open: joystick_open,
      release: joystick_release
};

// Major number
// $ mknod /dev/vcjs c 90 0
// $ mknod js0 c 13 0 , 13 é major dos joystick(?)
int major_number = 90;

int joystick_open(struct inode *inode, struct file *filp) {
  // MOD_INC_USE_COUNT;
  return 0;
}

int joystick_release(struct inode *inode, struct file *filp) {
  // MOD_DEC_USE_COUNT;
  return 0;
}

ssize_t joystick_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
  return 1;
}

ssize_t joystick_write(struct file *filp, char *buf,
    size_t count, loff_t *f_pos) {
  static char localbuf[64];
  int i, retval;

  if(count>64)
    count=64;

  printk(KERN_INFO "joystick.c: write\n");

  retval = copy_from_user(localbuf,buf,count);
  //if(!retval)
  //	return -EFAULT;

  for(i=0;i<count;i++) {
    switch(localbuf[i]) {
      case 'l':
        input_report_rel(jdev->dev, REL_X,-5);
        //  input_report_abs(jdev->dev, ABS_X,-5);
        break;
      case 'r':
        input_report_rel(jdev->dev, REL_X,+5);
        break;
      case 'u':
        input_report_rel(jdev->dev, REL_Y,-5);
        break;
      case 'd':
        input_report_rel(jdev->dev, REL_Y,+5);
        break;
      case 'z': // Right click
        input_report_key(jdev->dev, BTN_RIGHT, 1);
        break;
      case 'Z': // Right release
        input_report_key(jdev->dev, BTN_RIGHT, 0);
        break;
      case 'x': // Middlee press
        input_report_key(jdev->dev, BTN_MIDDLE, 1);
        break;
      case 'X': // Middle release
        input_report_key(jdev->dev, BTN_MIDDLE, 0);
        break;
      case 'c': // Left Press
        input_report_key(jdev->dev, BTN_LEFT, 1);
        break;
      case 'C': // Left Release
        input_report_key(jdev->dev, BTN_LEFT, 0);
        break;
    }
  }

  input_sync(jdev->dev);

  return count;
}

static int joystick_init(void) {
  int result;
  printk(KERN_INFO "joystick.c: init\n");

  // Registrando dispositivos de caracter
  result = register_chrdev(major_number, "vcjs", &joystick_fops);
  if(result < 0) {
    printk(KERN_INFO "joystick.c: can't register device\n");
    goto exit;
  }

  // Registrando dispositivo de entrada
  jdev = kmalloc(sizeof(struct joystick_device), GFP_KERNEL);
  if(!jdev) 
    return -ENOMEM;
  memset(jdev,0,sizeof(*jdev));

  jdev->dev = input_allocate_device();
  if(!jdev->dev)
    return -ENOMEM;

  // kfree(jdev->dev->name);
  jdev->dev->name = "eyeMouse/eyeJoystick";
  jdev->dev->id.bustype = BUS_GAMEPORT;
  jdev->dev->id.vendor = 0x0001;
  jdev->dev->id.product = 0x0003;
  jdev->dev->id.version = 0x0100;
  jdev->dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REL);
  jdev->dev->relbit[0] = BIT(REL_X) | BIT(REL_Y);
  jdev->dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y);
  jdev->dev->keybit[LONG(BTN_LEFT)] = BIT(BTN_LEFT) | BIT(BTN_MIDDLE) | BIT(BTN_RIGHT);

  // Associando os botoes
  set_bit(KEY_UP, jdev->dev->keybit); 
  set_bit(KEY_LEFT, jdev->dev->keybit);
  set_bit(KEY_RIGHT, jdev->dev->keybit);
  set_bit(KEY_DOWN, jdev->dev->keybit);

  set_bit(BTN_RIGHT, jdev->dev->keybit);
  set_bit(BTN_LEFT, jdev->dev->keybit);
  set_bit(BTN_MIDDLE, jdev->dev->keybit);

  // registrando device (falso, claro)
  input_register_device(jdev->dev);

exit:
  return result;
}

static void joystick_exit(void) {
  unregister_chrdev(major_number, "vcjs");
  input_unregister_device(jdev->dev);
  printk(KERN_INFO "joystick.c: exit\n");
}

module_init(joystick_init);
module_exit(joystick_exit);
