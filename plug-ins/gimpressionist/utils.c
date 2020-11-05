/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * utils.c - various utility routines that don't fit anywhere else. Usually
 * these routines don't affect the state of the program.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>

#include "gimpressionist.h"

#include "libgimp/stdplugins-intl.h"


/* Mathematical Utilities */

double
dist (double x, double y, double end_x, double end_y)
{
  double dx = end_x - x;
  double dy = end_y - y;
  return sqrt (dx * dx + dy * dy);
}

double
getsiz_proto (double x, double y, int n, smvector_t *vec,
              double smstrexp, int voronoi)
{
  int    i;
  double sum, ssum, dst;
  int    first = 0, last;

  if ((x < 0.0) || (x > 1.0))
    g_warning ("HUH? x = %f\n",x);

#if 0
  if (from == 0)
    {
      n = numsmvect;
      vec = smvector;
      smstrexp = gtk_adjustment_get_value (smstrexpadjust);
      voronoi = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (size_voronoi));
    }
  else
    {
      n = pcvals.num_size_vectors;
      vec = pcvals.size_vectors;
      smstrexp = pcvals.size_strength_exponent;
      voronoi = pcvals.size_voronoi;
    }
#endif

  if (voronoi)
    {
      gdouble bestdist = -1.0;
      for (i = 0; i < n; i++)
         {
           dst = dist (x, y, vec[i].x, vec[i].y);
           if ((bestdist < 0.0) || (dst < bestdist))
             {
               bestdist = dst;
               first = i;
             }
         }
      last = first+1;
    }
  else
    {
      first = 0;
      last = n;
    }

  sum = ssum = 0.0;
  for (i = first; i < last; i++)
    {
      gdouble s = vec[i].str;

      dst = dist (x,y,vec[i].x,vec[i].y);
      dst = pow (dst, smstrexp);
      if (dst < 0.0001)
        dst = 0.0001;
      s = s / dst;

      sum += vec[i].siz * s;
      ssum += 1.0/dst;
  }
  sum = sum / ssum / 100.0;
  return CLAMP (sum, 0.0, 1.0);
}


/* String and Path Manipulation Routines */


static GList *parsepath_cached_path = NULL;

/* This function is memoized. Once it finds the value it permanently
 * caches it
 * */
GList *
parsepath (void)
{
  gchar *rc_path, *path;

  if (parsepath_cached_path)
    return parsepath_cached_path;

  path = gimp_gimprc_query ("gimpressionist-path");
  if (path)
    {
      rc_path = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
      g_free (path);
    }
  else
    {
      GFile *gimprc    = gimp_directory_file ("gimprc", NULL);
      gchar *full_path = gimp_config_build_data_path ("gimpressionist");
      gchar *esc_path  = g_strescape (full_path, NULL);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                 "gflare-path", "gflare-path",
                 esc_path, gimp_file_get_utf8_name (gimprc));

      g_object_unref (gimprc);
      g_free (esc_path);

      rc_path = gimp_config_path_expand (full_path, TRUE, NULL);
      g_free (full_path);
    }

  parsepath_cached_path = gimp_path_parse (rc_path, 256, FALSE, NULL);

  g_free (rc_path);

  return parsepath_cached_path;
}

void
free_parsepath_cache (void)
{
  if (parsepath_cached_path != NULL)
    return;

  g_list_free_full (parsepath_cached_path, (GDestroyNotify) g_free);
  parsepath_cached_path = NULL;
}

gchar *
findfile (const gchar *fn)
{
  GList *rcpath;
  GList *thispath;
  gchar *filename;

  g_return_val_if_fail (fn != NULL, NULL);

  rcpath = parsepath ();

  thispath = rcpath;

  while (thispath)
    {
      filename = g_build_filename (thispath->data, fn, NULL);
      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        return filename;
      g_free (filename);
      thispath = thispath->next;
    }
  return NULL;
}

/* GUI Routines */

void
reselect (GtkWidget *view,
          gchar     *fname)
{
  GtkTreeModel     *model;
  GtkTreeSelection *selection;
  GtkTreeIter       iter;
  char             *tmpfile;

  tmpfile = strrchr (fname, '/');
  if (tmpfile)
    fname = ++tmpfile;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      gboolean quit = FALSE;
      do
        {
          gchar *name;

          gtk_tree_model_get (model, &iter, 0, &name, -1);
          if (!strcmp(name, fname))
            {
              GtkTreePath *tree_path;
              gtk_tree_selection_select_iter (selection, &iter);
              tree_path = gtk_tree_model_get_path (model,
                                                   &iter);
              gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view),
                                            tree_path,
                                            NULL,
                                            TRUE,
                                            0.5,
                                            0.5);
              gtk_tree_path_free (tree_path);
              quit = TRUE;
            }
          g_free (name);

        } while ((!quit) && gtk_tree_model_iter_next (model, &iter));
    }
}

static void
readdirintolist_real (const char   *subdir,
                      GtkWidget    *view,
                      char         *selected,
                      gboolean      with_filename_column,
                      gchar      *(*get_object_name_cb) (const gchar *dir,
                                                         gchar       *filename,
                                                         void        *context),
                      void         *context)
{
  gchar           *fpath;
  const gchar     *de;
  GDir            *dir;
  GList           *flist = NULL;
  GtkTreeIter      iter;
  GtkListStore     *store;
  GtkTreeSelection *selection;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

  if (selected)
    {
      if (!selected[0])
        selected = NULL;
      else
        {
          char *nsel;

          nsel = strrchr (selected, '/');
          if (nsel) selected = ++nsel;
        }
    }

  dir = g_dir_open (subdir, 0, NULL);

  if (!dir)
    return;

  for (;;)
    {
      gboolean file_exists;

      de = g_dir_read_name (dir);
      if (!de)
        break;

      fpath = g_build_filename (subdir, de, NULL);
      file_exists = g_file_test (fpath, G_FILE_TEST_IS_REGULAR);
      g_free (fpath);

      if (!file_exists)
        continue;

      flist = g_list_insert_sorted (flist, g_strdup (de),
                                    (GCompareFunc)g_ascii_strcasecmp);
    }
  g_dir_close (dir);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  while (flist)
    {
      gtk_list_store_append (store, &iter);
      /* Set the filename */
      gtk_list_store_set (store, &iter, PRESETS_LIST_COLUMN_FILENAME,
                          flist->data, -1);
      /* Set the object name */
      if (with_filename_column)
        {
          gchar * object_name;
          object_name = get_object_name_cb (subdir, flist->data, context);
          if (object_name)
            {
              gtk_list_store_set (store, &iter,
                                  PRESETS_LIST_COLUMN_OBJECT_NAME,
                                  object_name, -1);
              g_free (object_name);
            }
          else
            {
              /* Default to the filename */
              gtk_list_store_set (store, &iter, 1, flist->data, -1);
            }
        }

      if (selected)
        {
          if (!strcmp (flist->data, selected))
            {
              gtk_tree_selection_select_iter (selection, &iter);
            }
        }
      g_free (flist->data);
      flist = g_list_remove (flist, flist->data);
    }

  if (!selected)
    {
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
        gtk_tree_selection_select_iter (selection, &iter);
    }
}

void
readdirintolist_extended (const char   *subdir,
                          GtkWidget    *view,
                          char         *selected,
                          gboolean      with_filename_column,
                          gchar      *(*get_object_name_cb) (const gchar *dir,
                                                             gchar       *filename,
                                                             void        *context),
                          void         *context)
{
  char *tmpdir;
  GList *thispath = parsepath ();

  while (thispath)
    {
      tmpdir = g_build_filename ((gchar *) thispath->data, subdir, NULL);
      readdirintolist_real (tmpdir, view, selected, with_filename_column,
                            get_object_name_cb, context);
      g_free (tmpdir);
      thispath = thispath->next;
    }
}

void
readdirintolist (const char *subdir,
                 GtkWidget  *view,
                 char       *selected)
{
  readdirintolist_extended (subdir, view, selected, FALSE, NULL, NULL);
}

/*
 * Creates a radio button.
 * box - the containing box.
 * orient_type - The orientation ID
 * label, help_string - self-describing
 * radio_group -
 *      A pointer to a radio group. The function assigns its value
 *      as the radio group of the radio button. Afterwards, it assigns it
 *      a new value of the new radio group of the button.
 *      This is useful to group buttons. Just reset the variable to NULL,
 *      to create a new group.
 * */
GtkWidget *
create_radio_button (GtkWidget    *box,
                     int           orient_type,
                     void        (*callback) (GtkWidget *wg, void *d),
                     const gchar  *label,
                     const gchar  *help_string,
                     GSList      **radio_group,
                     GtkWidget   **buttons_array)
{
  GtkWidget *tmpw;

  buttons_array[orient_type] = tmpw =
      gtk_radio_button_new_with_label ((*radio_group), label);
  gtk_box_pack_start (GTK_BOX (box), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);

  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (callback), GINT_TO_POINTER (orient_type));
  gimp_help_set_help_data (tmpw, help_string, NULL);

  *radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw));

  return tmpw;
}

void
gimpressionist_scale_entry_update_double (GimpLabelSpin *entry,
                                          gdouble       *value)
{
  *value = gimp_label_spin_get_value (entry);
}

void
gimpressionist_scale_entry_update_int (GimpLabelSpin *entry,
                                       gint          *value)
{
  *value = (gint) gimp_label_spin_get_value (entry);
}
