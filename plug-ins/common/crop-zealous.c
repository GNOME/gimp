/*
 * ZealousCrop GIMP plug-in
 *
 * Version 1.00
 * by Adam D. Moss <adam@foxbox.org>, 1997
 *
 * Version 2.00
 * by Sergio Jiménez Herena <sergio.jimenez.herena@gmail.com>, 2018
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define PLUG_IN_PROC    "plug-in-zealouscrop"
#define PLUG_IN_BINARY  "crop-zealous"
#define PLUG_IN_ROLE    "gimp-zealouscrop"

#define EPSILON (1e-5)
/* FLOAT NUMBER IS ZERO */
#define F_ZERO(v) ((v) > -EPSILON && (v) < EPSILON)
/* FLOAT NUMBERS ARE EQUAL */
#define F_EQUAL(v1, v2)  (((v1) - (v2)) > -EPSILON && ((v1) - (v2)) < EPSILON)

typedef struct
{
  gint32 padding;
  gint32 all_layers;
} ZealousVals;

static ZealousVals zvals = {
  0,   /* PADDING */
  1    /* ALL LAYERS */
};

static void
query (void);

static void
run (const gchar *name,
     gint n_params,
     const GimpParam *param,
     gint *nreturn_vals,
     GimpParam **return_vals);

static inline gboolean
colors_equal (const gfloat *col1,
              const gfloat *col2,
              gint components,
              gboolean has_alpha);

static void
pad_lines (gboolean *kill_lines,
           gint32 *living_lines,
           gint32 padding,
           gint32 line_lenght);

static void
process_buffer (gint drawable_id,
                const gboolean *killrows,
                const gboolean *killcols);

static gboolean
zealous_dialog (void);

static void
zealous_crop (gint32 drawable_id,
              gint32 image_id,
              gint32 padding,
              gint32 all_layers);

const GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  /* INIT_PROC  */
  NULL,  /* QUIT_PROC  */
  query, /* QUERY_PROC */
  run,   /* RUN_PROC   */
};

MAIN ();

static void
query (void)
{
  static const GimpParamDef args[] = {
    {
      GIMP_PDB_INT32,    "run-mode",   "The run mode  {RUN-INTERACTIVE (0), "
                                         "RUN-NONINTERACTIVE (1)}"
    },
    { GIMP_PDB_IMAGE,    "image",      "Input image"},
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"},
    { GIMP_PDB_INT32,    "padding",    "Padding value"},
    { GIMP_PDB_INT32,    "all_layers", "Operate on all items"}
  };
  
  gimp_install_procedure (
    PLUG_IN_PROC,
    N_("Autocrop unused space from edges and middle"),
    "This plugin searches for 'empty' zones in the active element of the "
      "image (be it a layer a  mask or a channel). Then proceeds to strip "
      "said zones from the active element or from  all layers, masks and"
      "channels, if \"Process all elements\" is checked."
      "The padding value adds an area surrounding the zones of the image"
      "that have been detected as content to be kept as well.",
    "Adam D. Moss, Sergio Jiménez Herena",
    "Adam D. Moss",
    "1997",
    N_("_Zealous Crop..."),
    "*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args),
    0,
    args,
    NULL);
  
  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Image/Crop");
}

static void
run (const gchar *name,
     gint n_params,
     const GimpParam *param,
     gint *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  
  gint32 image_id, drawable_id, padding, all_layers;
  
  gegl_init (NULL, NULL);
  
  *nreturn_vals = 1;
  *return_vals  = values;
  
  run_mode = (GimpRunMode) param[0].data.d_int32;
  
  INIT_I18N ();
  
  switch (run_mode)
  {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &zvals);
      if (!zealous_dialog ())
        return;
      
      break;
    
    case GIMP_RUN_NONINTERACTIVE:
      if (n_params != 5)
        status = GIMP_PDB_CALLING_ERROR;
      
      if (status == GIMP_PDB_SUCCESS)
      {
        zvals.padding    = param[3].data.d_int32;
        zvals.all_layers = param[4].data.d_int32;
      }
      break;
    
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &zvals);
      break;
    
    default:
      break;
  }
  
  if (status == GIMP_PDB_SUCCESS)
  {
    image_id    = param[1].data.d_int32;
    drawable_id = param[2].data.d_int32;
    padding     = zvals.padding;
    all_layers  = zvals.all_layers;
    
    gimp_progress_init (_("Zealous cropping"));
    
    zealous_crop (drawable_id, image_id, padding, all_layers);
    
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
      gimp_displays_flush ();
  }
  
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  
  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data (PLUG_IN_PROC, &zvals, sizeof (ZealousVals));
  
  gegl_exit ();
}

static inline gboolean
colors_equal (const gfloat *col1,
              const gfloat *col2,
              gint components,
              gboolean has_alpha)
{
  if (has_alpha
      && F_ZERO (col1[components - 1])
      && F_ZERO (col2[components - 1]))
  {
    return TRUE;
  }
  else
  {
    for (gint b = 0; b < components; b++)
    {
      if (!F_EQUAL (col1[b], col2[b]))
        return FALSE;
    }
    return TRUE;
  }
}

static void
pad_lines (gboolean *kill_lines,
           gint32 *living_lines,
           gint32 padding,
           gint32 line_lenght)
{
  gboolean curr_line, prev_line, next_line;
  gint32   pad_end;
  
  for (gint line = 0; line < line_lenght; ++line)
  {
    curr_line = kill_lines[line];
    prev_line = kill_lines[MAX (line - 1, 0)];
    next_line = kill_lines[MIN (line + 1, line_lenght - 1)];
    
    if (!curr_line && prev_line)
    {
      pad_end = MAX (line - padding, 0);
      
      for (gint pad_line = line - 1; pad_line >= pad_end; --pad_line)
      {
        if (kill_lines[pad_line])
        {
          kill_lines[pad_line] = FALSE;
          (*living_lines)++;
        }
        else
        {
          break;
        }
      }
    }
    
    if (!curr_line && next_line)
    {
      pad_end = MIN (line + padding, line_lenght - 1);
      
      for (gint pad_line = line + 1; pad_line <= pad_end; ++pad_line)
      {
        if (kill_lines[pad_line])
        {
          kill_lines[pad_line] = FALSE;
          (*living_lines)++;
        }
        else
        {
          line = pad_line - 1;
          break;
        }
        
        if (pad_line == pad_end)
          line = pad_end;
      }
    }
  }
}

static void
process_buffer (gint drawable_id,
                const gboolean *killrows,
                const gboolean *killcols)
{
  GeglBuffer *r_buf  = gimp_drawable_get_buffer (drawable_id);
  GeglBuffer *w_buf  = gimp_drawable_get_shadow_buffer (drawable_id);
  gint32     width   = gegl_buffer_get_width (r_buf);
  gint32     height  = gegl_buffer_get_height (w_buf);
  gint32     destrow = 0;
  gint32     destcol = 0;
  
  /* RESTITUTE LIVING ROWS */
  
  for (gint y = 0; y < height; y++)
  {
    if (!killrows[y])
    {
      gegl_buffer_copy (r_buf,
                        GEGL_RECTANGLE (0, y, width, 1),
                        GEGL_ABYSS_NONE,
                        w_buf,
                        GEGL_RECTANGLE (0, destrow, width, 1));
      destrow++;
    }
  }
  
  /* RESTITUTE LIVING COLUMNS */
  
  for (gint x = 0; x < width; x++)
  {
    if (!killcols[x])
    {
      gegl_buffer_copy (w_buf,
                        GEGL_RECTANGLE (x, 0, 1, height),
                        GEGL_ABYSS_NONE,
                        w_buf,
                        GEGL_RECTANGLE (destcol, 0, 1, height));
      destcol++;
    }
  }
  
  gegl_buffer_flush (w_buf);
  gimp_drawable_merge_shadow (drawable_id, TRUE);
  
  g_object_unref (r_buf);
  g_object_unref (w_buf);
}

static void
zealous_crop (gint32 drawable_id,
              gint32 image_id,
              gint32 padding,
              gint32 all_layers)
{
  gint   num_layer, num_child, num_channel;
  gint   *layer_ids, *child_ids, *mask_ids, *channel_ids;
  gint32 selection_copy_id = FALSE;
  
  GeglBuffer *active_r_buf;
  gint32     buf_w, buf_h;
  const Babl *format;
  gint       components;
  gboolean   has_alpha;
  
  enum drawable_types
  {
    LAYER, MASK, CHANNEL, UNSUPPORTED
  }          drawable_type = UNSUPPORTED;
  
  gboolean *has_child_ids;
  gboolean recurse;
  
  gboolean *killrows, *killcols;
  gint32   livingrows, livingcols;
  gfloat   *linear_buffer;
  
  gimp_image_undo_group_start (image_id);
  
  /* GET ACTIVE DRAWABLE INFORMATION */
  
  gimp_progress_update (0);
  
  if (gimp_item_is_layer (drawable_id)
      && !(gimp_item_is_group (drawable_id)
           || gimp_item_is_text_layer (drawable_id)
           || gimp_layer_is_floating_sel (drawable_id)))
  {
    drawable_type = LAYER;
  }
  else if (gimp_item_is_layer_mask (drawable_id))
  {
    drawable_type = MASK;
  }
  else if (gimp_item_is_channel (drawable_id))
  {
    drawable_type = CHANNEL;
  }
  
  if (drawable_type == UNSUPPORTED)
  {
    gimp_message (_("The active element must be a normal layer, "
                      "a layer mask or a channel."));
    gimp_image_undo_group_end (image_id);
    return;
  }
  
  has_alpha = gimp_drawable_has_alpha (drawable_id);
  
  switch (gimp_drawable_type_with_alpha (drawable_id))
  {
    case GIMP_RGB_IMAGE:
    case GIMP_INDEXED_IMAGE:
    {
      format = babl_format ("R'G'B' float");
      break;
    }
    case GIMP_RGBA_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
    default:
    {
      format = babl_format ("R'G'B'A float");
      break;
    }
    case GIMP_GRAY_IMAGE:
    {
      format = babl_format ("Y' float");
      break;
    }
    case GIMP_GRAYA_IMAGE:
    {
      format = babl_format ("Y'A float");
      break;
    }
  }
  
  components = babl_format_get_n_components (format);
  
  /* ANALYZE ACTIVE DRAWABLE */
  
  active_r_buf = gimp_drawable_get_buffer (drawable_id);
  
  if (drawable_type == LAYER)
    gimp_layer_resize_to_image_size (drawable_id);
  
  buf_w         = gegl_buffer_get_width (active_r_buf);
  buf_h         = gegl_buffer_get_height (active_r_buf);
  killrows      = g_new (gboolean, buf_h);
  killcols      = g_new (gboolean, buf_w);
  linear_buffer = g_new (gfloat, MAX (buf_w, buf_h) * components);
  
  gimp_progress_update (0.10);
  
  livingrows = 0;
  
  for (gint y = 0; y < buf_h; y++)
  {
    gegl_buffer_get (active_r_buf,
                     GEGL_RECTANGLE (0, y, buf_w, 1),
                     1.0,
                     format,
                     linear_buffer,
                     GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);
    
    killrows[y] = TRUE;
    for (gint x = components; x < buf_w * components; x += components)
    {
      if (!colors_equal (linear_buffer,
                         &linear_buffer[x],
                         components,
                         has_alpha))
      {
        livingrows++;
        killrows[y] = FALSE;
        break;
      }
    }
  }
  
  gimp_progress_update (0.30);
  
  livingcols = 0;
  for (gint x = 0; x < buf_w; x++)
  {
    gegl_buffer_get (active_r_buf,
                     GEGL_RECTANGLE (x, 0, 1, buf_h),
                     1.0,
                     format,
                     linear_buffer,
                     GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);
    
    killcols[x] = TRUE;
    for (gint y = components; y < buf_h * components; y += components)
    {
      if (!colors_equal (linear_buffer, &linear_buffer[y],
                         components, has_alpha))
      {
        livingcols++;
        killcols[x] = FALSE;
        break;
      }
    }
  }
  
  gimp_progress_update (0.5);
  
  g_object_unref (active_r_buf);
  g_free (linear_buffer);
  
  if ((!livingcols || !livingrows)
      || (livingcols == buf_w && livingrows == buf_h))
  {
    gimp_message (_("Nothing to crop."));
    goto EXIT;
  }
  
  /* SAVE SELECTION TO CHANNEL */
  
  if (!gimp_selection_is_empty (image_id))
  {
    selection_copy_id = gimp_selection_save (image_id);
    gimp_selection_none (image_id);
  }
  
  /* GET ALL LAYER, MASK AND CHANNEL IDS */
  
  layer_ids     = gimp_image_get_layers (image_id, &num_layer);
  has_child_ids = g_new(gboolean, num_layer);
  
  for (gint item = 0; item < num_layer; ++item)
  {
    if (gimp_item_is_group (layer_ids[item]))
      has_child_ids[item] = TRUE;
    else
      has_child_ids[item] = FALSE;
  }
  
  do
  {
    recurse = FALSE;
    for (gint item = 0, num_item = 0; item < num_layer; ++item)
    {
      if (has_child_ids[item])
      {
        child_ids = gimp_item_get_children (layer_ids[item],
                                            &num_child);
        num_item  = num_layer + num_child;
        
        layer_ids = g_renew (gint, layer_ids, num_item);
        memcpy (layer_ids + num_layer,
                child_ids,
                sizeof (gint) * num_child);
        
        has_child_ids = g_renew (gboolean, has_child_ids, num_item);
        
        for (gint child = 0; child < num_child; ++child)
        {
          if (gimp_item_is_group (child_ids[child]))
          {
            has_child_ids[num_layer + child] = TRUE;
            recurse = TRUE;
          }
          else
          {
            has_child_ids[num_layer + child] = FALSE;
          }
        }
        
        has_child_ids[item] = FALSE;
        num_layer = num_item;
      }
    }
  }
  while (recurse);
  
  mask_ids = g_new (gint, num_layer);
  
  for (gint layer = 0; layer < num_layer; ++layer)
    mask_ids[layer] = gimp_layer_get_mask (layer_ids[layer]);
  
  channel_ids = gimp_image_get_channels (image_id, &num_channel);
  
  /* ADD PADDING */
  
  if (padding > 0)
  {
    pad_lines (killcols, &livingcols, padding, buf_w);
    pad_lines (killrows, &livingrows, padding, buf_h);
  }
  
  if (!all_layers)
  {
    /* PROCESS ACTIVE DRAWABLE ONLY */
    
    process_buffer (drawable_id, killrows, killcols);
    gimp_progress_update (0.75);
  }
  else
  {
    /* PROCESS ALL LAYERS AND LAYER MASKS */
    
    for (gint layer_i = 0; layer_i < num_layer; ++layer_i)
    {
      if (!gimp_item_is_group (layer_ids[layer_i]))
      {
        gimp_layer_resize_to_image_size (layer_ids[layer_i]);
        process_buffer (layer_ids[layer_i], killrows, killcols);
      }
      if (mask_ids[layer_i] != -1)
      {
        process_buffer (mask_ids[layer_i], killrows, killcols);
      }
    }
    
    /* PROCESS CHANNELS */
    
    if (num_channel > 0)
    {
      for (gint channel_i = 0; channel_i < num_channel; ++channel_i)
        process_buffer (channel_ids[channel_i], killrows, killcols);
    }
    
    gimp_progress_update (0.75);
  }
  
  /* ADJUST ITEMS/CANVAS SIZE */
  
  gimp_image_resize (image_id,
                     MAX (buf_w, livingcols),
                     MAX (buf_h, livingrows),
                     0, 0);
  
  gimp_image_select_rectangle (image_id, GIMP_CHANNEL_OP_REPLACE,
                               0, 0, livingcols, livingrows);
  
  gimp_selection_invert (image_id);
  
  if (all_layers || num_layer == 1)
  {
    for (gint layer = 0; layer < num_layer; ++layer)
    {
      gimp_layer_resize (layer_ids[layer], livingcols, livingrows, 0, 0);
      
      if (mask_ids[layer] != -1)
      {
        gimp_drawable_edit_fill (mask_ids[layer], GIMP_FILL_WHITE);
        gimp_drawable_invert (mask_ids[layer], FALSE);
      }
    }
    
    for (gint channel = 0; channel < num_channel; ++channel)
    {
      gimp_drawable_edit_fill (channel_ids[channel], GIMP_FILL_WHITE);
      gimp_drawable_invert (channel_ids[channel], FALSE);
    }
    
    gimp_image_crop (image_id, livingcols, livingrows, 0, 0);
  }
  else if (drawable_type == LAYER)
  {
    gimp_layer_resize (drawable_id, livingcols, livingrows, 0, 0);
  }
  else
  {
    gimp_drawable_edit_fill (drawable_id, GIMP_FILL_WHITE);
    gimp_drawable_invert (drawable_id, FALSE);
  }
  
  gimp_selection_none (image_id);
  
  /* RESTITUTE SELECTION AND ACTIVE ITEM */
  
  if (selection_copy_id)
  {
    gimp_image_select_item (image_id, GIMP_CHANNEL_OP_REPLACE,
                            selection_copy_id);
    gimp_image_remove_channel (image_id, selection_copy_id);
  }
  
  switch (drawable_type)
  {
    default:
      break;
    case LAYER:
    {
      gimp_image_set_active_layer (image_id, drawable_id);
      break;
    }
    case MASK:
    {
      gimp_image_set_active_layer (image_id,
                                   gimp_layer_from_mask (drawable_id));
      break;
    }
    case CHANNEL:
    {
      gimp_image_set_active_channel (image_id, drawable_id);
      break;
    }
  }
    
    EXIT:
  
  g_free (killrows);
  g_free (killcols);
  
  gimp_progress_update (1.00);
  gimp_image_undo_group_end (image_id);
}

static gboolean
zealous_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *padding_hbox;
  GtkWidget *padding_label;
  GtkWidget *spinbutton;
  GtkObject *spinbutton_adj;
  GtkWidget *layers_button;
  gboolean  run;
  
  gimp_ui_init (PLUG_IN_BINARY, FALSE);
  
  dialog = gimp_dialog_new (_("Zealous Crop"),
                            PLUG_IN_ROLE,
                            NULL,
                            GTK_DIALOG_MODAL,
                            gimp_standard_help_func,
                            PLUG_IN_PROC,
                            _("_Cancel"),
                            GTK_RESPONSE_CANCEL,
                            _("_OK"),
                            GTK_RESPONSE_OK,
                            NULL);
  
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  
  main_vbox = gtk_vbox_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  
  gtk_box_pack_start (
    GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
    main_vbox,
    TRUE,
    TRUE,
    0);
  
  gtk_widget_show (main_vbox);
  
  padding_hbox = gtk_hbox_new (TRUE, 6);
  gtk_container_add (GTK_CONTAINER (main_vbox), padding_hbox);
  gtk_widget_show (padding_hbox);
  
  padding_label = gtk_label_new (_("Padding"));
  gtk_box_pack_start (GTK_BOX (padding_hbox), padding_label, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (padding_label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC(padding_label), 0, 0.5);
  gtk_widget_show (padding_label);
  
  spinbutton_adj = gtk_adjustment_new (zvals.padding, 0, 100, 1, 5, 0);
  spinbutton     = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_adj),
                                        1,
                                        0);
  gtk_widget_show (spinbutton);
  gtk_box_pack_start (GTK_BOX (padding_hbox), spinbutton, TRUE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  
  g_signal_connect (spinbutton_adj,
                    "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &zvals.padding);
  
  layers_button
    = gtk_check_button_new_with_mnemonic (_("_Process all elements"));
  gtk_box_pack_start (GTK_BOX (main_vbox), layers_button, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (layers_button),
                                zvals.all_layers);
  gtk_widget_show (layers_button);
  
  g_signal_connect (layers_button,
                    "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &zvals.all_layers);
  
  gtk_widget_show (dialog);
  
  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);
  
  gtk_widget_destroy (dialog);
  
  return run;
}
