
#ifndef SIOD_WRAPPER_H
#define SIOD_WRAPPER_H

#include <stdio.h>
#include <glib.h>

void  siod_init (gboolean register_scripts);

FILE *siod_get_output_file (void);
void  siod_set_output_file (FILE *file);

int   siod_get_verbose_level (void);
void  siod_set_verbose_level (int verbose_level);

void  siod_print_welcome (void);

const char *siod_get_error_msg   (void);
const char *siod_get_success_msg (void);

/* if the return value is 0, success. error otherwise. */
int   siod_interpret_string (const char *expr);


#endif /* SIOD_WRAPPER_H */
