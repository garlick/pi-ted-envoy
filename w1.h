/* Read 1-wire therm sensor and return degrees C.
 * On error reurn NAN and set errno.
 */
double w1_therm_get (const char *addr);

/* Convert Celcuis to Farenheit.
 */
double c2f (double c);
