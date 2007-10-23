/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactiongroup.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ACTION_GROUP_H__
#define __GIMP_ACTION_GROUP_H__

#include <gtk/gtkactiongroup.h>


#define GIMP_TYPE_ACTION_GROUP              (gimp_action_group_get_type ())
#define GIMP_ACTION_GROUP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION_GROUP, GimpActionGroup))
#define GIMP_ACTION_GROUP_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), GIMP_TYPE_ACTION_GROUP, GimpActionGroupClass))
#define GIMP_IS_ACTION_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION_GROUP))
#define GIMP_IS_ACTION_GROUP_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), GIMP_TYPE_ACTION_GROUP))
#define GIMP_ACTION_GROUP_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), GIMP_TYPE_ACTION_GROUP, GimpActionGroupClass))


typedef struct _GimpActionGroupClass GimpActionGroupClass;

struct _GimpActionGroup
{
  GtkActionGroup             parent_instance;

  Gimp                      *gimp;
  gchar                     *label;
  gchar                     *stock_id;
  gboolean                   mnemonics;

  gpointer                   user_data;

  GimpActionGroupUpdateFunc  update_func;
};

struct _GimpActionGroupClass
{
  GtkActionGroupClass  parent_class;

  GHashTable          *groups;
};

struct _GimpActionEntry
{
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  GCallback    callback;

  const gchar *help_id;
};

struct _GimpToggleActionEntry
{
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  GCallback    callback;
  gboolean     is_active;

  const gchar *help_id;
};

struct _GimpRadioActionEntry
{
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  gint         value;

  const gchar *help_id;
};

struct _GimpEnumActionEntry
{
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  gint         value;
  gboolean     value_variable;

  const gchar *help_id;
};

struct _GimpStringActionEntry
{
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  const gchar *value;

  const gchar *help_id;
};

struct _GimpPlugInActionEntry
{
  const gchar         *name;
  const gchar         *stock_id;
  const gchar         *label;
  const gchar         *accelerator;
  const gchar         *tooltip;
  GimpPlugInProcedure *procedure;

  const gchar         *help_id;
};


GType            gimp_action_group_get_type   (void) G_GNUC_CONST;

GimpActionGroup *gimp_action_group_new        (Gimp                  *gimp,
                                               const gchar           *name,
                                               const gchar           *label,
                                               const gchar           *stock_id,
                                               gboolean               mnemonics,
                                               gpointer               user_data,
                                               GimpActionGroupUpdateFunc update_func);

GList *gimp_action_groups_from_name           (const gchar           *name);

void   gimp_action_group_update               (GimpActionGroup       *group,
                                               gpointer               update_data);

void   gimp_action_group_add_actions          (GimpActionGroup             *group,
                                               const GimpActionEntry       *entries,
                                               guint                        n_entries);
void   gimp_action_group_add_toggle_actions   (GimpActionGroup             *group,
                                               const GimpToggleActionEntry *entries,
                                               guint                        n_entries);
GSList *gimp_action_group_add_radio_actions   (GimpActionGroup             *group,
                                               const GimpRadioActionEntry  *entries,
                                               guint                        n_entries,
                                               GSList                      *radio_group,
                                               gint                         value,
                                               GCallback                    callback);
void   gimp_action_group_add_enum_actions     (GimpActionGroup             *group,
                                               const GimpEnumActionEntry   *entries,
                                               guint                        n_entries,
                                               GCallback                    callback);
void   gimp_action_group_add_string_actions   (GimpActionGroup             *group,
                                               const GimpStringActionEntry *entries,
                                               guint                         n_entries,
                                               GCallback                     callback);
void   gimp_action_group_add_plug_in_actions  (GimpActionGroup             *group,
                                               const GimpPlugInActionEntry *entries,
                                               guint                        n_entries,
                                               GCallback                    callback);

void   gimp_action_group_activate_action      (GimpActionGroup     *group,
                                               const gchar         *action_name);

void   gimp_action_group_set_action_visible   (GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               gboolean             visible);
void   gimp_action_group_set_action_sensitive (GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               gboolean             sensitive);
void   gimp_action_group_set_action_active    (GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               gboolean             active);
void   gimp_action_group_set_action_label     (GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               const gchar         *label);
void   gimp_action_group_set_action_color     (GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               const GimpRGB       *color,
                                               gboolean             set_label);
void   gimp_action_group_set_action_viewable  (GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               GimpViewable        *viewable);
void   gimp_action_group_set_action_hide_empty(GimpActionGroup     *group,
                                               const gchar         *action_name,
                                               gboolean             is_important);


#endif  /* __GIMP_ACTION_GROUP_H__ */
