/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "paint-types.h"

#include "paint.h"
#include "gimpairbrush.h"
#include "gimpclone.h"
#include "gimpconvolve.h"
#include "gimpdodgeburn.h"
#include "gimperaser.h"
#include "gimppaintbrush.h"
#include "gimppencil.h"
#include "gimpsmudge.h"


void
paint_init (Gimp *gimp)
{
  g_type_class_ref (GIMP_TYPE_AIRBRUSH);
  g_type_class_ref (GIMP_TYPE_CLONE);
  g_type_class_ref (GIMP_TYPE_CONVOLVE);
  g_type_class_ref (GIMP_TYPE_DODGEBURN);
  g_type_class_ref (GIMP_TYPE_ERASER);
  g_type_class_ref (GIMP_TYPE_PAINTBRUSH);
  g_type_class_ref (GIMP_TYPE_PENCIL);
  g_type_class_ref (GIMP_TYPE_SMUDGE);
}

void
paint_exit (Gimp *gimp)
{
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_AIRBRUSH));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CLONE));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONVOLVE));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_DODGEBURN));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_ERASER));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_PAINTBRUSH));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_PENCIL));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_SMUDGE));
}
