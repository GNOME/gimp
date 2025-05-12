/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcheckexpander.h
 * Copyright (C) 2025 Jehan
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

#ifndef __GIMP_CHECK_EXPANDER_H__
#define __GIMP_CHECK_EXPANDER_H__


#define GIMP_TYPE_CHECK_EXPANDER            (gimp_check_expander_get_type ())
#define GIMP_CHECK_EXPANDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CHECK_EXPANDER, GimpCheckExpander))
#define GIMP_CHECK_EXPANDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHECK_EXPANDER, GimpCheckExpanderClass))
#define GIMP_IS_CHECK_EXPANDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_CHECK_EXPANDER))
#define GIMP_IS_CHECK_EXPANDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHECK_EXPANDER))
#define GIMP_CHECK_EXPANDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CHECK_EXPANDER, GimpCheckExpanderClass))


typedef struct _GimpCheckExpanderPrivate GimpCheckExpanderPrivate;
typedef struct _GimpCheckExpanderClass   GimpCheckExpanderClass;

struct _GimpCheckExpander
{
  GimpFrame                  parent_instance;

  GimpCheckExpanderPrivate  *priv;
};

struct _GimpCheckExpanderClass
{
  GimpFrameClass             parent_class;
};


GType         gimp_check_expander_get_type     (void) G_GNUC_CONST;

GtkWidget   * gimp_check_expander_new          (const gchar       *label,
                                                GtkWidget         *child);

void          gimp_check_expander_set_label    (GimpCheckExpander *expander,
                                                const gchar       *label);
const gchar * gimp_check_expander_get_label    (GimpCheckExpander *expander,
                                                const gchar       *label);

void          gimp_check_expander_set_checked  (GimpCheckExpander *expander,
                                                gboolean           checked,
                                                gboolean           auto_expand);
void          gimp_check_expander_set_expanded (GimpCheckExpander *expander,
                                                gboolean           expanded);


#endif /* __GIMP_CHECK_EXPANDER_H__ */
