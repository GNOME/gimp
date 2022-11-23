/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmauiconfigurer.h
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __LIGMA_UI_CONFIGURER_H__
#define __LIGMA_UI_CONFIGURER_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_UI_CONFIGURER              (ligma_ui_configurer_get_type ())
#define LIGMA_UI_CONFIGURER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_UI_CONFIGURER, LigmaUIConfigurer))
#define LIGMA_UI_CONFIGURER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), LIGMA_TYPE_UI_CONFIGURER, LigmaUIConfigurerClass))
#define LIGMA_IS_UI_CONFIGURER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_UI_CONFIGURER))
#define LIGMA_IS_UI_CONFIGURER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), LIGMA_TYPE_UI_CONFIGURER))
#define LIGMA_UI_CONFIGURER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), LIGMA_TYPE_UI_CONFIGURER, LigmaUIConfigurerClass))


typedef struct _LigmaUIConfigurerClass   LigmaUIConfigurerClass;
typedef struct _LigmaUIConfigurerPrivate LigmaUIConfigurerPrivate;

struct _LigmaUIConfigurer
{
  LigmaObject parent_instance;

  LigmaUIConfigurerPrivate *p;
};

struct _LigmaUIConfigurerClass
{
  LigmaObjectClass parent_class;
};


GType         ligma_ui_configurer_get_type  (void) G_GNUC_CONST;
void          ligma_ui_configurer_configure (LigmaUIConfigurer *ui_configurer,
                                            gboolean          single_window_mode);


#endif  /* __LIGMA_UI_CONFIGURER_H__ */
