/**
 * Linux device driver for the Versalogic VCM-DAS-1 IO module.
 *
 * Based on code from the DSINCA/FDC project written by Guilherme A. S. Pereira
 * and Armando Alves Neto.
 */


#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/types.h>


MODULE_AUTHOR("Dimas Abreu Dutra <dimasad@ufmg.br>");
MODULE_DESCRIPTION("device driver for the Versalogic VCM-DAS-1 IO module");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");


#define BASE_ADDRESS 0x3E0 ///<Board base address

#define STATUS          0x00    /*  Read port */
#define CONTROL         0x00    /*  Write port */
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
#define PORT_RANGE      0x0A    /*  Number of IO ports used */

#define DONE_BIT        0x40    /* bit mask for A/D conversion complete */
#define BUSY_BIT        0x80    /* bit mask for A/D converion busy */
#define DA              0x01    /* bit mask for D/A chip select */
#define DPOT            0x02    /* bit mask for DPOT chip select */
#define EEPROM          0x04    /* bit mask for EEPROM chip select */


/**
 * Resets the VCM-DAS-1 analog and digital IO circuits. The card is restored
 * to its power-on reset state as summarized below:
 *
 *  ANALOG INPUT                            ANALOG OUTPUT
 *  ------------                            -------------
 *  - Channel 0 is selected                 - All Channels set to 0 Volts
 *  - Auto Increment Disabled
 *  - Auto Trigger Disabled                 PARALLEL I/O
 *  - Scan Range Limit = Unrestricted       ------------
 *  - A/D Interrupts Disabled               - All Channels set to Input
 *  - A/D Interrupt Request Cleared         - Parallel Interrupts Disabled
 *                                          - Parallel Interrupt Request Cleared
 *  EEPROM
 *  ------
 *  - Enables writes to the EEPROM
 */
static inline void vcmdas1_reset(void) {
    outb(0, BASE_ADDRESS + CONTROL);
    outb(0, BASE_ADDRESS);
}


static inline u8 vcmdas1_conversion_done(void) {
    return inb(BASE_ADDRESS + STATUS) & DONE_BIT;
}


static inline u8 vcmdas1_analog_read(u8 channel, s16* value) {
    u16 count;
    
    outb(channel, BASE_ADDRESS + SELECT); //select channel
    outb(1, BASE_ADDRESS + CONVERT);

    for(count=0; !vcmdas1_conversion_done() && count < 8000; count++);
    
    if (vcmdas1_conversion_done()) {
        printk(KERN_DEBUG "h: %d\n", (int) inb(BASE_ADDRESS + ADCHI));
        *value = ((u16)inb(BASE_ADDRESS + ADCLO) 
                  | ((u16)inb(BASE_ADDRESS + ADCHI) << 8));
        return 0;
    } else {
        return 1;
    }
}


static int vcmdas1_init(void) {
    u8 channel;
    
    if (!request_region(BASE_ADDRESS, PORT_RANGE, "vcmdas1"))
        return -1;
    
    vcmdas1_reset();
    
    for (channel=0; channel<16; channel++) {
        s16 value;
        u8 status = vcmdas1_analog_read(channel, &value);
        printk(KERN_INFO "channel %d: status=%d val=%d \n",
               (int)channel, (int)status, (int)value);
    }
    
    return 0;
}


static void vcmdas1_exit(void) {
    release_region(BASE_ADDRESS, PORT_RANGE);
}


module_init(vcmdas1_init);
module_exit(vcmdas1_exit);
