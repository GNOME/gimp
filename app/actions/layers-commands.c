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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitemundo.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"
#include "core/gimpprogress.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpdock.h"
#include "widgets/gimpenumwidgets.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpprogressdialog.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "tools/gimptexttool.h"
#include "tools/tool_manager.h"

#include "dialogs/resize-dialog.h"
#include "dialogs/scale-dialog.h"

#include "actions.h"
#include "layers-commands.h"

#include "gimp-intl.h"


static const GimpLayerModeEffects layer_modes[] =
{
  GIMP_NORMAL_MODE,
  GIMP_DISSOLVE_MODE,
  GIMP_MULTIPLY_MODE,
  GIMP_DIVIDE_MODE,
  GIMP_SCREEN_MODE,
  GIMP_OVERLAY_MODE,
  GIMP_DODGE_MODE,
  GIMP_BURN_MODE,
  GIMP_HARDLIGHT_MODE,
  GIMP_SOFTLIGHT_MODE,
  GIMP_GRAIN_EXTRACT_MODE,
  GIMP_GRAIN_MERGE_MODE,
  GIMP_DIFFERENCE_MODE,
  GIMP_ADDITION_MODE,
  GIMP_SUBTRACT_MODE,
  GIMP_DARKEN_ONLY_MODE,
  GIMP_LIGHTEN_ONLY_MODE,
  GIMP_HUE_MODE,
  GIMP_SATURATION_MODE,
  GIMP_COLOR_MODE,
  GIMP_VALUE_MODE
};


/*  local function prototypes  */

static void   layers_add_mask_query     (GimpLayer            *layer,
                                         GtkWidget            *parent);
static void   layers_scale_layer_query  (GimpDisplay          *gdisp,
                                         GimpImage            *gimage,
                                         GimpLayer            *layer,
                                         GtkWidget            *parent);
static void   layers_resize_layer_query (GimpDisplay          *gdisp,
                                         GimpImage            *gimage,
                                         GimpLayer            *layer,
                                         GimpContext          *context,
                                         GtkWidget            *parent);
static gint   layers_mode_index         (GimpLayerModeEffects  layer_mode);


/*  private variables  */

static GimpFillType  fill_type  = GIMP_TRANSPARENT_FILL;
static gchar        *layer_name = NULL;


/*  public functions  */

void
layers_text_tool_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (gimage, layer, data);
  return_if_no_widget (widget, data);

  layers_text_tool (layer, action_data_get_context (data), widget);
}

void
layers_edit_attributes_cmd_callback (GtkAction *action,
				     gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (gimage, layer, data);
  return_if_no_widget (widget, data);

  layers_edit_layer_query (layer, action_data_get_context (data), widget);
}

void
layers_new_cmd_callback (GtkAction *action,
			 gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  layers_new_layer_query (gimage, action_data_get_context (data),
                          NULL, TRUE, widget);
}

void
layers_select_cmd_callback (GtkAction *action,
                            gint       value,
                            gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GimpLayer *new_layer;
  return_if_no_image (gimage, data);

  layer = gimp_image_get_active_layer (gimage);

  new_layer = (GimpLayer *) action_select_object ((GimpActionSelectType) value,
                                                  gimage->layers,
                                                  (GimpObject *) layer);

  if (new_layer && new_layer != layer)
    {
      gimp_image_set_active_layer (gimage, new_layer);
      gimp_image_flush (gimage);
    }
}

void
layers_raise_cmd_callback (GtkAction *action,
			   gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  gimp_image_raise_layer (gimage, layer);
  gimp_image_flush (gimage);
}

void
layers_raise_to_top_cmd_callback (GtkAction *action,
				  gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  gimp_image_raise_layer_to_top (gimage, layer);
  gimp_image_flush (gimage);
}

void
layers_lower_cmd_callback (GtkAction *action,
			   gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  gimp_image_lower_layer (gimage, layer);
  gimp_image_flush (gimage);
}

void
layers_lower_to_bottom_cmd_callback (GtkAction *action,
				     gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  gimp_image_lower_layer_to_bottom (gimage, layer);
  gimp_image_flush (gimage);
}

void
layers_duplicate_cmd_callback (GtkAction *action,
			       gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GimpLayer *new_layer;
  return_if_no_layer (gimage, layer, data);

  new_layer =
    GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (layer),
                                     G_TYPE_FROM_INSTANCE (layer),
                                     TRUE));
  gimp_image_add_layer (gimage, new_layer, -1);

  gimp_image_flush (gimage);
}

void
layers_anchor_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  if (gimp_layer_is_floating_sel (layer))
    {
      floating_sel_anchor (layer);
      gimp_image_flush (gimage);
    }
}

void
layers_merge_down_cmd_callback (GtkAction *action,
				gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  gimp_image_merge_down (gimage, layer, action_data_get_context (data),
                         GIMP_EXPAND_AS_NECESSARY);
  gimp_image_flush (gimage);
}

void
layers_delete_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimp_image_remove_layer (gimage, layer);

  gimp_image_flush (gimage);
}

void
layers_text_discard_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  if (GIMP_IS_TEXT_LAYER (layer))
    gimp_text_layer_discard (GIMP_TEXT_LAYER (layer));
}

void
layers_resize_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (gimage, layer, data);
  return_if_no_widget (widget, data);

  layers_resize_layer_query (GIMP_IS_DISPLAY (data) ? data : NULL,
                             gimage, layer, action_data_get_context (data),
                             widget);
}

void
layers_resize_to_image_cmd_callback (GtkAction *action,
				     gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  gimp_layer_resize_to_image (layer, action_data_get_context (data));
  gimp_image_flush (gimage);
}

void
layers_scale_cmd_callback (GtkAction *action,
			   gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (gimage, layer, data);
  return_if_no_widget (widget, data);

  layers_scale_layer_query (action_data_get_display (data),
                            gimage, layer, widget);
}

void
layers_crop_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  gint       x1, y1, x2, y2;
  gint       off_x, off_y;
  return_if_no_layer (gimage, layer, data);

  if (! gimp_channel_bounds (gimp_image_get_mask (gimage),
                             &x1, &y1, &x2, &y2))
    {
      g_message (_("Cannot crop because the current selection is empty."));
      return;
    }

  gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

  off_x -= x1;
  off_y -= y1;

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_ITEM_RESIZE,
                               _("Crop Layer"));

  gimp_item_resize (GIMP_ITEM (layer), action_data_get_context (data),
                    x2 - x1, y2 - y1, off_x, off_y);

  gimp_image_undo_group_end (gimage);

  gimp_image_flush (gimage);
}

void
layers_mask_add_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GtkWidget *widget;
  return_if_no_layer (gimage, layer, data);
  return_if_no_widget (widget, data);

  layers_add_mask_query (layer, widget);
}

void
layers_mask_apply_cmd_callback (GtkAction *action,
                                gint       value,
                                gpointer   data)
{
  GimpImage         *gimage;
  GimpLayer         *layer;
  GimpMaskApplyMode  mode;
  return_if_no_layer (gimage, layer, data);

  mode = (GimpMaskApplyMode) value;

  if (gimp_layer_get_mask (layer))
    {
      gimp_layer_apply_mask (layer, mode, TRUE);
      gimp_image_flush (gimage);
    }
}

void
layers_mask_edit_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage     *gimage;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (gimage, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_layer_mask_set_edit (mask, active);
      gimp_image_flush (gimage);
    }
}

void
layers_mask_show_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage     *gimage;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (gimage, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_layer_mask_set_show (mask, active);
      gimp_image_flush (gimage);
    }
}

void
layers_mask_disable_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage     *gimage;
  GimpLayer     *layer;
  GimpLayerMask *mask;
  return_if_no_layer (gimage, layer, data);

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_layer_mask_set_apply (mask, ! active);
      gimp_image_flush (gimage);
    }
}

void
layers_mask_to_selection_cmd_callback (GtkAction *action,
                                       gint       value,
                                       gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *gimage;
  GimpLayer      *layer;
  GimpLayerMask  *mask;
  return_if_no_layer (gimage, layer, data);

  op = (GimpChannelOps) value;

  mask = gimp_layer_get_mask (layer);

  if (mask)
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (mask), &off_x, &off_y);

      gimp_channel_select_channel (gimp_image_get_mask (gimage),
                                   _("Layer Mask to Selection"),
                                   GIMP_CHANNEL (mask),
                                   off_x, off_y,
                                   op, FALSE, 0.0, 0.0);
      gimp_image_flush (gimage);
    }
}

void
layers_alpha_add_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  return_if_no_layer (gimage, layer, data);

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      gimp_layer_add_alpha (layer);
      gimp_image_flush (gimage);
    }
}

void
layers_alpha_to_selection_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *gimage;
  GimpLayer      *layer;
  return_if_no_layer (gimage, layer, data);

  op = (GimpChannelOps) value;

  gimp_channel_select_alpha (gimp_image_get_mask (gimage),
                             GIMP_DRAWABLE (layer),
                             op, FALSE, 0.0, 0.0);
  gimp_image_flush (gimage);
}

void
layers_opacity_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpImage      *gimage;
  GimpLayer      *layer;
  gdouble         opacity;
  GimpUndo       *undo;
  gboolean        push_undo = TRUE;
  return_if_no_layer (gimage, layer, data);

  undo = gimp_image_undo_can_compress (gimage, GIMP_TYPE_ITEM_UNDO,
                                       GIMP_UNDO_LAYER_OPACITY);

  if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
    push_undo = FALSE;

  opacity = action_select_value ((GimpActionSelectType) value,
                                 gimp_layer_get_opacity (layer),
                                 0.0, 1.0,
                                 0.01, 0.1, FALSE);
  gimp_layer_set_opacity (layer, opacity, push_undo);
  gimp_image_flush (gimage);
}

void
layers_mode_cmd_callback (GtkAction *action,
                          gint       value,
                          gpointer   data)
{
  GimpImage            *gimage;
  GimpLayer            *layer;
  GimpLayerModeEffects  layer_mode;
  gint                  index;
  GimpUndo             *undo;
  gboolean              push_undo = TRUE;
  return_if_no_layer (gimage, layer, data);

  undo = gimp_image_undo_can_compress (gimage, GIMP_TYPE_ITEM_UNDO,
                                       GIMP_UNDO_LAYER_MODE);

  if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
    push_undo = FALSE;

  layer_mode = gimp_layer_get_mode (layer);

  index = action_select_value ((GimpActionSelectType) value,
                               layers_mode_index (layer_mode),
                               0, G_N_ELEMENTS (layer_modes) - 1,
                               1.0, 1.0, FALSE);
  gimp_layer_set_mode (layer, layer_modes[index], push_undo);
  gimp_image_flush (gimage);
}

void
layers_preserve_trans_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  gboolean   preserve;
  return_if_no_layer (gimage, layer, data);

  preserve = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (preserve != gimp_layer_get_preserve_trans (layer))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (gimage, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_LAYER_PRESERVE_TRANS);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        push_undo = FALSE;

      gimp_layer_set_preserve_trans (layer, preserve, push_undo);
      gimp_image_flush (gimage);
    }
}

void
layers_text_tool (GimpLayer   *layer,
                  GimpContext *context,
                  GtkWidget   *parent)
{
  GimpImage *gimage;
  GimpTool  *active_tool;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (! gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer)))
    {
      layers_edit_layer_query (layer, context, parent);
      return;
    }

  gimage = gimp_item_get_image (GIMP_ITEM (layer));

  active_tool = tool_manager_get_active (gimage->gimp);

  if (! GIMP_IS_TEXT_TOOL (active_tool))
    {
      GimpContainer *tool_info_list = gimage->gimp->tool_info_list;
      GimpToolInfo  *tool_info;

      tool_info = (GimpToolInfo *)
        gimp_container_get_child_by_name (tool_info_list,
                                          "gimp-text-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (context, tool_info);
          active_tool = tool_manager_get_active (gimage->gimp);
        }
    }

  if (GIMP_IS_TEXT_TOOL (active_tool))
    gimp_text_tool_set_layer (GIMP_TEXT_TOOL (active_tool), layer);
}


/********************************/
/*  The new layer query dialog  */
/********************************/

typedef struct _NewLayerOptions NewLayerOptions;

struct _NewLayerOptions
{
  GtkWidget    *query_box;
  GtkWidget    *name_entry;
  GtkWidget    *size_se;

  GimpFillType  fill_type;
  gint          xsize;
  gint          ysize;

  GimpContext  *context;
  GimpImage    *gimage;
};


static void
new_layer_query_response (GtkWidget       *widget,
                          gint             response_id,
                          NewLayerOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpLayer *layer;

      if (layer_name)
        g_free (layer_name);
      layer_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

      options->xsize =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (options->size_se),
                                          0));
      options->ysize =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (options->size_se),
                                          1));

      fill_type = options->fill_type;

      layer = gimp_layer_new (options->gimage,
                              options->xsize,
                              options->ysize,
                              gimp_image_base_type_with_alpha (options->gimage),
                              layer_name,
                              GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

      if (layer)
        {
          gimp_drawable_fill_by_type (GIMP_DRAWABLE (layer),
                                      options->context,
                                      fill_type);
          gimp_image_add_layer (options->gimage, layer, -1);

          gimp_image_flush (options->gimage);
        }
      else
        {
          g_message ("new_layer_query_response: "
                     "could not allocate new layer");
        }
    }

  gtk_widget_destroy (options->query_box);
}

void
layers_new_layer_query (GimpImage   *gimage,
                        GimpContext *context,
                        GimpLayer   *template,
                        gboolean     interactive,
                        GtkWidget   *parent)
{
  NewLayerOptions *options;
  GimpLayer       *floating_sel;
  GtkWidget       *vbox;
  GtkWidget       *table;
  GtkWidget       *label;
  GtkObject       *adjustment;
  GtkWidget       *spinbutton;
  GtkWidget       *frame;
  GtkWidget       *button;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (template == NULL || GIMP_IS_LAYER (template));

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((floating_sel = gimp_image_floating_sel (gimage)))
    {
      floating_sel_to_layer (floating_sel);
      gimp_image_flush (gimage);
      return;
    }

  if (template || ! interactive)
    {
      GimpLayer            *new_layer;
      gint                  width, height;
      gint                  off_x, off_y;
      gdouble               opacity;
      GimpLayerModeEffects  mode;

      if (template)
        {
          gimp_item_offsets (GIMP_ITEM (template), &off_x, &off_y);
          width   = gimp_item_width  (GIMP_ITEM (template));
          height  = gimp_item_height (GIMP_ITEM (template));
          opacity = template->opacity;
          mode    = template->mode;
        }
      else
        {
          width   = gimp_image_get_width (gimage);
          height  = gimp_image_get_height (gimage);
          off_x   = 0;
          off_y   = 0;
          opacity = 1.0;
          mode    = GIMP_NORMAL_MODE;
        }

      gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("New Layer"));

      new_layer = gimp_layer_new (gimage, width, height,
                                  gimp_image_base_type_with_alpha (gimage),
                                  _("Empty Layer"), opacity, mode);

      gimp_drawable_fill_by_type (GIMP_DRAWABLE (new_layer), context,
                                  GIMP_TRANSPARENT_FILL);
      gimp_item_translate (GIMP_ITEM (new_layer), off_x, off_y, FALSE);

      gimp_image_add_layer (gimage, new_layer, -1);

      gimp_image_undo_group_end (gimage);
      return;
    }

  options = g_new0 (NewLayerOptions, 1);

  options->fill_type = fill_type;
  options->gimage    = gimage;
  options->context   = context;

  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                              _("New Layer"), "gimp-layer-new",
                              GIMP_STOCK_LAYER,
                              _("Create a New Layer"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_LAYER_NEW,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (new_layer_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The name label and entry  */
  options->name_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      (layer_name ? layer_name : _("New Layer")));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Layer _Name:"), 0.0, 0.5,
                             options->name_entry, 1, FALSE);

  /*  The size labels  */
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /*  The size sizeentry  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 1,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  options->size_se = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
					  TRUE, TRUE, FALSE, 10,
					  GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (options->size_se), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (options->size_se), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (options->size_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (options->size_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gtk_table_attach (GTK_TABLE (table), options->size_se, 1, 2, 1, 3,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->size_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (options->size_se),
			    GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 0,
                                  gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 1,
                                  gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 0,
			    0, gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 1,
			    0, gimage->height);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 0,
			      gimage->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 1,
			      gimage->height);

  /*  The radio frame  */
  frame = gimp_enum_radio_frame_new_with_range (GIMP_TYPE_FILL_TYPE,
                                                GIMP_FOREGROUND_FILL,
                                                GIMP_TRANSPARENT_FILL,
                                                gtk_label_new (_("Layer Fill Type")),
                                                G_CALLBACK (gimp_radio_button_update),
                                                &options->fill_type,
                                                &button);
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                   options->fill_type);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (options->query_box);
}


/**************************************/
/*  The edit layer attributes dialog  */
/**************************************/

typedef struct _EditLayerOptions EditLayerOptions;

struct _EditLayerOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *toggle;

  GimpLayer *layer;
  GimpImage *gimage;
};

static void
edit_layer_query_response (GtkWidget        *widget,
                           gint              response_id,
                           EditLayerOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpLayer   *layer = options->layer;
      const gchar *new_name;

      new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

      if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (layer))))
        {
          gimp_item_rename (GIMP_ITEM (layer), new_name);
          gimp_image_flush (options->gimage);
        }

      if (options->toggle &&
          gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer)))
        {
          g_object_set (layer,
                        "auto-rename",
                        GTK_TOGGLE_BUTTON (options->toggle)->active,
                        NULL);
        }
    }

  gtk_widget_destroy (options->query_box);
}

static void
edit_layer_toggle_rename (GtkWidget        *widget,
                          EditLayerOptions *options)
{
  if (GTK_TOGGLE_BUTTON (widget)->active &&
      gimp_drawable_is_text_layer (GIMP_DRAWABLE (options->layer)))
    {
      GimpTextLayer *text_layer = GIMP_TEXT_LAYER (options->layer);
      GimpText      *text       = gimp_text_layer_get_text (text_layer);

      if (text && text->text)
        {
          gchar *name = gimp_utf8_strtrim (text->text, 30);

          gtk_entry_set_text (GTK_ENTRY (options->name_entry), name);

          g_free (name);
        }
    }
}

void
layers_edit_layer_query (GimpLayer   *layer,
                         GimpContext *context,
                         GtkWidget   *parent)
{
  EditLayerOptions *options;
  GtkWidget        *vbox;
  GtkWidget        *table;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  options = g_new0 (EditLayerOptions, 1);

  options->layer  = layer;
  options->gimage = gimp_item_get_image (GIMP_ITEM (layer));

  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (layer),
                              _("Layer Attributes"), "gimp-layer-edit",
                              GIMP_STOCK_EDIT,
                              _("Edit Layer Attributes"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_LAYER_EDIT,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (edit_layer_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->name_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (options->name_entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      gimp_object_get_name (GIMP_OBJECT (layer)));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Layer _Name"), 1.0, 0.5,
                             options->name_entry, 1, FALSE);

  /*  For text layers add a toggle to control "auto-rename"  */
  if (gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer)))
    {
      options->toggle =
        gtk_check_button_new_with_mnemonic (_("Set Name from _Text"));

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->toggle),
                                    GIMP_TEXT_LAYER (layer)->auto_rename);

      gtk_table_attach (GTK_TABLE (table), options->toggle, 1, 2, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (options->toggle);

      g_signal_connect (options->toggle, "toggled",
                        G_CALLBACK (edit_layer_toggle_rename),
                        options);
    }

  gtk_widget_show (options->query_box);
}


/*******************************/
/*  The add mask query dialog  */
/*******************************/

typedef struct _AddMaskOptions AddMaskOptions;

struct _AddMaskOptions
{
  GtkWidget       *query_box;
  GimpLayer       *layer;
  GimpAddMaskType  add_mask_type;
  gboolean         invert;
};

static void
add_mask_query_response (GtkWidget      *widget,
                         gint            response_id,
                         AddMaskOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpImage     *gimage;
      GimpLayerMask *mask;
      GimpLayer     *layer;

      if ((layer = (options->layer)) && (gimage = GIMP_ITEM (layer)->gimage))
        {
          gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_LAYER_ADD_MASK,
                                       _("Add Layer Mask"));

          mask = gimp_layer_create_mask (layer, options->add_mask_type);

          if (options->invert)
            gimp_channel_invert (GIMP_CHANNEL (mask), FALSE);

          gimp_layer_add_mask (layer, mask, TRUE);

          gimp_image_undo_group_end (gimage);

          gimp_image_flush (gimage);
        }
    }

  gtk_widget_destroy (options->query_box);
}

static void
layers_add_mask_query (GimpLayer *layer,
                       GtkWidget *parent)
{
  AddMaskOptions *options;
  GtkWidget      *vbox;
  GtkWidget      *frame;
  GtkWidget      *button;
  GimpImage      *gimage;

  /*  The new options structure  */
  options = g_new (AddMaskOptions, 1);
  options->layer         = layer;
  options->add_mask_type = GIMP_ADD_WHITE_MASK;
  options->invert        = FALSE;

  gimage = gimp_item_get_image (GIMP_ITEM (layer));

  /*  The dialog  */
  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (layer),
                              _("Add Layer Mask"), "gimp-layer-add-mask",
                              GIMP_STOCK_LAYER_MASK,
                              _("Add a Mask to the Layer"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_LAYER_MASK_ADD,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (add_mask_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free, options);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);
  gtk_widget_show (vbox);

  frame =
    gimp_enum_radio_frame_new (GIMP_TYPE_ADD_MASK_TYPE,
                               gtk_label_new (_("Initialize Layer Mask to:")),
                               G_CALLBACK (gimp_radio_button_update),
                               &options->add_mask_type,
                               &button);
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                   options->add_mask_type);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_check_button_new_with_mnemonic (_("In_vert Mask"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), options->invert);
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->invert);

  gtk_widget_show (options->query_box);
}


/****************************/
/*  The scale layer dialog  */
/****************************/

static void
scale_layer_callback (GtkWidget             *dialog,
                      GimpViewable          *viewable,
                      gint                   width,
                      gint                   height,
                      GimpUnit               unit,
                      GimpInterpolationType  interpolation,
                      gpointer               data)
{
  GimpDisplay *gdisp = data ? GIMP_DISPLAY (data) : NULL;

  if (width > 0 && height > 0)
    {
      GimpItem     *item = GIMP_ITEM (viewable);
      GimpProgress *progress;
      GtkWidget    *progress_dialog = NULL;

      gtk_widget_destroy (dialog);

      if (width == gimp_item_width (item) && height == gimp_item_height (item))
        return;

      if (gdisp)
        {
          progress = GIMP_PROGRESS (gdisp);
        }
      else
        {
          progress_dialog = gimp_progress_dialog_new ();
          progress = GIMP_PROGRESS (progress_dialog);
        }

      progress = gimp_progress_start (progress, _("Scaling..."), FALSE);

      gimp_item_scale_by_origin (item,
                                 width, height, interpolation,
                                 progress, TRUE);

      if (progress)
        gimp_progress_end (progress);

      if (progress_dialog)
        gtk_widget_destroy (progress_dialog);

      gimp_image_flush (gimp_item_get_image (item));
    }
  else
    {
      g_message (_("Invalid width or height. Both must be positive."));
    }
}

static void
layers_scale_layer_query (GimpDisplay *gdisp,
                          GimpImage   *gimage,
                          GimpLayer   *layer,
                          GtkWidget   *parent)
{
  GtkWidget *dialog;
  GimpUnit   unit;

  unit = gdisp ? GIMP_DISPLAY_SHELL (gdisp->shell)->unit : GIMP_UNIT_PIXEL;

  dialog = scale_dialog_new (GIMP_VIEWABLE (layer),
                             _("Scale Layer"), "gimp-layer-scale",
                             parent,
                             gimp_standard_help_func, GIMP_HELP_LAYER_SCALE,
                             unit, gimage->gimp->config->interpolation_type,
                             scale_layer_callback,
                             gdisp);

  gtk_widget_show (dialog);
}


/*****************************/
/*  The resize layer dialog  */
/*****************************/

typedef struct _ResizeLayerOptions ResizeLayerOptions;

struct _ResizeLayerOptions
{
  GimpLayer    *layer;
  GimpContext  *context;
  ResizeDialog *dialog;
};

static void
resize_layer_query_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  ResizeLayerOptions *options = data;

  if (options->dialog->width > 0 && options->dialog->height > 0)
    {
      GimpImage *gimage = gimp_item_get_image (GIMP_ITEM (options->layer));

      gtk_widget_set_sensitive (options->dialog->shell, FALSE);

      gimp_item_resize (GIMP_ITEM (options->layer),
                        options->context,
                        options->dialog->width,
                        options->dialog->height,
                        options->dialog->offset_x,
                        options->dialog->offset_y);

      gimp_image_flush (gimage);

      gtk_widget_destroy (options->dialog->shell);
    }
  else
    {
      g_message (_("Invalid width or height. Both must be positive."));
    }
}

static void
layers_resize_layer_query (GimpDisplay *gdisp,
                           GimpImage   *gimage,
                           GimpLayer   *layer,
                           GimpContext *context,
                           GtkWidget   *parent)
{
  ResizeLayerOptions *options;

  options = g_new0 (ResizeLayerOptions, 1);

  options->context = context;
  options->layer   = layer;

  options->dialog =
    resize_dialog_new (GIMP_VIEWABLE (layer), parent,
                       RESIZE_DIALOG,
		       gimp_item_width  (GIMP_ITEM (layer)),
		       gimp_item_height (GIMP_ITEM (layer)),
		       gimage->xresolution,
		       gimage->yresolution,
		       (gdisp ?
                        GIMP_DISPLAY_SHELL (gdisp->shell)->unit :
                        GIMP_UNIT_PIXEL),
		       G_CALLBACK (resize_layer_query_ok_callback),
                       options);

  g_object_weak_ref (G_OBJECT (options->dialog->shell),
		     (GWeakNotify) g_free,
		     options);

  gtk_widget_show (options->dialog->shell);
}

static gint
layers_mode_index (GimpLayerModeEffects layer_mode)
{
  gint i = 0;

  while (i < (G_N_ELEMENTS (layer_modes) - 1) && layer_modes[i] != layer_mode)
    i++;

  return i;
}
