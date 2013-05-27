int led_init(int addr);
void led_fini(int fd);

void led_printf (int fd, const char *fmt, ...);

void led_addr_set (uint8_t oldaddr, uint8_t newaddr); /* not working */
void led_brightness_set (int fd, uint8_t val); /* untested */
void led_sleep_set (int fd, int val); /* untested */
uint8_t led_status_get (int fd); /* untested */
