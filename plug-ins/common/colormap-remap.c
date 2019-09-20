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


#define PLUG_IN_PROC_REMAP  "plug-in-colormap-remap"
#define PLUG_IN_PROC_SWAP   "plug-in-colormap-swap"
#define PLUG_IN_BINARY      "colormap-remap"
#define PLUG_IN_ROLE        "gimp-colormap-remap"


typedef struct _Remap      Remap;
typedef struct _RemapClass RemapClass;

struct _Remap
{
  GimpPlugIn      parent_instance;
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
                                                GimpDrawable         *drawable,
                                                const GimpValueArray *args,
                                                gpointer              run_data);

static gboolean         remap                  (GimpImage            *image,
                                                gint                  num_colors,
                                                guchar               *map);

static gboolean         remap_dialog           (GimpImage            *image,
                                                guchar               *map);


G_DEFINE_TYPE (Remap, remap, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (REMAP_TYPE)


static void
remap_class_init (RemapClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = remap_query_procedures;
  plug_in_class->create_procedure = remap_create_procedure;
}

static void
remap_init (Remap *remap)
{
}

static GList *
remap_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  return g_list_append (list, g_strdup (PLUG_IN_PROC_REMAP));
  return g_list_append (list, g_strdup (PLUG_IN_PROC_SWAP));

  return list;
}

static GimpProcedure *
remap_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC_REMAP))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            remap_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED*");

      gimp_procedure_set_menu_label (procedure, N_("R_earrange Colormap..."));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_COLORMAP);
      gimp_procedure_add_menu_path (procedure, "<Image>/Colors/Map/Colormap");
      gimp_procedure_add_menu_path (procedure, "<Colormap>");

      gimp_procedure_set_documentation (procedure,
                                        N_("Rearrange the colormap"),
                                        "This procedure takes an indexed "
                                        "image and lets you alter the "
                                        "positions of colors in the colormap "
                                        "without visually changing the image.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "June 2006");

      GIMP_PROC_ARG_INT (procedure, "num-colors",
                         "Num colors",
                         "Length of 'map' argument",
                         1, 256, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_UINT8_ARRAY (procedure, "map",
                                 "Map",
                                 "Remap array for the colormap",
                                 G_PARAM_READWRITE);
    }
  else if (! strcmp (name, PLUG_IN_PROC_SWAP))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            remap_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED*");

      gimp_procedure_set_menu_label (procedure, N_("_Swap Colors"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_COLORMAP);

      gimp_procedure_set_documentation (procedure,
                                        N_("Swap two colors in the colormap"),
                                        "This procedure takes an indexed "
                                        "image and lets you swap the "
                                        "positions of two colors in the "
                                        "colormap without visually changing "
                                        "the image.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "June 2006");

      GIMP_PROC_ARG_UCHAR (procedure, "index1",
                           "Index 1",
                           "First index in the colormap",
                           0, 255, 0,
                           G_PARAM_READWRITE);

      GIMP_PROC_ARG_UCHAR (procedure, "index2",
                           "Index 2",
                           "Second (other) index in the colormap",
                           0, 255, 0,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
remap_run (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           GimpDrawable         *drawable,
           const GimpValueArray *args,
           gpointer              run_data)
{
  guchar map[256];
  gint   i;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  /*  Make sure that the image is indexed  */
  if (gimp_image_base_type (image) != GIMP_INDEXED)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  for (i = 0; i < 256; i++)
    map[i] = i;

  if (strcmp (gimp_procedure_get_name (procedure),
              PLUG_IN_PROC_REMAP) == 0)
    {
      gint          n_cols;
      gint          n_col_args;
      const guchar *col_args;

      g_free (gimp_image_get_colormap (image, &n_cols));

      n_col_args = GIMP_VALUES_GET_INT        (args, 0);
      col_args   = GIMP_VALUES_GET_UINT8_ARRAY (args, 1);

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          if (! remap_dialog (image, map))
            return gimp_procedure_new_return_values (procedure,
                                                     GIMP_PDB_CANCEL,
                                                     NULL);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (n_cols != n_col_args)
            return gimp_procedure_new_return_values (procedure,
                                                     GIMP_PDB_CALLING_ERROR,
                                                     NULL);

          for (i = 0; i < n_cols; i++)
            map[i] = col_args[i];
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (PLUG_IN_PROC_REMAP, map);
          break;
        }

      if (! remap (image, n_cols, map))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_EXECUTION_ERROR,
                                                 NULL);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC_REMAP, map, sizeof (map));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else if (strcmp (gimp_procedure_get_name (procedure),
                   PLUG_IN_PROC_SWAP) == 0)
    {
      guchar index1 = GIMP_VALUES_GET_UCHAR (args, 0);
      guchar index2 = GIMP_VALUES_GET_UCHAR (args, 1);
      guchar tmp;
      gint   n_cols;

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 NULL);

      g_free (gimp_image_get_colormap (image, &n_cols));

      if (index1 >= n_cols || index2 >= n_cols)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 NULL);

      tmp = map[index1];
      map[index1] = map[index2];
      map[index2] = tmp;

      if (! remap (image, n_cols, map))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_EXECUTION_ERROR,
                                                 NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}


static gboolean
remap (GimpImage *image,
       gint       num_colors,
       guchar    *map)
{
  guchar   *cmap;
  guchar   *new_cmap;
  guchar   *new_cmap_i;
  gint      ncols;
  GList    *layers;
  GList    *list;
  glong     pixels    = 0;
  glong     processed = 0;
  guchar    pixel_map[256];
  gboolean  valid[256];
  gint      i;

  cmap = gimp_image_get_colormap (image, &ncols);

  g_return_val_if_fail (cmap != NULL, FALSE);
  g_return_val_if_fail (ncols > 0, FALSE);

  if (num_colors != ncols)
    {
      g_message (_("Invalid remap array was passed to remap function"));
      return FALSE;
    }

  for (i = 0; i < ncols; i++)
    valid[i] = FALSE;

  for (i = 0; i < ncols; i++)
    {
      if (map[i] >= ncols)
        {
          g_message (_("Invalid remap array was passed to remap function"));
          return FALSE;
        }

      valid[map[i]] = TRUE;
      pixel_map[map[i]] = i;
    }

  for (i = 0; i < ncols; i++)
    if (valid[i] == FALSE)
      {
        g_message (_("Invalid remap array was passed to remap function"));
        return FALSE;
      }

  new_cmap = g_new (guchar, ncols * 3);

  new_cmap_i = new_cmap;

  for (i = 0; i < ncols; i++)
    {
      gint j = map[i] * 3;

      *new_cmap_i++ = cmap[j];
      *new_cmap_i++ = cmap[j + 1];
      *new_cmap_i++ = cmap[j + 2];
    }

  gimp_image_undo_group_start (image);

  gimp_image_set_colormap (image, new_cmap, ncols);

  g_free (cmap);
  g_free (new_cmap);

  gimp_progress_init (_("Rearranging the colormap"));

  /*  There is no needs to process the layers recursively, because
   *  indexed images cannot have layer groups.
   */
  layers = gimp_image_list_layers (image);

  for (list = layers; list; list = list->next)
    pixels +=
      gimp_drawable_width (list->data) * gimp_drawable_height (list->data);

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

      buffer = gimp_drawable_get_buffer (list->data);
      shadow = gimp_drawable_get_shadow_buffer (list->data);

      width   = gegl_buffer_get_width  (buffer);
      height  = gegl_buffer_get_height (buffer);
      format  = gegl_buffer_get_format (buffer);
      bpp     = babl_format_get_bytes_per_pixel (format);

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
                    *dest++ = pixel_map[*src++];
                }
              else
                {
                  for (x = 0; x < src_roi->width; x++)
                    {
                      *dest++ = pixel_map[*src++];
                      *dest++ = *src++;
                    }
                }

              src_row  += src_roi->width  * bpp;
              dest_row += dest_roi->width * bpp;
            }

          processed += src_roi->width * src_roi->height;
          update %= 16;

          if (update == 0)
            gimp_progress_update ((gdouble) processed / pixels);

          update++;
        }

      g_object_unref (buffer);
      g_object_unref (shadow);

      gimp_drawable_merge_shadow (list->data, TRUE);
      gimp_drawable_update (list->data, 0, 0, width, height);
    }

  g_list_free (layers);

  gimp_progress_update (1.0);

  gimp_image_undo_group_end (image);

  return TRUE;
}


/* dialog */

#define RESPONSE_RESET 1

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

static  GtkUIManager *remap_ui  = NULL;
static  gboolean      remap_ok  = FALSE;
static  gint          reverse_order[256];


static void
remap_sort (GtkTreeSortable *store,
            gint             column,
            GtkSortType      order)
{
  gtk_tree_sortable_set_sort_column_id (store, column, order);
  gtk_tree_sortable_set_sort_column_id (store,
                                        GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, 0);
}

static void
remap_sort_callback (GtkAction       *action,
                     GtkTreeSortable *store)
{
  const gchar *name   = gtk_action_get_name (action);
  gint         column = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;

  g_return_if_fail (g_str_has_prefix (name, "sort-"));

  if (strncmp (name + 5, "hue", 3) == 0)
    column = COLOR_H;
  else if (strncmp (name + 5, "sat", 3) == 0)
    column = COLOR_S;
  else if (strncmp (name + 5, "val", 3) == 0)
    column = COLOR_V;

  remap_sort (store, column, GTK_SORT_ASCENDING);
}

static void
remap_reset_callback (GtkAction       *action,
                      GtkTreeSortable *store)
{
  remap_sort (store, COLOR_INDEX, GTK_SORT_ASCENDING);
}

static void
remap_reverse_callback (GtkAction    *action,
                        GtkListStore *store)
{
  gtk_list_store_reorder (store, reverse_order);
}

static GtkUIManager *
remap_ui_manager_new (GtkWidget    *window,
                      GtkListStore *store)
{
  static const GtkActionEntry actions[] =
  {
    {
      "sort-hue", NULL, N_("Sort on Hue"), NULL, NULL,
      G_CALLBACK (remap_sort_callback)
    },
    {
      "sort-sat", NULL, N_("Sort on Saturation"), NULL, NULL,
      G_CALLBACK (remap_sort_callback)
    },
    {
      "sort-val", NULL, N_("Sort on Value"), NULL, NULL,
      G_CALLBACK (remap_sort_callback)
    },
    {
      "reverse", NULL, N_("Reverse Order"), NULL, NULL,
      G_CALLBACK (remap_reverse_callback)
    },
    {
      "reset", GIMP_ICON_RESET, N_("Reset Order"), NULL, NULL,
      G_CALLBACK (remap_reset_callback)
    },
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GtkActionGroup *group      = gtk_action_group_new ("Actions");
  GError         *error      = NULL;

  gtk_action_group_set_translation_domain (group, NULL);
  gtk_action_group_add_actions (group, actions, G_N_ELEMENTS (actions), store);

  gtk_ui_manager_insert_action_group (ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <popup name=\"remap-popup\">"
                                     "    <menuitem action=\"sort-hue\" />"
                                     "    <menuitem action=\"sort-sat\" />"
                                     "    <menuitem action=\"sort-val\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"reverse\" />"
                                     "    <menuitem action=\"reset\" />"
                                     "  </popup>"
                                     "</ui>",
                                     -1, &error);
  if (error)
    {
      g_warning ("error parsing ui: %s", error->message);
      g_clear_error (&error);
    }

  return ui_manager;
}

static gboolean
remap_popup_menu (GtkWidget      *widget,
                  GdkEventButton *event)
{
  GtkWidget *menu = gtk_ui_manager_get_widget (remap_ui, "/remap-popup");

  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);

  return TRUE;
}

static gboolean
remap_button_press (GtkWidget      *widget,
                    GdkEventButton *event)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    return remap_popup_menu (widget, event);

  return FALSE;
}

static void
remap_response (GtkWidget       *dialog,
                gint             response_id,
                GtkTreeSortable *store)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      remap_reset_callback (NULL, store);
      break;

    case GTK_RESPONSE_OK:
      remap_ok = TRUE;
      /* fallthrough */

    default:
      gtk_main_quit ();
      break;
    }
}

static gboolean
remap_dialog (GimpImage *image,
              guchar    *map)
{
  GtkWidget       *dialog;
  GtkWidget       *vbox;
  GtkWidget       *box;
  GtkWidget       *iconview;
  GtkListStore    *store;
  GtkCellRenderer *renderer;
  GtkTreeIter      iter;
  guchar          *cmap;
  gint             ncols, i;
  gboolean         valid;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Rearrange Colormap"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC_REMAP,

                            _("_Reset"),  RESPONSE_RESET,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);

  cmap = gimp_image_get_colormap (image, &ncols);

  g_return_val_if_fail ((ncols > 0) && (ncols <= 256), FALSE);

  store = gtk_list_store_new (NUM_COLS,
                              G_TYPE_INT, G_TYPE_STRING, GIMP_TYPE_RGB,
                              G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

  for (i = 0; i < ncols; i++)
    {
      GimpRGB  rgb;
      GimpHSV  hsv;
      gint     index = map[i];
      gchar   *text  = g_strdup_printf ("%d", index);

      gimp_rgb_set_uchar (&rgb,
                          cmap[index * 3],
                          cmap[index * 3 + 1],
                          cmap[index * 3 + 2]);
      gimp_rgb_to_hsv (&rgb, &hsv);

      reverse_order[i] = ncols - i - 1;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLOR_INDEX,      index,
                          COLOR_INDEX_TEXT, text,
                          COLOR_RGB,        &rgb,
                          COLOR_H,          hsv.h,
                          COLOR_S,          hsv.s,
                          COLOR_V,          hsv.v,
                          -1);
      g_free (text);
    }

  g_free (cmap);

  remap_ui = remap_ui_manager_new (dialog, store);

  iconview = gtk_icon_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_box_pack_start (GTK_BOX (vbox), iconview, TRUE, TRUE, 0);

  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (iconview),
                                    GTK_SELECTION_SINGLE);
  gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (iconview),
                                      GTK_ORIENTATION_VERTICAL);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (iconview), 16);
  gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (iconview), 0);
  gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (iconview), 0);
  gtk_icon_view_set_reorderable (GTK_ICON_VIEW (iconview), TRUE);

  renderer = gimp_cell_renderer_color_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (iconview), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (iconview), renderer,
                                  "color", COLOR_RGB,
                                  NULL);
  g_object_set (renderer,
                "width", 24,
                NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (iconview), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (iconview), renderer,
                                  "text", COLOR_INDEX_TEXT,
                                  NULL);
  g_object_set (renderer,
                "size-points", 6.0,
                "xalign",      0.5,
                "ypad",        0,
                NULL);

  g_signal_connect (iconview, "popup-menu",
                    G_CALLBACK (remap_popup_menu),
                    NULL);

  g_signal_connect (iconview, "button-press-event",
                    G_CALLBACK (remap_button_press),
                    NULL);

  box = gimp_hint_box_new (_("Drag and drop colors to rearrange the colormap.  "
                             "The numbers shown are the original indices.  "
                             "Right-click for a menu with sort options."));

  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (remap_response),
                    store);

  gtk_widget_show_all (dialog);

  gtk_main ();

  i = 0;

  for (valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
       valid;
       valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter))
    {
      gint index;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          COLOR_INDEX, &index,
                          -1);
      map[i++] = index;
    }

  gtk_widget_destroy (dialog);

  return remap_ok;
}
