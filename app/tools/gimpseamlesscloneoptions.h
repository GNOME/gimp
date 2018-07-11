/* GIMP - The GNU Image Manipulation Program
 *
 * gimpseamlesscloneoptions.h
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#ifndef __GIMP_SEAMLESS_CLONE_OPTIONS_H__
#define __GIMP_SEAMLESS_CLONE_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_TYPE_SEAMLESS_CLONE_OPTIONS            (gimp_seamless_clone_options_get_type ())
#define GIMP_SEAMLESS_CLONE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SEAMLESS_CLONE_OPTIONS, GimpSeamlessCloneOptions))
#define GIMP_SEAMLESS_CLONE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SEAMLESS_CLONE_OPTIONS, GimpSeamlessCloneOptionsClass))
#define GIMP_IS_SEAMLESS_CLONE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SEAMLESS_CLONE_OPTIONS))
#define GIMP_IS_SEAMLESS_CLONE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SEAMLESS_CLONE_OPTIONS))
#define GIMP_SEAMLESS_CLONE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SEAMLESS_CLONE_OPTIONS, GimpSeamlessCloneOptionsClass))


typedef struct _GimpSeamlessCloneOptions      GimpSeamlessCloneOptions;
typedef struct _GimpSeamlessCloneOptionsClass GimpSeamlessCloneOptionsClass;

struct _GimpSeamlessCloneOptions
{
  GimpToolOptions parent_instance;

  gint            max_refine_scale;
  gboolean        temp;
};

struct _GimpSeamlessCloneOptionsClass
{
  GimpToolOptionsClass  parent_class;
};


GType       gimp_seamless_clone_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_seamless_clone_options_gui      (GimpToolOptions *tool_options);


#endif  /*  __GIMP_SEAMLESS_CLONE_OPTIONS_H__  */
