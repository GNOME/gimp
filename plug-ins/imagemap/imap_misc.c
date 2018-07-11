/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "imap_main.h"
#include "imap_misc.h"

#define SASH_SIZE 8

static gint _sash_size = SASH_SIZE;

void
set_sash_size(gboolean double_size)
{
   _sash_size = (double_size) ? 2 * SASH_SIZE : SASH_SIZE;
}

void
draw_sash(cairo_t *cr, gint x, gint y)
{
   draw_rectangle(cr, TRUE, x - _sash_size / 2, y - _sash_size / 2,
                  _sash_size, _sash_size);
}

gboolean
near_sash(gint sash_x, gint sash_y, gint x, gint y)
{
   return x >= sash_x - _sash_size / 2 && x <= sash_x + _sash_size / 2 &&
      y >= sash_y - _sash_size / 2 && y <= sash_y + _sash_size / 2;
}
