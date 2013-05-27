void oled_clear(int fd);
void oled_cursor_set (int fd, bool val);
void oled_sleep_set (int fd, bool val);

void oled_text_pos_set (int fd, uint8_t x, uint8_t y);
void oled_printf (int fd, const char *fmt, ...);

void oled_addr_set (int oldaddr, int newaddr);

int oled_init(int addr);
void oled_fini(int fd);

#define OLED_TEXT_COL	32
