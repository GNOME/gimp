#ifndef __INFILE_H
#define __INFILE_H

#include "ppmtool.h"

/* Globals */

extern gboolean img_has_alpha;

/* Prototypes */

void infile_copy_to_ppm(ppm_t * p);
void infile_copy_alpha_to_ppm(ppm_t * p);

#endif
