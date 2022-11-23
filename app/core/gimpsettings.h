/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasettings.h
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SETTINGS_H__
#define __LIGMA_SETTINGS_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_SETTINGS            (ligma_settings_get_type ())
#define LIGMA_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SETTINGS, LigmaSettings))
#define LIGMA_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_SETTINGS, LigmaSettingsClass))
#define LIGMA_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SETTINGS))
#define LIGMA_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_SETTINGS))
#define LIGMA_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_SETTINGS, LigmaSettingsClass))


typedef struct _LigmaSettingsClass LigmaSettingsClass;

struct _LigmaSettings
{
  LigmaViewable  parent_instance;

  gint64        time;
};

struct _LigmaSettingsClass
{
  LigmaViewableClass  parent_class;
};


GType   ligma_settings_get_type (void) G_GNUC_CONST;

gint    ligma_settings_compare  (LigmaSettings *a,
                                LigmaSettings *b);


#endif /* __LIGMA_SETTINGS_H__ */
