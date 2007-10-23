#ifndef __BRUSH_H
#define __BRUSH_H

#include "ppmtool.h"

void brush_store(void);
void brush_restore(void);
void brush_free(void);
void create_brushpage(GtkNotebook *);
void brush_get_selected (ppm_t *p);

#endif /* #ifndef __BRUSH_H */
