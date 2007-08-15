/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprofilestore.c
 * Copyright (C) 2004-2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorprofilestore.h"
#include "gimpcolorprofilestore-private.h"

#include "libgimp/libgimp-intl.h"


enum
{
  PROP_0,
  PROP_HISTORY
};


static GObject * gimp_color_profile_store_constructor    (GType                  type,
                                                          guint                  n_params,
                                                          GObjectConstructParam *params);
static void      gimp_color_profile_store_dispose        (GObject               *object);
static void      gimp_color_profile_store_finalize       (GObject               *object);
static void      gimp_color_profile_store_set_property   (GObject               *object,
                                                          guint                  property_id,
                                                          const GValue          *value,
                                                          GParamSpec            *pspec);
static void      gimp_color_profile_store_get_property   (GObject               *object,
                                                          guint                  property_id,
                                                          GValue                *value,
                                                          GParamSpec            *pspec);

static gboolean  gimp_color_profile_store_history_insert (GimpColorProfileStore *store,
                                                          GtkTreeIter           *iter,
                                                          const gchar           *filename,
                                                          const gchar           *label,
                                                          gint                   index);
static void      gimp_color_profile_store_get_separator  (GimpColorProfileStore  *store,
                                                          GtkTreeIter            *iter,
                                                          gboolean                top);
static gboolean  gimp_color_profile_store_save           (GimpColorProfileStore  *store,
                                                          const gchar            *filename,
                                                          GError                **error);
static gboolean  gimp_color_profile_store_load           (GimpColorProfileStore  *store,
                                                          const gchar            *filename,
                                                          GError                **error);


G_DEFINE_TYPE (GimpColorProfileStore,
               gimp_color_profile_store, GTK_TYPE_LIST_STORE)

#define parent_class gimp_color_profile_store_parent_class


static void
gimp_color_profile_store_class_init (GimpColorProfileStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_color_profile_store_constructor;
  object_class->dispose      = gimp_color_profile_store_dispose;
  object_class->finalize     = gimp_color_profile_store_finalize;
  object_class->set_property = gimp_color_profile_store_set_property;
  object_class->get_property = gimp_color_profile_store_get_property;

  /**
   * GimpColorProfileStore:history:
   *
   * Filename of the color history used to populate the profile store.
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_HISTORY,
                                   g_param_spec_string ("history", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_color_profile_store_init (GimpColorProfileStore *store)
{
  GType types[] =
    {
      G_TYPE_INT,     /*  GIMP_COLOR_PROFILE_STORE_ITEM_TYPE  */
      G_TYPE_STRING,  /*  GIMP_COLOR_PROFILE_STORE_LABEL      */
      G_TYPE_STRING,  /*  GIMP_COLOR_PROFILE_STORE_FILENAME   */
      G_TYPE_INT      /*  GIMP_COLOR_PROFILE_STORE_INDEX      */
    };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);
}

static GObject *
gimp_color_profile_store_constructor  (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params)
{
  GObject               *object;
  GimpColorProfileStore *store;
  GtkTreeIter            iter;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  store = GIMP_COLOR_PROFILE_STORE (object);

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
                      GIMP_COLOR_PROFILE_STORE_ITEM_DIALOG,
                      GIMP_COLOR_PROFILE_STORE_LABEL,
                      _("Select color profile from disk..."),
                      -1);

  if (store->history)
    {
      gimp_color_profile_store_load (store, store->history, NULL);
    }

  return object;
}

static void
gimp_color_profile_store_dispose (GObject *object)
{
  GimpColorProfileStore *store = GIMP_COLOR_PROFILE_STORE (object);

  if (store->history)
    {
      gimp_color_profile_store_save (store, store->history, NULL);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_profile_store_finalize (GObject *object)
{
  GimpColorProfileStore *store = GIMP_COLOR_PROFILE_STORE (object);

  if (store->history)
    {
      g_free (store->history);
      store->history = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_profile_store_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpColorProfileStore *store = GIMP_COLOR_PROFILE_STORE (object);

  switch (property_id)
    {
    case PROP_HISTORY:
      g_return_if_fail (store->history == NULL);
      store->history = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_profile_store_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpColorProfileStore *store = GIMP_COLOR_PROFILE_STORE (object);

  switch (property_id)
    {
    case PROP_HISTORY:
      g_value_set_string (value, store->history);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/**
 * gimp_color_profile_store_new:
 * @history: filename of the profilerc (or %NULL for no history)
 *
 * Creates a new #GimpColorProfileStore object and populates it with
 * last used profiles read from the file @history. The updated history
 * is written back to disk when the store is disposed.
 *
 * The filename passed as @history is typically created using the
 * following code snippet:
 * <informalexample><programlisting>
 *  gchar *history = gimp_personal_rc_file ("profilerc");
 * </programlisting></informalexample>
 *
 * Return value: a new #GimpColorProfileStore
 *
 * Since: GIMP 2.4
 **/
GtkListStore *
gimp_color_profile_store_new (const gchar *history)
{
  return g_object_new (GIMP_TYPE_COLOR_PROFILE_STORE,
                       "history", history,
                       NULL);
}

/**
 * gimp_color_profile_store_add:
 * @store:    a #GimpColorProfileStore
 * @filename: filename of the profile to add (or %NULL)
 * @label:    label to use for the profile
 *            (may only be %NULL if @filename is %NULL)
 *
 * Adds a color profile item to the #GimpColorProfileStore. Items
 * added with this function will be kept at the top, separated from
 * the history of last used color profiles.
 *
 * This function is often used to add a selectable item for the %NULL
 * filename. If you pass %NULL for both @filename and @label, the
 * @label will be set to the string "None" for you (and translated for
 * the user).
 *
 * Since: GIMP 2.4
 **/
void
gimp_color_profile_store_add (GimpColorProfileStore *store,
                              const gchar           *filename,
                              const gchar           *label)
{
  GtkTreeIter  separator;
  GtkTreeIter  iter;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_STORE (store));
  g_return_if_fail (label != NULL || filename == NULL);

  if (! filename && ! label)
    label = Q_("profile|None");

  gimp_color_profile_store_get_separator (store, &separator, TRUE);

  gtk_list_store_insert_before (GTK_LIST_STORE (store), &iter, &separator);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
                      GIMP_COLOR_PROFILE_STORE_ITEM_FILE,
                      GIMP_COLOR_PROFILE_STORE_FILENAME, filename,
                      GIMP_COLOR_PROFILE_STORE_LABEL, label,
                      GIMP_COLOR_PROFILE_STORE_INDEX, -1,
                      -1);
}

/**
 * _gimp_color_profile_store_history_add:
 * @store:    a #GimpColorProfileStore
 * @filename: filename of the profile to add (or %NULL)
 * @label:    label to use for the profile (or %NULL)
 * @iter:     a #GtkTreeIter
 *
 * Return value: %TRUE if the iter is valid and pointing to the item
 *
 * Since: GIMP 2.4
 **/
gboolean
_gimp_color_profile_store_history_add (GimpColorProfileStore *store,
                                       const gchar           *filename,
                                       const gchar           *label,
                                       GtkTreeIter           *iter)
{
  GtkTreeModel *model;
  gboolean      iter_valid;
  gint          max = -1;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_STORE (store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint   type;
      gint   index;
      gchar *this;

      gtk_tree_model_get (model, iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          GIMP_COLOR_PROFILE_STORE_INDEX,     &index,
                          -1);

      if (type != GIMP_COLOR_PROFILE_STORE_ITEM_FILE)
        continue;

      if (index > max)
        max = index;

      /*  check if we found a filename match  */
      gtk_tree_model_get (model, iter,
                          GIMP_COLOR_PROFILE_STORE_FILENAME, &this,
                          -1);

      if ((this && filename && strcmp (filename, this) == 0) ||
          (! this && ! filename))
        {
          /*  update the label  */
          if (label && *label)
            gtk_list_store_set (GTK_LIST_STORE (store), iter,
                                GIMP_COLOR_PROFILE_STORE_LABEL, label,
                                -1);

          g_free (this);
          return TRUE;
        }
    }

  if (! filename)
    return FALSE;

  if (label && *label)
    {
      iter_valid = gimp_color_profile_store_history_insert (store, iter,
                                                            filename, label,
                                                            ++max);
    }
  else
    {
      gchar *basename = g_filename_display_basename (filename);

      iter_valid = gimp_color_profile_store_history_insert (store, iter,
                                                            filename, basename,
                                                            ++max);
      g_free (basename);
    }

  return iter_valid;
}

static gboolean
gimp_color_profile_store_history_insert (GimpColorProfileStore *store,
                                         GtkTreeIter           *iter,
                                         const gchar           *filename,
                                         const gchar           *label,
                                         gint                   index)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter   sibling;
  gboolean      iter_valid;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (label != NULL, FALSE);
  g_return_val_if_fail (index > -1, FALSE);

  gimp_color_profile_store_get_separator (store, iter, FALSE);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &sibling);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &sibling))
    {
      gint type;
      gint this_index;

      gtk_tree_model_get (model, &sibling,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          GIMP_COLOR_PROFILE_STORE_INDEX,     &this_index,
                          -1);

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM)
        {
          gtk_list_store_insert_before (GTK_LIST_STORE (store),
                                        iter, &sibling);
          break;
        }

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_FILE && this_index > -1)
        {
          gchar *this_label;

          gtk_tree_model_get (model, &sibling,
                              GIMP_COLOR_PROFILE_STORE_LABEL, &this_label,
                              -1);

          if (this_label && g_utf8_collate (label, this_label) < 0)
            {
              gtk_list_store_insert_before (GTK_LIST_STORE (store),
                                            iter, &sibling);
              g_free (this_label);
              break;
            }

          g_free (this_label);
        }
    }

  if (iter_valid)
    gtk_list_store_set (GTK_LIST_STORE (store), iter,
                        GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
                        GIMP_COLOR_PROFILE_STORE_ITEM_FILE,
                        GIMP_COLOR_PROFILE_STORE_FILENAME, filename,
                        GIMP_COLOR_PROFILE_STORE_LABEL, label,
                        GIMP_COLOR_PROFILE_STORE_INDEX, index,
                        -1);

  return iter_valid;
}

static void
gimp_color_profile_store_create_separator (GimpColorProfileStore *store,
                                           GtkTreeIter           *iter,
                                           gboolean               top)
{
  if (top)
    {
      gtk_list_store_prepend (GTK_LIST_STORE (store), iter);
    }
  else
    {
      GtkTreeModel *model = GTK_TREE_MODEL (store);
      GtkTreeIter   sibling;
      gboolean      iter_valid;

      for (iter_valid = gtk_tree_model_get_iter_first (model, &sibling);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (model, &sibling))
        {
          gint type;

          gtk_tree_model_get (model, &sibling,
                              GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                              -1);

          if (type == GIMP_COLOR_PROFILE_STORE_ITEM_DIALOG)
            break;
        }

      if (iter_valid)
        gtk_list_store_insert_before (GTK_LIST_STORE (store), iter, &sibling);
    }

  gtk_list_store_set (GTK_LIST_STORE (store), iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
                      top ?
                      GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP :
                      GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM,
                      GIMP_COLOR_PROFILE_STORE_INDEX, -1,
                      -1);
}

static void
gimp_color_profile_store_get_separator (GimpColorProfileStore *store,
                                        GtkTreeIter           *iter,
                                        gboolean               top)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  gboolean      iter_valid;
  gint          needle;

  needle = (top ?
            GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP :
            GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint type;

      gtk_tree_model_get (model, iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          -1);

      if (type == needle)
        return;
    }

  gimp_color_profile_store_create_separator (store, iter, top);
}

static GTokenType
gimp_color_profile_store_load_profile (GimpColorProfileStore *store,
                                       GScanner              *scanner,
                                       gint                   index)
{
  GtkTreeIter  iter;
  gchar       *label = NULL;
  gchar       *uri   = NULL;

  if (gimp_scanner_parse_string (scanner, &label) &&
      gimp_scanner_parse_string (scanner, &uri))
    {
      gchar *filename = g_filename_from_uri (uri, NULL, NULL);

      if (filename)
        gimp_color_profile_store_history_insert (store, &iter,
                                                 filename, label, index);

      g_free (filename);
      g_free (label);
      g_free (uri);

      return G_TOKEN_RIGHT_PAREN;
    }

  g_free (label);
  g_free (uri);

  return G_TOKEN_STRING;
}

static gboolean
gimp_color_profile_store_load (GimpColorProfileStore  *store,
                               const gchar            *filename,
                               GError                **error)
{
  GScanner   *scanner;
  GTokenType  token;
  gint        i = 0;

  scanner = gimp_scanner_new_file (filename, error);
  if (! scanner)
    return FALSE;

  g_scanner_scope_add_symbol (scanner, 0, "color-profile", 0);

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
          token = gimp_color_profile_store_load_profile (store, scanner, i++);
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  gimp_scanner_destroy (scanner);

  return TRUE;
}

static gboolean
gimp_color_profile_store_save (GimpColorProfileStore  *store,
                               const gchar            *filename,
                               GError                **error)
{
  GimpConfigWriter *writer;
  GtkTreeModel     *model = GTK_TREE_MODEL (store);
  GtkTreeIter       iter;
  gboolean          iter_valid;

  writer = gimp_config_writer_new_file (filename,
                                        TRUE,
                                        "GIMP color profile history",
                                        error);
  if (! writer)
    return FALSE;

  /*  FIXME: store the history in MRU order  */

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint type;

      gtk_tree_model_get (model, &iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          -1);

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_FILE)
        {
          gchar *label;
          gchar *filename;

          gtk_tree_model_get (model, &iter,
                              GIMP_COLOR_PROFILE_STORE_LABEL,    &label,
                              GIMP_COLOR_PROFILE_STORE_FILENAME, &filename,
                              -1);

          if (filename && label)
            {
              gchar *uri = g_filename_to_uri (filename, NULL, NULL);

              if (uri)
                {
                  gimp_config_writer_open (writer, "color-profile");
                  gimp_config_writer_string (writer, label);
                  gimp_config_writer_string (writer, uri);
                  gimp_config_writer_close (writer);

                  g_free (uri);
                }
            }

          g_free (filename);
          g_free (label);
        }
    }

  return gimp_config_writer_finish (writer,
                                    "end of color profile history", error);
}
