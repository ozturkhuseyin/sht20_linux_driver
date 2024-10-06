#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define SHT20_MODE_PERIODIC 0
#define SHT20_MODE_ONESHOT 1

#define SHT20_IOCTL_READ_TEMP   _IOR('s', 1, int32_t)
#define SHT20_IOCTL_READ_HUM    _IOR('s', 2, int32_t)
#define SHT20_IOCTL_SET_MODE    _IOW('s', 3, int32_t *)     // Command to set mode
#define SHT20_IOCTL_READ_MODE   _IOR('s', 4, int32_t *)     // Command to read mode 

int main() 
{
    int32_t answer;
    int dev = open("/dev/sht20", O_RDWR); // Open with read and write permissions
    if (dev == -1) 
    {
        perror("Opening device failed"); // Use perror for better error messages
        return -1;
    }

    // Read temperature
    if (ioctl(dev, SHT20_IOCTL_READ_TEMP, &answer) == -1) 
    {
        perror("IOCTL read temp failed");
        close(dev);
        return -1;
    }
    printf("Temperature readed %d\n", answer);

    // Read hum
    if (ioctl(dev, SHT20_IOCTL_READ_HUM, &answer) == -1) 
    {
        perror("IOCTL read hum failed");
        close(dev);
        return -1;
    }
    printf("Hum readed %d\n", answer);

    // Read mode
    if (ioctl(dev, SHT20_IOCTL_READ_MODE, &answer) == -1) 
    {
        perror("IOCTL read mode failed");
        close(dev);
        return -1;
    }
    printf("Mode readed %d\n", answer);

    // Set mode
    answer = SHT20_MODE_PERIODIC;

    // Write new value
    if (ioctl(dev, SHT20_IOCTL_SET_MODE, &answer) == -1) 
    {
        perror("IOCTL write mode failed");
        close(dev);
        return -1;
    }

    usleep(10000 * 1000);

    // Read temperature
    if (ioctl(dev, SHT20_IOCTL_READ_TEMP, &answer) == -1) 
    {
        perror("IOCTL read temp failed");
        close(dev);
        return -1;
    }
    printf("Temperature readed %d\n", answer);

    // Read hum
    if (ioctl(dev, SHT20_IOCTL_READ_HUM, &answer) == -1) 
    {
        perror("IOCTL read hum failed");
        close(dev);
        return -1;
    }
    printf("Hum readed %d\n", answer);

    close(dev); // Close the device
    return 0;
}
