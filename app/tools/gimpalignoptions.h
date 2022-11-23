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

#ifndef __LIGMA_ALIGN_OPTIONS_H__
#define __LIGMA_ALIGN_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_ALIGN_OPTIONS            (ligma_align_options_get_type ())
#define LIGMA_ALIGN_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ALIGN_OPTIONS, LigmaAlignOptions))
#define LIGMA_ALIGN_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ALIGN_OPTIONS, LigmaAlignOptionsClass))
#define LIGMA_IS_ALIGN_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ALIGN_OPTIONS))
#define LIGMA_IS_ALIGN_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ALIGN_OPTIONS))
#define LIGMA_ALIGN_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ALIGN_OPTIONS, LigmaAlignOptionsClass))


typedef struct _LigmaAlignOptions        LigmaAlignOptions;
typedef struct _LigmaAlignOptionsPrivate LigmaAlignOptionsPrivate;
typedef struct _LigmaAlignOptionsClass   LigmaAlignOptionsClass;

struct _LigmaAlignOptions
{
  LigmaToolOptions          parent_instance;

  LigmaAlignReferenceType   align_reference;

  LigmaAlignOptionsPrivate *priv;
};

struct _LigmaAlignOptionsClass
{
  LigmaToolOptionsClass  parent_class;

  void (* align_button_clicked) (LigmaAlignOptions  *options,
                                 LigmaAlignmentType  align_type);
};


GType       ligma_align_options_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_align_options_gui               (LigmaToolOptions  *tool_options);

GList     * ligma_align_options_get_objects       (LigmaAlignOptions *options);
void        ligma_align_options_get_pivot         (LigmaAlignOptions *options,
                                                  gdouble          *x,
                                                  gdouble          *y);

void        ligma_align_options_pick_reference    (LigmaAlignOptions *options,
                                                  GObject          *object);
GObject   * ligma_align_options_get_reference     (LigmaAlignOptions *options,
                                                  gboolean          blink_if_none);
gboolean    ligma_align_options_align_contents    (LigmaAlignOptions *options);

void        ligma_align_options_pick_guide        (LigmaAlignOptions *options,
                                                  LigmaGuide        *guide,
                                                  gboolean          extend);


#endif /* __LIGMA_ALIGN_OPTIONS_H__ */
