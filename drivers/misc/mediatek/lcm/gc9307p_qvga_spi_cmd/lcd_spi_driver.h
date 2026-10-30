
#ifndef __LCD_SPI_DRIVER_H__
#define __LCD_SPI_DRIVER_H__

extern void mt_lcd_spi_gpio_init(void);
extern void mt_lcd_spi_write_com(unsigned char i);
extern void mt_lcd_spi_write_data(unsigned char i);
extern void lcd_spi_lock(void);
extern void lcd_spi_unlock(void);
extern void spi_lcm_update(u16 x, u16 y, u16 w, u16 h);
extern void lcm_spi_reset(int state);
#endif
