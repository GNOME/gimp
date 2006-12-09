/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Recent File Storage,
 * see http://freedesktop.org/Standards/recent-file-spec/
 *
 * This code is taken from libegg and has been adapted to the GIMP needs.
 * The original author is James Willcox <jwillcox@cs.indiana.edu>,
 * responsible for bugs in this version is Sven Neumann <sven@gimp.org>.
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

#ifndef __GIMP_RECENT_ITEM_H__
#define __GIMP_RECENT_ITEM_H__


typedef struct _GimpRecentItem GimpRecentItem;


GimpRecentItem * gimp_recent_item_new            (void);

GimpRecentItem * gimp_recent_item_ref            (GimpRecentItem       *item);
GimpRecentItem * gimp_recent_item_unref          (GimpRecentItem       *item);

GimpRecentItem * gimp_recent_item_new_from_uri   (const gchar          *uri);

gboolean         gimp_recent_item_set_uri        (GimpRecentItem       *item,
                                                  const gchar          *uri);
const gchar    * gimp_recent_item_get_uri        (const GimpRecentItem *item);

gchar          * gimp_recent_item_get_uri_utf8   (const GimpRecentItem *item);

void             gimp_recent_item_set_mime_type  (GimpRecentItem       *item,
                                                  const gchar          *mime);
const gchar    * gimp_recent_item_get_mime_type  (const GimpRecentItem *item);

void             gimp_recent_item_set_timestamp  (GimpRecentItem       *item,
                                                  time_t                timestamp);
time_t           gimp_recent_item_get_timestamp  (const GimpRecentItem *item);



/* groups */
const GList    * gimp_recent_item_get_groups     (const GimpRecentItem *item);

gboolean         gimp_recent_item_in_group       (const GimpRecentItem *item,
                                                  const gchar          *group_name);
void             gimp_recent_item_add_group      (GimpRecentItem       *item,
                                                  const gchar          *group_name);
void             gimp_recent_item_remove_group   (GimpRecentItem       *item,
                                                  const gchar          *group_name);

void             gimp_recent_item_set_private    (GimpRecentItem       *item,
                                                  gboolean              priv);
gboolean         gimp_recent_item_get_private    (const GimpRecentItem *item);


#endif /* __GIMP_RECENT_ITEM_H__ */
