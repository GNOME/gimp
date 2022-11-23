/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaRc
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_RC_H__
#define __LIGMA_RC_H__

#include "config/ligmapluginconfig.h"


#define LIGMA_TYPE_RC            (ligma_rc_get_type ())
#define LIGMA_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_RC, LigmaRc))
#define LIGMA_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_RC, LigmaRcClass))
#define LIGMA_IS_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_RC))
#define LIGMA_IS_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_RC))


typedef struct _LigmaRcClass LigmaRcClass;

struct _LigmaRc
{
  LigmaPluginConfig  parent_instance;

  GFile            *user_ligmarc;
  GFile            *system_ligmarc;
  gboolean          verbose;
  gboolean          autosave;
  guint             save_idle_id;
};

struct _LigmaRcClass
{
  LigmaPluginConfigClass  parent_class;
};


GType     ligma_rc_get_type          (void) G_GNUC_CONST;

LigmaRc  * ligma_rc_new               (GObject     *ligma,
                                     GFile       *system_ligmarc,
                                     GFile       *user_ligmarc,
                                     gboolean     verbose);

void      ligma_rc_load_system       (LigmaRc      *rc);
void      ligma_rc_load_user         (LigmaRc      *rc);

void      ligma_rc_set_autosave      (LigmaRc      *rc,
                                     gboolean     autosave);
void      ligma_rc_save              (LigmaRc      *rc);

gchar   * ligma_rc_query             (LigmaRc      *rc,
                                     const gchar *key);

void      ligma_rc_set_unknown_token (LigmaRc      *rc,
                                     const gchar *token,
                                     const gchar *value);

void      ligma_rc_migrate           (LigmaRc      *rc);


#endif /* LIGMA_RC_H__ */
