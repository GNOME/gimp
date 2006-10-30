#ifndef __SIZE_H
#define __SIZE_H

enum SIZE_TYPE_ENUM
{
    SIZE_TYPE_VALUE = 0,
    SIZE_TYPE_RADIUS = 1,
    SIZE_TYPE_RANDOM = 2,
    SIZE_TYPE_RADIAL = 3,
    SIZE_TYPE_FLOWING = 4,
    SIZE_TYPE_HUE = 5,
    SIZE_TYPE_ADAPTIVE = 6,
    SIZE_TYPE_MANUAL = 7,
};

void size_restore (void);

void create_sizepage (GtkNotebook *);

int size_type_input (int in);
void size_map_free_resources (void);

#endif /* #ifndef __SIZE_H */
