/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpexport.c
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


typedef void (* ExportFunc) (gint32  imageID,
                             gint32 *drawable_ID);


/* the export action structure */
typedef struct
{
  ExportFunc  default_action;
  ExportFunc  alt_action;
  gchar      *reason;
  gchar      *possibilities[2];
  gint        choice;
} ExportAction;


/* the functions that do the actual export */

static void
export_merge (gint32  image_ID,
              gint32 *drawable_ID)
{
  gint32  nlayers;
  gint32  nvisible = 0;
  gint32  i;
  gint32 *layers;
  gint32  merged;
  gint32  transp;

  layers = gimp_image_get_layers (image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (gimp_drawable_get_visible (layers[i]))
        nvisible++;
    }

  if (nvisible <= 1)
    {
      /* if there is only one (or zero) visible layer, add a new transparent
         layer that has the same size as the canvas.  The merge that follows
         will ensure that the offset, opacity and size are correct */
      transp = gimp_layer_new (image_ID, "-",
                               gimp_image_width (image_ID),
                               gimp_image_height (image_ID),
                               gimp_drawable_type (*drawable_ID) | 1,
                               100.0, GIMP_NORMAL_MODE);
      gimp_image_add_layer (image_ID, transp, 1);
      gimp_selection_none (image_ID);
      gimp_edit_clear (transp);
      nvisible++;
    }

  if (nvisible > 1)
    {
      g_free (layers);
      merged = gimp_image_merge_visible_layers (image_ID, GIMP_CLIP_TO_IMAGE);

      if (merged != -1)
        *drawable_ID = merged;
      else
        return;  /* shouldn't happen */

      layers = gimp_image_get_layers (image_ID, &nlayers);

      /*  make sure that the merged drawable matches the image size  */
      if (gimp_drawable_width  (merged) != gimp_image_width  (image_ID) ||
          gimp_drawable_height (merged) != gimp_image_height (image_ID))
        {
          gint off_x, off_y;

          gimp_drawable_offsets (merged, &off_x, &off_y);
          gimp_layer_resize (merged,
                             gimp_image_width (image_ID),
                             gimp_image_height (image_ID),
                             off_x, off_y);
        }
    }

  /* remove any remaining (invisible) layers */
  for (i = 0; i < nlayers; i++)
    {
      if (layers[i] != *drawable_ID)
        gimp_image_remove_layer (image_ID, layers[i]);
    }
  g_free (layers);
}

static void
export_flatten (gint32  image_ID,
                gint32 *drawable_ID)
{
  gint32 flattened;

  flattened = gimp_image_flatten (image_ID);

  if (flattened != -1)
    *drawable_ID = flattened;
}

static void
export_apply_masks (gint32  image_ID,
                    gint   *drawable_ID)
{
  gint32  n_layers;
  gint32 *layers;
  gint    i;

  layers = gimp_image_get_layers (image_ID, &n_layers);

  for (i = 0; i < n_layers; i++)
    {
      if (gimp_layer_get_mask (layers[i]) != -1)
        gimp_layer_remove_mask (layers[i], GIMP_MASK_APPLY);
    }

  g_free (layers);
}

static void
export_convert_rgb (gint32  image_ID,
                    gint32 *drawable_ID)
{
  gimp_image_convert_rgb (image_ID);
}

static void
export_convert_grayscale (gint32  image_ID,
                          gint32 *drawable_ID)
{
  gimp_image_convert_grayscale (image_ID);
}

static void
export_convert_indexed (gint32  image_ID,
                        gint32 *drawable_ID)
{
  gint32 nlayers;

  /* check alpha */
  g_free (gimp_image_get_layers (image_ID, &nlayers));
  if (nlayers > 1 || gimp_drawable_has_alpha (*drawable_ID))
    gimp_image_convert_indexed (image_ID, GIMP_NO_DITHER,
                                GIMP_MAKE_PALETTE, 255, FALSE, FALSE, "");
  else
    gimp_image_convert_indexed (image_ID, GIMP_NO_DITHER,
                                GIMP_MAKE_PALETTE, 256, FALSE, FALSE, "");
}

static void
export_convert_bitmap (gint32  image_ID,
                       gint32 *drawable_ID)
{
  if (gimp_image_base_type (image_ID) == GIMP_INDEXED)
    gimp_image_convert_rgb (image_ID);

  gimp_image_convert_indexed (image_ID, GIMP_FS_DITHER,
                              GIMP_MAKE_PALETTE, 2, FALSE, FALSE, "");
}

static void
export_add_alpha (gint32  image_ID,
                  gint32 *drawable_ID)
{
  gint32  nlayers;
  gint32  i;
  gint32 *layers;

  layers = gimp_image_get_layers (image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (!gimp_drawable_has_alpha (layers[i]))
        gimp_layer_add_alpha (layers[i]);
    }
  g_free (layers);
}


/* a set of predefined actions */

static ExportAction export_action_merge =
{
  export_merge,
  NULL,
  N_("%s plug-in can't handle layers"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_merge_single =
{
  export_merge,
  NULL,
  N_("%s plug-in can't handle layer offsets, size or opacity"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_animate_or_merge =
{
  export_merge,
  NULL,
  N_("%s plug-in can only handle layers as animation frames"),
  { N_("Merge Visible Layers"), N_("Save as Animation")},
  0
};

static ExportAction export_action_animate_or_flatten =
{
  export_flatten,
  NULL,
  N_("%s plug-in can only handle layers as animation frames"),
  { N_("Flatten Image"), N_("Save as Animation") },
  0
};

static ExportAction export_action_merge_or_flatten =
{
  export_flatten,
  export_merge,
  N_("%s plug-in can't handle layers"),
  { N_("Flatten Image"), N_("Merge Visible Layers") },
  1
};

static ExportAction export_action_flatten =
{
  export_flatten,
  NULL,
  N_("%s plug-in can't handle transparency"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_apply_masks =
{
  export_apply_masks,
  NULL,
  N_("%s plug-in can't handle layer masks"),
  { N_("Apply Layer Masks"), NULL },
  0
};

static ExportAction export_action_convert_rgb =
{
  export_convert_rgb,
  NULL,
  N_("%s plug-in can only handle RGB images"),
  { N_("Convert to RGB"), NULL },
  0
};

static ExportAction export_action_convert_grayscale =
{
  export_convert_grayscale,
  NULL,
  N_("%s plug-in can only handle grayscale images"),
  { N_("Convert to Grayscale"), NULL },
  0
};

static ExportAction export_action_convert_indexed =
{
  export_convert_indexed,
  NULL,
  N_("%s plug-in can only handle indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_bitmap =
{
  export_convert_bitmap,
  NULL,
  N_("%s plug-in can only handle bitmap (two color) indexed images"),
  { N_("Convert to Indexed using bitmap default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_rgb_or_grayscale =
{
  export_convert_rgb,
  export_convert_grayscale,
  N_("%s plug-in can only handle RGB or grayscale images"),
  { N_("Convert to RGB"), N_("Convert to Grayscale")},
  0
};

static ExportAction export_action_convert_rgb_or_indexed =
{
  export_convert_rgb,
  export_convert_indexed,
  N_("%s plug-in  can only handle RGB or indexed images"),
  { N_("Convert to RGB"), N_("Convert to Indexed using default settings\n"
                             "(Do it manually to tune the result)")},
  0
};

static ExportAction export_action_convert_indexed_or_grayscale =
{
  export_convert_indexed,
  export_convert_grayscale,
  N_("%s plug-in can only handle grayscale or indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"),
    N_("Convert to Grayscale") },
  0
};

static ExportAction export_action_add_alpha =
{
  export_add_alpha,
  NULL,
  N_("%s plug-in needs an alpha channel"),
  { N_("Add Alpha Channel"), NULL},
  0
};


/* dialog functions */

static void
export_toggle_callback (GtkWidget *widget,
                        gpointer   data)
{
  gint *choice = (gint *) data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *choice = FALSE;
  else
    *choice = TRUE;
}

static GimpExportReturn
confirm_save_dialog (const gchar *message,
                     const gchar *format_name)
{
  GtkWidget        *dialog;
  GtkWidget        *hbox;
  GtkWidget        *image;
  GtkWidget        *main_vbox;
  GtkWidget        *label;
  gchar            *text;
  GimpExportReturn  retval;

  g_return_val_if_fail (message != NULL, GIMP_EXPORT_CANCEL);
  g_return_val_if_fail (format_name != NULL, GIMP_EXPORT_CANCEL);

  dialog = gimp_dialog_new (_("Confirm Save"), "gimp-export-image-confirm",
                            NULL, 0,
                            gimp_standard_help_func,
                            "gimp-export-confirm-dialog",

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("Confirm"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  text = g_strdup_printf (message, format_name);
  label = gtk_label_new (text);
  g_free (text);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  switch (gimp_dialog_run (GIMP_DIALOG (dialog)))
    {
    case GTK_RESPONSE_OK:
      retval = GIMP_EXPORT_EXPORT;
      break;

    default:
      retval = GIMP_EXPORT_CANCEL;
      break;
    }

  gtk_widget_destroy (dialog);

  return retval;
}

static GimpExportReturn
export_dialog (GSList      *actions,
               const gchar *format_name)
{
  GtkWidget        *dialog;
  GtkWidget        *hbox;
  GtkWidget        *image;
  GtkWidget        *main_vbox;
  GtkWidget        *label;
  GSList           *list;
  gchar            *text;
  GimpExportReturn  retval;

  g_return_val_if_fail (actions != NULL, GIMP_EXPORT_CANCEL);
  g_return_val_if_fail (format_name != NULL, GIMP_EXPORT_CANCEL);

  dialog = gimp_dialog_new (_("Export File"), "gimp-export-image",
                            NULL, 0,
                            gimp_standard_help_func, "gimp-export-dialog",

                            _("_Ignore"),     GTK_RESPONSE_NO,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("_Export"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_NO,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  /* the headline */
  text = g_strdup_printf (_("Your image should be exported before it "
                            "can be saved as %s for the following reasons:"),
                          format_name);
  label = gtk_label_new (text);
  g_free (text);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  for (list = actions; list; list = g_slist_next (list))
    {
      ExportAction *action = list->data;
      GtkWidget    *frame;
      GtkWidget    *vbox;

      text = g_strdup_printf (gettext (action->reason), format_name);
      frame = gimp_frame_new (text);
      g_free (text);

      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      if (action->possibilities[0] && action->possibilities[1])
        {
          GtkWidget *button;
          GSList    *radio_group = NULL;

          button = gtk_radio_button_new_with_label (radio_group,
                                                    gettext (action->possibilities[0]));
          gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child),
                                 GTK_JUSTIFY_LEFT);
          radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
          g_signal_connect (button, "toggled",
                            G_CALLBACK (export_toggle_callback),
                            &action->choice);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                        (action->choice == 0));
          gtk_widget_show (button);

          button = gtk_radio_button_new_with_label (radio_group,
                                                    gettext (action->possibilities[1]));
          gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child),
                                 GTK_JUSTIFY_LEFT);
          radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
          gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                        (action->choice == 1));
          gtk_widget_show (button);
        }
      else if (action->possibilities[0])
        {
          label = gtk_label_new (gettext (action->possibilities[0]));
          gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
          gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
          gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
          gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
          gtk_widget_show (label);
          action->choice = 0;
        }

      gtk_widget_show (vbox);
    }

  /* the footline */
  label = gtk_label_new (_("The export conversion won't modify your "
                           "original image."));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  switch (gimp_dialog_run (GIMP_DIALOG (dialog)))
    {
    case GTK_RESPONSE_OK:
      retval = GIMP_EXPORT_EXPORT;
      break;

    case GTK_RESPONSE_NO:
      retval = GIMP_EXPORT_IGNORE;
      break;

    default:
      retval = GIMP_EXPORT_CANCEL;
      break;
    }

  gtk_widget_destroy (dialog);

  return retval;
}

/**
 * gimp_export_image:
 * @image_ID: Pointer to the image_ID.
 * @drawable_ID: Pointer to the drawable_ID.
 * @format_name: The (short) name of the image_format (e.g. JPEG or GIF).
 * @capabilities: What can the image_format do?
 *
 * Takes an image and a drawable to be saved together with a
 * description of the capabilities of the image_format. If the
 * type of image doesn't match the capabilities of the format
 * a dialog is opened that informs the user that the image has
 * to be exported and offers to do the necessary conversions.
 *
 * If the user chooses to export the image, a copy is created.
 * This copy is then converted, the image_ID and drawable_ID
 * are changed to point to the new image and the procedure returns
 * GIMP_EXPORT_EXPORT. The save_plugin has to take care of deleting the
 * created image using gimp_image_delete() when it has saved it.
 *
 * If the user chooses to Ignore the export problem, the image_ID
 * and drawable_ID is not altered, GIMP_EXPORT_IGNORE is returned and
 * the save_plugin should try to save the original image. If the
 * user chooses Cancel, GIMP_EXPORT_CANCEL is returned and the
 * save_plugin should quit itself with status #GIMP_PDB_CANCEL.
 *
 * Returns: An enum of #GimpExportReturn describing the user_action.
 **/
GimpExportReturn
gimp_export_image (gint32                 *image_ID,
                   gint32                 *drawable_ID,
                   const gchar            *format_name,
                   GimpExportCapabilities  capabilities)
{
  GSList            *actions = NULL;
  GSList            *list;
  GimpImageBaseType  type;
  gint32             i;
  gint32             n_layers;
  gint32            *layers;
  gboolean           added_flatten        = FALSE;
  gboolean           has_layer_masks      = FALSE;
  gboolean           background_has_alpha = TRUE;
  GimpExportReturn   retval               = GIMP_EXPORT_CANCEL;

  g_return_val_if_fail (*image_ID > -1 && *drawable_ID > -1, FALSE);

  /* do some sanity checks */
  if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_ALPHA;

  if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

  if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

  if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_ALPHA;

  /* ask for confirmation if the user is not saving a layer (see bug #51114) */
  if (format_name &&
      ! gimp_drawable_is_layer (*drawable_ID) &&
      ! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
    {
      if (gimp_drawable_is_layer_mask (*drawable_ID))
        {
          retval = confirm_save_dialog
            (_("You are about to save a layer mask as %s.\n"
               "This will not save the visible layers."), format_name);
        }
      else if (gimp_drawable_is_channel (*drawable_ID))
        {
          retval = confirm_save_dialog
            (_("You are about to save a channel (saved selection) as %s.\n"
               "This will not save the visible layers."), format_name);
        }
      else
        {
          /* this should not happen */
          g_warning ("%s: unknown drawable type!", G_STRFUNC);
        }

      /* cancel - the user can then select an appropriate layer to save */
      if (retval == GIMP_EXPORT_CANCEL)
        return GIMP_EXPORT_CANCEL;
    }


  /* check alpha and layer masks */
  layers = gimp_image_get_layers (*image_ID, &n_layers);

  for (i = 0; i < n_layers; i++)
    {
      if (gimp_drawable_has_alpha (layers[i]))
        {
          if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_ALPHA))
            {
              actions = g_slist_prepend (actions, &export_action_flatten);
              added_flatten = TRUE;
              break;
            }
        }
      else
        {
          /*  If this is the last layer, it's visible and has no alpha
           *  channel, then the image has a "flat" background
           */
                if (i == n_layers - 1 && gimp_drawable_get_visible (layers[i]))
            background_has_alpha = FALSE;

          if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
            {
              actions = g_slist_prepend (actions, &export_action_add_alpha);
              break;
            }
        }
    }

  if (! added_flatten)
    {
      for (i = 0; i < n_layers; i++)
        {
          if (gimp_layer_get_mask (layers[i]) != -1)
            has_layer_masks = TRUE;
        }
    }

  g_free (layers);

  if (! added_flatten)
    {
      /* check if layer size != canvas size, opacity != 100%, or offsets != 0 */
      if (n_layers == 1                         &&
          gimp_drawable_is_layer (*drawable_ID) &&
          ! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
        {
          gint offset_x;
          gint offset_y;

          gimp_drawable_offsets (*drawable_ID, &offset_x, &offset_y);

          if ((gimp_layer_get_opacity (*drawable_ID) < 100.0) ||
              (gimp_image_width (*image_ID) !=
               gimp_drawable_width (*drawable_ID))            ||
              (gimp_image_height (*image_ID) !=
               gimp_drawable_height (*drawable_ID))           ||
              offset_x || offset_y)
            {
              if (capabilities & GIMP_EXPORT_CAN_HANDLE_ALPHA)
                {
                  actions = g_slist_prepend (actions,
                                             &export_action_merge_single);
                }
              else
                {
                  actions = g_slist_prepend (actions,
                                             &export_action_flatten);
                  added_flatten = TRUE;
                }
            }
        }
      /* check multiple layers */
      else if (n_layers > 1)
        {
          if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
            {
              if (background_has_alpha ||
                  capabilities & GIMP_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_animate_or_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_animate_or_flatten);
            }
          else if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
            {
              if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_merge_or_flatten);
            }
        }

      /* check layer masks */
      if (has_layer_masks &&
          ! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS))
        actions = g_slist_prepend (actions, &export_action_apply_masks);
    }

  /* check the image type */
  type = gimp_image_base_type (*image_ID);
  switch (type)
    {
    case GIMP_RGB:
      if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB))
        {
          if ((capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED) &&
              (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed_or_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_BITMAP)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_bitmap);
        }
      break;

    case GIMP_GRAY:
      if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
        {
          if ((capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) &&
              (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb_or_indexed);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_BITMAP)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_bitmap);
        }
      break;

    case GIMP_INDEXED:
      if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED))
        {
          if ((capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) &&
              (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb_or_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_BITMAP)
            {
              gint n_colors;

              g_free (gimp_image_get_colormap (*image_ID, &n_colors));

              if (n_colors > 2)
                actions = g_slist_prepend (actions,
                                           &export_action_convert_bitmap);
            }
        }
      break;
    }

  if (actions)
    {
      actions = g_slist_reverse (actions);

      if (format_name)
        retval = export_dialog (actions, format_name);
      else
        retval = GIMP_EXPORT_EXPORT;
    }
  else
    {
      retval = GIMP_EXPORT_IGNORE;
    }

  if (retval == GIMP_EXPORT_EXPORT)
    {
      *image_ID = gimp_image_duplicate (*image_ID);
      *drawable_ID = gimp_image_get_active_layer (*image_ID);

      gimp_image_undo_disable (*image_ID);

      for (list = actions; list; list = list->next)
        {
          ExportAction *action = list->data;

          if (action->choice == 0 && action->default_action)
            action->default_action (*image_ID, drawable_ID);
          else if (action->choice == 1 && action->alt_action)
            action->alt_action (*image_ID, drawable_ID);
        }
    }

  g_slist_free (actions);

  return retval;
}
