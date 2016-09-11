/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpaction-history.c
 * Copyright (C) 2013  Jehan <jehan at girinstud.io>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "gimpuimanager.h"
#include "gimpaction.h"
#include "gimpaction-history.h"


#define GIMP_ACTION_HISTORY_FILENAME "action-history"

enum
{
  HISTORY_ITEM = 1
};

typedef struct
{
  gchar *action_name;
  gint   count;
} GimpActionHistoryItem;

static struct
{
  GList *items;
} history;


static GimpActionHistoryItem *
            gimp_action_history_item_new          (const gchar           *action_name,
                                                   gint                   count);
static void gimp_action_history_item_free         (GimpActionHistoryItem *item);

static gint gimp_action_history_init_compare_func (GimpActionHistoryItem *a,
                                                   GimpActionHistoryItem *b);
static gint gimp_action_history_compare_func      (GimpActionHistoryItem *a,
                                                   GimpActionHistoryItem *b);


/*  public functions  */

void
gimp_action_history_init (Gimp *gimp)
{
  GimpGuiConfig *config;
  GFile         *file;
  GScanner      *scanner;
  GTokenType     token;
  gint           count = 0;
  gint           n_items = 0;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  if (history.items != NULL)
    {
      g_warning ("%s: must be run only once.", G_STRFUNC);
      return;
    }

  file = gimp_directory_file (GIMP_ACTION_HISTORY_FILENAME, NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  scanner = gimp_scanner_new_gfile (file, NULL);
  g_object_unref (file);

  if (! scanner)
    return;

  g_scanner_scope_add_symbol (scanner, 0, "history-item",
                              GINT_TO_POINTER (HISTORY_ITEM));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GINT_TO_POINTER (HISTORY_ITEM))
            {
              gchar *action_name;

              token = G_TOKEN_STRING;

              if (g_scanner_peek_next_token (scanner) != token)
                break;

              if (! gimp_scanner_parse_string (scanner, &action_name))
                break;

              token = G_TOKEN_INT;

              if (g_scanner_peek_next_token (scanner) != token ||
                  ! gimp_scanner_parse_int (scanner, &count))
                {
                  g_free (action_name);
                  break;
                }

              history.items =
                g_list_insert_sorted (history.items,
                                      gimp_action_history_item_new (action_name, count),
                                      (GCompareFunc) gimp_action_history_init_compare_func);

              n_items++;

              g_free (action_name);
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;

          if (n_items >= config->action_history_size)
            goto done;
          break;

        default: /* do nothing */
          break;
        }
    }

 done:
  gimp_scanner_destroy (scanner);

  if (count > 1)
    {
      GList *actions;
      gint   i;

      for (actions = history.items, i = 0;
           actions && i < config->action_history_size;
           actions = g_list_next (actions), i++)
        {
          GimpActionHistoryItem *action = actions->data;

          action->count -= count - 1;
        }
    }
}

void
gimp_action_history_exit (Gimp *gimp)
{
  GimpGuiConfig         *config;
  GimpActionHistoryItem *item;
  GList                 *actions;
  GFile                 *file;
  GimpConfigWriter      *writer;
  gint                   min_count = 0;
  gint                   i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  /* If we have more items than current history size, trim the history
   * and move down all count so that 1 is lower.
   */
  item = g_list_nth_data (history.items, config->action_history_size);
  if (item)
    min_count = item->count - 1;

  file = gimp_directory_file (GIMP_ACTION_HISTORY_FILENAME, NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  writer = gimp_config_writer_new_gfile (file, TRUE, "GIMP action-history",
                                         NULL);
  g_object_unref (file);

  for (actions = history.items, i = 0;
       actions && i < config->action_history_size;
       actions = g_list_next (actions), i++)
    {
      item = actions->data;

      gimp_config_writer_open (writer, "history-item");
      gimp_config_writer_string (writer, item->action_name);
      gimp_config_writer_printf (writer, "%d", item->count - min_count);
      gimp_config_writer_close (writer);
    }

  gimp_config_writer_finish (writer, "end of action-history", NULL);

  gimp_action_history_clear (gimp);
}

void
gimp_action_history_clear (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_list_free_full (history.items,
                    (GDestroyNotify) gimp_action_history_item_free);
  history.items = NULL;
}

/* Search all history actions which match "keyword" with function
 * match_func(action, keyword).
 *
 * @return a list of GtkAction*, to free with:
 * g_list_free_full (result, (GDestroyNotify) g_object_unref);
 */
GList *
gimp_action_history_search (Gimp                *gimp,
                            GimpActionMatchFunc  match_func,
                            const gchar         *keyword)
{
  GimpGuiConfig *config;
  GimpUIManager *manager;
  GList         *actions;
  GList         *result = NULL;
  gint           i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (match_func != NULL, NULL);

  config  = GIMP_GUI_CONFIG (gimp->config);
  manager = gimp_ui_managers_from_name ("<Image>")->data;

  for (actions = history.items, i = 0;
       actions && i < config->action_history_size;
       actions = g_list_next (actions), i++)
    {
      GimpActionHistoryItem *item   = actions->data;
      GtkAction             *action;

      action = gimp_ui_manager_find_action (manager, NULL, item->action_name);
      if (action == NULL)
        continue;

      if (! gtk_action_is_sensitive (action) &&
          ! config->search_show_unavailable)
        continue;

      if (match_func (action, keyword, NULL, gimp))
        result = g_list_prepend (result, g_object_ref (action));
    }

  return g_list_reverse (result);
}

/* gimp_action_history_excluded_action:
 *
 * Returns whether an action should be excluded from history.
 */
gboolean
gimp_action_history_excluded_action (const gchar *action_name)
{
  if (gimp_action_is_gui_blacklisted (action_name))
    return TRUE;

  return (g_str_has_suffix (action_name, "-set")            ||
          g_str_has_suffix (action_name, "-accel")          ||
          g_str_has_prefix (action_name, "context-")        ||
          g_str_has_prefix (action_name, "plug-in-recent-") ||
          g_strcmp0 (action_name, "plug-in-repeat") == 0    ||
          g_strcmp0 (action_name, "plug-in-reshow") == 0    ||
          g_strcmp0 (action_name, "dialogs-action-search") == 0);
}

/* Callback run on the `activate` signal of an action.
 * It allows us to log all used action.
 */
void
gimp_action_history_activate_callback (GtkAction *action,
                                       gpointer   user_data)
{
  GList       *actions;
  const gchar *action_name;
  gint         previous_count = 0;

  action_name = gtk_action_get_name (action);

  /* Some specific actions are of no log interest. */
  if (gimp_action_history_excluded_action (action_name))
    return;

  for (actions = history.items; actions; actions = g_list_next (actions))
    {
      GimpActionHistoryItem *item = actions->data;

      if (g_strcmp0 (action_name, item->action_name) == 0)
        {
          GimpActionHistoryItem *next_item;

          next_item = g_list_next (actions) ? g_list_next (actions)->data : NULL;

          /* Is there any other item with the same count?  We don't
           * want to leave any count gap to always accept new items.
           * This means that if we increment the only item with a
           * given count, we must decrement the next item.  Other
           * consequence is that an item with higher count won't be
           * incremented at all if no other items have the same count.
           */
          if (previous_count == item->count ||
              (next_item && next_item->count == item->count))
            {
              item->count++;

              history.items = g_list_remove (history.items, item);
              history.items = g_list_insert_sorted (history.items, item,
                                                    (GCompareFunc) gimp_action_history_compare_func);
            }
          else if (previous_count != 0 &&
                   previous_count != item->count)
            {
              GimpActionHistoryItem *previous_item = g_list_previous (actions)->data;

              item->count++;

              history.items = g_list_remove (history.items, item);
              history.items = g_list_insert_sorted (history.items, item,
                                                    (GCompareFunc) gimp_action_history_compare_func);

              previous_item->count--;

              history.items = g_list_remove (history.items, previous_item);
              history.items = g_list_insert_sorted (history.items, previous_item,
                                                    (GCompareFunc) gimp_action_history_compare_func);
            }

          return;
        }

      previous_count = item->count;
    }

  /* If we are here, this action is not logged yet. */
  history.items =
    g_list_insert_sorted (history.items,
                          gimp_action_history_item_new (action_name, 1),
                          (GCompareFunc) gimp_action_history_compare_func);
}


/*  private functions  */

static GimpActionHistoryItem *
gimp_action_history_item_new (const gchar *action_name,
                              gint         count)
{
  GimpActionHistoryItem *item = g_new0 (GimpActionHistoryItem, 1);

  item->action_name = g_strdup (action_name);
  item->count       = count;

  return item;
}

static void
gimp_action_history_item_free (GimpActionHistoryItem *item)
{
  g_free (item->action_name);
  g_free (item);
}

/* Compare function used at list initialization.
 * We use a slightly different compare function as for runtime insert,
 * because we want to keep history file order for equal values.
 */
static gint
gimp_action_history_init_compare_func (GimpActionHistoryItem *a,
                                       GimpActionHistoryItem *b)
{
  return (a->count <= b->count);
}

/* Compare function used when updating the list.
 * There is no equality case. If they have the same count,
 * I ensure that the first action (last inserted) will be before.
 */
static gint
gimp_action_history_compare_func (GimpActionHistoryItem *a,
                                  GimpActionHistoryItem *b)
{
  return (a->count < b->count);
}
