/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafontfactory.h
 * Copyright (C) 2018 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_FONT_FACTORY_H__
#define __LIGMA_FONT_FACTORY_H__


#include "core/ligmadatafactory.h"


#define LIGMA_TYPE_FONT_FACTORY            (ligma_font_factory_get_type ())
#define LIGMA_FONT_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FONT_FACTORY, LigmaFontFactory))
#define LIGMA_FONT_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FONT_FACTORY, LigmaFontFactoryClass))
#define LIGMA_IS_FONT_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FONT_FACTORY))
#define LIGMA_IS_FONT_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FONT_FACTORY))
#define LIGMA_FONT_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FONT_FACTORY, LigmaFontFactoryClass))


typedef struct _LigmaFontFactoryPrivate LigmaFontFactoryPrivate;
typedef struct _LigmaFontFactoryClass   LigmaFontFactoryClass;

struct _LigmaFontFactory
{
  LigmaDataFactory         parent_instance;

  LigmaFontFactoryPrivate *priv;
};

struct _LigmaFontFactoryClass
{
  LigmaDataFactoryClass  parent_class;
};


GType             ligma_font_factory_get_type (void) G_GNUC_CONST;

LigmaDataFactory * ligma_font_factory_new      (Ligma        *ligma,
                                              const gchar *path_property_name);


#endif  /*  __LIGMA_FONT_FACTORY_H__  */
