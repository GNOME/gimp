/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on gtk+/gtk/gtkcolorsel.c
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcairo-utils.h"
#include "gimphelpui.h"
#include "gimpicons.h"
#include "gimppickbutton.h"
#include "gimppickbutton-default.h"
#include "gimppickbutton-kwin.h"

#ifdef GDK_WINDOWING_QUARTZ
#include "gimppickbutton-quartz.h"
#endif

#include "libgimp/libgimp-intl.h"

/**
 * SECTION: gimppickbutton
 * @title: GimpPickButton
 * @short_description: Widget to pick a color from screen.
 *
 * #GimpPickButton is a specialized button. When clicked, it changes
 * the cursor to a color-picker pipette and allows the user to pick a
 * color from any point on the screen.
 **/


enum
{
  COLOR_PICKED,
  LAST_SIGNAL
};

static void       gimp_pick_button_dispose         (GObject        *object);

static void       gimp_pick_button_clicked         (GtkButton      *button);


G_DEFINE_TYPE (GimpPickButton, gimp_pick_button, GTK_TYPE_BUTTON)

#define parent_class gimp_pick_button_parent_class

static guint pick_button_signals[LAST_SIGNAL] = { 0 };


static void
gimp_pick_button_class_init (GimpPickButtonClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  /**
   * GimpPickButton::color-picked:
   * @gimppickbutton: the object which received the signal.
   * @arg1: pointer to a #GimpRGB structure that holds the picked color
   *
   * This signal is emitted when the user has picked a color.
   **/
  pick_button_signals[COLOR_PICKED] =
    g_signal_new ("color-picked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPickButtonClass, color_picked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->dispose = gimp_pick_button_dispose;

  button_class->clicked = gimp_pick_button_clicked;

  klass->color_picked   = NULL;
}

static void
gimp_pick_button_init (GimpPickButton *button)
{
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (GIMP_ICON_COLOR_PICK_FROM_SCREEN,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (GTK_WIDGET (button),
                           _("Click the eyedropper, then click a color "
                             "anywhere on your screen to select that color."),
                           NULL);
}

static void
gimp_pick_button_dispose (GObject *object)
{
  GimpPickButton *button = GIMP_PICK_BUTTON (object);

  if (button->cursor)
    {
      g_object_unref (button->cursor);
      button->cursor = NULL;
    }

  if (button->grab_widget)
    {
      gtk_widget_destroy (button->grab_widget);
      button->grab_widget = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_pick_button_clicked (GtkButton *button)
{
#ifdef GDK_WINDOWING_QUARTZ
  _gimp_pick_button_quartz_pick (GIMP_PICK_BUTTON (button));
#else
  if (_gimp_pick_button_kwin_available ())
    _gimp_pick_button_kwin_pick (GIMP_PICK_BUTTON (button));
  else
    _gimp_pick_button_default_pick (GIMP_PICK_BUTTON (button));
#endif
}


/*  public functions  */

/**
 * gimp_pick_button_new:
 *
 * Creates a new #GimpPickButton widget.
 *
 * Returns: A new #GimpPickButton widget.
 **/
GtkWidget *
gimp_pick_button_new (void)
{
  return g_object_new (GIMP_TYPE_PICK_BUTTON, NULL);
}
