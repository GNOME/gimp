/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpEarlyRc: pre-parsing of gimprc suitable for use during early
 * initialization, when the gimp singleton is not constructed yet
 *
 * Copyright (C) 2017  Jehan <jehan@gimp.org>
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

#pragma once

#include "core/core-enums.h"


#define GIMP_TYPE_EARLY_RC            (gimp_early_rc_get_type ())
#define GIMP_EARLY_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EARLY_RC, GimpEarlyRc))
#define GIMP_EARLY_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EARLY_RC, GimpEarlyRcClass))
#define GIMP_IS_EARLY_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EARLY_RC))
#define GIMP_IS_EARLY_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EARLY_RC))


typedef struct _GimpEarlyRcClass GimpEarlyRcClass;

struct _GimpEarlyRc
{
  GObject        parent_instance;

  GFile         *user_gimprc;
  GFile         *system_gimprc;
  gboolean       verbose;

  gchar         *language;

#ifdef G_OS_WIN32
  GimpWin32PointerInputAPI win32_pointer_input_api;
#endif
};

struct _GimpEarlyRcClass
{
  GObjectClass   parent_class;
};


GType          gimp_early_rc_get_type     (void) G_GNUC_CONST;

GimpEarlyRc  * gimp_early_rc_new          (GFile      *system_gimprc,
                                           GFile      *user_gimprc,
                                           gboolean    verbose);
gchar        * gimp_early_rc_get_language (GimpEarlyRc *rc);

#ifdef G_OS_WIN32
GimpWin32PointerInputAPI gimp_early_rc_get_win32_pointer_input_api (GimpEarlyRc *rc);
#endif
