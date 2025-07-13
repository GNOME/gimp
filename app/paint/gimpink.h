/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#pragma once

#include "gimppaintcore.h"
#include "gimpink-blob.h"


#define GIMP_TYPE_INK            (gimp_ink_get_type ())
#define GIMP_INK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INK, GimpInk))
#define GIMP_INK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INK, GimpInkClass))
#define GIMP_IS_INK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INK))
#define GIMP_IS_INK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INK))
#define GIMP_INK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INK, GimpInkClass))


typedef struct _GimpInkClass GimpInkClass;

struct _GimpInk
{
  GimpPaintCore  parent_instance;

  GList         *start_blobs;      /*  starting blobs per stroke (for undo) */

  GimpBlob      *cur_blob;         /*  current blob                         */
  GList         *last_blobs;       /*  blobs for last stroke positions      */
  GList         *blobs_to_render;
};

struct _GimpInkClass
{
  GimpPaintCoreClass  parent_class;
};


void    gimp_ink_register (Gimp                      *gimp,
                           GimpPaintRegisterCallback  callback);

GType   gimp_ink_get_type (void) G_GNUC_CONST;

