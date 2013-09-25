/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2013  Jehan <jehan at girinstud.io>
 *
 * gimpaction-history.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "gimpuimanager.h"
#include "gimpaction.h"
#include "gimpaction-history.h"

#define GIMP_ACTION_HISTORY_FILENAME "action_history"

typedef struct {
  GtkAction    *action;
  gchar        *name;
  gint          count;
} GimpActionHistoryItem;

static struct {
  GimpGuiConfig *config;
  GList         *items;
} history;

static void gimp_action_history_item_free         (GimpActionHistoryItem *item);

static gint gimp_action_history_init_compare_func (GimpActionHistoryItem *a,
                                                   GimpActionHistoryItem *b);
static gint gimp_action_history_compare_func      (GimpActionHistoryItem *a,
                                                   GimpActionHistoryItem *b);

static void gimp_action_insert                    (const gchar           *action_name,
                                                   gint                   count);

/*  public functions  */

void
gimp_action_history_init (GimpGuiConfig *config)
{
  gchar *history_file_path;
  gint   count;
  FILE  *fp;
  gint   i;

  if (history.items != NULL)
    {
      g_warning ("%s: must be run only once.", G_STRFUNC);
      return;
    }

  history.config    = config;
  history_file_path = g_build_filename (gimp_directory (),
                                        GIMP_ACTION_HISTORY_FILENAME,
                                        NULL);
  fp = fopen (history_file_path, "r");
  if (fp == NULL)
    /* Probably a first use case. Not necessarily an error. */
    return;

  for (i = 0; i < config->action_history_size; i++)
    {
      /* Let's assume an action name will never be more than 256 character. */
      gchar action_name[256];

      if (fscanf (fp, "%s %d", action_name, &count) == EOF)
        break;

      gimp_action_insert (action_name, count);
    }

  if (count > 1)
    {
      GList *actions = history.items;

      for (; actions && i; actions = g_list_next (actions), i--)
        {
          GimpActionHistoryItem *action = actions->data;

          action->count -= count - 1;
        }
    }

  g_free (history_file_path);

  fclose (fp);
}

void
gimp_action_history_exit (GimpGuiConfig *config)
{
  GList *actions = history.items;
  gchar *history_file_path;
  gint   min_count = 0;
  FILE  *fp;
  gint   i = config->action_history_size;

  /* If we have more items than current history size, trim the history
     and move down all count so that 1 is lower. */
  for (; actions && i; actions = g_list_next (actions), i--)
    ;
  if (actions)
    {
      GimpActionHistoryItem *action = actions->data;

      min_count = action->count - 1;
    }

  actions = history.items;
  i       = config->action_history_size;

  history_file_path = g_build_filename (gimp_directory (),
                                        GIMP_ACTION_HISTORY_FILENAME,
                                        NULL);

  fp = fopen (history_file_path, "w");
  for (; actions && i; actions = g_list_next (actions), i--)
    {
      GimpActionHistoryItem *action = actions->data;

      fprintf (fp, "%s %d \n", action->name, action->count - min_count);
    }

  gimp_action_history_empty ();
  fclose (fp);
  g_free (history_file_path);
}

/* gimp_action_history_excluded_action:
 *
 * Returns whether an action should be excluded from history.
 */
gboolean
gimp_action_history_excluded_action (const gchar *action_name)
{
  return (g_str_has_suffix (action_name, "-menu")           ||
          g_str_has_suffix (action_name, "-popup")          ||
          g_str_has_suffix (action_name, "-set")            ||
          g_str_has_suffix (action_name, "-accel")          ||
          g_str_has_prefix (action_name, "context-")        ||
          g_str_has_prefix (action_name, "plug-in-recent-") ||
          g_strcmp0 (action_name, "plug-in-repeat") == 0    ||
          g_strcmp0 (action_name, "plug-in-reshow") == 0    ||
          g_strcmp0 (action_name, "help-action-search") == 0);
}

/* Callback run on the `activate` signal of an action.
   It allows us to log all used action. */
void
gimp_action_history_activate_callback (GtkAction *action,
                                       gpointer   user_data)
{
  GList                 *actions;
  GimpActionHistoryItem *history_item;
  const gchar           *action_name;
  gint                   previous_count = 0;

  action_name = gtk_action_get_name (action);

  /* Some specific actions are of no log interest. */
  if (gimp_action_history_excluded_action (action_name))
    return;

  for (actions = history.items; actions; actions = g_list_next (actions))
    {
      history_item = actions->data;

      if (g_strcmp0 (action_name, history_item->name) == 0)
        {
          GimpActionHistoryItem *next_history_item = g_list_next (actions) ?
            g_list_next (actions)->data : NULL;

          /* Is there any other item with the same count?
             We don't want to leave any count gap to always accept new items.
             This means that if we increment the only item with a given count,
             we must decrement the next item.
             Other consequence is that an item with higher count won't be
             incremented at all if no other items have the same count. */
          if (previous_count == history_item->count ||
              (next_history_item && next_history_item->count == history_item->count))
            {
              history_item->count++;
              /* Remove then reinsert to reorder. */
              history.items = g_list_remove (history.items, history_item);
              history.items = g_list_insert_sorted (history.items, history_item,
                                                    (GCompareFunc) gimp_action_history_compare_func);
            }
          else if (previous_count != 0                   &&
                   previous_count != history_item->count)
            {
              GimpActionHistoryItem *previous_history_item = g_list_previous (actions)->data;

              history_item->count++;
              /* Remove then reinsert to reorder. */
              history.items = g_list_remove (history.items, history_item);
              history.items = g_list_insert_sorted (history.items, history_item,
                                                    (GCompareFunc) gimp_action_history_compare_func);

              previous_history_item->count--;
              /* Remove then reinsert to reorder. */
              history.items = g_list_remove (history.items, previous_history_item);
              history.items = g_list_insert_sorted (history.items, previous_history_item,
                                                    (GCompareFunc) gimp_action_history_compare_func);
            }
          return;
        }

      previous_count = history_item->count;
    }

  /* If we are here, this action is not logged yet. */
  history_item = g_malloc0 (sizeof (GimpActionHistoryItem));

  history_item->action = g_object_ref (action);
  history_item->name = g_strdup (action_name);
  history_item->count = 1;
  history.items = g_list_insert_sorted (history.items,
                                        history_item,
                                        (GCompareFunc) gimp_action_history_compare_func);
}

void
gimp_action_history_empty (void)
{
  g_list_free_full (history.items, (GDestroyNotify) gimp_action_history_item_free);
  history.items = NULL;
}

/* Search all history actions which match "keyword"
   with function match_func(action, keyword).

   @return a list of GtkAction*, to free with:
   g_list_free_full (result, (GDestroyNotify) g_object_unref);
  */
GList*
gimp_action_history_search (const gchar         *keyword,
                            GimpActionMatchFunc  match_func,
                            GimpGuiConfig       *config)
{
  GList                 *actions;
  GimpActionHistoryItem *history_item;
  GtkAction             *action;
  GList                 *search_result = NULL;
  gint                   i             = config->action_history_size;

  for (actions = history.items; actions && i; actions = g_list_next (actions), i--)
    {
      history_item = actions->data;
      action = history_item->action;

      if (! gtk_action_get_sensitive (action) && ! config->search_show_unavailable)
        continue;

      if (match_func (action, keyword, NULL, FALSE))
        search_result = g_list_prepend (search_result, g_object_ref (action));
    }

  return g_list_reverse (search_result);
}

/*  private functions  */

static void
gimp_action_history_item_free (GimpActionHistoryItem *item)
{
  g_object_unref (item->action);
  g_free (item->name);
  g_free (item);
}

/* Compare function used at list initialization.
   We use a slightly different compare function as for runtime insert,
   because we want to keep history file order for equal values. */
static gint
gimp_action_history_init_compare_func (GimpActionHistoryItem *a,
                                       GimpActionHistoryItem *b)
{
  return (a->count <= b->count);
}

/* Compare function used when updating the list.
   There is no equality case. If they have the same count,
   I ensure that the first action (last inserted) will be before. */
static gint
gimp_action_history_compare_func (GimpActionHistoryItem *a,
                                  GimpActionHistoryItem *b)
{
  return (a->count < b->count);
}

static void
gimp_action_insert (const gchar *action_name,
                    gint         count)
{
  GList             *action_groups;
  GimpUIManager     *manager;

  /* We do not insert some categories of actions. */
  if (gimp_action_history_excluded_action (action_name))
    return;

  manager = gimp_ui_managers_from_name ("<Image>")->data;

  for (action_groups = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
       action_groups;
       action_groups = g_list_next (action_groups))
    {
      GimpActionGroup *group = action_groups->data;
      GList           *actions;
      GList           *list2;
      gboolean         found = FALSE;

      actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (group));
      actions = g_list_sort (actions, (GCompareFunc) gimp_action_name_compare);

      for (list2 = actions; list2; list2 = g_list_next (list2))
        {
          GtkAction *action             = list2->data;
          gint       unavailable_action = -1;

          unavailable_action = strcmp (action_name, gtk_action_get_name (action));

          if (unavailable_action == 0)
            {
              /* We found our action. */
              GimpActionHistoryItem *new_action = g_malloc0 (sizeof (GimpActionHistoryItem));

              new_action->action = g_object_ref (action);
              new_action->name = g_strdup (action_name);
              new_action->count = count;
              history.items = g_list_insert_sorted (history.items,
                                                    new_action,
                                                    (GCompareFunc) gimp_action_history_init_compare_func);
              found = TRUE;
              break;
            }
          else if (unavailable_action < 0)
            {
              /* Since the actions list is sorted, it means we passed
                 all possible actions already and it is not in this group. */
              break;
            }

        }
      g_list_free (actions);

      if (found)
        break;
   }
}
