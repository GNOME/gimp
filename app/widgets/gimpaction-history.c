/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaaction-history.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"

#include "ligmauimanager.h"
#include "ligmaaction.h"
#include "ligmaaction-history.h"


#define LIGMA_ACTION_HISTORY_FILENAME "action-history"

/* History items are stored in a queue, sorted by frequency (number of times
 * the action was activated), from most frequent to least frequent.  Each item,
 * in addition to the corresponding action name and its index in the queue,
 * stores a "delta": the difference in frequency between it, and the next item
 * in the queue; note that the frequency itself is not stored anywhere.
 *
 * To keep items from remaining at the top of the queue for too long, the delta
 * is capped above, such the the maximal delta of the first item is MAX_DELTA,
 * and the maximal delta of each subsequent item is the maximal delta of the
 * previous item, times MAX_DELTA_FALLOFF.
 *
 * When an action is activated, its frequency grows by 1, meaning that the
 * delta of the corresponding item is incremented (if below the maximum), and
 * the delta of the previous item is decremented (if above 0).  If the delta of
 * the previous item is already 0, then, before the above, the current and
 * previous items swap frequencies, and the current item is moved up the queue
 * until the preceding item's frequency is greater than 0 (or until it reaches
 * the front of the queue).
 */
#define MAX_DELTA         5
#define MAX_DELTA_FALLOFF 0.95


enum
{
  HISTORY_ITEM = 1
};

typedef struct
{
  gchar *action_name;
  gint   index;
  gint   delta;
} LigmaActionHistoryItem;

static struct
{
  Ligma       *ligma;
  GQueue     *items;
  GHashTable *links;
} history;


static LigmaActionHistoryItem * ligma_action_history_item_new       (const gchar           *action_name,
                                                                   gint                   index,
                                                                   gint                   delta);
static void                    ligma_action_history_item_free      (LigmaActionHistoryItem *item);

static gint                    ligma_action_history_item_max_delta (gint                   index);


/*  public functions  */

void
ligma_action_history_init (Ligma *ligma)
{
  LigmaGuiConfig *config;
  GFile         *file;
  GScanner      *scanner;
  GTokenType     token;
  gint           delta = 0;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  config = LIGMA_GUI_CONFIG (ligma->config);

  if (history.ligma != NULL)
    {
      g_warning ("%s: must be run only once.", G_STRFUNC);
      return;
    }

  history.ligma  = ligma;
  history.items = g_queue_new ();
  history.links = g_hash_table_new (g_str_hash, g_str_equal);

  file = ligma_directory_file (LIGMA_ACTION_HISTORY_FILENAME, NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  scanner = ligma_scanner_new_file (file, NULL);
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

              if (! ligma_scanner_parse_string (scanner, &action_name))
                break;

              token = G_TOKEN_INT;

              if (g_scanner_peek_next_token (scanner) != token ||
                  ! ligma_scanner_parse_int (scanner, &delta))
                {
                  g_free (action_name);
                  break;
                }

              if (! ligma_action_history_is_excluded_action (action_name) &&
                  ! g_hash_table_contains (history.links, action_name))
                {
                  LigmaActionHistoryItem *item;

                  item = ligma_action_history_item_new (
                    action_name,
                    g_queue_get_length (history.items),
                    delta);

                  g_queue_push_tail (history.items, item);

                  g_hash_table_insert (history.links,
                                       item->action_name,
                                       g_queue_peek_tail_link (history.items));
                }

              g_free (action_name);
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;

          if (g_queue_get_length (history.items) >= config->action_history_size)
            goto done;
          break;

        default: /* do nothing */
          break;
        }
    }

 done:
  ligma_scanner_unref (scanner);
}

void
ligma_action_history_exit (Ligma *ligma)
{
  LigmaGuiConfig         *config;
  LigmaActionHistoryItem *item;
  GList                 *actions;
  GFile                 *file;
  LigmaConfigWriter      *writer;
  gint                   i;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  config = LIGMA_GUI_CONFIG (ligma->config);

  file = ligma_directory_file (LIGMA_ACTION_HISTORY_FILENAME, NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  writer = ligma_config_writer_new_from_file (file, TRUE, "LIGMA action-history",
                                             NULL);
  g_object_unref (file);

  for (actions = history.items->head, i = 0;
       actions && i < config->action_history_size;
       actions = g_list_next (actions), i++)
    {
      item = actions->data;

      ligma_config_writer_open (writer, "history-item");
      ligma_config_writer_string (writer, item->action_name);
      ligma_config_writer_printf (writer, "%d", item->delta);
      ligma_config_writer_close (writer);
    }

  ligma_config_writer_finish (writer, "end of action-history", NULL);

  ligma_action_history_clear (ligma);

  g_clear_pointer (&history.links, g_hash_table_unref);
  g_clear_pointer (&history.items, g_queue_free);
  history.ligma = NULL;
}

void
ligma_action_history_clear (Ligma *ligma)
{
  LigmaActionHistoryItem *item;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_hash_table_remove_all (history.links);

  while ((item = g_queue_pop_head (history.items)))
    ligma_action_history_item_free (item);
}

/**
 * ligma_action_history_search:
 * @ligma:
 * @match_func:
 * @keyword:
 *
 * Search all history #LigmaAction which match @keyword with function
 * @match_func(action, keyword).
 * It will also return inactive actions, but will discard non-visible
 * actions.
 *
 * returns: a #GList of #LigmaAction, which must be freed with
 *          g_list_free_full (result, (GDestroyNotify) g_object_unref)
 */
GList *
ligma_action_history_search (Ligma                *ligma,
                            LigmaActionMatchFunc  match_func,
                            const gchar         *keyword)
{
  LigmaGuiConfig *config;
  LigmaUIManager *manager;
  GList         *actions;
  GList         *result = NULL;
  gint           i;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (match_func != NULL, NULL);

  config  = LIGMA_GUI_CONFIG (ligma->config);
  manager = ligma_ui_managers_from_name ("<Image>")->data;

  for (actions = history.items->head, i = 0;
       actions && i < config->action_history_size;
       actions = g_list_next (actions), i++)
    {
      LigmaActionHistoryItem *item   = actions->data;
      LigmaAction            *action;

      action = ligma_ui_manager_find_action (manager, NULL, item->action_name);
      if (action == NULL)
        continue;

      if (! ligma_action_is_visible (action))
        continue;

      if (match_func (action, keyword, NULL, ligma))
        result = g_list_prepend (result, g_object_ref (action));
    }

  return g_list_reverse (result);
}

/* ligma_action_history_is_blacklisted_action:
 *
 * Returns whether an action should be excluded from both
 * history and search results.
 */
gboolean
ligma_action_history_is_blacklisted_action (const gchar *action_name)
{
  if (ligma_action_is_gui_blacklisted (action_name))
    return TRUE;

  return (g_str_has_suffix (action_name, "-set")            ||
          g_str_has_suffix (action_name, "-accel")          ||
          g_str_has_prefix (action_name, "context-")        ||
          g_str_has_prefix (action_name, "filters-recent-") ||
          g_strcmp0 (action_name, "dialogs-action-search") == 0);
}

/* ligma_action_history_is_excluded_action:
 *
 * Returns whether an action should be excluded from history.
 *
 * Some actions should not be logged in the history, but should
 * otherwise appear in the search results, since they correspond
 * to different functions at different times, or since their
 * label may interfere with more relevant, but less frequent,
 * actions.
 */
gboolean
ligma_action_history_is_excluded_action (const gchar *action_name)
{
  if (ligma_action_history_is_blacklisted_action (action_name))
    return TRUE;

  return (g_strcmp0 (action_name, "edit-undo") == 0        ||
          g_strcmp0 (action_name, "edit-strong-undo") == 0 ||
          g_strcmp0 (action_name, "edit-redo") == 0        ||
          g_strcmp0 (action_name, "edit-strong-redo") == 0 ||
          g_strcmp0 (action_name, "filters-repeat") == 0   ||
          g_strcmp0 (action_name, "filters-reshow") == 0);
}

/* Called whenever a LigmaAction is activated.
 * It allows us to log all used actions.
 */
void
ligma_action_history_action_activated (LigmaAction *action)
{
  LigmaGuiConfig         *config;
  const gchar           *action_name;
  GList                 *link;
  LigmaActionHistoryItem *item;

  /* Silently return when called at the wrong time, like when the
   * activated action was "quit" and the history is already gone.
   */
  if (! history.ligma)
    return;

  config = LIGMA_GUI_CONFIG (history.ligma->config);

  if (config->action_history_size == 0)
    return;

  action_name = ligma_action_get_name (action);

  /* Some specific actions are of no log interest. */
  if (ligma_action_history_is_excluded_action (action_name))
    return;

  g_return_if_fail (action_name != NULL);

  /* Remove excessive items. */
  while (g_queue_get_length (history.items) > config->action_history_size)
    {
      item = g_queue_pop_tail (history.items);

      g_hash_table_remove (history.links, item->action_name);

      ligma_action_history_item_free (item);
    }

  /* Look up the action in the history. */
  link = g_hash_table_lookup (history.links, action_name);

  /* If the action is not in the history, insert it
   * at the back of the history queue, possibly
   * replacing the last item.
   */
  if (! link)
    {
      if (g_queue_get_length (history.items) == config->action_history_size)
        {
          item = g_queue_pop_tail (history.items);

          g_hash_table_remove (history.links, item->action_name);

          ligma_action_history_item_free (item);
        }

      item = ligma_action_history_item_new (
        action_name,
        g_queue_get_length (history.items),
        0);

      g_queue_push_tail (history.items, item);
      link = g_queue_peek_tail_link (history.items);

      g_hash_table_insert (history.links, item->action_name, link);
    }
  else
    {
      item = link->data;
    }

  /* Update the history, according to the logic described
   * in the comment at the beginning of the file.
   */
  if (item->index > 0)
    {
      GList                 *prev_link = g_list_previous (link);
      LigmaActionHistoryItem *prev_item = prev_link->data;

      if (prev_item->delta == 0)
        {
          for (; prev_link; prev_link = g_list_previous (prev_link))
            {
              prev_item = prev_link->data;

              if (prev_item->delta > 0)
                break;

              prev_item->index++;
              item->index--;

              prev_item->delta = item->delta;
              item->delta      = 0;
            }

          g_queue_unlink (history.items, link);

          if (prev_link)
            {
              link->prev = prev_link;
              link->next = prev_link->next;

              link->prev->next = link;
              link->next->prev = link;

              history.items->length++;
            }
          else
            {
              g_queue_push_head_link (history.items, link);
            }
        }

      if (item->index > 0)
        prev_item->delta--;
    }

  if (item->delta < ligma_action_history_item_max_delta (item->index))
    item->delta++;
}


/*  private functions  */

static LigmaActionHistoryItem *
ligma_action_history_item_new (const gchar *action_name,
                              gint         index,
                              gint         delta)
{
  LigmaActionHistoryItem *item = g_slice_new (LigmaActionHistoryItem);

  item->action_name = g_strdup (action_name);
  item->index       = index;
  item->delta       = CLAMP (delta, 0, ligma_action_history_item_max_delta (index));

  return item;
}

static void
ligma_action_history_item_free (LigmaActionHistoryItem *item)
{
  g_free (item->action_name);

  g_slice_free (LigmaActionHistoryItem, item);
}

static gint
ligma_action_history_item_max_delta (gint index)
{
  return floor (MAX_DELTA * exp (log (MAX_DELTA_FALLOFF) * index));
}
