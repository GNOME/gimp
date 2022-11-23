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

#ifndef __LIGMA_DISPLAY_IMPL_H__
#define __LIGMA_DISPLAY_IMPL_H__


#include "core/ligmadisplay.h"


#define LIGMA_TYPE_DISPLAY_IMPL            (ligma_display_impl_get_type ())
#define LIGMA_DISPLAY_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DISPLAY_IMPL, LigmaDisplayImpl))
#define LIGMA_DISPLAY_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DISPLAY_IMPL, LigmaDisplayImplClass))
#define LIGMA_IS_DISPLAY_IMPL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DISPLAY_IMPL))
#define LIGMA_IS_DISPLAY_IMPL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DISPLAY_IMPL))
#define LIGMA_DISPLAY_IMPL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DISPLAY_IMPL, LigmaDisplayImplClass))


typedef struct _LigmaDisplayImpl        LigmaDisplayImpl;
typedef struct _LigmaDisplayImplClass   LigmaDisplayImplClass;
typedef struct _LigmaDisplayImplPrivate LigmaDisplayImplPrivate;


struct _LigmaDisplayImpl
{
  LigmaDisplay             parent_instance;

  LigmaDisplayImplPrivate *priv;

};

struct _LigmaDisplayImplClass
{
  LigmaDisplayClass  parent_class;
};


GType              ligma_display_impl_get_type   (void) G_GNUC_CONST;

LigmaDisplay      * ligma_display_new             (Ligma              *ligma,
                                                 LigmaImage         *image,
                                                 LigmaUnit           unit,
                                                 gdouble            scale,
                                                 LigmaUIManager     *popup_manager,
                                                 LigmaDialogFactory *dialog_factory,
                                                 GdkMonitor        *monitor);
void               ligma_display_delete          (LigmaDisplay       *display);
void               ligma_display_close           (LigmaDisplay       *display);

gchar            * ligma_display_get_action_name (LigmaDisplay       *display);

LigmaImage        * ligma_display_get_image       (LigmaDisplay       *display);
void               ligma_display_set_image       (LigmaDisplay       *display,
                                                 LigmaImage         *image);

gint               ligma_display_get_instance    (LigmaDisplay       *display);

LigmaDisplayShell * ligma_display_get_shell       (LigmaDisplay       *display);

void               ligma_display_empty           (LigmaDisplay       *display);
void               ligma_display_fill            (LigmaDisplay       *display,
                                                 LigmaImage         *image,
                                                 LigmaUnit           unit,
                                                 gdouble            scale);

void               ligma_display_update_bounding_box
                                                (LigmaDisplay       *display);

void               ligma_display_update_area     (LigmaDisplay       *display,
                                                 gboolean           now,
                                                 gint               x,
                                                 gint               y,
                                                 gint               w,
                                                 gint               h);

void               ligma_display_flush           (LigmaDisplay       *display);
void               ligma_display_flush_now       (LigmaDisplay       *display);


#endif /*  __LIGMA_DISPLAY_IMPL_H__  */
