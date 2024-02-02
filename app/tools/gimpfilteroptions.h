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

#ifndef __GIMP_FILTER_OPTIONS_H__
#define __GIMP_FILTER_OPTIONS_H__


#include "gimpcoloroptions.h"


#define GIMP_TYPE_FILTER_OPTIONS            (gimp_filter_options_get_type ())
#define GIMP_FILTER_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILTER_OPTIONS, GimpFilterOptions))
#define GIMP_FILTER_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILTER_OPTIONS, GimpFilterOptionsClass))
#define GIMP_IS_FILTER_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILTER_OPTIONS))
#define GIMP_IS_FILTER_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILTER_OPTIONS))
#define GIMP_FILTER_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILTER_OPTIONS, GimpFilterOptionsClass))


typedef struct _GimpFilterOptionsClass GimpFilterOptionsClass;

struct _GimpFilterOptions
{
  GimpColorOptions   parent_instance;

  gboolean           preview;
  gboolean           preview_split;
  GimpAlignmentType  preview_split_alignment;
  gint               preview_split_position;
  gboolean           controller;

  gboolean           blending_options_expanded;
  gboolean           color_options_expanded;

  gboolean           merge_filter;
};

struct _GimpFilterOptionsClass
{
  GimpColorOptionsClass  parent_instance;
};


GType   gimp_filter_options_get_type                   (void) G_GNUC_CONST;

void    gimp_filter_options_switch_preview_side        (GimpFilterOptions *options);
void    gimp_filter_options_switch_preview_orientation (GimpFilterOptions *options,
                                                        gint               position_x,
                                                        gint               position_y);


#endif /* __GIMP_FILTER_OPTIONS_H__ */
