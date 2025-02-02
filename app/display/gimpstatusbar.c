/* GIMP - The GNU Image Manipulation Program Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
#include "core/gimpprogress.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpimagewindow.h"
#include "gimpscalecombobox.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


/*  maximal width of the string holding the cursor-coordinates  */
#define CURSOR_LEN                      256

/*  the spacing of the hbox  */
#define HBOX_SPACING                      1

/*  spacing between the icon and the statusbar label  */
#define ICON_SPACING                      2

/*  timeout (in milliseconds) for temporary statusbar messages  */
#define MESSAGE_TIMEOUT                8000

/*  minimal interval (in microseconds) between progress updates  */
#define MIN_PROGRESS_UPDATE_INTERVAL  50000


typedef struct _GimpStatusbarMsg GimpStatusbarMsg;

struct _GimpStatusbarMsg
{
  guint  context_id;
  gchar *icon_name;
  gchar *text;
};


static void     gimp_statusbar_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_statusbar_dispose            (GObject           *object);
static void     gimp_statusbar_finalize           (GObject           *object);

static void     gimp_statusbar_screen_changed     (GtkWidget         *widget,
                                                   GdkScreen         *previous);
static void     gimp_statusbar_style_updated      (GtkWidget         *widget);

static GimpProgress *
                gimp_statusbar_progress_start     (GimpProgress      *progress,
                                                   gboolean           cancellable,
                                                   const gchar       *message);
static void     gimp_statusbar_progress_end       (GimpProgress      *progress);
static gboolean gimp_statusbar_progress_is_active (GimpProgress      *progress);
static void     gimp_statusbar_progress_set_text  (GimpProgress      *progress,
                                                   const gchar       *message);
static void     gimp_statusbar_progress_set_value (GimpProgress      *progress,
                                                   gdouble            percentage);
static gdouble  gimp_statusbar_progress_get_value (GimpProgress      *progress);
static void     gimp_statusbar_progress_pulse     (GimpProgress      *progress);
static gboolean gimp_statusbar_progress_message   (GimpProgress      *progress,
                                                   Gimp              *gimp,
                                                   GimpMessageSeverity severity,
                                                   const gchar       *domain,
                                                   const gchar       *message);
static void     gimp_statusbar_progress_canceled  (GtkWidget         *button,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_soft_proof_button_toggled
                                                  (GtkWidget         *button,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_soft_proof_profile_changed
                                                  (GtkComboBox       *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_soft_proof_rendering_intent_changed
                                                  (GtkComboBox       *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_soft_proof_bpc_toggled
                                                  (GtkWidget         *button,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_soft_proof_optimize_changed
                                                  (GtkWidget         *button,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_soft_proof_gamut_toggled
                                                  (GtkWidget         *button,
                                                   GimpStatusbar     *statusbar);
static gboolean gimp_statusbar_soft_proof_popover_shown
                                                 (GtkWidget          *button,
                                                  GdkEventButton     *bevent,
                                                  GimpStatusbar      *statusbar);
static void     gimp_statusbar_soft_proof_size_allocate
                                                 (GtkWidget          *widget,
                                                  GtkAllocation      *allocation,
                                                  GimpStatusbar      *statusbar);

static gboolean gimp_statusbar_label_draw         (GtkWidget         *widget,
                                                   cairo_t           *cr,
                                                   GimpStatusbar     *statusbar);

static void     gimp_statusbar_update             (GimpStatusbar     *statusbar);
static void     gimp_statusbar_unit_changed       (GimpUnitComboBox  *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_scale_changed      (GimpScaleComboBox *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_scale_activated    (GimpScaleComboBox *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_image_changed(GimpStatusbar     *statusbar,
                                                   GimpImage         *image,
                                                   GimpContext       *context);
static void     gimp_statusbar_shell_image_simulation_changed
                                                  (GimpImage        *image,
                                                   GimpStatusbar     *statusbar);

static gboolean gimp_statusbar_rotate_pressed     (GtkWidget         *event_box,
                                                   GdkEvent          *event,
                                                   GimpStatusbar     *statusbar);
static gboolean gimp_statusbar_horiz_flip_pressed (GtkWidget         *event_box,
                                                   GdkEvent          *event,
                                                   GimpStatusbar     *statusbar);
static gboolean gimp_statusbar_vert_flip_pressed  (GtkWidget         *event_box,
                                                   GdkEvent          *event,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_scaled       (GimpDisplayShell  *shell,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_rotated      (GimpDisplayShell  *shell,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_status_notify(GimpDisplayShell  *shell,
                                                   const GParamSpec  *pspec,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_color_config_notify
                                                  (GObject           *config,
                                                   const GParamSpec  *pspec,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_set_image    (GimpStatusbar     *statusbar,
                                                   GimpImage         *image);

static guint    gimp_statusbar_get_context_id     (GimpStatusbar     *statusbar,
                                                   const gchar       *context);
static gboolean gimp_statusbar_temp_timeout       (GimpStatusbar     *statusbar);

static void     gimp_statusbar_add_size_widget    (GimpStatusbar     *statusbar,
                                                   GtkWidget         *widget);
static void     gimp_statusbar_update_size        (GimpStatusbar     *statusbar);

static void     gimp_statusbar_add_message        (GimpStatusbar     *statusbar,
                                                   guint              context_id,
                                                   const gchar       *icon_name,
                                                   const gchar       *format,
                                                   va_list            args,
                                                   gboolean           move_to_front) G_GNUC_PRINTF (4, 0);
static void     gimp_statusbar_remove_message     (GimpStatusbar     *statusbar,
                                                   guint              context_id);
static void     gimp_statusbar_msg_free           (GimpStatusbarMsg  *msg);

static gchar *  gimp_statusbar_vprintf            (const gchar       *format,
                                                   va_list            args) G_GNUC_PRINTF (1, 0);

static GdkPixbuf * gimp_statusbar_load_icon       (GimpStatusbar     *statusbar,
                                                   const gchar       *icon_name);

static gboolean gimp_statusbar_queue_pos_redraw   (gpointer           data);


G_DEFINE_TYPE_WITH_CODE (GimpStatusbar, gimp_statusbar, GTK_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_statusbar_progress_iface_init))

#define parent_class gimp_statusbar_parent_class


static void
gimp_statusbar_class_init (GimpStatusbarClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose        = gimp_statusbar_dispose;
  object_class->finalize       = gimp_statusbar_finalize;

  widget_class->screen_changed = gimp_statusbar_screen_changed;
  widget_class->style_updated  = gimp_statusbar_style_updated;

  gtk_widget_class_set_css_name (widget_class, "statusbar");
}

static void
gimp_statusbar_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_statusbar_progress_start;
  iface->end       = gimp_statusbar_progress_end;
  iface->is_active = gimp_statusbar_progress_is_active;
  iface->set_text  = gimp_statusbar_progress_set_text;
  iface->set_value = gimp_statusbar_progress_set_value;
  iface->get_value = gimp_statusbar_progress_get_value;
  iface->pulse     = gimp_statusbar_progress_pulse;
  iface->message   = gimp_statusbar_progress_message;
}

static void
gimp_statusbar_init (GimpStatusbar *statusbar)
{
  GtkWidget     *hbox;
  GtkWidget     *hbox2;
  GtkWidget     *image;
  GtkWidget     *label;
  GtkWidget     *grid;
  GtkWidget     *separator;
  GtkWidget     *profile_chooser;
  GimpUnitStore *store;
  gchar         *text;
  GFile         *file;
  GtkListStore  *combo_store;
  gint           row;

  gtk_frame_set_shadow_type (GTK_FRAME (statusbar), GTK_SHADOW_IN);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, HBOX_SPACING);
  gtk_container_add (GTK_CONTAINER (statusbar), hbox);
  gtk_widget_show (hbox);

  /* When changing the text of the cursor_label, it requests a resize
   * bubbling up to containers, up to the display shell. If the resizing
   * actually happens (even when the size is the same), a full "draw"
   * signal is triggered on the whole canvas, which is not a good thing
   * as a general rule (and very bad on some platforms such as macOS).
   * It's too late to do anything when processing the "draw" signal
   * because then we can't know if some part of the invalidated
   * rectangle really needs to be redrawn. What we do is not propagate
   * the size request back to container parents. Instead we queue the
   * resize directly on the widget.
   *
   * Note that the "resize-mode" property seems to be unrecommended now
   * (though only the public functions are deprecated, we get no
   * deprecation setting as a property) but it's still here in GTK3 and
   * still seems like the proper way to avoid propagating useless
   * no-actual-size-change resizes to container widgets.
   * XXX On GTK4, we will likely have to test again and if it's still a
   * problem, make a different fix.
   *
   * See discussion in MR !572.
   */
  g_object_set (statusbar,
                "resize-mode", GTK_RESIZE_QUEUE,
                NULL);

  statusbar->shell          = NULL;
  statusbar->messages       = NULL;
  statusbar->context_ids    = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, NULL);
  statusbar->seq_context_id = 1;

  statusbar->temp_context_id =
    gimp_statusbar_get_context_id (statusbar, "gimp-statusbar-temp");

  statusbar->cursor_format_str[0]   = '\0';
  statusbar->cursor_format_str_f[0] = '\0';
  statusbar->length_format_str[0]   = '\0';

  statusbar->progress_active      = FALSE;
  statusbar->progress_shown       = FALSE;

  statusbar->cursor_label = gtk_label_new ("8888, 8888");
  gimp_statusbar_add_size_widget (statusbar, statusbar->cursor_label);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->cursor_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (statusbar->cursor_label);

  store = gimp_unit_store_new (2);
  statusbar->unit_combo = gimp_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  /* see issue #2642 */
  gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (statusbar->unit_combo), 1);

  gtk_widget_set_can_focus (statusbar->unit_combo, FALSE);
  g_object_set (statusbar->unit_combo, "focus-on-click", FALSE, NULL);
  gimp_statusbar_add_size_widget (statusbar, statusbar->unit_combo);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->unit_combo,
                      FALSE, FALSE, 0);
  gtk_widget_show (statusbar->unit_combo);

  g_signal_connect (statusbar->unit_combo, "changed",
                    G_CALLBACK (gimp_statusbar_unit_changed),
                    statusbar);

  statusbar->scale_combo = gimp_scale_combo_box_new ();
  gtk_widget_set_can_focus (statusbar->scale_combo, FALSE);
  g_object_set (statusbar->scale_combo, "focus-on-click", FALSE, NULL);
  gimp_statusbar_add_size_widget (statusbar, statusbar->scale_combo);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->scale_combo,
                      FALSE, FALSE, 0);
  gtk_widget_show (statusbar->scale_combo);

  g_signal_connect (statusbar->scale_combo, "changed",
                    G_CALLBACK (gimp_statusbar_scale_changed),
                    statusbar);

  g_signal_connect (statusbar->scale_combo, "entry-activated",
                    G_CALLBACK (gimp_statusbar_scale_activated),
                    statusbar);

  /* Shell transform status */
  statusbar->rotate_widget = gtk_event_box_new ();
  gimp_statusbar_add_size_widget (statusbar, statusbar->rotate_widget);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->rotate_widget,
                      FALSE, FALSE, 1);
  gtk_widget_show (statusbar->rotate_widget);

  statusbar->rotate_label = gtk_label_new (NULL);
  gtk_container_add (GTK_CONTAINER (statusbar->rotate_widget),
                     statusbar->rotate_label);
  gtk_widget_show (statusbar->rotate_label);

  g_signal_connect (statusbar->rotate_widget, "button-press-event",
                    G_CALLBACK (gimp_statusbar_rotate_pressed),
                    statusbar);

  statusbar->horizontal_flip_icon = gtk_event_box_new ();
  gimp_statusbar_add_size_widget (statusbar, statusbar->horizontal_flip_icon);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->horizontal_flip_icon,
                      FALSE, FALSE, 1);
  gtk_widget_show (statusbar->horizontal_flip_icon);

  image = gtk_image_new_from_icon_name ("object-flip-horizontal",
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (statusbar->horizontal_flip_icon), image);
  gtk_widget_show (image);

  g_signal_connect (statusbar->horizontal_flip_icon, "button-press-event",
                    G_CALLBACK (gimp_statusbar_horiz_flip_pressed),
                    statusbar);

  statusbar->vertical_flip_icon = gtk_event_box_new ();
  gimp_statusbar_add_size_widget (statusbar, statusbar->vertical_flip_icon);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->vertical_flip_icon,
                      FALSE, FALSE, 1);
  gtk_widget_show (statusbar->vertical_flip_icon);

  image = gtk_image_new_from_icon_name ("object-flip-vertical",
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (statusbar->vertical_flip_icon), image);
  gtk_widget_show (image);

  g_signal_connect (statusbar->vertical_flip_icon, "button-press-event",
                    G_CALLBACK (gimp_statusbar_vert_flip_pressed),
                    statusbar);

  statusbar->label = gtk_label_new ("");
  gtk_label_set_ellipsize (GTK_LABEL (statusbar->label), PANGO_ELLIPSIZE_END);
  gtk_label_set_justify (GTK_LABEL (statusbar->label), GTK_JUSTIFY_LEFT);
  gtk_widget_set_halign (statusbar->label, GTK_ALIGN_START);
  gimp_statusbar_add_size_widget (statusbar, statusbar->label);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->label, TRUE, TRUE, 1);
  gtk_widget_show (statusbar->label);

  g_signal_connect_after (statusbar->label, "draw",
                          G_CALLBACK (gimp_statusbar_label_draw),
                          statusbar);

  statusbar->progressbar = g_object_new (GTK_TYPE_PROGRESS_BAR,
                                         "show-text", TRUE,
                                         "ellipsize", PANGO_ELLIPSIZE_END,
                                         NULL);
  gimp_statusbar_add_size_widget (statusbar, statusbar->progressbar);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->progressbar,
                      TRUE, TRUE, 0);
  /*  don't show the progress bar  */

  /*  construct the cancel button's contents manually because we
   *  always want image and label regardless of settings, and we want
   *  a menu size image.
   */
  statusbar->cancel_button = gtk_button_new ();
  gtk_widget_set_can_focus (statusbar->cancel_button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (statusbar->cancel_button),
                         GTK_RELIEF_NONE);
  gtk_widget_set_sensitive (statusbar->cancel_button, FALSE);
  gimp_statusbar_add_size_widget (statusbar, statusbar->cancel_button);
  gtk_box_pack_end (GTK_BOX (hbox), statusbar->cancel_button,
                    FALSE, FALSE, 0);
  /*  don't show the cancel button  */

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (statusbar->cancel_button), hbox2);
  gtk_widget_show (hbox2);

  image = gtk_image_new_from_icon_name ("gtk-cancel", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox2), image, FALSE, FALSE, 2);
  gtk_widget_show (image);

  label = gtk_label_new (_("Cancel"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  g_signal_connect (statusbar->cancel_button, "clicked",
                    G_CALLBACK (gimp_statusbar_progress_canceled),
                    statusbar);

  /* soft proofing button */
  statusbar->soft_proof_button = gtk_toggle_button_new();
  gtk_widget_set_can_focus (statusbar->soft_proof_button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (statusbar->soft_proof_button),
                         GTK_RELIEF_NONE);
  image = gtk_image_new_from_icon_name (GIMP_ICON_DISPLAY_FILTER_PROOF,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (statusbar->soft_proof_button), image);
  gtk_widget_show (image);

  gtk_widget_show (statusbar->soft_proof_button);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->soft_proof_button),
                                FALSE);

  /* The soft-proof toggle button is placed in a GtkEventBox
   * so it can be disabled while still allowing users to right-click
   * and access the soft-proofing menu
   */
  statusbar->soft_proof_container = gtk_event_box_new ();
  g_signal_connect_after (statusbar->soft_proof_container, "size-allocate",
                          G_CALLBACK (gimp_statusbar_soft_proof_size_allocate),
                          statusbar);
  gtk_container_add (GTK_CONTAINER (statusbar->soft_proof_container),
                     statusbar->soft_proof_button);
  gtk_box_pack_end (GTK_BOX (hbox), statusbar->soft_proof_container,
                    FALSE, FALSE, 0);
  gimp_statusbar_add_size_widget (statusbar, statusbar->soft_proof_container);
  gtk_widget_show (statusbar->soft_proof_container);
  gimp_help_set_help_data (statusbar->soft_proof_container,
                           _("Toggle soft-proofing view when "
                             "a soft-proofing profile is set\n"
                             "Right-click to show the soft-proofing "
                             "options"),
                           NULL);
  gtk_widget_set_events (statusbar->soft_proof_container, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (statusbar->soft_proof_container, "button-press-event",
                    G_CALLBACK (gimp_statusbar_soft_proof_popover_shown),
                    statusbar);
  gtk_event_box_set_above_child (GTK_EVENT_BOX (statusbar->soft_proof_container), FALSE);

  /* soft proofing popover */
  row = 0;

  statusbar->soft_proof_popover = gtk_popover_new (statusbar->soft_proof_container);
  gtk_popover_set_modal (GTK_POPOVER (statusbar->soft_proof_popover), TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);

  label = gtk_label_new (NULL);
  text = g_strdup_printf ("<b>%s</b>",
                          _("Soft-Proofing"));
  gtk_label_set_markup (GTK_LABEL (label), text);
  g_free (text);
  gtk_grid_attach (GTK_GRID (grid),
                   label,
                   0, row++, 2, 1);
  gtk_widget_show (label);

  statusbar->proof_colors_toggle =
    gtk_check_button_new_with_mnemonic (_("_Proof Colors"));
  gtk_grid_attach (GTK_GRID (grid),
                   statusbar->proof_colors_toggle,
                   0, row++, 1, 1);
  g_signal_connect (statusbar->proof_colors_toggle, "clicked",
                    G_CALLBACK (gimp_statusbar_soft_proof_button_toggled),
                    statusbar);
  gtk_widget_show (statusbar->proof_colors_toggle);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->proof_colors_toggle),
                                FALSE);

  statusbar->profile_label = gtk_label_new (NULL);
  text = g_strdup_printf ("<b>%s</b>: %s",
                          _("Current Soft-Proofing Profile"),
                          _("None"));
  gtk_label_set_markup (GTK_LABEL (statusbar->profile_label), text);
  g_free (text);
  gtk_grid_attach (GTK_GRID (grid),
                   statusbar->profile_label,
                   0, row++, 2, 1);
  gtk_widget_show (statusbar->profile_label);

  file = gimp_directory_file ("profilerc", NULL);
  combo_store = gimp_color_profile_store_new (file);
  g_object_unref (file);
  gimp_color_profile_store_add_file (GIMP_COLOR_PROFILE_STORE (combo_store),
                                     NULL, NULL);
  profile_chooser = gimp_color_profile_chooser_dialog_new (_("Soft-Proofing Profile"), NULL,
                                                           GTK_FILE_CHOOSER_ACTION_OPEN);
  statusbar->profile_combo = gimp_color_profile_combo_box_new_with_model (profile_chooser,
                                                                          GTK_TREE_MODEL (combo_store));

  gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (statusbar->profile_combo),
                                                NULL, NULL);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Soft-proofing Profile: "),
                            0.0, 0.5,
                            statusbar->profile_combo, 1);
  gtk_widget_show (statusbar->profile_combo);
  g_signal_connect (statusbar->profile_combo, "changed",
                    G_CALLBACK (gimp_statusbar_soft_proof_profile_changed),
                    statusbar);

  combo_store =
    gimp_int_store_new (_("Perceptual"),            GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                        _("Relative Colorimetric"), GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                        _("Saturation"),            GIMP_COLOR_RENDERING_INTENT_SATURATION,
                        _("Absolute Colorimetric"), GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
                        NULL);
  statusbar->rendering_intent_combo = g_object_new (GIMP_TYPE_INT_COMBO_BOX,
                            "model", combo_store,
                            "visible", TRUE,
                            NULL);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Rendering Intent: "),
                            0.0, 0.5,
                            statusbar->rendering_intent_combo, 1);
  gtk_widget_show (statusbar->rendering_intent_combo);
  g_signal_connect (statusbar->rendering_intent_combo, "changed",
                    G_CALLBACK (gimp_statusbar_soft_proof_rendering_intent_changed),
                    statusbar);

  statusbar->bpc_toggle =
    gtk_check_button_new_with_mnemonic (_("Use _Black Point Compensation"));
  gtk_grid_attach (GTK_GRID (grid),
                   statusbar->bpc_toggle,
                   0, row++, 1, 1);
  gtk_widget_show (statusbar->bpc_toggle);
  g_signal_connect (statusbar->bpc_toggle, "clicked",
                    G_CALLBACK (gimp_statusbar_soft_proof_bpc_toggled),
                    statusbar);

  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grid),separator,
                   0, row++, 1, 1);
  gtk_widget_show (separator);

  statusbar->optimize_combo =
    gimp_int_combo_box_new (_("Speed"),  TRUE,
                            _("Precision / Color Fidelity"), FALSE,
                            NULL);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("O_ptimize soft-proofing for: "),
                            0.0, 0.5,
                            statusbar->optimize_combo, 1);
  gtk_widget_show (statusbar->optimize_combo);
  g_signal_connect (statusbar->optimize_combo, "changed",
                    G_CALLBACK (gimp_statusbar_soft_proof_optimize_changed),
                    statusbar);

  statusbar->out_of_gamut_toggle =
    gtk_check_button_new_with_mnemonic (_("_Mark Out of Gamut Colors"));
  gtk_grid_attach (GTK_GRID (grid),
                   statusbar->out_of_gamut_toggle,
                   0, row++, 1, 1);
  gtk_widget_show (statusbar->out_of_gamut_toggle);
  g_signal_connect (statusbar->out_of_gamut_toggle, "clicked",
                    G_CALLBACK (gimp_statusbar_soft_proof_gamut_toggled),
                    statusbar);

  gtk_container_add (GTK_CONTAINER (statusbar->soft_proof_popover), grid);
  gtk_widget_show (grid);

  gimp_statusbar_update_size (statusbar);
}

static void
gimp_statusbar_dispose (GObject *object)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (object);

  if (statusbar->gimp)
    {
      g_signal_handlers_disconnect_by_func (gimp_get_user_context (statusbar->gimp),
                                            gimp_statusbar_shell_image_changed,
                                            statusbar);
      statusbar->gimp = NULL;
    }

  gimp_statusbar_shell_set_image (statusbar, NULL);

  if (statusbar->temp_timeout_id)
    {
      g_source_remove (statusbar->temp_timeout_id);
      statusbar->temp_timeout_id = 0;
    }

  if (statusbar->statusbar_pos_redraw_idle_id)
    {
      g_source_remove (statusbar->statusbar_pos_redraw_idle_id);
      statusbar->statusbar_pos_redraw_idle_id = 0;
    }

  g_clear_pointer (&statusbar->size_widgets, g_slist_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_statusbar_finalize (GObject *object)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (object);

  g_clear_object (&statusbar->icon);
  g_clear_pointer (&statusbar->icon_hash, g_hash_table_unref);
  g_clear_pointer (&statusbar->cursor_string_last,   g_free);
  g_clear_pointer (&statusbar->cursor_string_todraw, g_free);

  g_slist_free_full (statusbar->messages,
                     (GDestroyNotify) gimp_statusbar_msg_free);
  statusbar->messages = NULL;

  g_clear_pointer (&statusbar->context_ids, g_hash_table_destroy);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_statusbar_screen_changed (GtkWidget *widget,
                               GdkScreen *previous)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (widget);

  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous);

  g_clear_pointer (&statusbar->icon_hash, g_hash_table_unref);
}

static void
gimp_statusbar_style_updated (GtkWidget *widget)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (widget);
  PangoLayout   *layout;
  GList         *children;
  gint           pixel_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_icon_size_lookup (statusbar->icon_size, &pixel_size, NULL);

  g_clear_pointer (&statusbar->icon_hash, g_hash_table_unref);

  children =
    gtk_container_get_children (GTK_CONTAINER (statusbar->soft_proof_button));
  gtk_image_set_pixel_size (GTK_IMAGE (children->data), pixel_size);
  g_list_free (children);

  layout = gtk_widget_create_pango_layout (widget, " ");
  pango_layout_get_pixel_size (layout, &statusbar->icon_space_width, NULL);
  g_object_unref (layout);

  gimp_statusbar_update_size (statusbar);
}

static GimpProgress *
gimp_statusbar_progress_start (GimpProgress *progress,
                               gboolean      cancellable,
                               const gchar  *message)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (! statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      statusbar->progress_active           = TRUE;
      statusbar->progress_value            = 0.0;
      statusbar->progress_last_update_time = g_get_monotonic_time ();

      gimp_statusbar_push (statusbar, "progress", NULL, "%s", message);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);
      gtk_widget_set_sensitive (statusbar->cancel_button, cancellable);

      if (cancellable)
        {
          if (message)
            {
              gchar *tooltip = g_strdup_printf (_("Cancel <i>%s</i>"), message);

              gimp_help_set_help_data_with_markup (statusbar->cancel_button,
                                                   tooltip, NULL);
              g_free (tooltip);
            }

          gtk_widget_show (statusbar->cancel_button);
        }

      gtk_widget_show (statusbar->progressbar);
      gtk_widget_hide (statusbar->label);

      if (! gtk_widget_get_visible (GTK_WIDGET (statusbar)))
        {
          gtk_widget_show (GTK_WIDGET (statusbar));
          statusbar->progress_shown = TRUE;
        }

      gimp_widget_flush_expose ();

      gimp_statusbar_override_window_title (statusbar);

      return progress;
    }

  return NULL;
}

static void
gimp_statusbar_progress_end (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      if (statusbar->progress_shown)
        {
          gtk_widget_hide (GTK_WIDGET (statusbar));
          statusbar->progress_shown = FALSE;
        }

      statusbar->progress_active = FALSE;
      statusbar->progress_value  = 0.0;

      gtk_widget_hide (bar);
      gtk_widget_show (statusbar->label);

      gimp_statusbar_pop (statusbar, "progress");

      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);
      gtk_widget_set_sensitive (statusbar->cancel_button, FALSE);
      gtk_widget_hide (statusbar->cancel_button);

      gimp_statusbar_restore_window_title (statusbar);
    }
}

static gboolean
gimp_statusbar_progress_is_active (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  return statusbar->progress_active;
}

static void
gimp_statusbar_progress_set_text (GimpProgress *progress,
                                  const gchar  *message)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      gimp_statusbar_replace (statusbar, "progress", NULL, "%s", message);

      gimp_widget_flush_expose ();

      gimp_statusbar_override_window_title (statusbar);
    }
}

static void
gimp_statusbar_progress_set_value (GimpProgress *progress,
                                   gdouble       percentage)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      guint64 time = g_get_monotonic_time ();

      if (time - statusbar->progress_last_update_time >=
          MIN_PROGRESS_UPDATE_INTERVAL)
        {
          GtkWidget     *bar = statusbar->progressbar;
          GtkAllocation  allocation;
          gdouble        diff;

          gtk_widget_get_allocation (bar, &allocation);

          statusbar->progress_value = percentage;

          diff = fabs (percentage -
                       gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (bar)));

          /* only update the progress bar if this causes a visible change */
          if (allocation.width * diff >= 1.0)
            {
              statusbar->progress_last_update_time = time;

              gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar),
                                             percentage);

              gimp_widget_flush_expose ();
            }
        }
    }
}

static gdouble
gimp_statusbar_progress_get_value (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    return statusbar->progress_value;

  return 0.0;
}

static void
gimp_statusbar_progress_pulse (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      guint64 time = g_get_monotonic_time ();

      if (time - statusbar->progress_last_update_time >=
          MIN_PROGRESS_UPDATE_INTERVAL)
        {
          GtkWidget *bar = statusbar->progressbar;

          statusbar->progress_last_update_time = time;

          gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));

          gimp_widget_flush_expose ();
        }
    }
}

static gboolean
gimp_statusbar_progress_message (GimpProgress        *progress,
                                 Gimp                *gimp,
                                 GimpMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message)
{
  GimpStatusbar *statusbar  = GIMP_STATUSBAR (progress);
  PangoLayout   *layout;
  const gchar   *icon_name;
  gboolean       handle_msg = FALSE;

  /*  don't accept a message if we are already displaying a more severe one  */
  if (statusbar->temp_timeout_id && statusbar->temp_severity > severity)
    return FALSE;

  /*  we can only handle short one-liners  */
  layout = gtk_widget_create_pango_layout (statusbar->label, message);

  icon_name = gimp_get_message_icon_name (severity);

  if (pango_layout_get_line_count (layout) == 1)
    {
      GtkWidget     *label_box = gtk_widget_get_parent (statusbar->label);
      GtkAllocation  label_allocation;
      gint           text_width, max_label_width, x;

      gtk_widget_get_allocation (label_box, &label_allocation);
      if (gtk_widget_translate_coordinates (statusbar->label, label_box, 0, 0,
                                            &x, NULL))
        {
          max_label_width = label_allocation.width - x;
          pango_layout_get_pixel_size (layout, &text_width, NULL);

          if (text_width < max_label_width)
            {
              if (icon_name)
                {
                  GdkPixbuf *pixbuf;
                  gint       scale_factor;

                  pixbuf = gimp_statusbar_load_icon (statusbar, icon_name);

                  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (statusbar));
                  text_width += ICON_SPACING + gdk_pixbuf_get_width (pixbuf) / scale_factor;

                  g_object_unref (pixbuf);

                  handle_msg = (text_width < max_label_width);
                }
              else
                {
                  handle_msg = TRUE;
                }
            }
        }
    }

  g_object_unref (layout);

  if (handle_msg)
    gimp_statusbar_push_temp (statusbar, severity, icon_name, "%s", message);

  return handle_msg;
}

static void
gimp_statusbar_progress_canceled (GtkWidget     *button,
                                  GimpStatusbar *statusbar)
{
  if (statusbar->progress_active)
    gimp_progress_cancel (GIMP_PROGRESS (statusbar));
}

static void
gimp_statusbar_soft_proof_button_toggled (GtkWidget     *button,
                                          GimpStatusbar *statusbar)
{
  GimpColorConfig        *color_config;
  GimpColorManagementMode mode;
  gboolean                active;

  color_config = gimp_display_shell_get_color_config (statusbar->shell);
  mode = gimp_color_config_get_mode (color_config);
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  if (active)
    {
      mode = GIMP_COLOR_MANAGEMENT_SOFTPROOF;
    }
  else
    {
      if (mode != GIMP_COLOR_MANAGEMENT_OFF)
        mode = GIMP_COLOR_MANAGEMENT_DISPLAY;
    }

  if (mode != gimp_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      statusbar->shell->color_config_set = TRUE;
    }

  /* Updates soft-proofing buttons */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->soft_proof_button),
                                active);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->proof_colors_toggle),
                                active);

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static void
gimp_statusbar_soft_proof_profile_changed (GtkComboBox   *combo,
                                           GimpStatusbar *statusbar)
{
  GimpImage        *image;
  GimpColorConfig  *color_config;
  GFile            *file;
  GimpColorProfile *simulation_profile = NULL;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  image = statusbar->image;
  color_config = gimp_display_shell_get_color_config (statusbar->shell);
  file =
    gimp_color_profile_combo_box_get_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo));

  if (file)
    {
      simulation_profile = gimp_color_profile_new_from_file (file, NULL);
      g_object_unref (file);
    }

  if (image)
    gimp_image_set_simulation_profile (image, simulation_profile);

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static void
gimp_statusbar_soft_proof_rendering_intent_changed (GtkComboBox   *combo,
                                                    GimpStatusbar *statusbar)
{
  GimpImage                *image;
  GimpColorConfig          *color_config;
  GimpColorRenderingIntent  intent;
  GimpColorRenderingIntent  active;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  image = statusbar->image;
  color_config = gimp_display_shell_get_color_config (statusbar->shell);

  if (image)
    {
      intent = gimp_image_get_simulation_intent (image);
      active =
        (GimpColorRenderingIntent) gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

      if (active != intent)
        {
          gimp_image_set_simulation_intent (image, active);
          gimp_image_flush (image);
        }
    }

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static void
gimp_statusbar_soft_proof_bpc_toggled (GtkWidget     *button,
                                       GimpStatusbar *statusbar)
{
  GimpImage       *image;
  GimpColorConfig *color_config;
  gboolean         bpc_enabled;
  gboolean         active;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  image = statusbar->image;
  color_config = gimp_display_shell_get_color_config (statusbar->shell);

  if (image)
    {
      bpc_enabled = gimp_image_get_simulation_bpc (image);
      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

      if (active != bpc_enabled)
        {
          gimp_image_set_simulation_bpc (image, active);
          gimp_image_flush (image);
        }
    }

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static void
gimp_statusbar_soft_proof_optimize_changed (GtkWidget     *combo,
                                            GimpStatusbar *statusbar)
{
  GimpColorConfig *color_config;
  gint             optimize;
  gint             active;

  color_config = gimp_display_shell_get_color_config (statusbar->shell);
  optimize = gimp_color_config_get_simulation_optimize (color_config);
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &active);

  if (active != optimize)
    {
      g_object_set (color_config,
                    "simulation-optimize", active,
                    NULL);
      statusbar->shell->color_config_set = TRUE;
    }

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static void
gimp_statusbar_soft_proof_gamut_toggled (GtkWidget     *button,
                                         GimpStatusbar *statusbar)
{
  GimpColorConfig *color_config;
  gboolean         out_of_gamut;
  gboolean         active;

  color_config = gimp_display_shell_get_color_config (statusbar->shell);
  out_of_gamut = gimp_color_config_get_simulation_gamut_check (color_config);
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  if (active != out_of_gamut)
    {
      g_object_set (color_config,
                    "simulation-gamut-check", active,
                    NULL);
      statusbar->shell->color_config_set = TRUE;
    }

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static gboolean
gimp_statusbar_soft_proof_popover_shown (GtkWidget      *button,
                                         GdkEventButton *bevent,
                                         GimpStatusbar  *statusbar)
{
  if (bevent->type == GDK_BUTTON_PRESS)
    {
      if (bevent->button == 3)
        gtk_widget_show (statusbar->soft_proof_popover);

      if (bevent->button == 1 &&
          gtk_widget_get_sensitive (statusbar->soft_proof_button))
        {
          /* Since a GtkEventBox now covers the toggle so we can't click it,
           * directly, we have to flip the toggle ourselves before we call
           * the soft-proof button so it produces the right result
           */
          gboolean active =
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (statusbar->soft_proof_button));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->soft_proof_button),
                                        ! active);

          gimp_statusbar_soft_proof_button_toggled (statusbar->soft_proof_button,
                                                    statusbar);
        }
    }

  return TRUE;
}

static void
gimp_statusbar_soft_proof_size_allocate (GtkWidget     *widget,
                                         GtkAllocation *allocation,
                                         GimpStatusbar *statusbar)
{
  /* This is an ugly hack for what looks a bug in GtkEventBox. In some cases,
   * button events were not reaching the box at all, even though the event box
   * was above the inactive child. Yet when setting it back down and up, it
   * works. I have not figured out the exact bug cause yet, so we settle with
   * this workaround for the time being. I'm setting it on the size-allocate
   * signal, because when trying on "show" or "map-event", it was not working
   * either. I guess it doesn't work when done before the widget is actually
   * drawn. FIXME.
   */
  gtk_event_box_set_above_child (GTK_EVENT_BOX (statusbar->soft_proof_container), FALSE);
  gtk_event_box_set_above_child (GTK_EVENT_BOX (statusbar->soft_proof_container), TRUE);
}

static void
gimp_statusbar_set_text (GimpStatusbar *statusbar,
                         const gchar   *icon_name,
                         const gchar   *text)
{
  if (statusbar->progress_active)
    {
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                                 text);
    }
  else
    {
      g_clear_object (&statusbar->icon);

      if (icon_name)
        statusbar->icon = gimp_statusbar_load_icon (statusbar, icon_name);

      if (statusbar->icon)
        {
          gchar *tmp;
          gint   scale_factor;
          gint   n_spaces;
          gchar  spaces[] = "                                 ";

          scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (statusbar));

          /* Make sure icon_space_width has been initialized to avoid a
           * division by zero.
           */
          if (statusbar->icon_space_width == 0)
            gimp_statusbar_style_updated (GTK_WIDGET (statusbar));
          g_return_if_fail (statusbar->icon_space_width != 0);

          /* prepend enough spaces for the icon plus one space */
          n_spaces = (gdk_pixbuf_get_width (statusbar->icon) / scale_factor +
                      ICON_SPACING) / statusbar->icon_space_width;
          n_spaces++;

          tmp = g_strconcat (spaces + strlen (spaces) - n_spaces, text, NULL);
          gtk_label_set_text (GTK_LABEL (statusbar->label), tmp);
          g_free (tmp);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (statusbar->label), text);
        }
    }
}

static void
gimp_statusbar_update (GimpStatusbar *statusbar)
{
  GimpStatusbarMsg *msg = NULL;

  if (statusbar->messages)
    msg = statusbar->messages->data;

  if (msg && msg->text)
    {
      gimp_statusbar_set_text (statusbar, msg->icon_name, msg->text);
    }
  else
    {
      gimp_statusbar_set_text (statusbar, NULL, "");
    }
}


/*  public functions  */

GtkWidget *
gimp_statusbar_new (void)
{
  return g_object_new (GIMP_TYPE_STATUSBAR, NULL);
}

void
gimp_statusbar_set_shell (GimpStatusbar    *statusbar,
                          GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell == statusbar->shell)
    return;

  if (statusbar->shell)
    {
      g_signal_handlers_disconnect_by_func (statusbar->shell,
                                            gimp_statusbar_shell_scaled,
                                            statusbar);
      g_signal_handlers_disconnect_by_func (statusbar->shell,
                                            gimp_statusbar_shell_rotated,
                                            statusbar);
      g_signal_handlers_disconnect_by_func (statusbar->shell,
                                            gimp_statusbar_shell_status_notify,
                                            statusbar);
      if (statusbar->shell->color_config)
        g_signal_handlers_disconnect_by_func (statusbar->shell->color_config,
                                              gimp_statusbar_shell_color_config_notify,
                                              statusbar);
    }

  if (statusbar->gimp)
    {
      GimpContext *context;

      context = gimp_get_user_context (statusbar->gimp);

      g_signal_handlers_disconnect_by_func (context,
                                            gimp_statusbar_shell_image_changed,
                                            statusbar);
      gimp_statusbar_shell_set_image (statusbar, NULL);
    }

  statusbar->shell = shell;

  g_signal_connect_object (statusbar->shell, "scaled",
                           G_CALLBACK (gimp_statusbar_shell_scaled),
                           statusbar, 0);
  g_signal_connect_object (statusbar->shell, "rotated",
                           G_CALLBACK (gimp_statusbar_shell_rotated),
                           statusbar, 0);
  g_signal_connect_object (statusbar->shell, "notify::status",
                           G_CALLBACK (gimp_statusbar_shell_status_notify),
                           statusbar, 0);

  if (statusbar->shell->color_config)
    g_signal_connect (statusbar->shell->color_config, "notify",
                      G_CALLBACK (gimp_statusbar_shell_color_config_notify),
                      statusbar);

  statusbar->gimp = gimp_display_get_gimp (statusbar->shell->display);
  if (statusbar->gimp)
    {
      GimpContext *context;
      GimpImage   *image;

      context = gimp_get_user_context (statusbar->gimp);
      image   = gimp_context_get_image (context);

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (gimp_statusbar_shell_image_changed),
                                statusbar);
      gimp_statusbar_shell_image_changed (statusbar, image, context);
    }

  gimp_statusbar_shell_rotated (shell, statusbar);
}

gboolean
gimp_statusbar_get_visible (GimpStatusbar *statusbar)
{
  g_return_val_if_fail (GIMP_IS_STATUSBAR (statusbar), FALSE);

  if (statusbar->progress_shown)
    return FALSE;

  return gtk_widget_get_visible (GTK_WIDGET (statusbar));
}

void
gimp_statusbar_set_visible (GimpStatusbar *statusbar,
                            gboolean       visible)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  if (statusbar->progress_shown)
    {
      if (visible)
        {
          statusbar->progress_shown = FALSE;
          return;
        }
    }

  gtk_widget_set_visible (GTK_WIDGET (statusbar), visible);
}

void
gimp_statusbar_empty (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  gtk_widget_hide (statusbar->cursor_label);
  gtk_widget_hide (statusbar->unit_combo);
  gtk_widget_hide (statusbar->scale_combo);
  gtk_widget_hide (statusbar->rotate_widget);
  gtk_widget_hide (statusbar->horizontal_flip_icon);
  gtk_widget_hide (statusbar->vertical_flip_icon);
  gtk_widget_hide (statusbar->soft_proof_button);
}

void
gimp_statusbar_fill (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  gtk_widget_show (statusbar->cursor_label);
  gtk_widget_show (statusbar->unit_combo);
  gtk_widget_show (statusbar->scale_combo);
  gtk_widget_show (statusbar->rotate_widget);
  gtk_widget_show (statusbar->soft_proof_button);
  gimp_statusbar_shell_rotated (statusbar->shell, statusbar);
}

void
gimp_statusbar_override_window_title (GimpStatusbar *statusbar)
{
  GtkWidget *toplevel;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (statusbar));

  if (gimp_image_window_is_iconified (GIMP_IMAGE_WINDOW (toplevel)))
    {
      const gchar *message = gimp_statusbar_peek (statusbar, "progress");

      if (message)
        gtk_window_set_title (GTK_WINDOW (toplevel), message);
    }
}

void
gimp_statusbar_restore_window_title (GimpStatusbar *statusbar)
{
  GtkWidget *toplevel;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (statusbar));

  if (gimp_image_window_is_iconified (GIMP_IMAGE_WINDOW (toplevel)))
    {
      g_object_notify (G_OBJECT (statusbar->shell), "title");
    }
}

void
gimp_statusbar_push (GimpStatusbar *statusbar,
                     const gchar   *context,
                     const gchar   *icon_name,
                     const gchar   *format,
                     ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_push_valist (statusbar, context, icon_name, format, args);
  va_end (args);
}

void
gimp_statusbar_push_valist (GimpStatusbar *statusbar,
                            const gchar   *context,
                            const gchar   *icon_name,
                            const gchar   *format,
                            va_list        args)
{
  guint context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  gimp_statusbar_add_message (statusbar,
                              context_id,
                              icon_name, format, args,
                              /*  move_to_front =  */ TRUE);
}

void
gimp_statusbar_push_coords (GimpStatusbar       *statusbar,
                            const gchar         *context,
                            const gchar         *icon_name,
                            GimpCursorPrecision  precision,
                            const gchar         *title,
                            gdouble              x,
                            const gchar         *separator,
                            gdouble              y,
                            const gchar         *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);
  g_return_if_fail (separator != NULL);

  if (help == NULL)
    help = "";

  shell = statusbar->shell;

  switch (precision)
    {
    case GIMP_CURSOR_PRECISION_PIXEL_CENTER:
      x = (gint) x;
      y = (gint) y;
      break;

    case GIMP_CURSOR_PRECISION_PIXEL_BORDER:
      x = RINT (x);
      y = RINT (y);
      break;

    case GIMP_CURSOR_PRECISION_SUBPIXEL:
      break;
    }

  if (shell->unit == gimp_unit_pixel ())
    {
      if (precision == GIMP_CURSOR_PRECISION_SUBPIXEL)
        {
          gimp_statusbar_push (statusbar, context,
                               icon_name,
                               statusbar->cursor_format_str_f,
                               title,
                               x,
                               separator,
                               y,
                               help);
        }
      else
        {
          gimp_statusbar_push (statusbar, context,
                               icon_name,
                               statusbar->cursor_format_str,
                               title,
                               (gint) RINT (x),
                               separator,
                               (gint) RINT (y),
                               help);
        }
    }
  else /* show real world units */
    {
      gdouble xres;
      gdouble yres;

      gimp_image_get_resolution (gimp_display_get_image (shell->display),
                                 &xres, &yres);

      gimp_statusbar_push (statusbar, context,
                           icon_name,
                           statusbar->cursor_format_str,
                           title,
                           gimp_pixels_to_units (x, shell->unit, xres),
                           separator,
                           gimp_pixels_to_units (y, shell->unit, yres),
                           help);
    }
}

void
gimp_statusbar_push_length (GimpStatusbar       *statusbar,
                            const gchar         *context,
                            const gchar         *icon_name,
                            const gchar         *title,
                            GimpOrientationType  axis,
                            gdouble              value,
                            const gchar         *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);

  if (help == NULL)
    help = "";

  shell = statusbar->shell;

  if (shell->unit == gimp_unit_pixel ())
    {
      gimp_statusbar_push (statusbar, context,
                           icon_name,
                           statusbar->length_format_str,
                           title,
                           (gint) RINT (value),
                           help);
    }
  else /* show real world units */
    {
      gdouble xres;
      gdouble yres;
      gdouble resolution;

      gimp_image_get_resolution (gimp_display_get_image (shell->display),
                                 &xres, &yres);

      switch (axis)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          resolution = xres;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          resolution = yres;
          break;

        default:
          g_return_if_reached ();
          break;
        }

      gimp_statusbar_push (statusbar, context,
                           icon_name,
                           statusbar->length_format_str,
                           title,
                           gimp_pixels_to_units (value, shell->unit, resolution),
                           help);
    }
}

void
gimp_statusbar_replace (GimpStatusbar *statusbar,
                        const gchar   *context,
                        const gchar   *icon_name,
                        const gchar   *format,
                        ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_replace_valist (statusbar, context, icon_name, format, args);
  va_end (args);
}

void
gimp_statusbar_replace_valist (GimpStatusbar *statusbar,
                               const gchar   *context,
                               const gchar   *icon_name,
                               const gchar   *format,
                               va_list        args)
{
  guint context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  gimp_statusbar_add_message (statusbar,
                              context_id,
                              icon_name, format, args,
                              /*  move_to_front =  */ FALSE);
}

const gchar *
gimp_statusbar_peek (GimpStatusbar *statusbar,
                     const gchar   *context)
{
  GSList *list;
  guint   context_id;

  g_return_val_if_fail (GIMP_IS_STATUSBAR (statusbar), NULL);
  g_return_val_if_fail (context != NULL, NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  for (list = statusbar->messages; list; list = list->next)
    {
      GimpStatusbarMsg *msg = list->data;

      if (msg->context_id == context_id)
        {
          return msg->text;
        }
    }

  return NULL;
}

void
gimp_statusbar_pop (GimpStatusbar *statusbar,
                    const gchar   *context)
{
  guint context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  gimp_statusbar_remove_message (statusbar,
                                 context_id);
}

void
gimp_statusbar_push_temp (GimpStatusbar       *statusbar,
                          GimpMessageSeverity  severity,
                          const gchar         *icon_name,
                          const gchar         *format,
                          ...)
{
  va_list args;

  va_start (args, format);
  gimp_statusbar_push_temp_valist (statusbar, severity, icon_name, format, args);
  va_end (args);
}

void
gimp_statusbar_push_temp_valist (GimpStatusbar       *statusbar,
                                 GimpMessageSeverity  severity,
                                 const gchar         *icon_name,
                                 const gchar         *format,
                                 va_list              args)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (severity <= GIMP_MESSAGE_WARNING);
  g_return_if_fail (format != NULL);

  /*  don't accept a message if we are already displaying a more severe one  */
  if (statusbar->temp_timeout_id && statusbar->temp_severity > severity)
    return;

  if (statusbar->temp_timeout_id)
    g_source_remove (statusbar->temp_timeout_id);

  statusbar->temp_timeout_id =
    g_timeout_add (MESSAGE_TIMEOUT,
                   (GSourceFunc) gimp_statusbar_temp_timeout, statusbar);

  statusbar->temp_severity = severity;

  gimp_statusbar_add_message (statusbar,
                              statusbar->temp_context_id,
                              icon_name, format, args,
                              /*  move_to_front =  */ TRUE);

  if (severity >= GIMP_MESSAGE_WARNING)
    gimp_widget_blink (statusbar->label);
}

void
gimp_statusbar_pop_temp (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  if (statusbar->temp_timeout_id)
    {
      g_source_remove (statusbar->temp_timeout_id);
      statusbar->temp_timeout_id = 0;

      gimp_statusbar_remove_message (statusbar,
                                     statusbar->temp_context_id);
    }
}

void
gimp_statusbar_update_cursor (GimpStatusbar       *statusbar,
                              GimpCursorPrecision  precision,
                              gdouble              x,
                              gdouble              y)
{
  GimpDisplayShell *shell;
  GimpImage        *image;
  gchar             buffer[CURSOR_LEN];

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  shell = statusbar->shell;
  image = gimp_display_get_image (shell->display);

  if (! image                            ||
      x <  0                             ||
      y <  0                             ||
      x >= gimp_image_get_width  (image) ||
      y >= gimp_image_get_height (image))
    {
      gtk_widget_set_sensitive (statusbar->cursor_label, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (statusbar->cursor_label, TRUE);
    }

  switch (precision)
    {
    case GIMP_CURSOR_PRECISION_PIXEL_CENTER:
      x = (gint) x;
      y = (gint) y;
      break;

    case GIMP_CURSOR_PRECISION_PIXEL_BORDER:
      x = RINT (x);
      y = RINT (y);
      break;

    case GIMP_CURSOR_PRECISION_SUBPIXEL:
      break;
    }
  statusbar->cursor_precision = precision;

  if (shell->unit == gimp_unit_pixel ())
    {
      if (precision == GIMP_CURSOR_PRECISION_SUBPIXEL)
        {
          g_snprintf (buffer, sizeof (buffer),
                      statusbar->cursor_format_str_f,
                      "", x, ", ", y, "");
        }
      else
        {
          g_snprintf (buffer, sizeof (buffer),
                      statusbar->cursor_format_str,
                      "", (gint) RINT (x), ", ", (gint) RINT (y), "");
        }
    }
  else /* show real world units */
    {
      GtkTreeModel  *model;
      GimpUnitStore *store;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
      store = GIMP_UNIT_STORE (model);

      gimp_unit_store_set_pixel_values (store, x, y);
      gimp_unit_store_get_values (store, shell->unit, &x, &y);

      g_snprintf (buffer, sizeof (buffer),
                  statusbar->cursor_format_str,
                  "", x, ", ", y, "");
    }

  if (g_strcmp0 (buffer, statusbar->cursor_string_last) == 0)
    return;

  g_free (statusbar->cursor_string_todraw);
  statusbar->cursor_string_todraw = g_strdup (buffer);

  if (statusbar->statusbar_pos_redraw_idle_id == 0)
    {
      statusbar->statusbar_pos_redraw_idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         gimp_statusbar_queue_pos_redraw,
                         statusbar,
                         NULL);
    }
}

void
gimp_statusbar_clear_cursor (GimpStatusbar *statusbar)
{
  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), "");
  gtk_widget_set_sensitive (statusbar->cursor_label, TRUE);
}


/*  private functions  */

static gboolean
gimp_statusbar_label_draw (GtkWidget     *widget,
                           cairo_t       *cr,
                           GimpStatusbar *statusbar)
{
  if (statusbar->icon)
    {
      cairo_surface_t *surface;
      PangoRectangle   rect;
      GtkAllocation    allocation;
      gint             scale_factor;
      gint             x, y;

      gtk_label_get_layout_offsets (GTK_LABEL (widget), &x, &y);

      gtk_widget_get_allocation (widget, &allocation);
      x -= allocation.x;
      y -= allocation.y;

      pango_layout_index_to_pos (gtk_label_get_layout (GTK_LABEL (widget)), 0,
                                 &rect);

      /*  the rectangle width is negative when rendering right-to-left  */
      x += PANGO_PIXELS (rect.x) + (rect.width < 0 ?
                                    PANGO_PIXELS (rect.width) : 0);
      y += PANGO_PIXELS (rect.y);

      scale_factor = gtk_widget_get_scale_factor (widget);
      surface = gdk_cairo_surface_create_from_pixbuf (statusbar->icon,
                                                      scale_factor, NULL);
      cairo_set_source_surface (cr, surface, x, y);
      cairo_surface_destroy (surface);

      cairo_paint (cr);
    }

  return FALSE;
}

static void
gimp_statusbar_shell_scaled (GimpDisplayShell *shell,
                             GimpStatusbar    *statusbar)
{
  static PangoLayout *layout = NULL;

  GimpImage    *image = gimp_display_get_image (shell->display);
  GtkTreeModel *model;
  const gchar  *text;
  gint          image_width;
  gint          image_height;
  gdouble       image_xres;
  gdouble       image_yres;
  gint          width;

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);
      gimp_image_get_resolution (image, &image_xres, &image_yres);
    }
  else
    {
      image_width  = shell->disp_width;
      image_height = shell->disp_height;
      image_xres   = shell->display->config->monitor_xres;
      image_yres   = shell->display->config->monitor_yres;
    }

  g_signal_handlers_block_by_func (statusbar->scale_combo,
                                   gimp_statusbar_scale_changed, statusbar);
  gimp_scale_combo_box_set_scale (GIMP_SCALE_COMBO_BOX (statusbar->scale_combo),
                                  gimp_zoom_model_get_factor (shell->zoom));
  g_signal_handlers_unblock_by_func (statusbar->scale_combo,
                                     gimp_statusbar_scale_changed, statusbar);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  gimp_unit_store_set_resolutions (GIMP_UNIT_STORE (model),
                                   image_xres, image_yres);

  g_signal_handlers_block_by_func (statusbar->unit_combo,
                                   gimp_statusbar_unit_changed, statusbar);
  gimp_unit_combo_box_set_active (GIMP_UNIT_COMBO_BOX (statusbar->unit_combo),
                                  shell->unit);
  g_signal_handlers_unblock_by_func (statusbar->unit_combo,
                                     gimp_statusbar_unit_changed, statusbar);

  if (shell->unit == gimp_unit_pixel ())
    {
      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
                  "%%s%%d%%s%%d%%s");
      g_snprintf (statusbar->cursor_format_str_f,
                  sizeof (statusbar->cursor_format_str_f),
                  "%%s%%.1f%%s%%.1f%%s");
      g_snprintf (statusbar->length_format_str,
                  sizeof (statusbar->length_format_str),
                  "%%s%%d%%s");

      statusbar->cursor_w_digits = 0;
      statusbar->cursor_h_digits = 0;
    }
  else /* show real world units */
    {
      gint w_digits;
      gint h_digits;

      w_digits = gimp_unit_get_scaled_digits (shell->unit, image_xres);
      h_digits = gimp_unit_get_scaled_digits (shell->unit, image_yres);

      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
                  "%%s%%.%df%%s%%.%df%%s",
                  w_digits, h_digits);
      strcpy (statusbar->cursor_format_str_f, statusbar->cursor_format_str);
      g_snprintf (statusbar->length_format_str,
                  sizeof (statusbar->length_format_str),
                  "%%s%%.%df%%s", MAX (w_digits, h_digits));

      statusbar->cursor_w_digits = w_digits;
      statusbar->cursor_h_digits = h_digits;
    }

  gimp_statusbar_update_cursor (statusbar, GIMP_CURSOR_PRECISION_SUBPIXEL,
                                -image_width, -image_height);

  text = gtk_label_get_text (GTK_LABEL (statusbar->cursor_label));

  /* one static layout for all displays should be fine */
  if (! layout)
    layout = gtk_widget_create_pango_layout (statusbar->cursor_label, NULL);

  pango_layout_set_text (layout, text, -1);
  pango_layout_get_pixel_size (layout, &width, NULL);

  gtk_widget_set_size_request (statusbar->cursor_label, width, -1);

  gimp_statusbar_clear_cursor (statusbar);
}

static void
gimp_statusbar_shell_rotated (GimpDisplayShell *shell,
                              GimpStatusbar    *statusbar)
{
  if (shell->rotate_angle != 0.0)
    {
      /* Degree symbol U+00B0. There are no spaces between the value and the
       * unit for angular rotation.
       */
      gchar *text = g_strdup_printf (" %.2f\xC2\xB0", shell->rotate_angle);

      gtk_label_set_text (GTK_LABEL (statusbar->rotate_label), text);
      g_free (text);

      gtk_widget_show (statusbar->rotate_widget);
    }
  else
    {
      gtk_widget_hide (statusbar->rotate_widget);
    }

  if (shell->flip_horizontally)
    gtk_widget_show (statusbar->horizontal_flip_icon);
  else
    gtk_widget_hide (statusbar->horizontal_flip_icon);

  if (shell->flip_vertically)
    gtk_widget_show (statusbar->vertical_flip_icon);
  else
    gtk_widget_hide (statusbar->vertical_flip_icon);
}

static void
gimp_statusbar_shell_status_notify (GimpDisplayShell *shell,
                                    const GParamSpec *pspec,
                                    GimpStatusbar    *statusbar)
{
  gimp_statusbar_replace (statusbar, "title",
                          NULL, "%s", shell->status);
}

static void
gimp_statusbar_shell_color_config_notify (GObject          *config,
                                          const GParamSpec *pspec,
                                          GimpStatusbar    *statusbar)
{
  gboolean                 active       = FALSE;
  gint                     optimize     = 0;
  GimpColorConfig         *color_config = GIMP_COLOR_CONFIG (config);
  GimpColorManagementMode  mode         = gimp_color_config_get_mode (color_config);

  if (mode == GIMP_COLOR_MANAGEMENT_SOFTPROOF)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->soft_proof_button),
                                    TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->proof_colors_toggle),
                                    TRUE);
      gtk_button_set_relief (GTK_BUTTON (statusbar->soft_proof_button),
                             GTK_RELIEF_NORMAL);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->soft_proof_button),
                                    FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->proof_colors_toggle),
                                    FALSE);
      gtk_button_set_relief (GTK_BUTTON (statusbar->soft_proof_button),
                             GTK_RELIEF_NONE);
    }

  optimize = gimp_color_config_get_simulation_optimize (color_config);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (statusbar->optimize_combo),
                                 optimize);

  active = gimp_color_config_get_simulation_gamut_check (color_config);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->out_of_gamut_toggle),
                                active);
}

static void
gimp_statusbar_shell_set_image (GimpStatusbar *statusbar,
                                GimpImage     *image)
{
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  if (statusbar->image)
    g_signal_handlers_disconnect_by_func (statusbar->image,
                                          gimp_statusbar_shell_image_simulation_changed,
                                          statusbar);

  g_set_weak_pointer (&statusbar->image, image);

  if (statusbar->image)
    {
      g_signal_connect (statusbar->image, "simulation-profile-changed",
                        G_CALLBACK (gimp_statusbar_shell_image_simulation_changed),
                        statusbar);
      g_signal_connect (statusbar->image, "simulation-intent-changed",
                        G_CALLBACK (gimp_statusbar_shell_image_simulation_changed),
                        statusbar);
      g_signal_connect (statusbar->image, "simulation-bpc-changed",
                        G_CALLBACK (gimp_statusbar_shell_image_simulation_changed),
                        statusbar);

      gimp_statusbar_shell_image_simulation_changed (statusbar->image,
                                                     statusbar);
    }
}

static void
gimp_statusbar_unit_changed (GimpUnitComboBox *combo,
                             GimpStatusbar    *statusbar)
{
  gimp_display_shell_set_unit (statusbar->shell,
                               gimp_unit_combo_box_get_active (combo));
}

static void
gimp_statusbar_scale_changed (GimpScaleComboBox *combo,
                              GimpStatusbar     *statusbar)
{
  gimp_display_shell_scale (statusbar->shell,
                            GIMP_ZOOM_TO,
                            gimp_scale_combo_box_get_scale (combo),
                            GIMP_ZOOM_FOCUS_BEST_GUESS);
}

static void
gimp_statusbar_scale_activated (GimpScaleComboBox *combo,
                                GimpStatusbar     *statusbar)
{
  gtk_widget_grab_focus (statusbar->shell->canvas);
}

static void
gimp_statusbar_shell_image_changed (GimpStatusbar *statusbar,
                                    GimpImage     *image,
                                    GimpContext   *context)
{
  GimpColorConfig *color_config = NULL;

  if (image == statusbar->image)
    return;

  if (statusbar->shell && statusbar->shell->display)
    color_config = gimp_display_shell_get_color_config (statusbar->shell);

  gimp_statusbar_shell_set_image (statusbar, image);

  gimp_statusbar_shell_color_config_notify (G_OBJECT (color_config), NULL,
                                            statusbar);
}

static void
gimp_statusbar_shell_image_simulation_changed (GimpImage     *image,
                                               GimpStatusbar *statusbar)
{
  GimpColorProfile        *simulation_profile = NULL;
  gchar                   *text;
  const gchar             *profile_label;
  GimpColorRenderingIntent intent             =
    GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  gboolean                 bpc                = FALSE;

  if (image)
    {
      simulation_profile = gimp_image_get_simulation_profile (image);
      intent             = gimp_image_get_simulation_intent (image);
      bpc                = gimp_image_get_simulation_bpc (image);
    }

  if (simulation_profile && GIMP_IS_COLOR_PROFILE (simulation_profile))
    {
      profile_label = gimp_color_profile_get_label (simulation_profile);
      gtk_widget_set_sensitive (statusbar->soft_proof_button, TRUE);
      gtk_widget_set_sensitive (statusbar->proof_colors_toggle, TRUE);
    }
  else
    {
      profile_label = _("None");
      gtk_widget_set_sensitive (statusbar->soft_proof_button, FALSE);
      gtk_widget_set_sensitive (statusbar->proof_colors_toggle, FALSE);
    }
  gtk_event_box_set_above_child (GTK_EVENT_BOX (statusbar->soft_proof_container), TRUE);

  text = g_strdup_printf ("<b>%s</b>: %s",
                          _("Current Soft-Proofing Profile"),
                          profile_label);
  gtk_label_set_markup (GTK_LABEL (statusbar->profile_label), text);
  g_free (text);

  gimp_color_profile_combo_box_set_active_profile (GIMP_COLOR_PROFILE_COMBO_BOX (statusbar->profile_combo),
                                                   simulation_profile);
  gtk_combo_box_set_active (GTK_COMBO_BOX (statusbar->rendering_intent_combo),
                            intent);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (statusbar->bpc_toggle),
                                bpc);
}

static gboolean
gimp_statusbar_rotate_pressed (GtkWidget     *event_box,
                               GdkEvent      *event,
                               GimpStatusbar *statusbar)
{
  GAction *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (statusbar->gimp->app),
                                       "view-rotate-other");
  gimp_action_activate (GIMP_ACTION (action));

  return FALSE;
}

static gboolean
gimp_statusbar_horiz_flip_pressed (GtkWidget     *event_box,
                                   GdkEvent      *event,
                                   GimpStatusbar *statusbar)
{
  GAction *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (statusbar->gimp->app),
                                       "view-flip-horizontally");
  gimp_action_activate (GIMP_ACTION (action));

  return FALSE;
}

static gboolean
gimp_statusbar_vert_flip_pressed (GtkWidget     *event_box,
                                  GdkEvent      *event,
                                  GimpStatusbar *statusbar)
{
  GAction *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (statusbar->gimp->app),
                                       "view-flip-vertically");
  gimp_action_activate (GIMP_ACTION (action));

  return FALSE;
}

static guint
gimp_statusbar_get_context_id (GimpStatusbar *statusbar,
                               const gchar   *context)
{
  guint id = GPOINTER_TO_UINT (g_hash_table_lookup (statusbar->context_ids,
                                                    context));

  if (! id)
    {
      id = statusbar->seq_context_id++;

      g_hash_table_insert (statusbar->context_ids,
                           g_strdup (context), GUINT_TO_POINTER (id));
    }

  return id;
}

static gboolean
gimp_statusbar_temp_timeout (GimpStatusbar *statusbar)
{
  gimp_statusbar_pop_temp (statusbar);

  return FALSE;
}

static void
gimp_statusbar_add_size_widget (GimpStatusbar *statusbar,
                                GtkWidget     *widget)
{
  statusbar->size_widgets = g_slist_prepend (statusbar->size_widgets, widget);
}

static void
gimp_statusbar_update_size (GimpStatusbar *statusbar)
{
  GSList *iter;
  gint    height = -1;

  for (iter = statusbar->size_widgets; iter; iter = g_slist_next (iter))
    {
      GtkWidget *widget = iter->data;
      gint       natural_height;
      gboolean   visible;

      visible = gtk_widget_get_visible (widget);
      gtk_widget_set_visible (widget, TRUE);

      gtk_widget_get_preferred_height (widget, NULL, &natural_height);

      gtk_widget_set_visible (widget, visible);

      height = MAX (height, natural_height);
    }

  gtk_widget_set_size_request (GTK_WIDGET (statusbar), -1, height);
}

static void
gimp_statusbar_add_message (GimpStatusbar *statusbar,
                            guint          context_id,
                            const gchar   *icon_name,
                            const gchar   *format,
                            va_list        args,
                            gboolean       move_to_front)
{
  gchar            *message;
  GSList           *list;
  GimpStatusbarMsg *msg;
  gint              position;

  message = gimp_statusbar_vprintf (format, args);

  for (list = statusbar->messages; list; list = g_slist_next (list))
    {
      msg = list->data;

      if (msg->context_id == context_id)
        {
          gboolean is_front_message = (list == statusbar->messages);

          if ((is_front_message || ! move_to_front) &&
              strcmp (msg->text, message) == 0      &&
              g_strcmp0 (msg->icon_name, icon_name) == 0)
            {
              g_free (message);
              return;
            }

          if (move_to_front)
            {
              statusbar->messages = g_slist_remove (statusbar->messages, msg);
              gimp_statusbar_msg_free (msg);

              break;
            }
          else
            {
              g_free (msg->icon_name);
              msg->icon_name = g_strdup (icon_name);

              g_free (msg->text);
              msg->text = message;

              if (is_front_message)
                gimp_statusbar_update (statusbar);

              return;
            }
        }
    }

  msg = g_slice_new (GimpStatusbarMsg);

  msg->context_id = context_id;
  msg->icon_name  = g_strdup (icon_name);
  msg->text       = message;

  /*  find the position at which to insert the new message  */
  position = 0;
  /*  progress messages are always at the front of the list  */
  if (! (statusbar->progress_active &&
         context_id == gimp_statusbar_get_context_id (statusbar, "progress")))
    {
      if (statusbar->progress_active)
        position++;

      /*  temporary messages are in front of all other non-progress messages  */
      if (statusbar->temp_timeout_id &&
          context_id != statusbar->temp_context_id)
        position++;
    }

  statusbar->messages = g_slist_insert (statusbar->messages, msg, position);

  if (position == 0)
    gimp_statusbar_update (statusbar);
}

static void
gimp_statusbar_remove_message (GimpStatusbar *statusbar,
                               guint          context_id)
{
  GSList   *list;
  gboolean  needs_update = FALSE;

  for (list = statusbar->messages; list; list = g_slist_next (list))
    {
      GimpStatusbarMsg *msg = list->data;

      if (msg->context_id == context_id)
        {
          needs_update = (list == statusbar->messages);

          statusbar->messages = g_slist_remove (statusbar->messages, msg);
          gimp_statusbar_msg_free (msg);

          break;
        }
    }

  if (needs_update)
    gimp_statusbar_update (statusbar);
}

static void
gimp_statusbar_msg_free (GimpStatusbarMsg *msg)
{
  g_free (msg->icon_name);
  g_free (msg->text);

  g_slice_free (GimpStatusbarMsg, msg);
}

static gchar *
gimp_statusbar_vprintf (const gchar *format,
                        va_list      args)
{
  gchar *message;
  gchar *newline;

  message = g_strdup_vprintf (format, args);

  /*  guard us from multi-line strings  */
  newline = strchr (message, '\r');
  if (newline)
    *newline = '\0';

  newline = strchr (message, '\n');
  if (newline)
    *newline = '\0';

  return message;
}

static GdkPixbuf *
gimp_statusbar_load_icon (GimpStatusbar *statusbar,
                          const gchar   *icon_name)
{
  GdkPixbuf *icon;
  gint       pixel_size;

  if (G_UNLIKELY (! statusbar->icon_hash))
    {
      statusbar->icon_hash =
        g_hash_table_new_full (g_str_hash,
                               g_str_equal,
                               (GDestroyNotify) g_free,
                               (GDestroyNotify) g_object_unref);
    }

  icon = g_hash_table_lookup (statusbar->icon_hash, icon_name);

  if (icon)
    return g_object_ref (icon);

  gtk_icon_size_lookup (statusbar->icon_size, &pixel_size, NULL);

  icon = gimp_widget_load_icon (statusbar->label, icon_name,
                                pixel_size);

  /* this is not optimal but so what */
  if (g_hash_table_size (statusbar->icon_hash) > pixel_size)
    g_hash_table_remove_all (statusbar->icon_hash);

  g_hash_table_insert (statusbar->icon_hash,
                       g_strdup (icon_name), g_object_ref (icon));

  return icon;
}

static gboolean
gimp_statusbar_queue_pos_redraw (gpointer data)
{
  GimpStatusbar    *statusbar = GIMP_STATUSBAR (data);
  GimpDisplayShell *shell;
  GimpImage        *image;
  gint              image_width       = 0;
  gint              image_height      = 0;
  gint              label_width_chars = 2;

  shell = statusbar->shell;
  image = gimp_display_get_image (shell->display);

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);

      /* The number of chars within up to 2 times the image bounds is:
       * - max width chars: floor (log10 (2 * max_width)) + 1
       * - max height chars: floor (log10 (2 * max_height)) + 1
       * - the comma and a space: + 2
       * - optional 2 minus characters when going in negative
       *   dimensions: + 2
       * - optional decimal separators and digits when showing
       *   fractional values.
       *
       * The goal of this is to avoid the label size jumping up and
       * down. Actually it was not a problem on Linux, but this was
       * reported on macOS.
       * See: https://gitlab.gnome.org/GNOME/gimp/-/merge_requests/572#note_1389445
       * So we just compute what looks like a reasonable "biggest size"
       * in worst cases.
       * Of course, it could still happen for people going way
       * off-canvas but that's acceptable edge-case.
       */
      if (shell->unit == gimp_unit_pixel ())
        {
          label_width_chars = floor (log10 (2 * image_width)) + floor (log10 (2 * image_height)) + 6;

          if (statusbar->cursor_precision == GIMP_CURSOR_PRECISION_SUBPIXEL)
            /* In subpixel precision, we have a 1 digit precision per
             * dimension, so 4 additional characters, decimal
             * separators-included.
             */
            label_width_chars += 4;
        }
      else /* showing real world units */
        {
          GtkTreeModel  *model;
          GimpUnitStore *store;
          gdouble        max_x = (gdouble) image_width;
          gdouble        max_y = (gdouble) image_height;

          model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
          store = GIMP_UNIT_STORE (model);

          gimp_unit_store_set_pixel_values (store, max_x, max_y);
          gimp_unit_store_get_values (store, shell->unit, &max_x, &max_y);

          label_width_chars = /* max width (int) up to 2 times image bounds - 1  */
                              floor (log10 (2 * max_x))                                             +
                              /* max height (int) up to 2 times image bounds - 1 */
                              floor (log10 (2 * max_y))                                             +
                              /* + 2 + comma + space + 2 minuses                 */
                              6                                                                     +
                              /* 2 (optional) decimal separators + digits.       */
                              (statusbar->cursor_w_digits > 0 ? 1 + statusbar->cursor_w_digits : 0) +
                              (statusbar->cursor_h_digits > 0 ? 1 + statusbar->cursor_h_digits : 0);
        }
    }

  g_free (statusbar->cursor_string_last);
  gtk_label_set_width_chars (GTK_LABEL (statusbar->cursor_label), label_width_chars);
  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), statusbar->cursor_string_todraw);
  statusbar->cursor_string_last = statusbar->cursor_string_todraw;
  statusbar->cursor_string_todraw = NULL;

  statusbar->statusbar_pos_redraw_idle_id = 0;

  return G_SOURCE_REMOVE;
}
