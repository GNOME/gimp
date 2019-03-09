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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#ifdef GDK_WINDOWING_QUARTZ
#import <Cocoa/Cocoa.h>
#elif defined (G_OS_WIN32)
#include <windows.h>
#endif

#include "tinyscheme/scheme-private.h"
#include "scheme-wrapper.h"

#include "script-fu-types.h"

#include "script-fu-interface.h"
#include "script-fu-scripts.h"
#include "script-fu-script.h"

#include "script-fu-intl.h"


#define RESPONSE_RESET         1

#define TEXT_WIDTH           100
#define COLOR_SAMPLE_WIDTH    60
#define COLOR_SAMPLE_HEIGHT   15
#define SLIDER_WIDTH          80


typedef struct
{
  GtkWidget  *dialog;

  GtkWidget  *table;
  GtkWidget **widgets;

  GtkWidget  *progress_label;
  GtkWidget  *progress_bar;

  gchar      *title;
  gchar      *last_command;
  gint        command_count;
  gint        consec_command_count;
} SFInterface;


/*
 *  Local Functions
 */

static void   script_fu_interface_quit      (SFScript      *script);

static void   script_fu_response            (GtkWidget     *widget,
                                             gint           response_id,
                                             SFScript      *script);
static void   script_fu_ok                  (SFScript      *script);
static void   script_fu_reset               (SFScript      *script);

static void   script_fu_file_callback       (GtkWidget     *widget,
                                             SFFilename    *file);
static void   script_fu_combo_callback      (GtkWidget     *widget,
                                             SFOption      *option);
static void   script_fu_pattern_callback    (gpointer       data,
                                             const gchar   *name,
                                             gint           width,
                                             gint           height,
                                             gint           bytes,
                                             const guchar  *mask_data,
                                             gboolean       closing);
static void   script_fu_gradient_callback   (gpointer       data,
                                             const gchar   *name,
                                             gint           width,
                                             const gdouble *mask_data,
                                             gboolean       closing);
static void   script_fu_font_callback       (gpointer       data,
                                             const gchar   *name,
                                             gboolean       closing);
static void   script_fu_palette_callback    (gpointer       data,
                                             const gchar   *name,
                                             gboolean       closing);
static void   script_fu_brush_callback      (gpointer       data,
                                             const gchar   *name,
                                             gdouble        opacity,
                                             gint           spacing,
                                             GimpLayerMode  paint_mode,
                                             gint           width,
                                             gint           height,
                                             const guchar  *mask_data,
                                             gboolean       closing);


/*
 *  Local variables
 */

static SFInterface       *sf_interface = NULL;  /*  there can only be at most
                                                 *  one interactive interface
                                                 */

static GimpPDBStatusType  sf_status    = GIMP_PDB_SUCCESS;


/*
 *  Function definitions
 */

gboolean
script_fu_interface_is_active (void)
{
  return (sf_interface != NULL);
}

void
script_fu_interface_report_cc (const gchar *command)
{
  if (sf_interface == NULL)
    return;

  if (sf_interface->last_command &&
      strcmp (sf_interface->last_command, command) == 0)
    {
      sf_interface->command_count++;

      if (! g_str_has_prefix (command, "gimp-progress-"))
        {
          gchar *new_command;

          new_command = g_strdup_printf ("%s <%d>",
                                         command, sf_interface->command_count);
          gtk_label_set_text (GTK_LABEL (sf_interface->progress_label),
                              new_command);
          g_free (new_command);
        }
    }
  else
    {
      sf_interface->command_count = 1;

      g_free (sf_interface->last_command);
      sf_interface->last_command = g_strdup (command);

      if (! g_str_has_prefix (command, "gimp-progress-"))
        {
          gtk_label_set_text (GTK_LABEL (sf_interface->progress_label),
                              command);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (sf_interface->progress_label), "");
        }
    }

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

GimpPDBStatusType
script_fu_interface (SFScript  *script,
                     gint       start_arg)
{
  GtkWidget    *dialog;
  GtkWidget    *vbox;
  GtkWidget    *vbox2;
  GtkSizeGroup *group;
  GSList       *list;
  gchar        *title;
  gint          i;

  static gboolean gtk_initted = FALSE;

  /* Simply return if there is already an interface. This is an
   * ugly workaround for the fact that we can not process two
   * scripts at a time.
   */
  if (sf_interface != NULL)
    {
      gchar *message =
        g_strdup_printf ("%s\n\n%s",
                         _("Script-Fu cannot process two scripts "
                           "at the same time."),
                         _("You are already running the \"%s\" script."));

      g_message (message, sf_interface->title);
      g_free (message);

      return GIMP_PDB_CANCEL;
    }

  g_return_val_if_fail (script != NULL, FALSE);

  if (!gtk_initted)
    {
      INIT_I18N();

      gimp_ui_init ("script-fu", TRUE);

      gtk_initted = TRUE;
    }

  sf_status = GIMP_PDB_SUCCESS;

  sf_interface = g_slice_new0 (SFInterface);

  sf_interface->widgets = g_new0 (GtkWidget *, script->n_args);
  sf_interface->title   = script_fu_script_get_title (script);

  title = g_strdup_printf (_("Script-Fu: %s"), sf_interface->title);

  sf_interface->dialog = dialog =
    gimp_dialog_new (title, "gimp-script-fu",
                     NULL, 0,
                     gimp_standard_help_func, script->name,

                     _("_Reset"),  RESPONSE_RESET,
                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                     _("_OK"),     GTK_RESPONSE_OK,

                     NULL);
  g_free (title);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (script_fu_response),
                    script);

  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (script_fu_interface_quit),
                            script);

  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  The argument table  */
  sf_interface->table = gtk_table_new (script->n_args - start_arg, 3, FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (sf_interface->table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (sf_interface->table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), sf_interface->table, FALSE, FALSE, 0);
  gtk_widget_show (sf_interface->table);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (i = start_arg; i < script->n_args; i++)
    {
      GtkWidget *widget       = NULL;
      gchar     *label_text;
      gfloat     label_yalign = 0.5;
      gint      *ID_ptr       = NULL;
      gint       row          = i;
      gboolean   left_align   = FALSE;
      SFArg     *arg          = &script->args[i];

      row -= start_arg;

      /*  we add a colon after the label;
       *  some languages want an extra space here
       */
      label_text = g_strdup_printf (_("%s:"), arg->label);

      switch (arg->type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
          switch (arg->type)
            {
            case SF_IMAGE:
              widget = gimp_image_combo_box_new (NULL, NULL);
              ID_ptr = &arg->value.sfa_image;
              break;

            case SF_DRAWABLE:
              widget = gimp_drawable_combo_box_new (NULL, NULL);
              ID_ptr = &arg->value.sfa_drawable;
              break;

            case SF_LAYER:
              widget = gimp_layer_combo_box_new (NULL, NULL);
              ID_ptr = &arg->value.sfa_layer;
              break;

            case SF_CHANNEL:
              widget = gimp_channel_combo_box_new (NULL, NULL);
              ID_ptr = &arg->value.sfa_channel;
              break;

            case SF_VECTORS:
              widget = gimp_vectors_combo_box_new (NULL, NULL);
              ID_ptr = &arg->value.sfa_vectors;
              break;

            default:
              break;
            }

          gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (widget), *ID_ptr,
                                      G_CALLBACK (gimp_int_combo_box_get_active),
                                      ID_ptr);
          break;

        case SF_COLOR:
          {
            GimpColorConfig *config;

            left_align = TRUE;
            widget = gimp_color_button_new (_("Script-Fu Color Selection"),
                                            COLOR_SAMPLE_WIDTH,
                                            COLOR_SAMPLE_HEIGHT,
                                            &arg->value.sfa_color,
                                            GIMP_COLOR_AREA_FLAT);

            gimp_color_button_set_update (GIMP_COLOR_BUTTON (widget), TRUE);

            config = gimp_get_color_configuration ();
            gimp_color_button_set_color_config (GIMP_COLOR_BUTTON (widget),
                                                config);
            g_object_unref (config);

            g_signal_connect (widget, "color-changed",
                              G_CALLBACK (gimp_color_button_get_color),
                              &arg->value.sfa_color);
          }
          break;

        case SF_TOGGLE:
          g_free (label_text);
          label_text = NULL;
          widget = gtk_check_button_new_with_mnemonic (arg->label);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        arg->value.sfa_toggle);

          g_signal_connect (widget, "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &arg->value.sfa_toggle);
          break;

        case SF_VALUE:
        case SF_STRING:
          widget = gtk_entry_new ();
          gtk_widget_set_size_request (widget, TEXT_WIDTH, -1);
          gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);

          gtk_entry_set_text (GTK_ENTRY (widget), arg->value.sfa_value);
          break;

        case SF_TEXT:
          {
            GtkWidget     *view;
            GtkTextBuffer *buffer;

            widget = gtk_scrolled_window_new (NULL, NULL);
            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
                                                 GTK_SHADOW_IN);
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
                                            GTK_POLICY_AUTOMATIC,
                                            GTK_POLICY_AUTOMATIC);
            gtk_widget_set_size_request (widget, TEXT_WIDTH, -1);

            view = gtk_text_view_new ();
            gtk_container_add (GTK_CONTAINER (widget), view);
            gtk_widget_show (view);

            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
            gtk_text_view_set_editable (GTK_TEXT_VIEW (view), TRUE);

            gtk_text_buffer_set_text (buffer, arg->value.sfa_value, -1);

            label_yalign = 0.0;
          }
          break;

        case SF_ADJUSTMENT:
          switch (arg->default_value.sfa_adjustment.type)
            {
            case SF_SLIDER:
              arg->value.sfa_adjustment.adj = (GtkAdjustment *)
                gimp_scale_entry_new (GTK_TABLE (sf_interface->table),
                                      0, row,
                                      label_text, SLIDER_WIDTH, -1,
                                      arg->value.sfa_adjustment.value,
                                      arg->default_value.sfa_adjustment.lower,
                                      arg->default_value.sfa_adjustment.upper,
                                      arg->default_value.sfa_adjustment.step,
                                      arg->default_value.sfa_adjustment.page,
                                      arg->default_value.sfa_adjustment.digits,
                                      TRUE, 0.0, 0.0,
                                      NULL, NULL);
              gtk_entry_set_activates_default (GIMP_SCALE_ENTRY_SPINBUTTON (arg->value.sfa_adjustment.adj), TRUE);
              break;

            default:
              g_warning ("unexpected adjustment type: %d",
                         arg->default_value.sfa_adjustment.type);
              /* fallthrough */

            case SF_SPINNER:
              left_align = TRUE;
              arg->value.sfa_adjustment.adj = (GtkAdjustment *)
                gtk_adjustment_new (arg->value.sfa_adjustment.value,
                                    arg->default_value.sfa_adjustment.lower,
                                    arg->default_value.sfa_adjustment.upper,
                                    arg->default_value.sfa_adjustment.step,
                                    arg->default_value.sfa_adjustment.page,
                                    0);
              widget = gimp_spin_button_new (arg->value.sfa_adjustment.adj,
                                             arg->default_value.sfa_adjustment.step,
                                             arg->default_value.sfa_adjustment.digits);
              gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), TRUE);
              gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
              break;
            }

          g_signal_connect (arg->value.sfa_adjustment.adj,
                            "value-changed",
                            G_CALLBACK (gimp_double_adjustment_update),
                            &arg->value.sfa_adjustment.value);
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          if (arg->type == SF_FILENAME)
            widget = gtk_file_chooser_button_new (_("Script-Fu File Selection"),
                                                  GTK_FILE_CHOOSER_ACTION_OPEN);
          else
            widget = gtk_file_chooser_button_new (_("Script-Fu Folder Selection"),
                                                  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

          if (arg->value.sfa_file.filename)
            gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget),
                                           arg->value.sfa_file.filename);

          g_signal_connect (widget, "selection-changed",
                            G_CALLBACK (script_fu_file_callback),
                            &arg->value.sfa_file);
          break;

        case SF_FONT:
          widget = gimp_font_select_button_new (_("Script-Fu Font Selection"),
                                                arg->value.sfa_font);
          g_signal_connect_swapped (widget, "font-set",
                                    G_CALLBACK (script_fu_font_callback),
                                    &arg->value.sfa_font);
          break;

        case SF_PALETTE:
          widget = gimp_palette_select_button_new (_("Script-Fu Palette Selection"),
                                                   arg->value.sfa_palette);
          g_signal_connect_swapped (widget, "palette-set",
                                    G_CALLBACK (script_fu_palette_callback),
                                    &arg->value.sfa_palette);
          break;

        case SF_PATTERN:
          left_align = TRUE;
          widget = gimp_pattern_select_button_new (_("Script-Fu Pattern Selection"),
                                                   arg->value.sfa_pattern);
          g_signal_connect_swapped (widget, "pattern-set",
                                    G_CALLBACK (script_fu_pattern_callback),
                                    &arg->value.sfa_pattern);
          break;

        case SF_GRADIENT:
          left_align = TRUE;
          widget = gimp_gradient_select_button_new (_("Script-Fu Gradient Selection"),
                                                    arg->value.sfa_gradient);
          g_signal_connect_swapped (widget, "gradient-set",
                                    G_CALLBACK (script_fu_gradient_callback),
                                    &arg->value.sfa_gradient);
          break;

        case SF_BRUSH:
          left_align = TRUE;
          widget = gimp_brush_select_button_new (_("Script-Fu Brush Selection"),
                                                 arg->value.sfa_brush.name,
                                                 arg->value.sfa_brush.opacity,
                                                 arg->value.sfa_brush.spacing,
                                                 arg->value.sfa_brush.paint_mode);
          g_signal_connect_swapped (widget, "brush-set",
                                    G_CALLBACK (script_fu_brush_callback),
                                    &arg->value.sfa_brush);
          break;

        case SF_OPTION:
          widget = gtk_combo_box_text_new ();
          for (list = arg->default_value.sfa_option.list;
               list;
               list = g_slist_next (list))
            {
              gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
                                              list->data);
            }

          gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
                                    arg->value.sfa_option.history);

          g_signal_connect (widget, "changed",
                            G_CALLBACK (script_fu_combo_callback),
                            &arg->value.sfa_option);
          break;

        case SF_ENUM:
          widget = gimp_enum_combo_box_new (g_type_from_name (arg->default_value.sfa_enum.type_name));

          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget),
                                         arg->value.sfa_enum.history);

          g_signal_connect (widget, "changed",
                            G_CALLBACK (gimp_int_combo_box_get_active),
                            &arg->value.sfa_enum.history);
          break;

        case SF_DISPLAY:
          break;
        }

      if (widget)
        {
          if (label_text)
            {
              gimp_table_attach_aligned (GTK_TABLE (sf_interface->table),
                                         0, row,
                                         label_text, 0.0, label_yalign,
                                         widget, 2, left_align);
              g_free (label_text);
            }
          else
            {
              gtk_table_attach (GTK_TABLE (sf_interface->table),
                                widget, 0, 3, row, row + 1,
                                GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
              gtk_widget_show (widget);
            }

          if (left_align)
            gtk_size_group_add_widget (group, widget);
        }

      sf_interface->widgets[i] = widget;
    }

  g_object_unref (group);

  /* the script progress bar */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_end (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  sf_interface->progress_bar = gimp_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), sf_interface->progress_bar,
                      FALSE, FALSE, 0);
  gtk_widget_show (sf_interface->progress_bar);

  sf_interface->progress_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (sf_interface->progress_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (sf_interface->progress_label),
                           PANGO_ELLIPSIZE_MIDDLE);
  gimp_label_set_attributes (GTK_LABEL (sf_interface->progress_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), sf_interface->progress_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (sf_interface->progress_label);
#ifdef G_OS_WIN32
    {
      HWND foreground = GetForegroundWindow ();
#endif

  gtk_widget_show (dialog);

  gtk_main ();

#ifdef G_OS_WIN32
      if (! GetForegroundWindow ())
        SetForegroundWindow (foreground);
    }
#endif
  return sf_status;
}

static void
script_fu_interface_quit (SFScript *script)
{
  gint i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (sf_interface != NULL);

  g_free (sf_interface->title);

  for (i = 0; i < script->n_args; i++)
    switch (script->args[i].type)
      {
      case SF_FONT:
      case SF_PALETTE:
      case SF_PATTERN:
      case SF_GRADIENT:
      case SF_BRUSH:
        gimp_select_button_close_popup
          (GIMP_SELECT_BUTTON (sf_interface->widgets[i]));
        break;

      default:
        break;
      }

  g_free (sf_interface->widgets);
  g_free (sf_interface->last_command);

  g_slice_free (SFInterface, sf_interface);
  sf_interface = NULL;

  /*  We do not call gtk_main_quit() earlier to reduce the possibility
   *  that script_fu_script_proc() is called from gimp_extension_process()
   *  while we are not finished with the current script. This sucks!
   */

  gtk_main_quit ();
}

static void
script_fu_file_callback (GtkWidget  *widget,
                         SFFilename *file)
{
  if (file->filename)
    g_free (file->filename);

  file->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
}

static void
script_fu_combo_callback (GtkWidget *widget,
                          SFOption  *option)
{
  option->history = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static void
script_fu_string_update (gchar       **dest,
                         const gchar  *src)
{
  if (*dest)
    g_free (*dest);

  *dest = g_strdup (src);
}

static void
script_fu_pattern_callback (gpointer      data,
                            const gchar  *name,
                            gint          width,
                            gint          height,
                            gint          bytes,
                            const guchar *mask_data,
                            gboolean      closing)
{
  script_fu_string_update (data, name);
}

static void
script_fu_gradient_callback (gpointer       data,
                             const gchar   *name,
                             gint           width,
                             const gdouble *mask_data,
                             gboolean       closing)
{
  script_fu_string_update (data, name);
}

static void
script_fu_font_callback (gpointer     data,
                         const gchar *name,
                         gboolean     closing)
{
  script_fu_string_update (data, name);
}

static void
script_fu_palette_callback (gpointer     data,
                            const gchar *name,
                            gboolean     closing)
{
  script_fu_string_update (data, name);
}

static void
script_fu_brush_callback (gpointer       data,
                          const gchar   *name,
                          gdouble        opacity,
                          gint           spacing,
                          GimpLayerMode  paint_mode,
                          gint           width,
                          gint           height,
                          const guchar  *mask_data,
                          gboolean       closing)
{
  SFBrush *brush = data;

  g_free (brush->name);

  brush->name       = g_strdup (name);
  brush->opacity    = opacity;
  brush->spacing    = spacing;
  brush->paint_mode = paint_mode;
}

static void
unset_transient_for (GtkWidget *dialog)
{
  GdkWindow *window = gtk_widget_get_window (dialog);

  if (window)
    gdk_property_delete (window,
                         gdk_atom_intern_static_string ("WM_TRANSIENT_FOR"));
}

static void
script_fu_response (GtkWidget *widget,
                    gint       response_id,
                    SFScript  *script)
{
  GtkWidget *action_area;

  action_area = gtk_dialog_get_action_area (GTK_DIALOG (sf_interface->dialog));

  if (! gtk_widget_is_sensitive (action_area))
    return;

  switch (response_id)
    {
    case RESPONSE_RESET:
      script_fu_reset (script);
      break;

    case GTK_RESPONSE_OK:
      gtk_widget_set_sensitive (sf_interface->table, FALSE);
      gtk_widget_set_sensitive (action_area, FALSE);

      script_fu_ok (script);

#ifdef GDK_WINDOWING_QUARTZ
      [NSApp hide: nil];
      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, TRUE);
#endif
      /*
       * The script could have created a new GimpImageWindow, so
       * unset the transient-for property not to focus the
       * ImageWindow from which the script was started
       */
      unset_transient_for (sf_interface->dialog);

      gtk_widget_destroy (sf_interface->dialog);
      break;

    default:
      sf_status = GIMP_PDB_CANCEL;

#ifdef GDK_WINDOWING_QUARTZ
      [NSApp hide: nil];
      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, TRUE);
#endif
      gtk_widget_destroy (sf_interface->dialog);
      break;
    }
}

static void
script_fu_ok (SFScript *script)
{
  GString *output;
  gchar   *command;
  gint     i;

  for (i = 0; i < script->n_args; i++)
    {
      SFArgValue *arg_value = &script->args[i].value;
      GtkWidget  *widget    = sf_interface->widgets[i];

      switch (script->args[i].type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
        case SF_COLOR:
        case SF_TOGGLE:
          break;

        case SF_VALUE:
        case SF_STRING:
          g_free (arg_value->sfa_value);
          arg_value->sfa_value =
            g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
          break;

        case SF_TEXT:
          {
            GtkWidget     *view;
            GtkTextBuffer *buffer;
            GtkTextIter    start, end;

            view = gtk_bin_get_child (GTK_BIN (widget));
            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

            gtk_text_buffer_get_start_iter (buffer, &start);
            gtk_text_buffer_get_end_iter (buffer, &end);

            g_free (arg_value->sfa_value);
            arg_value->sfa_value = gtk_text_buffer_get_text (buffer,
                                                             &start, &end,
                                                             FALSE);
          }
          break;

        case SF_ADJUSTMENT:
        case SF_FILENAME:
        case SF_DIRNAME:
        case SF_FONT:
        case SF_PALETTE:
        case SF_PATTERN:
        case SF_GRADIENT:
        case SF_BRUSH:
        case SF_OPTION:
        case SF_ENUM:
          break;
        }
    }

  command = script_fu_script_get_command (script);

  /*  run the command through the interpreter  */
  output = g_string_new (NULL);
  ts_register_output_func (ts_gstring_output_func, output);

  gimp_plugin_set_pdb_error_handler (GIMP_PDB_ERROR_HANDLER_PLUGIN);

  if (ts_interpret_string (command))
    {
      gchar *message = g_strdup_printf (_("Error while executing %s:"),
                                        script->name);

      g_message ("%s\n\n%s", message, output->str);
      g_free (message);
    }

  gimp_plugin_set_pdb_error_handler (GIMP_PDB_ERROR_HANDLER_INTERNAL);

  g_string_free (output, TRUE);

  g_free (command);
}

static void
script_fu_reset (SFScript *script)
{
  gint i;

  script_fu_script_reset (script, FALSE);

  for (i = 0; i < script->n_args; i++)
    {
      SFArgValue *value  = &script->args[i].value;
      GtkWidget  *widget = sf_interface->widgets[i];

      switch (script->args[i].type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
          break;

        case SF_COLOR:
          gimp_color_button_set_color (GIMP_COLOR_BUTTON (widget),
                                       &value->sfa_color);
          break;

        case SF_TOGGLE:
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        value->sfa_toggle);
          break;

        case SF_VALUE:
        case SF_STRING:
          gtk_entry_set_text (GTK_ENTRY (widget), value->sfa_value);
          break;

        case SF_TEXT:
          {
            GtkWidget     *view;
            GtkTextBuffer *buffer;

            view = gtk_bin_get_child (GTK_BIN (widget));
            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

            gtk_text_buffer_set_text (buffer, value->sfa_value, -1);
          }
          break;

        case SF_ADJUSTMENT:
          gtk_adjustment_set_value (value->sfa_adjustment.adj,
                                    value->sfa_adjustment.value);
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget),
                                         value->sfa_file.filename);
          break;

        case SF_FONT:
          gimp_font_select_button_set_font (GIMP_FONT_SELECT_BUTTON (widget),
                                            value->sfa_font);
          break;

        case SF_PALETTE:
          gimp_palette_select_button_set_palette (GIMP_PALETTE_SELECT_BUTTON (widget),
                                                  value->sfa_palette);
          break;

        case SF_PATTERN:
          gimp_pattern_select_button_set_pattern (GIMP_PATTERN_SELECT_BUTTON (widget),
                                                  value->sfa_pattern);
          break;

        case SF_GRADIENT:
          gimp_gradient_select_button_set_gradient (GIMP_GRADIENT_SELECT_BUTTON (widget),
                                                    value->sfa_gradient);
          break;

        case SF_BRUSH:
          gimp_brush_select_button_set_brush (GIMP_BRUSH_SELECT_BUTTON (widget),
                                              value->sfa_brush.name,
                                              value->sfa_brush.opacity,
                                              value->sfa_brush.spacing,
                                              value->sfa_brush.paint_mode);
          break;

        case SF_OPTION:
          gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
                                    value->sfa_option.history);
          break;

        case SF_ENUM:
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget),
                                         value->sfa_enum.history);
          break;
        }
    }
}
