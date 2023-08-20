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

struct _GimpRemap
{
  GimpPlugIn      parent_instance;

  GtkApplication *app;
  GtkWindow      *window;
  GimpImage      *image;
  guchar          map[256];
  gint            n_cols;

  GMenu          *menu;
};


#define GIMP_TYPE_REMAP (gimp_remap_get_type ())
G_DECLARE_FINAL_TYPE (GimpRemap, gimp_remap, GIMP, REMAP, GimpPlugIn)


GType                   remap_get_type         (void) G_GNUC_CONST;

static void             gimp_remap_finalize    (GObject              *object);

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

static gboolean         real_remap             (GimpImage            *image,
                                                gint                  num_colors,
                                                guchar               *map);

static gboolean         remap_dialog           (GimpImage            *image,
                                                guchar               *map,
                                                GimpRemap            *remap);

static void             remap_sort_hue_action  (GSimpleAction        *action,
                                                GVariant             *parameter,
                                                gpointer              user_data);
static void             remap_sort_sat_action  (GSimpleAction        *action,
                                                GVariant             *parameter,
                                                gpointer              user_data);
static void             remap_sort_val_action  (GSimpleAction        *action,
                                                GVariant             *parameter,
                                                gpointer              user_data);
static void             remap_reverse_action   (GSimpleAction        *action,
                                                GVariant             *parameter,
                                                gpointer              user_data);
static void             remap_reset_action     (GSimpleAction        *action,
                                                GVariant             *parameter,
                                                gpointer              user_data);

static void             on_app_activate        (GApplication         *gapp,
                                                gpointer              user_data);



G_DEFINE_TYPE (GimpRemap, gimp_remap, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_TYPE_REMAP)
DEFINE_STD_SET_I18N


static GtkWindow    *window   = NULL;
static GtkListStore *store    = NULL;
static gboolean      remap_ok = FALSE;

static const GActionEntry ACTIONS[] =
{
  { "sort-hue", remap_sort_hue_action },
  { "sort-sat", remap_sort_sat_action },
  { "sort-val", remap_sort_val_action },

  { "reverse", remap_reverse_action },

  { "reset", remap_reset_action },
};

static void
gimp_remap_class_init (GimpRemapClass *klass)
{
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);
  GObjectClass    *object_class   = G_OBJECT_CLASS (klass);

  object_class->finalize          = gimp_remap_finalize;

  plug_in_class->query_procedures = remap_query_procedures;
  plug_in_class->create_procedure = remap_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_remap_finalize (GObject *object)
{
  GimpRemap *remap = GIMP_REMAP (object);

  G_OBJECT_CLASS (gimp_remap_parent_class)->finalize (object);

  g_clear_object (&remap->menu);
}

static void
gimp_remap_init (GimpRemap *remap)
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

  if (! strcmp (name, PLUG_IN_PROC_REMAP))
    {
      procedure = gimp_image_procedure_new2 (plug_in, name,
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
      gimp_procedure_add_menu_path (procedure, "<Colormap>");

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

      GIMP_PROC_ARG_BYTES (procedure, "map",
                           _("Map"),
                           _("Remap array for the colormap"),
                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, PLUG_IN_PROC_SWAP))
    {
      procedure = gimp_image_procedure_new2 (plug_in, name,
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

      GIMP_PROC_ARG_INT (procedure, "index1",
                         _("Index 1"),
                         _("First index in the colormap"),
                         0, 255, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "index2",
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
  GMenu *section;
  gint   i;

  GimpRemap *remap = GIMP_REMAP (run_data);

  remap = GIMP_REMAP (gimp_procedure_get_plug_in (procedure));
#if GLIB_CHECK_VERSION(2,74,0)
  remap->app = gtk_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
#else
  remap->app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
#endif
  remap->image = image;

  remap->menu = g_menu_new ();

  section = g_menu_new ();
  g_menu_append (section, _("Sort on Hue"), "win.sort-hue");
  g_menu_append (section, _("Sort on Saturation"), "win.sort-sat");
  g_menu_append (section, _("Sort on Value"), "win.sort-val");
  g_menu_append_section (remap->menu, NULL, G_MENU_MODEL (section));
  g_clear_object (&section);

  section = g_menu_new ();
  g_menu_append (section, _("Reverse Order"), "win.reverse");
  g_menu_append (section, _("Reset Order"), "win.reset");
  g_menu_append_section (remap->menu, NULL, G_MENU_MODEL (section));
  g_clear_object (&section);

  gegl_init (NULL, NULL);

  /*  Make sure that the image is indexed  */
  if (gimp_image_get_base_type (image) != GIMP_INDEXED)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  for (i = 0; i < 256; i++)
    remap->map[i] = i;

  if (strcmp (gimp_procedure_get_name (procedure),
              PLUG_IN_PROC_REMAP) == 0)
    {
      GBytes       *col_args_bytes;
      const guchar *col_args;

      g_free (gimp_image_get_colormap (image, NULL, &remap->n_cols));

      g_object_get (config,
                    "map", &col_args_bytes,
                    NULL);

      col_args = g_bytes_get_data (col_args_bytes, NULL);

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          g_signal_connect (remap->app, "activate", G_CALLBACK (on_app_activate), remap);
          g_application_run (G_APPLICATION (remap->app), 0, NULL);
          g_clear_object (&remap->app);

          if (! remap_ok)
            return gimp_procedure_new_return_values (procedure,
                                                     GIMP_PDB_CANCEL,
                                                     NULL);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (remap->n_cols != g_bytes_get_size (col_args_bytes))
            return gimp_procedure_new_return_values (procedure,
                                                     GIMP_PDB_CALLING_ERROR,
                                                     NULL);

          for (i = 0; i < remap->n_cols; i++)
            remap->map[i] = col_args[i];
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;
        }

      if (! real_remap (image, remap->n_cols, remap->map))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_EXECUTION_ERROR,
                                                 NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else if (strcmp (gimp_procedure_get_name (procedure),
                   PLUG_IN_PROC_SWAP) == 0)
    {
      guchar tmp;
      gint   n_cols;
      gint   index1;
      gint   index2;

      g_object_get (config,
                    "index1", &index1,
                    "index2", &index2,
                    NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 NULL);

      g_free (gimp_image_get_colormap (image, NULL, &n_cols));

      if (index1 >= n_cols || index2 >= n_cols)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 NULL);

      tmp = remap->map[index1];
      remap->map[index1] = remap->map[index2];
      remap->map[index2] = tmp;

      if (! real_remap (image, n_cols, remap->map))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_EXECUTION_ERROR,
                                                 NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}


static gboolean
real_remap (GimpImage *image,
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

  cmap = gimp_image_get_colormap (image, NULL, &ncols);

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
      gimp_drawable_get_width (list->data) * gimp_drawable_get_height (list->data);

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
remap_sort_hue_action  (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  remap_sort (GTK_TREE_SORTABLE (store), COLOR_H, GTK_SORT_ASCENDING);
}

static void
remap_sort_sat_action  (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  remap_sort (GTK_TREE_SORTABLE (store), COLOR_S, GTK_SORT_ASCENDING);
}

static void
remap_sort_val_action  (GSimpleAction *action,
                         GVariant     *parameter,
                         gpointer      user_data)
{
  remap_sort (GTK_TREE_SORTABLE (store), COLOR_V, GTK_SORT_ASCENDING);
}

static void
remap_reset_action (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  remap_sort (GTK_TREE_SORTABLE (store), COLOR_INDEX,
              GTK_SORT_ASCENDING);
}

static void
remap_reverse_action (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  gtk_list_store_reorder (store, reverse_order);
}

static gboolean
remap_popup_menu (GtkWidget      *widget,
                  GdkEventButton *event,
                  GimpRemap      *remap)
{
  GtkWidget  *menu;

  menu = gtk_menu_new_from_model (G_MENU_MODEL (remap->menu));

  gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (window), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);

  return TRUE;
}

static gboolean
remap_button_press (GtkWidget      *widget,
                    GdkEventButton *event,
                    GimpRemap      *remap)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    return remap_popup_menu (widget, event, remap);

  return FALSE;
}

static void
remap_response (GtkWidget *dialog,
                gint       response_id,
                GimpRemap *remap)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      remap_reset_action (NULL, NULL, NULL);
      break;

    case GTK_RESPONSE_OK:
      remap_ok = TRUE;
      /* fallthrough */

    default:
      gtk_widget_destroy (GTK_WIDGET (window));
      break;
    }
}

static gboolean
remap_dialog (GimpImage *image,
              guchar    *map,
              GimpRemap *remap)
{
  GtkWidget       *dialog;
  GtkWidget       *vbox;
  GtkWidget       *box;
  GtkWidget       *iconview;
  GtkCellRenderer *renderer;
  GtkTreeIter      iter;
  guchar          *cmap;
  gint             ncols, i;
  gboolean         valid;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Rearrange Colormap"), PLUG_IN_ROLE,
                            GTK_WIDGET (window), 0,
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

  cmap = gimp_image_get_colormap (image, NULL, &ncols);

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
                    remap);

  g_signal_connect (iconview, "button-press-event",
                    G_CALLBACK (remap_button_press),
                    remap);

  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   ACTIONS, G_N_ELEMENTS (ACTIONS),
                                   remap);

  box = gimp_hint_box_new (_("Drag and drop colors to rearrange the colormap.  "
                             "The numbers shown are the original indices.  "
                             "Right-click for a menu with sort options."));

  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (remap_response),
                    remap);

  gtk_widget_show_all (dialog);

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

  return remap_ok;
}

static void
on_app_activate (GApplication *gapp,
                 gpointer      user_data)
{
  GimpRemap      *remap = GIMP_REMAP (user_data);
  GtkApplication *app  = GTK_APPLICATION (gapp);

  window = GTK_WINDOW (gtk_application_window_new (app));
  gtk_window_set_title (window, _("Rearrange Colors"));
  gtk_window_set_role (window, PLUG_IN_ROLE);
  gimp_help_connect (GTK_WIDGET (window),
                     gimp_standard_help_func, PLUG_IN_PROC_REMAP,
                     window, NULL);

  remap_dialog (remap->image, remap->map, remap);

  gtk_application_set_accels_for_action (app, "win.sort-hue", (const char*[]) { NULL });
  gtk_application_set_accels_for_action (app, "win.sort-sat", (const char*[]) { NULL });
  gtk_application_set_accels_for_action (app, "win.sort-val", (const char*[]) { NULL });

  gtk_application_set_accels_for_action (app, "win.reverse", (const char*[]) { NULL });

  gtk_application_set_accels_for_action (app, "win.reset", (const char*[]) { NULL });
}
