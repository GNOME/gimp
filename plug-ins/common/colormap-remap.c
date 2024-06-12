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
 * Colormap remapping plug-in
 * Copyright (C) 2006 Mukund Sivaraman <muks@mukund.org>
 *
 * This plug-in takes the colormap and lets you move colors from one index
 * to another while keeping the original image visually unmodified.
 *
 * Such functionality is useful for creating graphics files for applications
 * which expect certain indices to contain some specific colors.
 *
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Magic numbers */

#define PLUG_IN_PROC_REMAP  "plug-in-colormap-remap"
#define PLUG_IN_PROC_SWAP   "plug-in-colormap-swap"
#define PLUG_IN_BINARY      "colormap-remap"
#define PLUG_IN_ROLE        "gimp-colormap-remap"

typedef struct _Remap Remap;
typedef struct _RemapClass RemapClass;

struct _Remap
{
  GimpPlugIn parent_instance;
};

struct _RemapClass
{
  GimpPlugInClass parent_class;
};

#define REMAP_TYPE  (remap_get_type ())
#define REMAP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMAP_TYPE, Remap))

GType                   remap_get_type         (void) G_GNUC_CONST;
static GList          * remap_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * remap_create_procedure (GimpPlugIn           *plug_in,
                                                const gchar          *name);

static GimpValueArray * remap_run              (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                gint                  n_drawables,
                                                GimpDrawable        **drawables,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);

static gboolean         remap_dialog           (GimpProcedure        *procedure,
                                                GObject              *config,
                                                GimpImage            *image);

static gboolean         read_image_palette     (GimpImage            *image,
                                                gint                 *ncolors_out);

static GtkWidget *      create_icon_view       (gint                  col_count,
                                                gint                 *item_width,
                                                gint                 *item_height);

static GtkWidget *      create_scrolled_window (GtkWidget            *dialog,
                                                gint                  item_width,
                                                gint                  item_height,
                                                gint                  ncolors,
                                                gint                  col_count);

static void             remap_sort             (gint             column,
                                                GtkSortType      order);
static void             remap_sort_hue         (GtkButton       *action,
                                                gpointer         user_data);
static void             remap_sort_sat         (GtkButton       *action,
                                                gpointer         user_data);
static void             remap_sort_val         (GtkButton       *action,
                                                gpointer        user_data);
static void             remap_sort_index       (GtkButton       *action,
                                                gpointer         user_data);
static void             remap_reverse_order    (GtkButton       *action,
                                                gpointer         user_data);

static gboolean         real_remap             (GimpImage       *image);

static gboolean         check_index_map        (GimpImage *image);

static void             populate_index_map_from_store (void);

static void             store_last_vals (GimpProcedureConfig *config);
static void             read_last_vals  (GimpProcedureConfig *config);


G_DEFINE_TYPE (Remap, remap, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (REMAP_TYPE)
DEFINE_STD_SET_I18N


enum
{
  COLOR_INDEX,
  COLOR_INDEX_TEXT,
  COLOR_RGB,
  COLOR_H,
  COLOR_S,
  COLOR_V,
  NUM_COLS
};


static GtkListStore *store; /* To store the color indices and other data */
static gint          reverse_order[256]; /* store indices to reverse order of icons*/
static guchar        index_map_old_new[256]; /* store remapped indices */

static void
remap_class_init (RemapClass *klass)
{
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = remap_query_procedures;
  plug_in_class->create_procedure = remap_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
remap_init (Remap *remap)
{
}

static GList *
remap_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_prepend (list, g_strdup (PLUG_IN_PROC_REMAP));
  list = g_list_prepend (list, g_strdup (PLUG_IN_PROC_SWAP));

  return list;
}

static GimpProcedure *
remap_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (strcmp (name, PLUG_IN_PROC_REMAP) == 0)
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            remap_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("R_earrange Colormap..."));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_COLORMAP);
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/Map/[Colormap]");
      gimp_procedure_add_menu_path (procedure, "<Colormap>/Colormap Menu");

      gimp_procedure_set_documentation (procedure,
                                        _("Rearrange the colormap"),
                                        _("This procedure takes an indexed "
                                          "image and lets you alter the "
                                          "positions of colors in the colormap "
                                          "without visually changing the image."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "June 2006");

      gimp_procedure_add_bytes_argument (procedure, "map",
                                         _("Map"),
                                         _("Remap array for the colormap"),
                                         G_PARAM_READWRITE);
    }
  else if (strcmp (name, PLUG_IN_PROC_SWAP) == 0)
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            remap_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Swap Colors"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_COLORMAP);

      gimp_procedure_set_documentation (procedure,
                                        _("Swap two colors in the colormap"),
                                        _("This procedure takes an indexed "
                                          "image and lets you swap the "
                                          "positions of two colors in the "
                                          "colormap without visually changing "
                                          "the image."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "June 2006");

      gimp_procedure_add_int_argument (procedure, "index1",
                                       _("Index 1"),
                                       _("First index in the colormap"),
                                       0, 255, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "index2",
                                       _("Index 2"),
                                       _("Second (other) index in the colormap"),
                                       0, 255, 0,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
remap_run (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GimpProcedureConfig  *config,
           gpointer              run_data)
{
  gint i;

  gegl_init (NULL, NULL);

  for (i = 0; i < 256; ++i)
    {
      index_map_old_new[i] = i;
    }

  if (gimp_image_get_base_type (image) != GIMP_INDEXED)
    {
      GError *error;
      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with indexed images."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  if (strcmp (gimp_procedure_get_name (procedure), PLUG_IN_PROC_SWAP) == 0)
    {
      gint          index1;
      gint          index2;
      gint          temp;
      GimpPalette  *palette;
      gint          ncolors;

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CALLING_ERROR,
                                                   NULL);
        }

      g_object_get (config,
                    "index1", &index1,
                    "index2", &index2,
                    NULL);

      palette = gimp_image_get_palette       (image);
      ncolors = gimp_palette_get_color_count (palette);

      if (index1 >= ncolors || index2 >= ncolors
          || index1 < 0 || index2 < 0)
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CALLING_ERROR,
                                                   NULL);
        }

      temp = index_map_old_new[index1];
      index_map_old_new[index1] = index_map_old_new[index2];
      index_map_old_new[index2] = temp;

      real_remap (image);
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_SUCCESS,
                                               NULL);
    }

  if (run_mode == GIMP_RUN_NONINTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    {
      read_last_vals (config);
      if (! check_index_map (image))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CALLING_ERROR,
                                                   NULL);
        }
      real_remap (image);
      gimp_displays_flush ();
      return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      remap_dialog (procedure, G_OBJECT (config), image))
    {
      populate_index_map_from_store ();
      real_remap (image);
      store_last_vals (config);
      gimp_displays_flush ();
      return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);
}

static gboolean
check_index_map (GimpImage *image)
{
  gint         i;
  gint         ncolors;
  GimpPalette *palette;

  palette = gimp_image_get_palette       (image);
  ncolors = gimp_palette_get_color_count (palette);

  for (i = 0; i < ncolors; ++i)
    {
      if (index_map_old_new[i] >= ncolors)
        {
          return FALSE;
        }
    }
  return TRUE;
}


static void
read_last_vals (GimpProcedureConfig *config)
{
  const guchar *last_vals;

  GBytes *bytes;
  gint    i;

  g_object_get (G_OBJECT (config), "map", &bytes, NULL);
  last_vals = g_bytes_get_data (bytes, NULL);

  if (bytes != NULL)
    {
      for (i = 0; i < 256; ++i)
        {
          index_map_old_new[i] = last_vals[i];
        }
    }

  g_bytes_unref (bytes);
}

static void
store_last_vals (GimpProcedureConfig *config)
{
  GBytes *bytes;
  bytes = g_bytes_new (&index_map_old_new, sizeof (guchar) * 256);
  g_object_set (G_OBJECT (config), "map", bytes, NULL);
  g_bytes_unref (bytes);
}


static gboolean
read_image_palette (GimpImage *image,
                    gint      *ncolors_out)
{
  GtkTreeIter   iter;
  gint          index;
  gint          ncolors;
  GimpPalette  *palette;
  GeglColor   **colors;

  palette = gimp_image_get_palette       (image);
  ncolors = gimp_palette_get_color_count (palette);

  g_return_val_if_fail ((ncolors > 0) && (ncolors <= 256), FALSE);

  colors = gimp_palette_get_colors (palette);
  for (index = 0; index < ncolors; ++index)
    {
      GeglColor *c   = colors[index];
      gfloat     hsv[3];
      gchar     *text = g_strdup_printf ("%d", index);

      gegl_color_get_pixel (c, babl_format ("HSV float"), hsv);

      reverse_order[index] = ncolors - index - 1;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLOR_INDEX,       index,
                          COLOR_INDEX_TEXT,  text,
                          COLOR_RGB,         c,
                          COLOR_H,           hsv[0],
                          COLOR_S,           hsv[1],
                          COLOR_V,           hsv[2],
                          -1);
      g_free (text);
    }
  gimp_color_array_free (colors);

  if (ncolors_out != NULL)
    {
      *ncolors_out = ncolors;
    }

  return TRUE;
}

static GtkWidget *
create_icon_view (gint  col_count,
                  gint *item_width,
                  gint *item_height)
{
  GtkWidget       *iv;
  GtkCellRenderer *renderer;
  gint             icon_width;
  gint             icon_height;
  GtkIconSize      icon_size    = GTK_ICON_SIZE_LARGE_TOOLBAR;
  gint             item_padding = 5;
  gint             row_spacing  = 2;
  gint             col_spacing  = 2;
  gint             text_height  = 21;

  gtk_icon_size_lookup (icon_size, &icon_width, &icon_height);

  *item_width  = icon_width + item_padding * 2 + row_spacing + col_spacing;

  *item_height = icon_height + text_height + item_padding * 2;

  iv = gtk_icon_view_new_with_model (GTK_TREE_MODEL (store));

  gtk_icon_view_set_selection_mode   (GTK_ICON_VIEW (iv), GTK_SELECTION_SINGLE);
  gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (iv), GTK_ORIENTATION_VERTICAL);
  gtk_icon_view_set_item_padding     (GTK_ICON_VIEW (iv), item_padding);
  gtk_icon_view_set_columns          (GTK_ICON_VIEW (iv), col_count);
  gtk_icon_view_set_row_spacing      (GTK_ICON_VIEW (iv), row_spacing);
  gtk_icon_view_set_column_spacing   (GTK_ICON_VIEW (iv), col_spacing);
  gtk_icon_view_set_reorderable      (GTK_ICON_VIEW (iv), TRUE);

  gtk_widget_set_can_focus (iv, TRUE);

  renderer = gimp_cell_renderer_color_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (iv), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (iv), renderer,
                                  "color", COLOR_RGB,
                                  NULL);
  g_object_set (renderer,
                "icon-size", icon_size,
                "xpad",      0,
                "ypad",      0,
                NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (iv), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (iv), renderer,
                                  "text", COLOR_INDEX_TEXT,
                                  NULL);
  g_object_set (renderer,
                "size-points", 9.0,   /*TODO: should it be scaled with the UI scale ? */
                "xalign",      0.5,
                NULL);
  return iv;
}

static GtkWidget *
create_scrolled_window (GtkWidget *dialog,
                        gint       item_width,
                        gint       item_height,
                        gint       ncolors,
                        gint       col_count)
{
  GtkWidget *sw;
  gint       nrows;
  gint       widget_padding = 6 * 2;

  nrows = MIN (ncolors / col_count + 1, 8);
  if (ncolors > 99)
    item_width += 4; /* take into account increased text width when indices > 99 */

  /* last parameter is intentionally set to 'dummy'
   * so we can create scrolled window before the container contents.
   * The window contents will be inserted at the later step */
  sw = gimp_procedure_dialog_fill_scrolled_window (GIMP_PROCEDURE_DIALOG (dialog), "sw",
                                                   "intentional-dummy");

  gtk_widget_set_size_request (sw, item_width * col_count, -1);
  gtk_widget_set_vexpand (sw, TRUE);

  gtk_scrolled_window_set_min_content_width  (GTK_SCROLLED_WINDOW (sw),
                                              item_width * col_count);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (sw),
                                              item_height * nrows + widget_padding);
  return sw;
}

static void
remap_sort (gint             column,
            GtkSortType      order)
{
#define REMAP_COL_ID GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), column, order);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), REMAP_COL_ID, 0);
#undef REMAP_COL_ID
}

static void
remap_sort_hue (GtkButton *button,
                gpointer  user_data)
{
  remap_sort (COLOR_H, GTK_SORT_ASCENDING);
}

static void
remap_sort_sat (GtkButton *button,
                gpointer  user_data)
{
  remap_sort (COLOR_S, GTK_SORT_ASCENDING);
}

static void
remap_sort_val (GtkButton *button,
                gpointer   user_data)
{
  remap_sort (COLOR_V, GTK_SORT_ASCENDING);
}

static void
remap_sort_index (GtkButton *button,
                  gpointer   user_data)
{
  remap_sort (COLOR_INDEX, GTK_SORT_ASCENDING);
}

static void
remap_reverse_order (GtkButton *button,
                     gpointer   user_data)
{
  gtk_list_store_reorder (store, reverse_order);
}


static gboolean
remap_dialog (GimpProcedure *procedure,
              GObject       *config,
              GimpImage     *image)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *w;
  GtkWidget *hintbox;
  GtkWidget *box;
  GtkWidget *box2;
  GtkWidget *icons;
  gboolean   run;
  gint       ncolors;
  gint       item_width;
  gint       item_height;
  gint       col_count = 16;

  gimp_ui_init (PLUG_IN_BINARY);


  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Rearrange Colors"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  store = gtk_list_store_new (NUM_COLS,
                              G_TYPE_INT, G_TYPE_STRING, GEGL_TYPE_COLOR,
                              G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

  if (! read_image_palette (image, &ncolors))
    {
      return FALSE;
    }

  icons = create_icon_view (col_count, &item_width, &item_height);

  /* scrolled window widget name is "sw" */
  w = create_scrolled_window (dialog, item_width, item_height, ncolors, col_count);
  gtk_container_add (GTK_CONTAINER (w), icons);

  /* "dummy" is intentional. so we can "fill the box" and then actually fill it with our controls */
  box2 = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "buttonbox",
                                         "intentional-dummy", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box2),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_bottom (box2, 12);
  gtk_widget_set_margin_top    (box2, 12);

  button = gtk_button_new_with_label (_("Sort on Hue"));
  gtk_box_pack_start (GTK_BOX (box2), button, TRUE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remap_sort_hue),
                    NULL);

  button = gtk_button_new_with_label (_("Sort on Saturation"));
  gtk_box_pack_start (GTK_BOX (box2), button, TRUE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remap_sort_sat),
                    NULL);

  button = gtk_button_new_with_label (_("Sort on Value"));
  gtk_box_pack_start (GTK_BOX (box2), button, TRUE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remap_sort_val),
                    NULL);

  button = gtk_button_new_with_label (_("Reverse Order"));
  gtk_box_pack_start (GTK_BOX (box2), button, TRUE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remap_reverse_order),
                    NULL);

  button = gtk_button_new_with_label (_("Reset Order"));
  gtk_box_pack_start (GTK_BOX (box2), button, TRUE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (remap_sort_index),
                    NULL);

  box = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog), "vbox",
                                        "sw", "buttonbox", NULL);

  hintbox = gimp_hint_box_new (_("Drag and drop colors to rearrange the colormap.\n"
                                 "The numbers shown are the original indices."));
  gtk_box_pack_start (GTK_BOX (box), hintbox, FALSE, FALSE, 0);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                               "vbox", NULL);

  gtk_widget_show_all (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);
  return run;
}

static void
populate_index_map_from_store (void)
{
  GtkTreeIter iter;
  gint        old_index = 0;
  gboolean    valid;

  for (valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
       valid;
       valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter))
    {
      gint new_index;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          COLOR_INDEX, &new_index,
                          -1);
      index_map_old_new[old_index] = new_index;
      ++old_index;
    }
}

static gboolean
real_remap (GimpImage *image)
{
  guchar        pixmap[256]; /* map new-pixel index to old. pixmap[new-index] = old-index */
  gint          i;
  gint          ncolors;
  GimpPalette  *palette;
  GeglColor   **colors;
  GList        *layers;
  GList        *list;
  glong         total_pixels = 0;
  glong         processed    = 0;

  palette = gimp_image_get_palette       (image);
  ncolors = gimp_palette_get_color_count (palette);
  colors  = gimp_palette_get_colors      (palette);

  gimp_image_undo_group_start (image);
  for (i = 0; i < ncolors; ++i)
    {
      gint new_index    = index_map_old_new[i];
      pixmap[new_index] = i;
      gimp_palette_entry_set_color (palette, i, colors[new_index]);
    }
  gimp_color_array_free (colors);

  gimp_progress_init (_("Rearranging the colormap"));
  layers = gimp_image_list_layers (image);

  /**  There is no needs to process the layers recursively, because
   *  indexed images cannot have layer groups.
   */
  for (list = layers; list; list = list->next)
    {
      total_pixels += gimp_drawable_get_width (list->data) *
                      gimp_drawable_get_height (list->data);
    }

  for (list = layers; list; list = list->next)
    {
      GeglBuffer         *buffer;
      GeglBuffer         *shadow;
      const Babl         *format;
      GeglBufferIterator *iter;
      GeglRectangle      *src_roi;
      GeglRectangle      *dest_roi;
      gint                width, height, bpp;
      gint                update = 0;

      buffer = gimp_drawable_get_buffer        (list->data);
      shadow = gimp_drawable_get_shadow_buffer (list->data);

      width  = gegl_buffer_get_width  (buffer);
      height = gegl_buffer_get_height (buffer);
      format = gegl_buffer_get_format (buffer);
      bpp    = babl_format_get_bytes_per_pixel (format);

      iter = gegl_buffer_iterator_new (buffer,
                                       GEGL_RECTANGLE (0, 0, width, height), 0,
                                       format,
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
      src_roi = &iter->items[0].roi;

      gegl_buffer_iterator_add (iter, shadow,
                                GEGL_RECTANGLE (0, 0, width, height), 0,
                                format,
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
      dest_roi = &iter->items[1].roi;

      while (gegl_buffer_iterator_next (iter))
        {
          const guchar *src_row  = iter->items[0].data;
          guchar       *dest_row = iter->items[1].data;
          gint          y;

          for (y = 0; y < src_roi->height; y++)
            {
              const guchar *src  = src_row;
              guchar       *dest = dest_row;
              gint          x;

              if (bpp == 1)
                {
                  for (x = 0; x < src_roi->width; x++)
                    *dest++ = pixmap[*src++];
                }
              else
                {
                  for (x = 0; x < src_roi->width; x++)
                    {
                      *dest++ = pixmap[*src++];
                      *dest++ = *src++;
                    }
                }

              src_row  += src_roi->width  * bpp;
              dest_row += dest_roi->width * bpp;
            }

          processed += src_roi->width * src_roi->height;
          update %= 16;

          if (update == 0)
            {
              gimp_progress_update ((gdouble) processed / total_pixels);
            }

          update++;
        }

      g_object_unref (buffer);
      g_object_unref (shadow);

      gimp_drawable_merge_shadow (list->data, TRUE);
      gimp_drawable_update (list->data, 0, 0, width, height);
    }

  gimp_image_undo_group_end (image);
  g_list_free (layers);
  gimp_progress_update (1.0);
  return TRUE;
}
