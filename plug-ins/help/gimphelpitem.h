/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#ifndef __GIMP_HELP_ITEM_H__
#define __GIMP_HELP_ITEM_H__


struct _GimpHelpItem
{
  gchar *ref;
  gchar *title;
  gchar *parent;

  /* eek */
  GList *children;
  gint   index;
};


GimpHelpItem * gimp_help_item_new  (const gchar   *ref,
                                    const gchar   *title,
                                    const gchar   *parent);
void           gimp_help_item_free (GimpHelpItem  *item);


#endif /* __GIMP_HELP_ITEM_H__ */
