/**
 * Linux device driver for the Versalogic VCM-DAS-1 IO module.
 *
 * Based on code from the DSINCA/FDC project written by Guilherme A. S. Pereira
 * and Armando Alves Neto.
 */


#include <asm/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/types.h>


MODULE_AUTHOR("Dimas Abreu Dutra <dimasad@ufmg.br>");
MODULE_DESCRIPTION("device driver for the Versalogic VCM-DAS-1 IO module");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");


static unsigned base_address = 0x3E0;
module_param(base_address, uint, S_IRUGO);


/// Number of IO ports used
#define PORT_RANGE 16

// Board register offsets
#define ADCSTAT 0x00
#define CONTROL 0x00
#define SELECT          0x01    /*  Write port */
#define CONVERT         0x02    /*  Write port */
#define TDELAY          0x03    /*  Write port */
#define ADCLO           0x04    /*  Read only port */
#define ADCHI           0x05    /*  Read only port */
#define DIGLO           0x06    /*  Digital low - read/write */
#define DIGHI           0x07    /*  Digital high - read/write */
#define SERCS           0x08    /*  Serial Chip select - write */
#define SERSTAT         0x09    /*  Serial status - read port */
#define SERDATA         0x09    /*  Serial data send and clock - write port */

// Register bit masks
#define DONE_BIT        0x40    /* bit mask for A/D conversion complete */
#define BUSY_BIT        0x80    /* bit mask for A/D converion busy */
#define DA              0x01    /* bit mask for D/A chip select */
#define DPOT            0x02    /* bit mask for DPOT chip select */
#define EEPROM          0x04    /* bit mask for EEPROM chip select */


static inline u8 vcmdas1_conversion_done(void) {
    return inb_p(base_address + ADCSTAT) & DONE_BIT;
}


static inline u8 vcmdas1_analog_read(u8 channel, s16* value) {
    u16 count;
    
    outb(channel, base_address + SELECT); //select channel
    outb(1, base_address + CONVERT);

    for(count=0; !vcmdas1_conversion_done() && count < 8000; count++);
    
    if (vcmdas1_conversion_done()) {
        printk(KERN_DEBUG "h: %d\n", (int) inb(base_address + ADCHI));
        *value = ((u16)inb(base_address + ADCLO) 
                  | ((u16)inb(base_address + ADCHI) << 8));
        return 0;
    } else {
        return 1;
    }
}


static int __init vcmdas1_init(void) {
    u8 channel;
    
    if (!request_region(base_address, PORT_RANGE, "vcmdas1")) {
        printk(KERN_ERR "unable to reserve IO port range (%u + %d)",
               base_address, PORT_RANGE);
        return -EADDRINUSE;
    }

    // Reset the control register
    outb_p(0, base_address + CONTROL);
    
    for (channel=0; channel<16; channel++) {
        s16 value = 0;
        u8 status = vcmdas1_analog_read(channel, &value);
        printk(KERN_INFO "channel %d: status=%d val=%d \n",
               (int)channel, (int)status, (int)value);
    }
    
    return 0;
}


static void __exit vcmdas1_exit(void) {
    release_region(base_address, PORT_RANGE);
}


module_init(vcmdas1_init);
module_exit(vcmdas1_exit);
