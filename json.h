int ted_serialize (char *s, int len, int addr, int count, int watts, int volts);
int ted_deserialize (char *s, int *addrp, int *countp, int *wattsp, int *voltsp);
int envoy_deserialize (char *s, int *lp, int *wp, int *dp, int *cp);
