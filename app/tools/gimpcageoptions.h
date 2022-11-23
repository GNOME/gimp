/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmacageoptions.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#ifndef __LIGMA_CAGE_OPTIONS_H__
#define __LIGMA_CAGE_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_CAGE_OPTIONS            (ligma_cage_options_get_type ())
#define LIGMA_CAGE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CAGE_OPTIONS, LigmaCageOptions))
#define LIGMA_CAGE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CAGE_OPTIONS, LigmaCageOptionsClass))
#define LIGMA_IS_CAGE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CAGE_OPTIONS))
#define LIGMA_IS_CAGE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CAGE_OPTIONS))
#define LIGMA_CAGE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CAGE_OPTIONS, LigmaCageOptionsClass))


typedef struct _LigmaCageOptions      LigmaCageOptions;
typedef struct _LigmaCageOptionsClass LigmaCageOptionsClass;

struct _LigmaCageOptions
{
  LigmaToolOptions  parent_instance;

  LigmaCageMode     cage_mode;
  gboolean         fill_plain_color;
};

struct _LigmaCageOptionsClass
{
  LigmaToolOptionsClass  parent_class;
};


GType       ligma_cage_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_cage_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_CAGE_OPTIONS_H__  */
