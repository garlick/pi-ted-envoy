char *temp_serialize (double c, double fr, double fz);
bool temp_deserialize (const char *s, double *cp, double *frp, double *fzp);
char *ted_serialize (int a, int c, int w, int v);
bool ted_deserialize (const char *s, int *ap, int *cp, int *wp, int *vp);
char *key_serialize (int n);
bool key_deserialize (const char *s, int *np);
bool envoy_deserialize (const char *s, int *lp, int *wp, int *dp, int *cp);
