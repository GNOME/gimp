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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "presets.h"

#include "color.h"
#include "general.h"
#include "orientation.h"
#include "placement.h"
#include "size.h"


#include "libgimp/stdplugins-intl.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#define PRESETMAGIC "Preset"

static GtkWidget    *presetnameentry  = NULL;
static GtkWidget    *presetlist       = NULL;
static GtkWidget    *presetdesclabel  = NULL;
static GtkWidget    *presetsavebutton = NULL;
static GtkWidget    *delete_button    = NULL;
static GtkListStore *store            = NULL;
static gchar        *selected_preset_orig_name = NULL;
static gchar        *selected_preset_filename  = NULL;

static gboolean
can_delete_preset (const gchar *abs)
{
  gchar *user_data_dir;
  gboolean ret;

  user_data_dir = g_strconcat (gimp_directory (),
                               G_DIR_SEPARATOR_S,
                               NULL);


  ret = (!strncmp (abs, user_data_dir, strlen (user_data_dir)));

  g_free (user_data_dir);

  return ret;
}

void
preset_save_button_set_sensitive (gboolean s)
{
  if (GTK_IS_WIDGET (presetsavebutton))
    gtk_widget_set_sensitive (GTK_WIDGET (presetsavebutton), s);
}

void
preset_free (void)
{
  g_free (selected_preset_orig_name);
  g_free (selected_preset_filename);
}

static void set_preset_description_text (const gchar *text)
{
  gtk_label_set_text (GTK_LABEL (presetdesclabel), text);
}

static char presetdesc[4096] = "";

static const char *factory_defaults = "<Factory defaults>";

static gchar *
get_early_line_from_preset (gchar *full_path, const gchar *prefix)
{
  FILE *f;
  gchar line[4096];
  gint prefix_len, line_idx;

  prefix_len = strlen (prefix);
  f = g_fopen (full_path, "rt");
  if (f)
    {
      /* Skip the preset magic. */
      fgets (line, 10, f);
      if (!strncmp (line, PRESETMAGIC, 4))
        {
          for (line_idx = 0; line_idx < 5; line_idx++)
            {
              if (!fgets (line, sizeof (line), f))
                break;
              g_strchomp (line);
              if (!strncmp (line, prefix, prefix_len))
                {
                  fclose(f);
                  return g_strdup (line+prefix_len);
                }
            }
        }
      fclose (f);
    }
  return NULL;
}

static gchar *
get_object_name (const gchar *dir,
                 gchar       *filename,
                 void        *context)
{
  gchar *ret = NULL, *unprocessed_line = NULL;
  gchar *full_path = NULL;

  /* First try to extract the object's name (= user-friendly description)
   * from the preset file
   * */

  full_path = g_build_filename (dir, filename, NULL);

  unprocessed_line = get_early_line_from_preset (full_path, "name=");
  if (unprocessed_line)
    {
      ret = g_strcompress (unprocessed_line);
      g_free (unprocessed_line);
    }
  else
    {
      /* The object name defaults to a filename-derived description */
      ret = g_filename_display_basename (full_path);
    }

  g_free (full_path);

  return ret;
}


static void
preset_read_dir_into_list (void)
{
  readdirintolist_extended ("Presets", presetlist, NULL, TRUE,
                            get_object_name, NULL);
}

static gchar *
preset_create_filename (const gchar *basename,
                        const gchar *dest_dir)
{
  gchar *fullpath;
  gchar *safe_name;
  gint   i;
  gint   unum = 1;

  g_return_val_if_fail (basename != NULL, NULL);
  g_return_val_if_fail (dest_dir != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (dest_dir), NULL);

  safe_name = g_filename_from_utf8 (basename, -1, NULL, NULL, NULL);

  if (safe_name[0] == '.')
    safe_name[0] = '-';

  for (i = 0; safe_name[i]; i++)
    if (safe_name[i] == G_DIR_SEPARATOR || g_ascii_isspace (safe_name[i]))
      safe_name[i] = '-';

  fullpath = g_build_filename (dest_dir, safe_name, NULL);

  while (g_file_test (fullpath, G_FILE_TEST_EXISTS))
    {
      gchar *filename;

      g_free (fullpath);

      filename = g_strdup_printf ("%s-%d", safe_name, unum++);

      fullpath = g_build_filename (dest_dir, filename, NULL);

      g_free (filename);
    }

  g_free (safe_name);

  return fullpath;
}


static void
add_factory_defaults (void)
{
  GtkTreeIter iter;

  gtk_list_store_append (store, &iter);
  /* Set the filename. */
  gtk_list_store_set (store, &iter, PRESETS_LIST_COLUMN_FILENAME,
                      factory_defaults, -1);
  /* Set the object name. */
  gtk_list_store_set (store, &iter, PRESETS_LIST_COLUMN_OBJECT_NAME,
                      factory_defaults, -1);

}

static void
preset_refresh_presets (void)
{
  gtk_list_store_clear (store);
  add_factory_defaults ();
  preset_read_dir_into_list ();
}

static int
load_old_preset (const gchar *fname)
{
  FILE *f;
  int   len;

  f = g_fopen (fname, "rb");
  if (!f)
    {
      g_printerr ("Error opening file \"%s\" for reading!\n",
                  gimp_filename_to_utf8 (fname));
      return -1;
    }
  len = fread (&pcvals, 1, sizeof (pcvals), f);
  fclose (f);

  return (len != sizeof (pcvals)) ? -1 : 0;
}

static unsigned int
hexval (char c)
{
  c = g_ascii_tolower (c);
  if ((c >= 'a') && (c <= 'f'))
    return c - 'a' + 10;
  if ((c >= '0') && (c <= '9'))
    return c - '0';
  return 0;
}

static char *
parse_rgb_string (const gchar *s)
{
  static char col[3];
  col[0] = (hexval (s[0]) << 4) | hexval (s[1]);
  col[1] = (hexval (s[2]) << 4) | hexval (s[3]);
  col[2] = (hexval (s[4]) << 4) | hexval (s[5]);
  return col;
}

static void
set_orient_vector (const gchar *str)
{
  const gchar *tmps = str;
  int n;

  n = atoi (tmps);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].x = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].y = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].dir = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].dx = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].dy = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].str = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.orient_vectors[n].type = atoi (++tmps);

}

static void set_size_vector (const gchar *str)
{
  const gchar *tmps = str;
  int          n;

  n = atoi (tmps);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.size_vectors[n].x = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.size_vectors[n].y = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.size_vectors[n].siz = g_ascii_strtod (++tmps, NULL);

  if (!(tmps = strchr (tmps, ',')))
    return;
  pcvals.size_vectors[n].str = g_ascii_strtod (++tmps, NULL);

}

static void
parse_desc (const gchar *str, gchar *d, gssize d_len)
{
  gchar *dest = g_strcompress (str);

  g_strlcpy (d, dest, d_len);

  g_free (dest);
}

static void
set_values (const gchar *key, const gchar *val)
{
  if (!strcmp (key, "desc"))
    parse_desc (val, presetdesc, sizeof (presetdesc));
  else if (!strcmp (key, "orientnum"))
    pcvals.orient_num = atoi (val);
  else if (!strcmp (key, "orientfirst"))
    pcvals.orient_first = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "orientlast"))
    pcvals.orient_last = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "orienttype"))
   pcvals.orient_type = orientation_type_input (atoi (val));

  else if (!strcmp (key, "sizenum"))
    pcvals.size_num = atoi (val);
  else if (!strcmp (key, "sizefirst"))
    pcvals.size_first = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "sizelast"))
    pcvals.size_last = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "sizetype"))
   pcvals.size_type = size_type_input (atoi (val));

  else if (!strcmp (key, "brushrelief"))
    pcvals.brush_relief = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "brushscale"))
    {
      /* For compatibility */
      pcvals.size_num = 1;
      pcvals.size_first = pcvals.size_last = g_ascii_strtod (val, NULL);
    }
  else if (!strcmp (key, "brushdensity"))
    pcvals.brush_density = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "brushgamma"))
    pcvals.brushgamma = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "brushaspect"))
    pcvals.brush_aspect = g_ascii_strtod (val, NULL);

  else if (!strcmp (key, "generalbgtype"))
    pcvals.general_background_type = general_bg_type_input (atoi (val));
  else if (!strcmp (key, "generaldarkedge"))
    pcvals.general_dark_edge = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "generalpaintedges"))
    pcvals.general_paint_edges = atoi (val);
  else if (!strcmp (key, "generaltileable"))
    pcvals.general_tileable = atoi (val);
  else if (!strcmp (key, "generaldropshadow"))
    pcvals.general_drop_shadow = atoi (val);
  else if (!strcmp (key, "generalshadowdarkness"))
    pcvals.general_shadow_darkness = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "generalshadowdepth"))
    pcvals.general_shadow_depth = atoi (val);
  else if (!strcmp (key, "generalshadowblur"))
    pcvals.general_shadow_blur = atoi (val);
  else if (!strcmp (key, "devthresh"))
    pcvals.devthresh = g_ascii_strtod (val, NULL);

  else if (!strcmp (key, "paperrelief"))
    pcvals.paper_relief = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "paperscale"))
    pcvals.paper_scale = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "paperinvert"))
    pcvals.paper_invert = atoi (val);
  else if (!strcmp (key, "paperoverlay"))
    pcvals.paper_overlay = atoi (val);

  else if (!strcmp (key, "placetype"))
    pcvals.place_type = place_type_input (atoi (val));
  else if (!strcmp (key, "placecenter"))
    pcvals.placement_center = atoi (val);

  else if (!strcmp (key, "selectedbrush"))
    g_strlcpy (pcvals.selected_brush, val, sizeof (pcvals.selected_brush));
  else if (!strcmp (key, "selectedpaper"))
    g_strlcpy (pcvals.selected_paper, val, sizeof (pcvals.selected_paper));

  else if (!strcmp (key, "color"))
    {
      char *c = parse_rgb_string (val);
      gimp_rgba_set_uchar (&pcvals.color, c[0], c[1], c[2], 255);
    }

  else if (!strcmp (key, "numorientvector"))
    pcvals.num_orient_vectors = atoi (val);
  else if (!strcmp (key, "orientvector"))
    set_orient_vector (val);
  else if (!strcmp (key, "orientangoff"))
   pcvals.orient_angle_offset = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "orientstrexp"))
   pcvals.orient_strength_exponent = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "orientvoronoi"))
   pcvals.orient_voronoi = atoi (val);

  else if (!strcmp (key, "numsizevector"))
    pcvals.num_size_vectors = atoi (val);
  else if (!strcmp (key, "sizevector"))
    set_size_vector (val);
  else if (!strcmp (key, "sizestrexp"))
   pcvals.size_strength_exponent = g_ascii_strtod (val, NULL);
  else if (!strcmp (key, "sizevoronoi"))
   pcvals.size_voronoi = atoi (val);

  else if (!strcmp (key, "colortype"))
    pcvals.color_type = color_type_input (atoi (val));
  else if (!strcmp (key, "colornoise"))
    pcvals.color_noise = g_ascii_strtod (val, NULL);
}

static int
load_preset (const gchar *fn)
{
  char  line[1024] = "";
  FILE *f;

  f = g_fopen (fn, "rt");
  if (!f)
    {
      g_printerr ("Error opening file \"%s\" for reading!\n",
                  gimp_filename_to_utf8 (fn));
      return -1;
    }
  fgets (line, 10, f);
  if (strncmp (line, PRESETMAGIC, 4))
    {
      fclose (f);
      return load_old_preset (fn);
    }

  restore_default_values ();

  while (!feof (f))
    {
      char *tmps;

      if (!fgets (line, 1024, f))
        break;
      g_strchomp (line);
      tmps = strchr (line, '=');
      if (!tmps)
        continue;
      *tmps = '\0';
      tmps++;
      set_values (line, tmps);
    }
  fclose (f);
  return 0;
}

int
select_preset (const gchar *preset)
{
  int ret = SELECT_PRESET_OK;
  /* I'm copying this behavior as is. As it seems applying the
   * factory_defaults preset does nothing, which I'm not sure
   * if that was what the author intended.
   *              -- Shlomi Fish
   */
  if (strcmp (preset, factory_defaults))
    {
      gchar *rel = g_build_filename ("Presets", preset, NULL);
      gchar *abs = findfile (rel);

      g_free (rel);

      if (abs)
        {
          if (load_preset (abs))
            ret = SELECT_PRESET_LOAD_FAILED;

          g_free (abs);
        }
      else
        {
          ret = SELECT_PRESET_FILE_NOT_FOUND;
        }
    }
  if (ret == SELECT_PRESET_OK)
    {
      /* This is so the colorbrushes param (that is not stored in the
       * preset will be set correctly upon the preset loading.
       * */
      set_colorbrushes (pcvals.selected_brush);
    }

  return ret;
}

static void
apply_preset (GtkWidget *w, GtkTreeSelection *selection)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *preset;

      gtk_tree_model_get (model, &iter, PRESETS_LIST_COLUMN_FILENAME,
                          &preset, -1);

      select_preset (preset);

      restore_values ();

      /* g_free (preset); */
      g_free (selected_preset_filename);
      selected_preset_filename = preset;
    }
}

static void
delete_preset (GtkWidget *w, GtkTreeSelection *selection)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *preset;

      gtk_tree_model_get (model, &iter, PRESETS_LIST_COLUMN_FILENAME,
                          &preset, -1);

      if (preset)
        {
          gchar *rel = g_build_filename ("Presets", preset, NULL);
          gchar *abs = findfile (rel);

          g_free (rel);

          if (abs)
            {
              /* Don't delete global presets - bug # 147483 */
              if (can_delete_preset (abs))
                g_unlink (abs);

              g_free (abs);
            }

          preset_refresh_presets ();

          g_free (preset);
        }
    }
}

static void save_preset (void);

static void
preset_desc_callback (GtkTextBuffer *buffer, gpointer data)
{
  char        *str;
  GtkTextIter  start, end;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  str = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  g_strlcpy (presetdesc, str, sizeof (presetdesc));
  g_free (str);
}

static void
save_preset_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  gtk_widget_destroy (widget);

  if (response_id == GTK_RESPONSE_OK)
    save_preset ();
}

static void
create_save_preset (GtkWidget *parent)
{
  static GtkWidget *window = NULL;
  GtkWidget        *box, *label;
  GtkWidget        *swin, *text;
  GtkTextBuffer    *buffer;

  if (window)
    {
      gtk_window_present (GTK_WINDOW (window));
      return;
    }

  window = gimp_dialog_new (_("Save Current"), PLUG_IN_ROLE,
                            gtk_widget_get_toplevel (parent), 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (window),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (window, "response",
                    G_CALLBACK (save_preset_response),
                    NULL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &window);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window))),
                      box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  label = gtk_label_new (_("Description:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), swin, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (swin);

  buffer = gtk_text_buffer_new (NULL);
  g_signal_connect (buffer, "changed",
                    G_CALLBACK (preset_desc_callback), NULL);
  gtk_text_buffer_set_text (buffer, presetdesc, -1);

  text = gtk_text_view_new_with_buffer (buffer);
  gtk_widget_set_size_request (text, -1, 192);
  gtk_container_add (GTK_CONTAINER (swin), text);
  gtk_widget_show (text);

  gtk_widget_show (window);
}

static void
save_preset (void)
{
  const gchar *preset_name;

  gchar *fname, *presets_dir_path;
  FILE  *f;
  GList *thispath;
  gchar  buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar  vbuf[6][G_ASCII_DTOSTR_BUF_SIZE];
  guchar color[3];
  gint   i;
  gchar *desc_escaped, *preset_name_escaped;

  preset_name = gtk_entry_get_text (GTK_ENTRY (presetnameentry));
  thispath = parsepath ();
  store_values ();

  if (!thispath)
    {
      g_printerr ("Internal error: (save_preset) thispath == NULL\n");
      return;
    }

  /* Create the ~/.gimp-$VER/gimpressionist/Presets directory if
   * it doesn't already exists.
   *
   */
  presets_dir_path = g_build_filename ((const gchar *) thispath->data,
                                       "Presets",
                                       NULL);

  if (! g_file_test (presets_dir_path, G_FILE_TEST_IS_DIR))
    {
      if (g_mkdir (presets_dir_path,
                   S_IRUSR | S_IWUSR | S_IXUSR |
                   S_IRGRP | S_IXGRP |
                   S_IROTH | S_IXOTH) == -1)
        {
          g_printerr ("Error creating folder \"%s\"!\n",
                      gimp_filename_to_utf8 (presets_dir_path));
          g_free (presets_dir_path);
          return;
        }
    }

  /* Check if the user-friendly name has changed. If so, then save it
   * under a new file. If not - use the same file name.
   */
  if (selected_preset_orig_name &&
      strcmp (preset_name, selected_preset_orig_name) == 0)
    {
      fname = g_build_filename (presets_dir_path,
                                selected_preset_filename,
                                NULL);
    }
  else
    {
      fname = preset_create_filename (preset_name, presets_dir_path);
    }

  g_free (presets_dir_path);

  if (!fname)
    {
      g_printerr ("Error building a filename for preset \"%s\"!\n",
                  preset_name);
      return;
    }

  f = g_fopen (fname, "wt");
  if (!f)
    {
      g_printerr ("Error opening file \"%s\" for writing!\n",
                  gimp_filename_to_utf8 (fname));
      g_free (fname);
      return;
    }

  fprintf (f, "%s\n", PRESETMAGIC);
  desc_escaped = g_strescape (presetdesc, NULL);
  fprintf (f, "desc=%s\n", desc_escaped);
  g_free (desc_escaped);
  preset_name_escaped = g_strescape (preset_name, NULL);
  fprintf (f, "name=%s\n", preset_name_escaped);
  g_free (preset_name_escaped);
  fprintf (f, "orientnum=%d\n", pcvals.orient_num);
  fprintf (f, "orientfirst=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_first));
  fprintf (f, "orientlast=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_last));
  fprintf (f, "orienttype=%d\n", pcvals.orient_type);

  fprintf (f, "sizenum=%d\n", pcvals.size_num);
  fprintf (f, "sizefirst=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_first));
  fprintf (f, "sizelast=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_last));
  fprintf (f, "sizetype=%d\n", pcvals.size_type);

  fprintf (f, "brushrelief=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brush_relief));
  fprintf (f, "brushdensity=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brush_density));
  fprintf (f, "brushgamma=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brushgamma));
  fprintf (f, "brushaspect=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brush_aspect));

  fprintf (f, "generalbgtype=%d\n", pcvals.general_background_type);
  fprintf (f, "generaldarkedge=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.general_dark_edge));
  fprintf (f, "generalpaintedges=%d\n", pcvals.general_paint_edges);
  fprintf (f, "generaltileable=%d\n", pcvals.general_tileable);
  fprintf (f, "generaldropshadow=%d\n", pcvals.general_drop_shadow);
  fprintf (f, "generalshadowdarkness=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.general_shadow_darkness));
  fprintf (f, "generalshadowdepth=%d\n", pcvals.general_shadow_depth);
  fprintf (f, "generalshadowblur=%d\n", pcvals.general_shadow_blur);
  fprintf (f, "devthresh=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.devthresh));

  fprintf (f, "paperrelief=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.paper_relief));
  fprintf (f, "paperscale=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.paper_scale));
  fprintf (f, "paperinvert=%d\n", pcvals.paper_invert);
  fprintf (f, "paperoverlay=%d\n", pcvals.paper_overlay);

  fprintf (f, "selectedbrush=%s\n", pcvals.selected_brush);
  fprintf (f, "selectedpaper=%s\n", pcvals.selected_paper);

  gimp_rgb_get_uchar (&pcvals.color, &color[0], &color[1], &color[2]);
  fprintf (f, "color=%02x%02x%02x\n", color[0], color[1], color[2]);

  fprintf (f, "placetype=%d\n", pcvals.place_type);
  fprintf (f, "placecenter=%d\n", pcvals.placement_center);

  fprintf (f, "numorientvector=%d\n", pcvals.num_orient_vectors);
  for (i = 0; i < pcvals.num_orient_vectors; i++)
    {
      g_ascii_formatd (vbuf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_vectors[i].x);
      g_ascii_formatd (vbuf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_vectors[i].y);
      g_ascii_formatd (vbuf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_vectors[i].dir);
      g_ascii_formatd (vbuf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_vectors[i].dx);
      g_ascii_formatd (vbuf[4], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_vectors[i].dy);
      g_ascii_formatd (vbuf[5], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orient_vectors[i].str);

      fprintf (f, "orientvector=%d,%s,%s,%s,%s,%s,%s,%d\n", i,
               vbuf[0], vbuf[1], vbuf[2], vbuf[3], vbuf[4], vbuf[5],
               pcvals.orient_vectors[i].type);
    }

  fprintf (f, "orientangoff=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            pcvals.orient_angle_offset));
  fprintf (f, "orientstrexp=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                            pcvals.orient_strength_exponent));
  fprintf (f, "orientvoronoi=%d\n", pcvals.orient_voronoi);

  fprintf (f, "numsizevector=%d\n", pcvals.num_size_vectors);
  for (i = 0; i < pcvals.num_size_vectors; i++)
    {
      g_ascii_formatd (vbuf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_vectors[i].x);
      g_ascii_formatd (vbuf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_vectors[i].y);
      g_ascii_formatd (vbuf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_vectors[i].siz);
      g_ascii_formatd (vbuf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_vectors[i].str);
      fprintf (f, "sizevector=%d,%s,%s,%s,%s\n", i,
               vbuf[0], vbuf[1], vbuf[2], vbuf[3]);
    }
  fprintf (f, "sizestrexp=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.size_strength_exponent));
  fprintf (f, "sizevoronoi=%d\n", pcvals.size_voronoi);

  fprintf (f, "colortype=%d\n", pcvals.color_type);
  fprintf (f, "colornoise=%s\n",
           g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.color_noise));

  fclose (f);
  preset_refresh_presets ();
  reselect (presetlist, fname);

  g_free (fname);
}

static void
read_description (const char *fn)
{
  char *rel_fname;
  char *fname;
  gchar *unprocessed_line;

  rel_fname = g_build_filename ("Presets", fn, NULL);
  fname   = findfile (rel_fname);
  g_free (rel_fname);

  if (!fname)
    {
      if (!strcmp (fn, factory_defaults))
        {
          gtk_widget_set_sensitive (delete_button, FALSE);
          set_preset_description_text (_("Gimpressionist Defaults"));
        }
      else
        {
          set_preset_description_text ("");
        }
      return;
    }

  /* Don't delete global presets - bug # 147483 */
  gtk_widget_set_sensitive (delete_button, can_delete_preset (fname));

  unprocessed_line = get_early_line_from_preset (fname, "desc=");
  g_free (fname);

  if (unprocessed_line)
    {
      char tmplabel[4096];
      parse_desc (unprocessed_line, tmplabel, sizeof (tmplabel));
      g_free (unprocessed_line);
      set_preset_description_text (tmplabel);
    }
  else
    {
      set_preset_description_text ("");
    }
}

static void
presets_list_select_preset (GtkTreeSelection *selection,
                            gpointer          data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *preset_name;
      gchar *preset_filename;

      gtk_tree_model_get (model, &iter, PRESETS_LIST_COLUMN_OBJECT_NAME,
                          &preset_name, -1);
      gtk_tree_model_get (model, &iter, PRESETS_LIST_COLUMN_FILENAME,
                          &preset_filename, -1);

      /* TODO : Maybe make the factory defaults behavior in regards
       * to the preset's object name and filename more robust?
       *
       */
      if (strcmp (preset_filename, factory_defaults))
        {
          gtk_entry_set_text (GTK_ENTRY (presetnameentry), preset_name);
          g_free (selected_preset_orig_name);
          g_free (selected_preset_filename);
          selected_preset_orig_name = g_strdup (preset_name);
          selected_preset_filename = g_strdup (selected_preset_filename);
        }

      read_description (preset_filename);

      g_free (preset_name);
      g_free (preset_filename);
    }
}

static GtkWidget *
create_presets_list (GtkWidget *parent)
{
  GtkListStore      *store;
  GtkTreeSelection  *selection;
  GtkCellRenderer   *renderer;
  GtkTreeViewColumn *column;
  GtkWidget         *swin, *view;

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (parent), swin, FALSE, FALSE, 0);
  gtk_widget_show (swin);
  gtk_widget_set_size_request (swin, 200, -1);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
  g_object_unref (store);
  gtk_widget_show (view);

  renderer = gtk_cell_renderer_text_new ();

  column =
      gtk_tree_view_column_new_with_attributes ("Preset", renderer,
                                                "text",
                                                PRESETS_LIST_COLUMN_OBJECT_NAME,
                                                NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);


  gtk_container_add (GTK_CONTAINER (swin), view);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (presets_list_select_preset),
                    NULL);

  return view;
}

void
create_presetpage (GtkNotebook *notebook)
{
  GtkWidget *vbox, *hbox, *box1, *box2, *thispage;
  GtkWidget *view;
  GtkWidget *tmpw;
  GtkWidget *label;
  GtkTreeSelection *selection;

  label = gtk_label_new_with_mnemonic (_("_Presets"));

  thispage = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  presetnameentry = tmpw = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box1), tmpw, FALSE, FALSE, 0);
  gtk_widget_set_size_request (tmpw, 200, -1);
  gtk_widget_show (tmpw);

  presetsavebutton = tmpw = gtk_button_new_with_label ( _("Save Current..."));
  gtk_button_set_image (GTK_BUTTON (presetsavebutton),
                        gtk_image_new_from_icon_name (GIMP_ICON_DOCUMENT_SAVE,
                                                      GTK_ICON_SIZE_BUTTON));
  gtk_box_pack_start (GTK_BOX (box1), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (create_save_preset), NULL);
  gimp_help_set_help_data
    (tmpw, _("Save the current settings to the specified file"), NULL);

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box1, TRUE, TRUE, 0);
  gtk_widget_show (box1);

  presetlist = view = create_presets_list (box1);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  add_factory_defaults ();

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (box1), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  tmpw = gtk_button_new_with_mnemonic (_("_Apply"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (apply_preset), selection);
  gimp_help_set_help_data
    (tmpw, _("Reads the selected Preset into memory"), NULL);

  tmpw = delete_button = gtk_button_new_with_mnemonic (_("_Delete"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (delete_preset), selection);
  gimp_help_set_help_data (tmpw, _("Deletes the selected Preset"), NULL);

  tmpw = gtk_button_new_with_mnemonic (_("_Refresh"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (preset_refresh_presets), NULL);
  gimp_help_set_help_data (tmpw, _("Reread the folder of Presets"), NULL);

  presetdesclabel = tmpw = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (tmpw),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_label_set_line_wrap (GTK_LABEL (tmpw), TRUE);
  /*
   * Make sure the label's width is reasonable and it won't stretch
   * the dialog more than its width.
   * */
  gtk_widget_set_size_request (tmpw, 240, -1);

  gtk_label_set_xalign (GTK_LABEL (tmpw), 0.0);
  gtk_label_set_yalign (GTK_LABEL (tmpw), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), tmpw, TRUE, TRUE, 0);
  gtk_widget_show (tmpw);

  preset_read_dir_into_list ();

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
