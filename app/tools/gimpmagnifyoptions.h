/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_MAGNIFY_OPTIONS_H__
#define __LIGMA_MAGNIFY_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_MAGNIFY_OPTIONS            (ligma_magnify_options_get_type ())
#define LIGMA_MAGNIFY_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MAGNIFY_OPTIONS, LigmaMagnifyOptions))
#define LIGMA_MAGNIFY_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MAGNIFY_OPTIONS, LigmaMagnifyOptionsClass))
#define LIGMA_IS_MAGNIFY_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MAGNIFY_OPTIONS))
#define LIGMA_IS_MAGNIFY_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MAGNIFY_OPTIONS))
#define LIGMA_MAGNIFY_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MAGNIFY_OPTIONS, LigmaMagnifyOptionsClass))


typedef struct _LigmaMagnifyOptions   LigmaMagnifyOptions;
typedef struct _LigmaToolOptionsClass LigmaMagnifyOptionsClass;

struct _LigmaMagnifyOptions
{
  LigmaToolOptions   parent_instance;

  gboolean          auto_resize;
  LigmaZoomType      zoom_type;
};


GType       ligma_magnify_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_magnify_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_MAGNIFY_OPTIONS_H__  */
