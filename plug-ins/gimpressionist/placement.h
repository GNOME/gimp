#ifndef __PLACEMENT_H
#define __PLACEMENT_H

enum PLACEMENT_TYPE_ENUM
{
    PLACEMENT_TYPE_RANDOM = 0,
    PLACEMENT_TYPE_EVEN_DIST = 1,
};

void place_store (void);
void place_restore (void);
void create_placementpage (GtkNotebook *);
int place_type_input (int in);

#endif
