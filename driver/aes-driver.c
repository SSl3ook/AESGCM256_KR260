/**
 * AES256GCM10G25GIP Kernel Driver with ioctl support
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "aes256gcm10g25g"
#define DRIVER_DESC "Driver for AES256GCM10G25GIP hardware"
#define DEVICE_NAME "aes256gcm"

/* Define ioctl commands */
#define AES_IOC_MAGIC 'a'
#define AES_IOC_READ_REG   _IOR(AES_IOC_MAGIC, 1, struct aes_reg_data)
#define AES_IOC_WRITE_REG  _IOW(AES_IOC_MAGIC, 2, struct aes_reg_data)

/* Structure for register access */
struct aes_reg_data {
    uint32_t offset;    /* Register offset */
    uint64_t value;     /* Value to read or write */
    uint8_t width;      /* Access width in bits: 8, 16, 32, 64 */
};

/* Device private data structure */
struct aes_dev {
    void __iomem *regs;         /* Virtual address for registers */
    struct resource *res;       /* Device resources */
    struct device *dev;         /* Device structure */
    struct cdev cdev;           /* Character device structure */
    dev_t devt;                 /* Device number */
};

/* Global variables */
static struct class *aes_class;
static int aes_major;

/* File operations */
static int aes_open(struct inode *inode, struct file *file)
{
    struct aes_dev *aes;
    
    aes = container_of(inode->i_cdev, struct aes_dev, cdev);
    file->private_data = aes;
    
    return 0;
}

static int aes_release(struct inode *inode, struct file *file)
{
    return 0;
}

// function for ioctl system call
static long aes_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct aes_dev *aes = file->private_data;
    struct aes_reg_data reg;
    
    switch (cmd) {
    case AES_IOC_READ_REG:
        if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)))
            return -EFAULT;
        
        /* Validate register offset */
        if (reg.offset >= resource_size(aes->res)) {
            dev_err(aes->dev, "Invalid register offset: 0x%x\n", reg.offset);
            return -EINVAL;
        }
        
        /* Read register value based on width */
        switch (reg.width) {
        case 8:
            reg.value = readb(aes->regs + reg.offset);
            break;
        case 16:
            reg.value = readw(aes->regs + reg.offset);
            break;
        case 32:
        default:
            reg.value = readl(aes->regs + reg.offset);
            break;
        case 64:
            reg.value = readq(aes->regs + reg.offset);
            break;
        }
        
        if (copy_to_user((void __user *)arg, &reg, sizeof(reg)))
            return -EFAULT;
        break;
        
    case AES_IOC_WRITE_REG:
        if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)))
            return -EFAULT;
        
        /* Validate register offset */
        if (reg.offset >= resource_size(aes->res)) {
            dev_err(aes->dev, "Invalid register offset: 0x%x\n", reg.offset);
            return -EINVAL;
        }
        
        /* Write register value based on width */
        switch (reg.width) {
        case 8:
            writeb(reg.value, aes->regs + reg.offset);
            break;
        case 16:
            writew(reg.value, aes->regs + reg.offset);
            break;
        case 32:
        default:
            writel(reg.value, aes->regs + reg.offset);
            break;
        case 64:
            writeq(reg.value, aes->regs + reg.offset);
            break;
        }
        break;
        
    default:
        return -ENOTTY;
    }
    
    return 0;
}

static const struct file_operations aes_fops = {
    .owner          = THIS_MODULE,
    .open           = aes_open,
    .release        = aes_release,
    .unlocked_ioctl = aes_ioctl,
};

/* Probe function - called when device is detected */
static int aes_probe(struct platform_device *pdev)
{
    struct aes_dev *aes;
    struct resource *res;
    int ret = 0;
    dev_t dev;

    /* Print information when device is detected */
    dev_info(&pdev->dev, "AES256GCM10G25GIP device detected\n");

    /* Allocate driver private data (common)*/
    aes = devm_kzalloc(&pdev->dev, sizeof(struct aes_dev), GFP_KERNEL);
    if (!aes)
        return -ENOMEM;

    aes->dev = &pdev->dev;

    /* Get memory resource for the device (register space) */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "Failed to get memory resource\n");
        return -ENODEV;
    }

    /* Request and remap the memory region */
    aes->res = res;
    aes->regs = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(aes->regs)) {
        dev_err(&pdev->dev, "Failed to map registers\n");
        return PTR_ERR(aes->regs);
    }

    /* Create character device */
    dev = MKDEV(aes_major, 0);
    cdev_init(&aes->cdev, &aes_fops);
    aes->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&aes->cdev, dev, 1);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add character device\n");
        return ret;
    }
    
    /* Create device node */
    if (aes_class) {
        aes->devt = dev;
        device_create(aes_class, &pdev->dev, dev, aes, DEVICE_NAME);
    }

    /* Store driver data for later use */
    platform_set_drvdata(pdev, aes);

    /* Print resource information */
    dev_info(&pdev->dev, "AES256GCM10G25GIP registered at physical address 0x%llx, virtual address 0x%p\n",
             (unsigned long long)res->start, aes->regs);
    dev_info(&pdev->dev, "Register space size: 0x%llx\n", 
             (unsigned long long)(res->end - res->start + 1));
    dev_info(&pdev->dev, "Created device node /dev/%s\n", DEVICE_NAME);

    return ret;
}

/* Remove function - called when device is removed */
static int aes_remove(struct platform_device *pdev)
{
    struct aes_dev *aes = platform_get_drvdata(pdev);
    
    /* Remove device node and character device */
    device_destroy(aes_class, aes->devt);
    cdev_del(&aes->cdev);
    
    dev_info(&pdev->dev, "AES256GCM10G25GIP device removed\n");
    return 0;
}

/* Compatible device table - must match the "compatible" string in DTS */
static const struct of_device_id aes_of_match[] = {
    { .compatible = "DG,AES256GCM10G25GIP" },
    { }
};
MODULE_DEVICE_TABLE(of, aes_of_match);

/* Platform driver structure */
static struct platform_driver aes_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = aes_of_match,
    },
    .probe = aes_probe,
    .remove = aes_remove,
};

/* Module initialization */
static int __init aes_init(void)
{
    int ret;
    dev_t dev;
    
    /* Allocate a character device region */
    ret = alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME);
    if (ret) {
        pr_err("Failed to allocate character device region\n");
        return ret;
    }
    
    aes_major = MAJOR(dev);
    
    /* Create device class */
    aes_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(aes_class)) {
        pr_err("Failed to create device class\n");
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(aes_class);
    }
    
    /* Register platform driver */
    ret = platform_driver_register(&aes_driver);
    if (ret) {
        pr_err("Failed to register platform driver\n");
        class_destroy(aes_class);
        unregister_chrdev_region(dev, 1);
        return ret;
    }
    
    return 0;
}

/* Module cleanup */
static void __exit aes_exit(void)
{
    platform_driver_unregister(&aes_driver);
    class_destroy(aes_class);
    unregister_chrdev_region(MKDEV(aes_major, 0), 1);
}

module_init(aes_init);
module_exit(aes_exit);

/* Module information */
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("DG");
MODULE_LICENSE("GPL");