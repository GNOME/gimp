/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaEarlyRc: pre-parsing of ligmarc suitable for use during early
 * initialization, when the ligma singleton is not constructed yet
 *
 * Copyright (C) 2017  Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_EARLY_RC_H__
#define __LIGMA_EARLY_RC_H__

#include "core/core-enums.h"

#define LIGMA_TYPE_EARLY_RC            (ligma_early_rc_get_type ())
#define LIGMA_EARLY_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EARLY_RC, LigmaEarlyRc))
#define LIGMA_EARLY_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EARLY_RC, LigmaEarlyRcClass))
#define LIGMA_IS_EARLY_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EARLY_RC))
#define LIGMA_IS_EARLY_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EARLY_RC))


typedef struct _LigmaEarlyRcClass LigmaEarlyRcClass;

struct _LigmaEarlyRc
{
  GObject        parent_instance;

  GFile         *user_ligmarc;
  GFile         *system_ligmarc;
  gboolean       verbose;

  gchar         *language;

#ifdef G_OS_WIN32
  LigmaWin32PointerInputAPI win32_pointer_input_api;
#endif
};

struct _LigmaEarlyRcClass
{
  GObjectClass   parent_class;
};


GType          ligma_early_rc_get_type     (void) G_GNUC_CONST;

LigmaEarlyRc  * ligma_early_rc_new          (GFile      *system_ligmarc,
                                           GFile      *user_ligmarc,
                                           gboolean    verbose);
gchar        * ligma_early_rc_get_language (LigmaEarlyRc *rc);

#ifdef G_OS_WIN32
LigmaWin32PointerInputAPI ligma_early_rc_get_win32_pointer_input_api (LigmaEarlyRc *rc);
#endif


#endif /* LIGMA_EARLY_RC_H__ */

