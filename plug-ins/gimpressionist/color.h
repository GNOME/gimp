#ifndef __COLOR_H
#define __COLOR_H

enum COLOR_TYPE_ENUM
{
    COLOR_TYPE_AVERAGE = 0,
    COLOR_TYPE_CENTER = 1,
};

void create_colorpage(GtkNotebook *);
void color_restore(void);

#endif /* #ifndef __COLOR_H */

