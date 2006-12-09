/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpRc
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_RC_H__
#define __GIMP_RC_H__

#include "config/gimppluginconfig.h"


#define GIMP_TYPE_RC            (gimp_rc_get_type ())
#define GIMP_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RC, GimpRc))
#define GIMP_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RC, GimpRcClass))
#define GIMP_IS_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RC))
#define GIMP_IS_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_RC))


typedef struct _GimpRcClass GimpRcClass;

struct _GimpRc
{
  GimpPluginConfig  parent_instance;

  gchar            *user_gimprc;
  gchar            *system_gimprc;
  gboolean          verbose;
  gboolean          autosave;
  guint             save_idle_id;
};

struct _GimpRcClass
{
  GimpPluginConfigClass  parent_class;
};


GType     gimp_rc_get_type          (void) G_GNUC_CONST;
GimpRc  * gimp_rc_new               (const gchar *system_gimprc,
                                     const gchar *user_gimprc,
                                     gboolean     verbose);
void      gimp_rc_set_autosave      (GimpRc      *gimprc,
                                     gboolean     autosave);
void      gimp_rc_save              (GimpRc      *gimprc);
gchar   * gimp_rc_query             (GimpRc      *rc,
                                     const gchar *key);
void      gimp_rc_set_unknown_token (GimpRc      *rc,
                                     const gchar *token,
                                     const gchar *value);

void      gimp_rc_migrate           (GimpRc      *rc);


#endif /* GIMP_RC_H__ */
