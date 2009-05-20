#ifndef DEVICE_H_
#define DEVICE_H_

#include <glib.h>
#include <gps.h>

extern gboolean check_device_files();

extern int usart_open(U4 baud_rate, gboolean verify_output);
extern gboolean usart_init();
extern void usart_flush_output();
extern void usart_cleanup();
extern void usart_close();

extern inline int read_check_gps_power(U1 *buf, int len);
extern inline int write_check_gps_power(U1 *buf, int len);
extern gboolean read_fixed_len(U1 *buf, int expected_len);

extern int sysfs_get_gps_device_power();
extern gboolean gps_device_power_on();

#endif /* DEVICE_H_ */
