/* docindexif.h - Interface file for the docindex for gimp.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __DOCINDEXIF_H__
#define __DOCINDEXIF_H__

#include <stdio.h>

struct bool_char_pair
{
  gboolean  boole;
  gchar    *string;
};

void   open_idea_window      (void);
void   open_or_raise         (gchar    *file_name);
void   open_file_in_position (gchar    *filename,
			      gint      position);
void   clear_white           (FILE     *fp);
gint   getinteger            (FILE     *fp);

#endif /* __DOCINDEXIF_H__ */
