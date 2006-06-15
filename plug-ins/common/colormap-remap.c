/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Colormap remapping plug-in
 * Copyright (C) 2006 Mukund Sivaraman <muks@mukund.org>
 *
 * This plug-in takes the colormap and lets you move colors from one index
 * to another while keeping the original image visually unmodified.
 *
 * Such functionality is useful for creating graphics files for applications
 * which expect certain indices to contain some specific colours.
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


/* Declare local functions.
 */
static void       query        (void);
static void       run          (const gchar      *name,
                                gint              nparams,
                                const GimpParam  *param,
                                gint             *nreturn_vals,
                                GimpParam       **return_vals);

static gboolean   remap        (gint32            image_ID,
                                gboolean          is_non_interactive,
                                gint              num_colors,
                                guchar           *map);

static gboolean   remap_swap   (gint32            image_ID,
                                guchar           *map,
                                guchar            index1,
                                guchar            index2);

static gboolean   remap_dialog (gint32            image_ID,
                                guchar           *map);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef remap_args[] =
  {
    { GIMP_PDB_INT32,     "run-mode",   "Interactive, non-interactive"        },
    { GIMP_PDB_IMAGE,     "image",      "Input image"                         },
    { GIMP_PDB_DRAWABLE,  "drawable",   "Input drawable"                      },
    { GIMP_PDB_INT32,     "num-colors", "Length of `map' argument "
                                        "(should be equal to colormap size)"  },
    { GIMP_PDB_INT8ARRAY, "map",        "Remap array for the colormap"        }
  };

  static const GimpParamDef swap_args[] =
  {
    { GIMP_PDB_INT32,     "run-mode",   "Interactive, non-interactive"        },
    { GIMP_PDB_IMAGE,     "image",      "Input image"                         },
    { GIMP_PDB_DRAWABLE,  "drawable",   "Input drawable"                      },
    { GIMP_PDB_INT8,      "index1",     "First index in the colormap"         },
    { GIMP_PDB_INT8,      "index2",     "Second (other) index in the colormap"}
  };

  gimp_install_procedure (PLUG_IN_PROC_REMAP,
                          N_("Rearrange the colormap"),
                          "This procedure takes an indexed image and lets you "
                          "alter the positions of colors in the colormap "
                          "without visually changing the image.",
                          "Mukund Sivaraman <muks@mukund.org>",
                          "Mukund Sivaraman <muks@mukund.org>",
                          "14th June 2006",
                          N_("_Rearrange Colormap..."),
                          "INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (remap_args), 0,
                          remap_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC_REMAP, "<Image>/Colors/Map");

  gimp_install_procedure (PLUG_IN_PROC_SWAP,
                          N_("Swap two colors in the colormap"),
                          "This procedure takes an indexed image and lets you "
                          "swap the positions of two colors in the colormap "
                          "without visually changing the image.",
                          "Mukund Sivaraman <muks@mukund.org>",
                          "Mukund Sivaraman <muks@mukund.org>",
                          "14th June 2006",
                          N_("_Swap Colors"),
                          "INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (swap_args), 0,
                          swap_args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint32             image_ID;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  guchar             ui_map[256];
  gint               num_colors = 256;
  gboolean           is_non_interactive = FALSE;
  guchar            *map = ui_map;
  gint               i;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  image_ID = param[1].data.d_image;

  if (strcmp (name, PLUG_IN_PROC_REMAP) == 0)
    {
      /*  Make sure that the drawable is indexed  */
      if (gimp_image_base_type (image_ID) == GIMP_INDEXED)
        {
          for (i = 0; i < 256; i++)
            ui_map[i] = i;

          switch (run_mode)
            {
              case GIMP_RUN_INTERACTIVE:
                if (! remap_dialog (image_ID, ui_map))
                    status = GIMP_PDB_EXECUTION_ERROR;
                break;

              case GIMP_RUN_NONINTERACTIVE:
                is_non_interactive = TRUE;
                num_colors = param[3].data.d_int32;
                map = (guchar *) param[4].data.d_int8array;
                break;

              case GIMP_RUN_WITH_LAST_VALS:
                gimp_get_data (PLUG_IN_PROC_REMAP, &ui_map);
                if (ui_map == NULL)
                  status = GIMP_PDB_EXECUTION_ERROR;
                break;
            }

          if (status == GIMP_PDB_SUCCESS)
            {
              if (! remap (image_ID, is_non_interactive, num_colors, map))
                {
                  status = GIMP_PDB_EXECUTION_ERROR;
                }
              else if (run_mode != GIMP_RUN_WITH_LAST_VALS)
                {
                  if (map == ui_map)
                    {
                      gimp_set_data (PLUG_IN_PROC_REMAP,
                                     &ui_map, sizeof (ui_map));
                    }
                  else
                    {
                      for (i = 0; i < num_colors; i++)
                        ui_map[i] = map[i];

                      gimp_set_data (PLUG_IN_PROC_REMAP,
                                     &ui_map, sizeof (ui_map));
                    }
                }

              if (run_mode != GIMP_RUN_NONINTERACTIVE)
                gimp_displays_flush ();
            }
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, PLUG_IN_PROC_SWAP) == 0)
    {
      /*  Make sure that the drawable is indexed  */
      if (gimp_image_base_type (image_ID) == GIMP_INDEXED)
        {
          for (i = 0; i < 256; i++)
            ui_map[i] = i;

          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
          else if (run_mode == GIMP_RUN_WITH_LAST_VALS)
            {
              gimp_get_data (PLUG_IN_PROC_REMAP, &ui_map);

              if (ui_map == NULL)
                status = GIMP_PDB_EXECUTION_ERROR;
              else if (! remap (image_ID, FALSE, 256, ui_map))
                status = GIMP_PDB_EXECUTION_ERROR;
            }
          else
            {
              guchar index1 = param[3].data.d_int8;
              guchar index2 = param[4].data.d_int8;

              if (! remap_swap (image_ID, ui_map, index1, index2))
                status = GIMP_PDB_CALLING_ERROR;

              if (! remap (image_ID, FALSE, 256, ui_map))
                status = GIMP_PDB_EXECUTION_ERROR;
              else
                gimp_set_data (PLUG_IN_PROC_SWAP, &ui_map, sizeof (ui_map));
            }
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
}


static gboolean
remap (gint32    image_ID,
       gboolean  is_non_interactive,
       gint      num_colors,
       guchar   *map)
{
  guchar   *cmap, *new_cmap;
  guchar   *new_cmap_i;
  gint      ncols;
  gint      num_layers;
  gint32   *layers;
  gint      i, j, k;
  glong     pixels = 0;
  glong     processed = 0;
  guchar    pixel_map[256];
  gboolean  valid[256];

  cmap = gimp_image_get_colormap (image_ID, &ncols);

  g_return_val_if_fail (cmap != NULL, FALSE);
  g_return_val_if_fail (ncols > 0, FALSE);

  if (is_non_interactive && (num_colors != ncols))
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
      j = map[i] * 3;

      *new_cmap_i++ = cmap[j];
      *new_cmap_i++ = cmap[j + 1];
      *new_cmap_i++ = cmap[j + 2];
    }

  gimp_image_undo_group_start (image_ID);

  gimp_image_set_colormap (image_ID, new_cmap, ncols);

  g_free (cmap);
  g_free (new_cmap);

  gimp_progress_init (_("Rearranging the colormap"));

  layers = gimp_image_get_layers (image_ID, &num_layers);

  for (k = 0; k < num_layers; k++)
    pixels +=
      gimp_drawable_width (layers[k]) * gimp_drawable_height (layers[k]);

  for (k = 0; k < num_layers; k++)
    {
      GimpDrawable *drawable;
      GimpPixelRgn  src_rgn, dest_rgn;
      gint          width, height, bytespp;
      gint          update;
      gpointer      pr;

      drawable = gimp_drawable_get (layers[k]);

      width   = drawable->width;
      height  = drawable->height;
      bytespp = drawable->bpp;

      gimp_pixel_rgn_init (&src_rgn,
                           drawable, 0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&dest_rgn,
                           drawable, 0, 0, width, height, TRUE, TRUE);

      for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn), update = 0;
           pr != NULL;
           pr = gimp_pixel_rgns_process (pr), update++)
        {
          const guchar *src_row = src_rgn.data;
          guchar       *dest_row = dest_rgn.data;

          for (i = 0; i < src_rgn.h; i++)
            {
              const guchar *src  = src_row;
              guchar       *dest = dest_row;

              if (bytespp == 1)
                {
                  for (j = 0; j < src_rgn.w; j++)
                    *dest++ = pixel_map[*src++];
                }
              else
                {
                  for (j = 0; j < src_rgn.w; j++)
                    {
                      *dest++ = pixel_map[*src++];
                      *dest++ = *src++;
                    }
                }

              src_row += src_rgn.rowstride;
              dest_row += dest_rgn.rowstride;
            }

          processed += src_rgn.w * src_rgn.h;
          update %= 16;

          if (update == 0)
            gimp_progress_update ((gdouble) processed / pixels);
        }

      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, 0, 0, width, height);
      gimp_drawable_detach (drawable);
    }

  gimp_progress_update (1.0);

  gimp_image_undo_group_end (image_ID);

  return TRUE;

}

static gboolean
remap_swap (gint32  image_ID,
            guchar *map,
            guchar  index1,
            guchar  index2)
{
  gint    ncols;
  guchar  tmp;

  g_free (gimp_image_get_colormap (image_ID, &ncols));

  g_return_val_if_fail (ncols > 0, FALSE);
  g_return_val_if_fail (index1 < ncols, FALSE);
  g_return_val_if_fail (index2 < ncols, FALSE);

  tmp = map[index1];
  map[index1] = map[index2];
  map[index2] = tmp;

  return TRUE;
}

enum
{
  COLOR_INDEX,
  COLOR_INDEX_TEXT,
  COLOR_RGB,
  NUM_COLS
};

static gboolean
remap_dialog (gint32  image_ID,
              guchar *map)
{
  GtkWidget       *dialog;
  GtkWidget       *vbox;
  GtkWidget       *iconview;
  GtkListStore    *store;
  gboolean         run;
  guchar          *cmap;
  gint             ncols, i;
  GtkTreeIter      iter;
  GtkCellRenderer *renderer;
  GtkWidget       *label;
  PangoAttrList   *pango_attr_list;
  PangoAttribute  *pango_attr;
  gboolean         valid;

  cmap = gimp_image_get_colormap (image_ID, &ncols);

  g_return_val_if_fail (cmap != NULL, FALSE);

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Rearrange Colormap"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC_REMAP,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);

  label = gtk_label_new(_("Drag and drop colors to rearrange the colormap.\n"
                          "The numbers shown are the original indices."));
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_container_add (GTK_CONTAINER (vbox), label);

  store = gtk_list_store_new (NUM_COLS,
                              G_TYPE_INT, G_TYPE_STRING, GIMP_TYPE_RGB);

  for (i = 0; i < ncols; i++)
    {
      GimpRGB  color;
      gchar   *text = g_strdup_printf ("%d", i);

      gimp_rgb_set_uchar (&color,
                          cmap[(i * 3)], cmap[(i * 3) + 1], cmap[(i * 3) + 2]);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLOR_INDEX,      i,
                          COLOR_INDEX_TEXT, text,
                          COLOR_RGB,        &color,
                          -1);

      g_free (text);
    }

  iconview = gtk_icon_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_container_add (GTK_CONTAINER (vbox), iconview);

  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (iconview),
                                    GTK_SELECTION_SINGLE);
  gtk_icon_view_set_orientation (GTK_ICON_VIEW (iconview),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (iconview), 24);
  gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (iconview), 1);
  gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (iconview), 1);
  gtk_icon_view_set_reorderable (GTK_ICON_VIEW (iconview), TRUE);

  renderer = gimp_cell_renderer_color_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (iconview), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (iconview), renderer,
                                      "color", COLOR_RGB,
                                      NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (iconview), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (iconview), renderer,
                                  "text", COLOR_INDEX_TEXT,
                                  NULL);

  pango_attr_list = pango_attr_list_new ();

  pango_attr = pango_attr_size_new (5000);
  pango_attr->start_index = 0;
  pango_attr->end_index   = -1;
  pango_attr_list_insert (pango_attr_list, pango_attr);

  g_object_set (renderer,
                "attributes", pango_attr_list,
                "xalign",     0.5,
                NULL);

  pango_attr_list_unref (pango_attr_list);

  gtk_widget_show_all (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

  i = 0;

  for (valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
       valid;
       valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter))
    {
      gint index;

      gtk_tree_model_get (GTK_TREE_MODEL (store),
                          &iter, COLOR_INDEX, &index, -1);
      map[i++] = index;
    }

  gtk_widget_destroy (dialog);

  g_free (cmap);

  return run;
}

