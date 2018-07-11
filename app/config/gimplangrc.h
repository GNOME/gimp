/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpLangRc: pre-parsing of gimprc returning the language.
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

#ifndef __GIMP_LANG_RC_H__
#define __GIMP_LANG_RC_H__


#define GIMP_TYPE_LANG_RC            (gimp_lang_rc_get_type ())
#define GIMP_LANG_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LANG_RC, GimpLangRc))
#define GIMP_LANG_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LANG_RC, GimpLangRcClass))
#define GIMP_IS_LANG_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LANG_RC))
#define GIMP_IS_LANG_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LANG_RC))


typedef struct _GimpLangRcClass GimpLangRcClass;

struct _GimpLangRc
{
  GObject        parent_instance;

  GFile         *user_gimprc;
  GFile         *system_gimprc;
  gboolean       verbose;

  gchar         *language;
};

struct _GimpLangRcClass
{
  GObjectClass   parent_class;
};


GType         gimp_lang_rc_get_type     (void) G_GNUC_CONST;

GimpLangRc  * gimp_lang_rc_new          (GFile      *system_gimprc,
                                         GFile      *user_gimprc,
                                         gboolean    verbose);
gchar       * gimp_lang_rc_get_language (GimpLangRc *rc);


#endif /* GIMP_LANG_RC_H__ */

