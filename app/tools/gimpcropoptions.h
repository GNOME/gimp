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

#ifndef __LIGMA_CROP_OPTIONS_H__
#define __LIGMA_CROP_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_CROP_OPTIONS            (ligma_crop_options_get_type ())
#define LIGMA_CROP_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CROP_OPTIONS, LigmaCropOptions))
#define LIGMA_CROP_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CROP_OPTIONS, LigmaCropOptionsClass))
#define LIGMA_IS_CROP_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CROP_OPTIONS))
#define LIGMA_IS_CROP_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CROP_OPTIONS))
#define LIGMA_CROP_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CROP_OPTIONS, LigmaCropOptionsClass))


typedef struct _LigmaCropOptions      LigmaCropOptions;
typedef struct _LigmaToolOptionsClass LigmaCropOptionsClass;

struct _LigmaCropOptions
{
  LigmaToolOptions  parent_instance;

  /* Work on the current layer rather than the image. */
  gboolean         layer_only;

  /* Allow the crop rectangle to be larger than the image/layer. This
   * will resize the image/layer.
   */
  gboolean         allow_growing;

  /* How to fill new areas created by 'allow_growing. */
  LigmaFillType     fill_type;

  /* Whether to discard layer data that falls out of the crop region */
  gboolean         delete_pixels;
};


GType       ligma_crop_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_crop_options_gui      (LigmaToolOptions *tool_options);


#endif /* __LIGMA_CROP_OPTIONS_H__ */
