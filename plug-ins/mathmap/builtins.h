
double color_to_double (int red, int green, int blue);
void double_to_color (double val, int *red, int *green, int *blue);

void builtin_sin (void *arg);
void builtin_cos (void *arg);
void builtin_tan (void *arg);
void builtin_asin (void *arg);
void builtin_acos (void *arg);
void builtin_atan (void *arg);
void builtin_abs (void *arg);
void builtin_sign (void *arg);
void builtin_min (void *arg);
void builtin_max (void *arg);
void builtin_or (void *arg);
void builtin_and (void *arg);
void builtin_less (void *arg);
void builtin_inintv (void *arg);
void builtin_rand (void *arg);
void builtin_origValXY (void *arg);
void builtin_origValXYIntersample (void *arg);
void builtin_origValRA (void *arg);
void builtin_origValRAIntersample (void *arg);
void builtin_red (void *arg);
void builtin_green (void *arg);
void builtin_blue (void *arg);
void builtin_rgbColor (void *arg);
void builtin_grayColor (void *arg);
