#ifndef __ORIENTATION_H
#define __ORIENTATION_H

#define NUMORIENTRADIO 8

enum ORIENTATION_ENUM
{
    ORIENTATION_VALUE = 0,
    ORIENTATION_RADIUS = 1,
    ORIENTATION_RANDOM = 2,
    ORIENTATION_RADIAL = 3,
    ORIENTATION_FLOWING = 4,
    ORIENTATION_HUE = 5,
    ORIENTATION_ADAPTIVE = 6,
    ORIENTATION_MANUAL = 7,
};

void create_orientationpage (GtkNotebook *);
void orientation_restore (void);
int orientation_type_input (int in);

#endif /* #ifndef __ORIENTATION_H */
