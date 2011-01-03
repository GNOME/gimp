/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprofilestore.c
 * Copyright (C) 2004-2008  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
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


/**
 * SECTION: gimpcolorprofilestore
 * @title: GimpColorProfileStore
 * @short_description: A #GtkListStore subclass that keep color profiles.
 *
 * A #GtkListStore subclass that keep color profiles.
 **/


#define HISTORY_SIZE  8

enum
{
  PROP_0,
  PROP_HISTORY
};


typedef struct _GimpColorProfileStorePrivate GimpColorProfileStorePrivate;

struct _GimpColorProfileStorePrivate
{
  gchar *history;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                      GIMP_TYPE_COLOR_PROFILE_STORE, \
                                                      GimpColorProfileStorePrivate)


static void      gimp_color_profile_store_constructed    (GObject               *object);
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
                                                          GFile                 *file,
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

  object_class->constructed  = gimp_color_profile_store_constructed;
  object_class->dispose      = gimp_color_profile_store_dispose;
  object_class->finalize     = gimp_color_profile_store_finalize;
  object_class->set_property = gimp_color_profile_store_set_property;
  object_class->get_property = gimp_color_profile_store_get_property;

  /**
   * GimpColorProfileStore:history:
   *
   * Filename of the color history used to populate the profile store.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_HISTORY,
                                   g_param_spec_string ("history", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GimpColorProfileStorePrivate));
}

static void
gimp_color_profile_store_init (GimpColorProfileStore *store)
{
  GType types[] =
    {
      G_TYPE_INT,    /*  GIMP_COLOR_PROFILE_STORE_ITEM_TYPE  */
      G_TYPE_STRING, /*  GIMP_COLOR_PROFILE_STORE_LABEL      */
      G_TYPE_FILE,   /*  GIMP_COLOR_PROFILE_STORE_FILE       */
      G_TYPE_INT     /*  GIMP_COLOR_PROFILE_STORE_INDEX      */
    };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);
}

static void
gimp_color_profile_store_constructed (GObject *object)
{
  GimpColorProfileStore        *store   = GIMP_COLOR_PROFILE_STORE (object);
  GimpColorProfileStorePrivate *private = GET_PRIVATE (store);
  GtkTreeIter                   iter;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
                      GIMP_COLOR_PROFILE_STORE_ITEM_DIALOG,
                      GIMP_COLOR_PROFILE_STORE_LABEL,
                      _("Select color profile from disk..."),
                      -1);

  if (private->history)
    gimp_color_profile_store_load (store, private->history, NULL);
}

static void
gimp_color_profile_store_dispose (GObject *object)
{
  GimpColorProfileStore        *store   = GIMP_COLOR_PROFILE_STORE (object);
  GimpColorProfileStorePrivate *private = GET_PRIVATE (store);

  if (private->history)
    gimp_color_profile_store_save (store, private->history, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_profile_store_finalize (GObject *object)
{
  GimpColorProfileStorePrivate *private = GET_PRIVATE (object);

  if (private->history)
    {
      g_free (private->history);
      private->history = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_profile_store_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpColorProfileStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_HISTORY:
      g_return_if_fail (private->history == NULL);
      private->history = g_value_dup_string (value);
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
  GimpColorProfileStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_HISTORY:
      g_value_set_string (value, private->history);
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
 * Since: 2.4
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
 * Deprecated: use gimp_color_profile_store_add_file() instead.
 *
 * Since: 2.4
 **/
void
gimp_color_profile_store_add (GimpColorProfileStore *store,
                              const gchar           *filename,
                              const gchar           *label)
{
  GFile *file = NULL;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_STORE (store));
  g_return_if_fail (label != NULL || filename == NULL);

  if (filename)
    file = g_file_new_for_path (filename);

  gimp_color_profile_store_add_file (store, file, label);

  g_object_unref (file);
}

/**
 * gimp_color_profile_store_add_file:
 * @store: a #GimpColorProfileStore
 * @file:  file of the profile to add (or %NULL)
 * @label: label to use for the profile
 *         (may only be %NULL if @filename is %NULL)
 *
 * Adds a color profile item to the #GimpColorProfileStore. Items
 * added with this function will be kept at the top, separated from
 * the history of last used color profiles.
 *
 * This function is often used to add a selectable item for the %NULL
 * file. If you pass %NULL for both @file and @label, the @label will
 * be set to the string "None" for you (and translated for the user).
 *
 * Since: 2.10
 **/
void
gimp_color_profile_store_add_file (GimpColorProfileStore *store,
                                   GFile                 *file,
                                   const gchar           *label)
{
  GtkTreeIter separator;
  GtkTreeIter iter;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_STORE (store));
  g_return_if_fail (label != NULL || file == NULL);
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (! file && ! label)
    label = C_("profile", "None");

  gimp_color_profile_store_get_separator (store, &separator, TRUE);

  gtk_list_store_insert_before (GTK_LIST_STORE (store), &iter, &separator);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
                      GIMP_COLOR_PROFILE_STORE_ITEM_FILE,
                      GIMP_COLOR_PROFILE_STORE_FILE,  file,
                      GIMP_COLOR_PROFILE_STORE_LABEL, label,
                      GIMP_COLOR_PROFILE_STORE_INDEX, -1,
                      -1);
}

/**
 * _gimp_color_profile_store_history_add:
 * @store: a #GimpColorProfileStore
 * @file:  file of the profile to add (or %NULL)
 * @label: label to use for the profile (or %NULL)
 * @iter:  a #GtkTreeIter
 *
 * Return value: %TRUE if the iter is valid and pointing to the item
 *
 * Since: 2.4
 **/
gboolean
_gimp_color_profile_store_history_add (GimpColorProfileStore *store,
                                       GFile                 *file,
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
      GFile *this;

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
                          GIMP_COLOR_PROFILE_STORE_FILE, &this,
                          -1);

      if ((this && file && g_file_equal (this, file)) ||
          (! this && ! file))
        {
          /*  update the label  */
          if (label && *label)
            gtk_list_store_set (GTK_LIST_STORE (store), iter,
                                GIMP_COLOR_PROFILE_STORE_LABEL, label,
                                -1);

          if (this)
            g_object_unref (this);

          return TRUE;
        }

      if (this)
        g_object_unref (this);
    }

  if (! file)
    return FALSE;

  if (label && *label)
    {
      iter_valid = gimp_color_profile_store_history_insert (store, iter,
                                                            file, label,
                                                            ++max);
    }
  else
    {
      const gchar *utf8     = gimp_file_get_utf8_name (file);
      gchar       *basename = g_path_get_basename (utf8);

      iter_valid = gimp_color_profile_store_history_insert (store, iter,
                                                            file, basename,
                                                            ++max);
      g_free (basename);
    }

  return iter_valid;
}

/**
 * _gimp_color_profile_store_history_reorder
 * @store: a #GimpColorProfileStore
 * @iter:  a #GtkTreeIter
 *
 * Moves the entry pointed to by @iter to the front of the MRU list.
 *
 * Since: 2.4
 **/
void
_gimp_color_profile_store_history_reorder (GimpColorProfileStore *store,
                                           GtkTreeIter           *iter)
{
  GtkTreeModel *model;
  gint          index;
  gboolean      iter_valid;

  g_return_if_fail (GIMP_IS_COLOR_PROFILE_STORE (store));
  g_return_if_fail (iter != NULL);

  model = GTK_TREE_MODEL (store);

  gtk_tree_model_get (model, iter,
                      GIMP_COLOR_PROFILE_STORE_INDEX, &index,
                      -1);

  if (index == 0)
    return;  /* already at the top */

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint type;
      gint this_index;

      gtk_tree_model_get (model, iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          GIMP_COLOR_PROFILE_STORE_INDEX,     &this_index,
                          -1);

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_FILE && this_index > -1)
        {
          if (this_index < index)
            {
              this_index++;
            }
          else if (this_index == index)
            {
              this_index = 0;
            }

          gtk_list_store_set (GTK_LIST_STORE (store), iter,
                              GIMP_COLOR_PROFILE_STORE_INDEX, this_index,
                              -1);
        }
    }
}

static gboolean
gimp_color_profile_store_history_insert (GimpColorProfileStore *store,
                                         GtkTreeIter           *iter,
                                         GFile                 *file,
                                         const gchar           *label,
                                         gint                   index)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter   sibling;
  gboolean      iter_valid;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);
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
                        GIMP_COLOR_PROFILE_STORE_FILE,  file,
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
  gchar       *path  = NULL;

  if (gimp_scanner_parse_string (scanner, &label) &&
      gimp_scanner_parse_string (scanner, &path))
    {
      GFile *file = NULL;

      if (g_str_has_prefix (path, "file://"))
        {
          file = g_file_new_for_uri (path);
        }
      else
        {
          file = gimp_file_new_for_config_path (path, NULL);
        }

      if (file)
        {
          if (g_file_query_file_type (file, 0, NULL) == G_FILE_TYPE_REGULAR)
            {
              gimp_color_profile_store_history_insert (store, &iter,
                                                       file, label, index);
            }

          g_object_unref (file);
        }

      g_free (label);
      g_free (path);

      return G_TOKEN_RIGHT_PAREN;
    }

  g_free (label);
  g_free (path);

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

  g_scanner_scope_add_symbol (scanner, 0, "color-profile", NULL);

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
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar            *labels[HISTORY_SIZE] = { NULL, };
  GFile            *files[HISTORY_SIZE]  = { NULL, };
  gboolean          iter_valid;
  gint              i;

  writer = gimp_config_writer_new_file (filename,
                                        TRUE,
                                        "GIMP color profile history",
                                        error);
  if (! writer)
    return FALSE;

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint type;
      gint index;

      gtk_tree_model_get (model, &iter,
                          GIMP_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          GIMP_COLOR_PROFILE_STORE_INDEX,     &index,
                          -1);

      if (type == GIMP_COLOR_PROFILE_STORE_ITEM_FILE &&
          index >= 0                                 &&
          index < HISTORY_SIZE)
        {
          if (labels[index] || files[index])
            g_warning ("%s: double index %d", G_STRFUNC, index);

          gtk_tree_model_get (model, &iter,
                              GIMP_COLOR_PROFILE_STORE_LABEL,
                              &labels[index],
                              GIMP_COLOR_PROFILE_STORE_FILE,
                              &files[index],
                              -1);
        }
    }


  for (i = 0; i < HISTORY_SIZE; i++)
    {
      if (files[i] && labels[i])
        {
          gchar *path = gimp_file_get_config_path (files[i], NULL);

          if (path)
            {
              gimp_config_writer_open (writer, "color-profile");
              gimp_config_writer_string (writer, labels[i]);
              gimp_config_writer_string (writer, path);
              gimp_config_writer_close (writer);

              g_free (path);
            }
        }

      if (files[i])
        g_object_unref (files[i]);

      g_free (labels[i]);
    }

  return gimp_config_writer_finish (writer,
                                    "end of color profile history", error);
}
