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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "siod/siod.h"

#include "script-fu-types.h"

#include "script-fu-interface.h"
#include "script-fu-scripts.h"

#include "script-fu-intl.h"


#define RESPONSE_RESET         1
#define RESPONSE_ABOUT         2

#define TEXT_WIDTH           100
#define COLOR_SAMPLE_WIDTH   100
#define COLOR_SAMPLE_HEIGHT   15
#define SLIDER_WIDTH          80

#define MAX_STRING_LENGTH   4096


typedef struct
{
  GtkWidget     *dialog;

  GtkWidget     *args_table;
  GtkWidget    **args_widgets;

  GtkWidget     *progress_label;
  GtkWidget     *progress;
  const gchar   *progress_callback;

  GtkWidget     *about_dialog;

  gchar         *title;
  gchar         *last_command;
  gint           command_count;
  gint           consec_command_count;
} SFInterface;


/*
 *  Local Functions
 */

static void   script_fu_interface_quit      (SFScript             *script);

static void   script_fu_response            (GtkWidget            *widget,
                                             gint                  response_id,
                                             SFScript             *script);
static void   script_fu_ok                  (SFScript             *script);
static void   script_fu_reset               (SFScript             *script);
static void   script_fu_about               (SFScript             *script);

static void   script_fu_file_entry_callback (GtkWidget            *widget,
                                             SFFilename           *file);
static void   script_fu_pattern_callback    (const gchar          *name,
                                             gint                  width,
                                             gint                  height,
                                             gint                  bytes,
                                             const guchar         *mask_data,
                                             gboolean              closing,
                                             gpointer              data);
static void   script_fu_gradient_callback   (const gchar          *name,
                                             gint                  width,
                                             const gdouble        *mask_data,
                                             gboolean              closing,
                                             gpointer              data);
static void   script_fu_font_callback       (const gchar          *name,
                                             gboolean              closing,
                                             gpointer              data);
static void   script_fu_palette_callback    (const gchar          *name,
                                             gboolean              closing,
                                             gpointer              data);
static void   script_fu_brush_callback      (const gchar          *name,
                                             gdouble               opacity,
                                             gint                  spacing,
                                             GimpLayerModeEffects  paint_mode,
                                             gint                  width,
                                             gint                  height,
                                             const guchar         *mask_data,
                                             gboolean              closing,
                                             gpointer              data);


/*
 *  Local variables
 */

static SFInterface *sf_interface = NULL;  /*  there can only be at most one
					      interactive interface  */


/*
 *  Function definitions
 */

void
script_fu_interface_report_cc (gchar *command)
{
  if (sf_interface == NULL)
    return;

  if (sf_interface->last_command &&
      strcmp (sf_interface->last_command, command) == 0)
    {
      gchar *new_command;

      sf_interface->command_count++;

      new_command = g_strdup_printf ("%s <%d>",
				     command, sf_interface->command_count);
      gtk_label_set_text (GTK_LABEL (sf_interface->progress_label), new_command);
      g_free (new_command);
    }
  else
    {
      sf_interface->command_count = 1;
      gtk_label_set_text (GTK_LABEL (sf_interface->progress_label), command);
      g_free (sf_interface->last_command);
      sf_interface->last_command = g_strdup (command);
    }

  while (gtk_main_iteration ());
}

static void
script_fu_progress_start (const gchar *message,
                          gboolean     cancelable,
                          gpointer     user_data)
{
  SFInterface *sf_interface = user_data;

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sf_interface->progress),
                             message ? message : " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sf_interface->progress), 0.0);

  if (GTK_WIDGET_DRAWABLE (sf_interface->progress))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
script_fu_progress_end (gpointer user_data)
{
  SFInterface *sf_interface = user_data;

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sf_interface->progress), " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sf_interface->progress), 0.0);

  if (GTK_WIDGET_DRAWABLE (sf_interface->progress))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
script_fu_progress_text (const gchar *message,
                         gpointer     user_data)
{
  SFInterface *sf_interface = user_data;

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sf_interface->progress),
                             message ? message : " ");

  if (GTK_WIDGET_DRAWABLE (sf_interface->progress))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
script_fu_progress_value (gdouble  percentage,
                          gpointer user_data)
{
  SFInterface *sf_interface = user_data;

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (sf_interface->progress),
                                 percentage);

  if (GTK_WIDGET_DRAWABLE (sf_interface->progress))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

void
script_fu_interface (SFScript *script)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *menu;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GSList    *list;
  gchar     *title;
  gchar     *buf;
  gint       start_args;
  gint       i;

  static gboolean gtk_initted = FALSE;

  /*  Simply return if there is already an interface. This is an
      ugly workaround for the fact that we can not process two
      scripts at a time.  */
  if (sf_interface != NULL)
    return;

  g_return_if_fail (script != NULL);

  if (!gtk_initted)
    {
      INIT_I18N();

      gimp_ui_init ("script-fu", TRUE);

      gtk_initted = TRUE;
    }

  sf_interface = g_new0 (SFInterface, 1);
  sf_interface->args_widgets = g_new0 (GtkWidget *, script->num_args);

  /* strip the first part of the menupath if it contains _("/Script-Fu/") */
  buf = strstr (gettext (script->description), _("/Script-Fu/"));
  if (buf)
    title = g_strdup (buf + strlen (_("/Script-Fu/")));
  else
    title = g_strdup (gettext (script->description));

  /* strip mnemonics from the menupath */
  sf_interface->title = gimp_strip_uline (title);
  g_free (title);

  buf = strstr (sf_interface->title, "...");
  if (buf)
    *buf = '\0';

  buf = g_strdup_printf (_("Script-Fu: %s"), sf_interface->title);

  sf_interface->dialog = dlg =
    gimp_dialog_new (buf, "script-fu",
                     NULL, 0,
                     gimp_standard_help_func, script->pdb_name,

                     _("_About"),      RESPONSE_ABOUT,
                     GIMP_STOCK_RESET, RESPONSE_RESET,
                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                     NULL);

  g_free (buf);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (script_fu_response),
                    script);

  g_signal_connect_swapped (dlg, "destroy",
                            G_CALLBACK (script_fu_interface_quit),
                            script);

  gtk_window_set_resizable (GTK_WINDOW (dlg), TRUE);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* the script arguments frame */
  frame = gimp_frame_new (_("Script Arguments"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  The argument table  */
  if (script->image_based)
    sf_interface->args_table = gtk_table_new (script->num_args - 1, 3, FALSE);
  else
    sf_interface->args_table = gtk_table_new (script->num_args + 1, 3, FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (sf_interface->args_table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (sf_interface->args_table), 6);
  gtk_container_add (GTK_CONTAINER (frame), sf_interface->args_table);
  gtk_widget_show (sf_interface->args_table);

  start_args = (script->image_based) ? 2 : 0;

  for (i = start_args; i < script->num_args; i++)
    {
      GtkWidget *widget       = NULL;
      gchar     *label_text;
      gfloat     label_yalign = 0.5;
      gboolean   leftalign    = FALSE;
      gint      *ID_ptr       = NULL;
      gint       row          = i;

      if (script->image_based)
        row -= 2;

      /*  we add a colon after the label;
          some languages want an extra space here  */
      label_text =  g_strdup_printf (_("%s:"),
                                     gettext (script->arg_labels[i]));

      switch (script->arg_types[i])
	{
	case SF_IMAGE:
	case SF_DRAWABLE:
	case SF_LAYER:
	case SF_CHANNEL:
	  switch (script->arg_types[i])
	    {
	    case SF_IMAGE:
              widget = gimp_image_combo_box_new (NULL, NULL);
              ID_ptr = &script->arg_values[i].sfa_image;
	      break;

	    case SF_DRAWABLE:
              widget = gimp_drawable_combo_box_new (NULL, NULL);
              ID_ptr = &script->arg_values[i].sfa_drawable;
              break;

	    case SF_LAYER:
              widget = gimp_layer_combo_box_new (NULL, NULL);
              ID_ptr = &script->arg_values[i].sfa_layer;
	      break;

	    case SF_CHANNEL:
              widget = gimp_channel_combo_box_new (NULL, NULL);
              ID_ptr = &script->arg_values[i].sfa_channel;
	      break;

	    default:
	      menu = NULL;
	      break;
	    }

          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget), *ID_ptr);
          g_signal_connect (widget, "changed",
                            G_CALLBACK (gimp_int_combo_box_get_active),
                            ID_ptr);
	  break;

	case SF_COLOR:
	  widget = gimp_color_button_new (_("Script-Fu Color Selection"),
                                          COLOR_SAMPLE_WIDTH,
                                          COLOR_SAMPLE_HEIGHT,
                                          &script->arg_values[i].sfa_color,
                                          GIMP_COLOR_AREA_FLAT);

          gimp_color_button_set_update (GIMP_COLOR_BUTTON (widget), TRUE);
	  g_signal_connect (widget, "color_changed",
			    G_CALLBACK (gimp_color_button_get_color),
			    &script->arg_values[i].sfa_color);
	  break;

	case SF_TOGGLE:
	  g_free (label_text);
	  label_text = NULL;
	  widget = gtk_check_button_new_with_mnemonic (gettext (script->arg_labels[i]));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				       script->arg_values[i].sfa_toggle);

	  g_signal_connect (widget, "toggled",
			    G_CALLBACK (gimp_toggle_button_update),
			    &script->arg_values[i].sfa_toggle);
	  break;

	case SF_VALUE:
	case SF_STRING:
	  widget = gtk_entry_new ();
	  gtk_widget_set_size_request (widget, TEXT_WIDTH, -1);
	  gtk_entry_set_text (GTK_ENTRY (widget),
                              script->arg_values[i].sfa_value);
	  break;

	case SF_ADJUSTMENT:
	  switch (script->arg_defaults[i].sfa_adjustment.type)
	    {
	    case SF_SLIDER:
	      script->arg_values[i].sfa_adjustment.adj = (GtkAdjustment *)
		gimp_scale_entry_new (GTK_TABLE (sf_interface->args_table),
                                      0, row,
				      label_text, SLIDER_WIDTH, -1,
				      script->arg_values[i].sfa_adjustment.value,
				      script->arg_defaults[i].sfa_adjustment.lower,
				      script->arg_defaults[i].sfa_adjustment.upper,
				      script->arg_defaults[i].sfa_adjustment.step,
				      script->arg_defaults[i].sfa_adjustment.page,
				      script->arg_defaults[i].sfa_adjustment.digits,
				      TRUE, 0.0, 0.0,
				      NULL, NULL);
	      break;

	    case SF_SPINNER:
              leftalign = TRUE;
	      script->arg_values[i].sfa_adjustment.adj = (GtkAdjustment *)
		gtk_adjustment_new (script->arg_values[i].sfa_adjustment.value,
				    script->arg_defaults[i].sfa_adjustment.lower,
				    script->arg_defaults[i].sfa_adjustment.upper,
				    script->arg_defaults[i].sfa_adjustment.step,
				    script->arg_defaults[i].sfa_adjustment.page, 0);
              widget = gtk_spin_button_new (script->arg_values[i].sfa_adjustment.adj,
                                            0,
                                            script->arg_defaults[i].sfa_adjustment.digits);
	      break;

	    default:
	      break;
	    }
	  break;

	case SF_FILENAME:
	case SF_DIRNAME:
          if (script->arg_types[i] == SF_FILENAME)
            widget = gimp_file_entry_new (_("Script-Fu File Selection"),
                                          script->arg_values[i].sfa_file.filename,
                                          FALSE, TRUE);
          else
            widget = gimp_file_entry_new (_("Script-Fu Folder Selection"),
                                          script->arg_values[i].sfa_file.filename,
                                          TRUE, TRUE);

	  script->arg_values[i].sfa_file.file_entry = widget;

	  g_signal_connect (widget, "filename_changed",
			    G_CALLBACK (script_fu_file_entry_callback),
			    &script->arg_values[i].sfa_file);
	  break;

	case SF_FONT:
	  widget = gimp_font_select_widget_new (_("Script-Fu Font Selection"),
                                                script->arg_values[i].sfa_font,
                                                script_fu_font_callback,
                                                &script->arg_values[i].sfa_font);
	  break;

	case SF_PALETTE:
	  widget = gimp_palette_select_widget_new (_("Script-Fu Palette Selection"),
                                                   script->arg_values[i].sfa_palette,
                                                   script_fu_palette_callback,
                                                   &script->arg_values[i].sfa_palette);
	  break;

	case SF_PATTERN:
          leftalign = TRUE;
	  widget = gimp_pattern_select_widget_new (_("Script-fu Pattern Selection"),
                                                   script->arg_values[i].sfa_pattern,
                                                   script_fu_pattern_callback,
                                                   &script->arg_values[i].sfa_pattern);
	  break;
	case SF_GRADIENT:
          leftalign = TRUE;
	  widget = gimp_gradient_select_widget_new (_("Script-Fu Gradient Selection"),
                                                    script->arg_values[i].sfa_gradient,
                                                    script_fu_gradient_callback,
                                                    &script->arg_values[i].sfa_gradient);
	  break;

	case SF_BRUSH:
          leftalign = TRUE;
	  widget = gimp_brush_select_widget_new (_("Script-Fu Brush Selection"),
                                                 script->arg_values[i].sfa_brush.name,
                                                 script->arg_values[i].sfa_brush.opacity,
                                                 script->arg_values[i].sfa_brush.spacing,
                                                 script->arg_values[i].sfa_brush.paint_mode,
                                                 script_fu_brush_callback,
                                                 &script->arg_values[i].sfa_brush);
	  break;

	case SF_OPTION:
	  widget = gtk_combo_box_new_text ();
	  for (list = script->arg_defaults[i].sfa_option.list;
	       list;
	       list = g_slist_next (list))
	    {
              gtk_combo_box_append_text (GTK_COMBO_BOX (widget),
                                         gettext ((const gchar *) list->data));
	    }

          gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
                                    script->arg_values[i].sfa_option.history);
	  break;

	default:
	  break;
	}

      if (widget)
        {
          if (label_text)
            {
              gimp_table_attach_aligned (GTK_TABLE (sf_interface->args_table),
                                         0, row,
                                         label_text, 0.0, label_yalign,
                                         widget, 2, leftalign);
              g_free (label_text);
            }
          else
            {
              gtk_table_attach (GTK_TABLE (sf_interface->args_table),
                                widget, 0, 3, row, row + 1,
                                GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
              gtk_widget_show (widget);
            }
        }

      sf_interface->args_widgets[i] = widget;
    }

  /* the script progress frame */
  frame = gimp_frame_new (_("Script Progress"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  sf_interface->progress_label = gtk_label_new (_("(none)"));
  gtk_misc_set_alignment (GTK_MISC (sf_interface->progress_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox2), sf_interface->progress_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (sf_interface->progress_label);

  sf_interface->progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (sf_interface->progress), " ");
  gtk_box_pack_start (GTK_BOX (vbox2), sf_interface->progress, FALSE, FALSE, 0);
  gtk_widget_show (sf_interface->progress);

  sf_interface->progress_callback =
    gimp_progress_install (script_fu_progress_start,
                           script_fu_progress_end,
                           script_fu_progress_text,
                           script_fu_progress_value,
                           sf_interface);

  gtk_widget_show (dlg);

  gtk_main ();
}

static void
script_fu_interface_quit (SFScript *script)
{
  gint i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (sf_interface != NULL);

  g_free (sf_interface->title);

  if (sf_interface->about_dialog)
    gtk_widget_destroy (sf_interface->about_dialog);

  for (i = 0; i < script->num_args; i++)
    switch (script->arg_types[i])
      {
      case SF_FONT:
  	gimp_font_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_PALETTE:
  	gimp_palette_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_PATTERN:
  	gimp_pattern_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_GRADIENT:
  	gimp_gradient_select_widget_close (sf_interface->args_widgets[i]);
	break;

      case SF_BRUSH:
  	gimp_brush_select_widget_close (sf_interface->args_widgets[i]);
	break;

      default:
	break;
      }

  g_free (sf_interface->args_widgets);
  g_free (sf_interface->last_command);

  g_free (sf_interface);
  sf_interface = NULL;

  /*
   *  We do not call gtk_main_quit() earlier to reduce the possibility
   *  that script_fu_script_proc() is called from gimp_extension_process()
   *  while we are not finished with the current script. This sucks!
   */

  gtk_main_quit ();
}

static void
script_fu_file_entry_callback (GtkWidget  *widget,
                               SFFilename *file)
{
  if (file->filename)
    g_free (file->filename);

  file->filename =
    gimp_file_entry_get_filename (GIMP_FILE_ENTRY(file->file_entry));
}

static void
script_fu_pattern_callback (const gchar  *name,
                            gint          width,
                            gint          height,
                            gint          bytes,
                            const guchar *mask_data,
                            gboolean      closing,
                            gpointer      data)
{
  gchar **pname = data;

  g_free (*pname);
  *pname = g_strdup (name);
}

static void
script_fu_gradient_callback (const gchar   *name,
                             gint           width,
                             const gdouble *mask_data,
                             gboolean       closing,
                             gpointer       data)
{
  gchar **gname = data;

  g_free (*gname);
  *gname = g_strdup (name);
}

static void
script_fu_font_callback (const gchar *name,
                         gboolean     closing,
                         gpointer     data)
{
  gchar **fname = data;

  g_free (*fname);
  *fname = g_strdup (name);
}

static void
script_fu_palette_callback (const gchar *name,
                            gboolean     closing,
                            gpointer     data)
{
  gchar **fname = data;

  g_free (*fname);
  *fname = g_strdup (name);
}

static void
script_fu_brush_callback (const gchar          *name,
                          gdouble               opacity,
                          gint                  spacing,
                          GimpLayerModeEffects  paint_mode,
                          gint                  width,
                          gint                  height,
                          const guchar         *mask_data,
                          gboolean              closing,
                          gpointer              data)
{
  SFBrush *brush = data;

  g_free (brush->name);

  brush->name       = g_strdup (name);
  brush->opacity    = opacity;
  brush->spacing    = spacing;
  brush->paint_mode = paint_mode;
}

static void
script_fu_response (GtkWidget *widget,
                    gint       response_id,
                    SFScript  *script)
{
  switch (response_id)
    {
    case RESPONSE_ABOUT:
      script_fu_about (script);
      break;

    case RESPONSE_RESET:
      script_fu_reset (script);
      break;

    case GTK_RESPONSE_OK:
      gtk_widget_set_sensitive (sf_interface->args_table, FALSE);
      gtk_widget_set_sensitive (GTK_DIALOG (sf_interface->dialog)->action_area,
                                FALSE);

      script_fu_ok (script);
      /* fallthru */

    default:
      gimp_progress_uninstall (sf_interface->progress_callback);
      gtk_widget_destroy (sf_interface->dialog);
      break;
    }
}

static void
script_fu_ok (SFScript *script)
{
  gchar  *escaped;
  gchar  *text = NULL;
  gchar  *command;
  gchar  *c;
  guchar  r, g, b;
  gchar   buffer[MAX_STRING_LENGTH];
  gint    length;
  gint    i;

  length = strlen (script->script_name) + 3;

  for (i = 0; i < script->num_args; i++)
    {
      GtkWidget *widget = sf_interface->args_widgets[i];

      switch (script->arg_types[i])
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
          length += 12;  /*  Maximum size of integer value  */
          break;

        case SF_COLOR:
          length += 16;  /*  Maximum size of color string: '(XXX XXX XXX)  */
          break;

        case SF_TOGGLE:
          length += 6;   /*  Maximum size of (TRUE, FALSE)  */
          break;

        case SF_VALUE:
          length += strlen (gtk_entry_get_text (GTK_ENTRY (widget))) + 1;
          break;

        case SF_STRING:
          escaped = g_strescape (gtk_entry_get_text (GTK_ENTRY (widget)),
                                 NULL);
          length += strlen (escaped) + 3;
          g_free (escaped);
          break;

        case SF_ADJUSTMENT:
          length += G_ASCII_DTOSTR_BUF_SIZE;
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          escaped = g_strescape (script->arg_values[i].sfa_file.filename,
                                 NULL);
          length += strlen (escaped) + 3;
          g_free (escaped);
          break;

        case SF_FONT:
          length += strlen (script->arg_values[i].sfa_font) + 3;
          break;

        case SF_PALETTE:
          length += strlen (script->arg_values[i].sfa_palette) + 3;
          break;

        case SF_PATTERN:
          length += strlen (script->arg_values[i].sfa_pattern) + 3;
          break;

        case SF_GRADIENT:
          length += strlen (script->arg_values[i].sfa_gradient) + 3;
          break;

        case SF_BRUSH:
          length += strlen (script->arg_values[i].sfa_brush.name) + 3;
          length += 36;  /*  Maximum size of three ints  */
          /*  for opacity, spacing, mode  */
          break;

        case SF_OPTION:
          length += 12;  /*  Maximum size of integer value  */
          break;

        default:
          break;
        }
    }

  c = command = g_new (gchar, length);

  sprintf (command, "(%s ", script->script_name);
  c += strlen (script->script_name) + 2;
  for (i = 0; i < script->num_args; i++)
    {
      GtkWidget *widget = sf_interface->args_widgets[i];

      switch (script->arg_types[i])
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
          g_snprintf (buffer, sizeof (buffer), "%d",
                      script->arg_values[i].sfa_image);
          text = buffer;
          break;

        case SF_COLOR:
          gimp_rgb_get_uchar (&script->arg_values[i].sfa_color, &r, &g, &b);
          g_snprintf (buffer, sizeof (buffer), "'(%d %d %d)",
                      (gint) r, (gint) g, (gint) b);
          text = buffer;
          break;

        case SF_TOGGLE:
          g_snprintf (buffer, sizeof (buffer), "%s",
                      (script->arg_values[i].sfa_toggle) ? "TRUE" : "FALSE");
          text = buffer;
          break;

        case SF_VALUE:
          text = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
          g_free (script->arg_values[i].sfa_value);
          script->arg_values[i].sfa_value = g_strdup (text);
          break;

        case SF_STRING:
          text = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
          g_free (script->arg_values[i].sfa_value);
          script->arg_values[i].sfa_value = g_strdup (text);
          escaped = g_strescape (text, NULL);
          g_snprintf (buffer, sizeof (buffer), "\"%s\"", escaped);
          g_free (escaped);
          text = buffer;
          break;

        case SF_ADJUSTMENT:
          switch (script->arg_defaults[i].sfa_adjustment.type)
            {
            case SF_SLIDER:
              script->arg_values[i].sfa_adjustment.value =
                script->arg_values[i].sfa_adjustment.adj->value;
              break;

            case SF_SPINNER:
              script->arg_values[i].sfa_adjustment.value =
                gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
              break;

            default:
              break;
            }
          text = g_ascii_dtostr (buffer, sizeof (buffer),
                                 script->arg_values[i].sfa_adjustment.value);
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          escaped = g_strescape (script->arg_values[i].sfa_file.filename,
                                 NULL);
          g_snprintf (buffer, sizeof (buffer), "\"%s\"", escaped);
          g_free (escaped);
          text = buffer;
          break;

        case SF_FONT:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_font);
          text = buffer;
          break;

        case SF_PALETTE:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_palette);
          text = buffer;
          break;

        case SF_PATTERN:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_pattern);
          text = buffer;
          break;

        case SF_GRADIENT:
          g_snprintf (buffer, sizeof (buffer), "\"%s\"",
                      script->arg_values[i].sfa_gradient);
          text = buffer;
          break;

        case SF_BRUSH:
          {
            gchar opacity[G_ASCII_DTOSTR_BUF_SIZE];

            g_ascii_dtostr (opacity, sizeof (opacity),
                            script->arg_values[i].sfa_brush.opacity);

            g_snprintf (buffer, sizeof (buffer), "'(\"%s\" %s %d %d)",
                        script->arg_values[i].sfa_brush.name,
                        opacity,
                        script->arg_values[i].sfa_brush.spacing,
                        script->arg_values[i].sfa_brush.paint_mode);
            text = buffer;
          }
          break;

        case SF_OPTION:
          script->arg_values[i].sfa_option.history =
            gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

          g_snprintf (buffer, sizeof (buffer), "%d",
                      script->arg_values[i].sfa_option.history);
          text = buffer;
          break;

        default:
          break;
        }

      if (i == script->num_args - 1)
        sprintf (c, "%s)", text);
      else
        sprintf (c, "%s ", text);
      c += strlen (text) + 1;
    }

  /*  run the command through the interpreter  */
  if (repl_c_string (command, 0, 0, 1) != 0)
    script_fu_error_msg (command);

  g_free (command);
}

static void
script_fu_reset (SFScript *script)
{
  gint i;

  for (i = 0; i < script->num_args; i++)
    {
      GtkWidget *widget = sf_interface->args_widgets[i];

      switch (script->arg_types[i])
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
          break;

        case SF_COLOR:
          script->arg_values[i].sfa_color = script->arg_defaults[i].sfa_color;
          gimp_color_button_set_color (GIMP_COLOR_BUTTON (widget),
                                       &script->arg_values[i].sfa_color);
	break;

        case SF_TOGGLE:
          script->arg_values[i].sfa_toggle = script->arg_defaults[i].sfa_toggle;
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        script->arg_values[i].sfa_toggle);
	break;

        case SF_VALUE:
        case SF_STRING:
          g_free (script->arg_values[i].sfa_value);
          script->arg_values[i].sfa_value =
            g_strdup (script->arg_defaults[i].sfa_value);
          gtk_entry_set_text (GTK_ENTRY (widget),
                              script->arg_values[i].sfa_value);
          break;

        case SF_ADJUSTMENT:
          script->arg_values[i].sfa_adjustment.value =
            script->arg_defaults[i].sfa_adjustment.value;
          gtk_adjustment_set_value (script->arg_values[i].sfa_adjustment.adj,
                                    script->arg_values[i].sfa_adjustment.value);
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          g_free (script->arg_values[i].sfa_file.filename);
          script->arg_values[i].sfa_file.filename =
            g_strdup (script->arg_defaults[i].sfa_file.filename);
          gimp_file_entry_set_filename
            (GIMP_FILE_ENTRY (script->arg_values[i].sfa_file.file_entry),
             script->arg_values[i].sfa_file.filename);
          break;

        case SF_FONT:
          gimp_font_select_widget_set (widget,
                                       script->arg_defaults[i].sfa_font);
          break;

        case SF_PALETTE:
          gimp_palette_select_widget_set (widget,
                                          script->arg_defaults[i].sfa_palette);
          break;

        case SF_PATTERN:
          gimp_pattern_select_widget_set (widget,
                                          script->arg_defaults[i].sfa_pattern);
          break;

        case SF_GRADIENT:
          gimp_gradient_select_widget_set (widget,
                                           script->arg_defaults[i].sfa_gradient);
          break;

        case SF_BRUSH:
          gimp_brush_select_widget_set (widget,
                                        script->arg_defaults[i].sfa_brush.name,
                                        script->arg_defaults[i].sfa_brush.opacity,
                                        script->arg_defaults[i].sfa_brush.spacing,
                                        script->arg_defaults[i].sfa_brush.paint_mode);
          break;

        case SF_OPTION:
          script->arg_values[i].sfa_option.history =
            script->arg_defaults[i].sfa_option.history;
          gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
                                    script->arg_values[i].sfa_option.history);
          break;

        default:
          break;
        }
    }
}

static void
script_fu_about (SFScript *script)
{
  GtkWidget     *dialog = sf_interface->about_dialog;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *label;
  GtkWidget     *scrolled_window;
  GtkWidget     *table;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  gchar         *text;

  if (! dialog)
    {
      sf_interface->about_dialog = dialog =
        gimp_dialog_new (sf_interface->title, "script-fu-about",
                         sf_interface->dialog, 0,
                         gimp_standard_help_func, script->pdb_name,

                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                         NULL);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      g_signal_connect (dialog, "destroy",
			G_CALLBACK (gtk_widget_destroyed),
			&sf_interface->about_dialog);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
			  TRUE, TRUE, 0);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      /* the name */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      label = gtk_label_new (script->script_name);
      gtk_misc_set_padding (GTK_MISC (label), 2, 2);
      gtk_container_add (GTK_CONTAINER (frame), label);
      gtk_widget_show (label);

      /* the help display */
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
      gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_widget_show (scrolled_window);

      text_buffer = gtk_text_buffer_new (NULL);
      text_view = gtk_text_view_new_with_buffer (text_buffer);
      g_object_unref (text_buffer);

      gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
      gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 12);
      gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 12);
      gtk_widget_set_size_request (text_view, 240, 120);
      gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
      gtk_widget_show (text_view);

      text = g_strconcat ("\n", script->help, "\n", NULL);
      gtk_text_buffer_set_text (text_buffer, text, -1);
      g_free (text);

      /* author, copyright, etc. */
      table = gtk_table_new (2, 4, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_table_set_row_spacings (GTK_TABLE (table), 6);
      gtk_container_set_border_width (GTK_CONTAINER (table), 12);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      label = gtk_label_new (script->author);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Author:"), 0.0, 0.5,
				 label, 1, FALSE);

      label = gtk_label_new (script->copyright);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Copyright:"), 0.0, 0.5,
				 label, 1, FALSE);

      label = gtk_label_new (script->date);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
				 _("Date:"), 0.0, 0.5,
				 label, 1, FALSE);

      if (strlen (script->img_types) > 0)
	{
	  label = gtk_label_new (script->img_types);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
				     _("Image Types:"), 0.0, 0.5,
				     label, 1, FALSE);
	}
    }

  gtk_window_present (GTK_WINDOW (dialog));

  /*  move focus from the text view to the Close button  */
  gtk_widget_child_focus (dialog, GTK_DIR_TAB_FORWARD);
}
