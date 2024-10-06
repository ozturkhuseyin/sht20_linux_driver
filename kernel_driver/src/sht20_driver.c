#include "sht20_driver.h"


// ---------- Kernel module ----------
//#define SHT20_DRIVER_MAJOR  1       // I used dynamic alloc, this is not required
#define SHT20_DRIVER_MAX_MINORS 1

// ---------- Driver ----------
struct sht20_device_data
{
    struct cdev cdev;
    struct i2c_client *client;
    struct class *cls;
    int major;

    // Kernel thread
    struct task_struct *thread;
    //struct mutex lock;            // No need to mutex, i use just one thread

    // SHT20 sensor configs
    int is_connected;
    int read_mode;
    int read_period_ms;

    // SHT20 sensor data
    int temp_read_count;
    int hum_read_count;
    int temperature;
    int hum;
};

// driver handle
static struct sht20_device_data sht20_dev;

// ---------- Function proto ----------
static int __init sht20_driver_init(void);
static void __exit sht20_driver_exit(void);
static int sht20_open(struct inode *inode, struct file *file);
static int sht20_release(struct inode *inode, struct file *file);
static int sht20_read(struct file *, char __user *buf, size_t, loff_t *offset);
static ssize_t sht20_write(struct file *filp, const char __user *buff, size_t len, loff_t *off);
static long sht20_ioctl (struct file *file, unsigned int cmd, unsigned long arg);
static int sht20_probe(struct i2c_client *client);
static void sht20_remove(struct i2c_client *client);

void *sht20_thread_func(void *data);
static int sht20_read_thread(void *data);

// ---------- Character device ----------
/**
 * Guard for access from multiple destinations
 */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);
static atomic_t stop_thread = ATOMIC_INIT(1);

/**
 * File operations for character device
 */
const struct file_operations sht20_driver_fops = {
    .owner = THIS_MODULE,
    .open = sht20_open,
    .release = sht20_release,
    .read = sht20_read,
    .write = sht20_write,
    .unlocked_ioctl = sht20_ioctl
};


// ---------- IOCTL ----------
// ioctl
#define MY_IOCTL_IN _IOC(_IOC_WRITE, 'k', 1, sizeof(struct sht20_ioctl_data))
#define WR_VALUE _IOW('a', 'a', int32_t *)
#define RD_VALUE _IOR('a', 'b', int32_t *)

#define SHT20_IOCTL_READ_TEMP       _IOR('s', 1, int32_t)
#define SHT20_IOCTL_READ_HUM        _IOR('s', 2, int32_t)

#define SHT20_IOCTL_SET_MODE        _IOW('s', 3, int32_t *)     // Command to set mode
#define SHT20_IOCTL_READ_MODE       _IOR('s', 4, int32_t *)     // Command to read mode 

#define SHT20_IOCTL_SET_PERIOD      _IOW('s', 5, int32_t *)     // Command to set period in ms
#define SHT20_IOCTL_READ_PERIOD     _IOR('s', 6, int32_t *)     // Command to read period in ms


// ---------- I2C client ----------
// Device ID table
static const struct i2c_device_id sht20_id[] = {
    { "sht20", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sht20_id);

static struct i2c_driver sht20_driver = 
{
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
    },

    .probe = sht20_probe,
    .remove = sht20_remove,
    .id_table = sht20_id,
};


/**************************************
 * 
 *  CHAR DEVICE FILE OPS.
 * 
 *************************************/
static int sht20_open(struct inode *inode, struct file *file)
{
    pr_info("sht20_open executed.\n");

    // Check if the device is already open
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
        pr_err("sht20_open device already opened.\n");
        return -EBUSY;
    }

    return 0; // Success
}

static int sht20_release(struct inode *inode, struct file *file)
{
    pr_info("sht20_release executed.\n");

    atomic_set(&already_open, CDEV_NOT_USED); // Ready for the next caller

    return 0;
}

static int sht20_read(struct file *, char __user *buf, size_t, loff_t *offset)
{
    pr_info("sht20_read executed.\n");

    //sht20_read_temperature(sht20_dev.client, &sht20_dev.temperature);

    return 0;
}

static ssize_t sht20_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{   
    //pr_info("sht20_write executed.\n");
    pr_err("sht20_write is not supported.\n");

    return -EINVAL;
}

int32_t answer = 42;

static long sht20_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    // Use the global sht20_dev object directly
    struct sht20_device_data *my_data = &sht20_dev;

    if (!my_data) {
        pr_err("sht20_ioctl: sht20_dev is NULL\n");
        return -EINVAL;
    }

    if (SHT20_MODE_PERIODIC == my_data->read_mode)
    {
        // stop thread
        atomic_set(&stop_thread, 1);
    }

    switch(cmd) 
    {
    // case WR_VALUE:
	// 	if(copy_from_user(&answer, (int32_t *) arg, sizeof(answer))) 
	// 	{
    //         printk("ioctl_example - Error copying data from user!\n");
    //     }
	// 	else
    //     {
    //         printk("ioctl_example - Update the answer to %d\n", answer);
    //     }
		
    //     break;

    // case RD_VALUE:
    //     if(copy_to_user((int32_t *) arg, &answer, sizeof(answer))) 
    //     {
    //         printk("ioctl_example - Error copying data to user!\n");
    //     }
	// 	else
    //     {
    //         printk("ioctl_example - The answer was copied!\n");
    //     }
				
	// 	break;

    case SHT20_IOCTL_READ_TEMP:
        // get data and if needed get its average
        if (SHT20_MODE_PERIODIC == my_data->read_mode)
        {
            // TODO: should i return average or sum of all reads and read count ??
            my_data->temperature = my_data->temperature / my_data->temp_read_count;
        }
        else
        {
            if (0 != sht20_read_temperature(my_data->client, &my_data->temperature)) 
            {
                pr_err("sht20_ioctl SHT20_IOCTL_READ_TEMP failed to read temperature\n");
                return -EFAULT;
            }
        }


        if (copy_to_user((int32_t *)arg, &my_data->temperature, sizeof(my_data->temperature)))
        {
            pr_err("sht20_ioctl SHT20_IOCTL_READ_TEMP err while copy to user.\n");
            return -EFAULT;
        }

        if (SHT20_MODE_PERIODIC == my_data->read_mode)
        {
            // reset
            sht20_dev.temperature = 0;  
            sht20_dev.temp_read_count = 0;
        }
        
        break;

    case SHT20_IOCTL_READ_HUM:
        // get data and if needed get its average
        if (SHT20_MODE_PERIODIC == my_data->read_mode)
        {
            // TODO: should i return average or sum of all reads and read count ??
            my_data->hum = my_data->hum / my_data->hum_read_count;
        }
        else
        {
            if (0 != sht20_read_hum(my_data->client, &my_data->hum)) 
            {
                pr_err("sht20_ioctl SHT20_IOCTL_READ_HUM failed to read humidity\n");
                return -EFAULT;
            }
        }

        if (copy_to_user((int32_t *)arg, &my_data->hum, sizeof(my_data->hum))) 
        {
            pr_err("sht20_ioctl SHT20_IOCTL_READ_HUM err while copy to user.\n");
            return -EFAULT;
        }

        if (SHT20_MODE_PERIODIC == my_data->read_mode)
        {
            // reset
            sht20_dev.hum = 0;  
            sht20_dev.hum_read_count = 0;
        }
        break;

    case SHT20_IOCTL_SET_MODE:

        if (copy_from_user(&my_data->read_mode, (int32_t *)arg, sizeof(my_data->read_mode))) 
        {
            pr_err("sht20_ioctl SHT20_IOCTL_SET_MODE err while copy from user.\n");
            return -EFAULT; // Handle user copy error
        }

        pr_info("sht20_ioctl SHT20_IOCTL_SET_MODE new mode set to %d\n",my_data->read_mode);

        if (!((SHT20_MODE_PERIODIC == my_data->read_mode) || (SHT20_MODE_ONESHOT == my_data->read_mode)))
        {
            my_data->read_mode = SHT20_MODE_ONESHOT;
            pr_err("sht20_ioctl SHT20_IOCTL_SET_MODE wrong mode selection given, mode is set to default.\n");
            return -EFAULT; // Handle user copy error
        }
        break;
    
    case SHT20_IOCTL_READ_MODE:

        if (copy_to_user((int32_t *)arg, &my_data->read_mode, sizeof(my_data->read_mode)))
        {
            pr_err("sht20_ioctl SHT20_IOCTL_READ_MODE err while copy to user.\n");
            return -EFAULT; // Handle user copy error
        }

        pr_info("sht20_ioctl SHT20_IOCTL_READ_MODE user read mode %d\n",my_data->read_mode);
        break;

    //case SHT20_IOCTL_SET_CONFIG:
        // Copy configuration from user space
        //if (copy_from_user(&my_data->read_cfg, (struct sht20_read_config_t *)arg, sizeof(my_data->read_cfg))) {
        //    return -EFAULT;
        //}
        // Apply the new configuration as needed
        //break;

    case SHT20_IOCTL_SET_PERIOD:
        if (copy_from_user(&my_data->read_period_ms, (int32_t *)arg, sizeof(my_data->read_period_ms))) 
        {
            pr_err("sht20_ioctl SHT20_IOCTL_SET_PERIOD err while copy from user.\n");
            return -EFAULT; // Handle user copy error
        }

        pr_info("sht20_ioctl SHT20_IOCTL_SET_PERIOD new read period set to %d\n", my_data->read_period_ms);
        break;

    case SHT20_IOCTL_READ_PERIOD:
        if (copy_to_user((int32_t *)arg, &my_data->read_period_ms, sizeof(my_data->read_period_ms)))
        {
            pr_err("sht20_ioctl SHT20_IOCTL_READ_PERIOD err while copy to user.\n");
            return -EFAULT; // Handle user copy error
        }

        pr_info("sht20_ioctl SHT20_IOCTL_READ_PERIOD user read period in ms %d\n", my_data->read_period_ms);
        break;

    default:
        return -ENOTTY;
    }

    if (SHT20_MODE_PERIODIC == my_data->read_mode)
    {
        // start thread
        atomic_set(&stop_thread, 0);
    }

    return 0;
}

/**************************************
 * 
 *  KERNEL MODULE PROBE & REMOVE
 * 
 *************************************/
static int sht20_probe(struct i2c_client *client)
{
    pr_info("sht20_probe driver probe executed.\n");

    // Register char device with dynamic major number
    sht20_dev.major = register_chrdev(0, DEVICE_NAME, &sht20_driver_fops);
    if (sht20_dev.major < 0) {
        pr_err("sht20_probe: registering char device failed with %d.\n", sht20_dev.major);
        return sht20_dev.major; // Return error code if registration fails
    }
    pr_info("sht20_probe: assigned major number %d.\n", sht20_dev.major);

    // Initialize cdev structure
    cdev_init(&sht20_dev.cdev, &sht20_driver_fops);
    
    // Create device class
    sht20_dev.cls = class_create(DEVICE_NAME);  // Corrected to use only one argument
    if (IS_ERR(sht20_dev.cls)) {
        pr_err("sht20_probe: failed to create device class.\n");
        unregister_chrdev(sht20_dev.major, DEVICE_NAME); // Clean up on error
        return PTR_ERR(sht20_dev.cls);
    }

    // Create device
    if (IS_ERR(device_create(sht20_dev.cls, NULL, MKDEV(sht20_dev.major, 0), NULL, DEVICE_NAME))) {
        pr_err("sht20_probe: failed to create device.\n");
        class_destroy(sht20_dev.cls); // Clean up class
        unregister_chrdev(sht20_dev.major, DEVICE_NAME); // Clean up on error
        return -ENOMEM;
    }

    pr_info("sht20_probe: device created on /dev/%s\n", DEVICE_NAME);

    // init default config
    sht20_dev.read_mode = SHT20_MODE_ONESHOT;
    sht20_dev.read_period_ms = 1000;
    atomic_set(&stop_thread, 1);

    // start thread
    sht20_dev.thread = kthread_run(sht20_read_thread, NULL, "sht20_reader");
    if (IS_ERR(sht20_dev.thread)) {
        pr_err("sht20_probe: Failed to create kernel thread\n");
        return PTR_ERR(sht20_dev.thread);
    }


    return 0;
}

static void sht20_remove(struct i2c_client *client)
{
    pr_info("sht20_remove driver remove executed.\n");

    // TODO: this doesnt work ????
    if (0 > kthread_stop(sht20_dev.thread))
    {
        pr_err("sht20_remove thread stop err\n");
    }

    device_destroy(sht20_dev.cls, MKDEV(sht20_dev.major, 0)); // Destroy the device
    class_destroy(sht20_dev.cls); // Destroy the class
    unregister_chrdev_region(MKDEV(sht20_dev.major, 0), SHT20_DRIVER_MAX_MINORS); // Unregister device region
}


/**************************************
 * 
 *  KERNEL MODULE INIT & EXIT
 * 
 *************************************/
static int __init sht20_driver_init(void)
{
    pr_info("sht20_driver_init executed.\n");

    struct i2c_adapter *adapter;
    struct i2c_board_info info;

    // Get specified I2C adapter
    adapter = i2c_get_adapter(DEVICE_I2C_BUS_NUMBER);
    if (NULL == adapter)
    {
        pr_err("sht20_driver_init failed to get I2C adapter.\n");
        return -ENODEV;
    }

    // Fill out the i2c_board_info struct with device details
    memset(&info, 0, sizeof(struct i2c_board_info));
    strlcpy(info.type, DEVICE_NAME, I2C_NAME_SIZE);  // Must match the id_table entry
    info.addr = SHT20_I2C_ADDRESS;
    
    // Create i2c device
    sht20_dev.client = i2c_new_client_device(adapter, &info);
    if (NULL == sht20_dev.client) 
    {
        pr_err("sht20_driver_init failed to create I2C client for SHT20.\n");
        i2c_put_adapter(adapter);

        return -ENODEV;
    }

    // Add driver
    return i2c_add_driver(&sht20_driver);
}

static void __exit sht20_driver_exit(void)
{
    pr_info("sht20_driver_exit\n");

    i2c_unregister_device(sht20_dev.client);
    i2c_del_driver(&sht20_driver);
}

/**************************************
 * 
 *  PROBE BACKGROUND TASK
 * 
 *************************************/
static int sht20_read_thread(void *data) 
{
    int curr_temp;
    int curr_hum;

    while (!atomic_read(&stop_thread)) 
    {
        struct sht20_device_data *my_data = &sht20_dev;

        if (my_data->read_mode == SHT20_MODE_PERIODIC)
        {
            // Read temperature
            if (0 != sht20_read_temperature(my_data->client, &curr_temp)) 
            {
                pr_err("sht20_read_thread: failed to read temperature\n");
            }
            else
            {
                my_data->temperature += curr_temp;
                my_data->temp_read_count++;
                pr_info("sht20_read_thread: count:%d temp:%d", my_data->temp_read_count, curr_temp);
            }

            // Read humidity
            if (0 != sht20_read_hum(my_data->client, &curr_hum)) 
            {
                pr_err("sht20_read_thread: failed to read humidity\n");
            }
            else
            {
                my_data->hum += curr_hum;
                my_data->hum_read_count++;
                pr_info("sht20_read_thread: count:%d hum:%d", my_data->hum_read_count, curr_hum);
            }

            msleep(my_data->read_period_ms);
        }
    }

    return 0;
}



/**************************************
 * 
 *  KERNEL MODULE
 * 
 *************************************/
module_init(sht20_driver_init);
module_exit(sht20_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huseyin Ozturk");
MODULE_DESCRIPTION("A kernel module for sht20 temperature and humidty sensor");
MODULE_VERSION("1.0");