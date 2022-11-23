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

#ifndef  __LIGMA_COLOR_PICKER_OPTIONS_H__
#define  __LIGMA_COLOR_PICKER_OPTIONS_H__


#include "ligmacoloroptions.h"


#define LIGMA_TYPE_COLOR_PICKER_OPTIONS            (ligma_color_picker_options_get_type ())
#define LIGMA_COLOR_PICKER_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_PICKER_OPTIONS, LigmaColorPickerOptions))
#define LIGMA_COLOR_PICKER_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_PICKER_OPTIONS, LigmaColorPickerOptionsClass))
#define LIGMA_IS_COLOR_PICKER_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_PICKER_OPTIONS))
#define LIGMA_IS_COLOR_PICKER_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_PICKER_OPTIONS))
#define LIGMA_COLOR_PICKER_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_PICKER_OPTIONS, LigmaColorPickerOptionsClass))


typedef struct _LigmaColorPickerOptions LigmaColorPickerOptions;
typedef struct _LigmaToolOptionsClass   LigmaColorPickerOptionsClass;

struct _LigmaColorPickerOptions
{
  LigmaColorOptions     parent_instance;

  LigmaColorPickTarget  pick_target;
  gboolean             use_info_window;
  LigmaColorPickMode    frame1_mode;
  LigmaColorPickMode    frame2_mode;
};


GType       ligma_color_picker_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_color_picker_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_COLOR_PICKER_OPTIONS_H__  */
