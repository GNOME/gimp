/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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


struct _GimpHelpItem
{
  gchar *ref;
  gchar *title;
  gchar *sort;   /* optional sort key provided by doc team */
  gchar *parent;

  /*  extra fields used by the help-browser  */
  GList *children;
  gulong index;
};


GimpHelpItem * gimp_help_item_new  (const gchar   *ref,
                                    const gchar   *title,
                                    const gchar   *sort,
                                    const gchar   *parent);
void           gimp_help_item_free (GimpHelpItem  *item);
