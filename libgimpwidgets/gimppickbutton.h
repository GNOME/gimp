/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on gtk-2-0/gtk/gtkcolorsel.c
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_PICK_BUTTON_H__
#define __GIMP_PICK_BUTTON_H__

G_BEGIN_DECLS


#define GIMP_TYPE_PICK_BUTTON            (gimp_pick_button_get_type ())
#define GIMP_PICK_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PICK_BUTTON, GimpPickButton))
#define GIMP_PICK_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PICK_BUTTON, GimpPickButtonClass))
#define GIMP_IS_PICK_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PICK_BUTTON))
#define GIMP_IS_PICK_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PICK_BUTTON))
#define GIMP_PICK_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PICK_BUTTON, GimpPickButtonClass))


typedef struct _GimpPickButtonPrivate GimpPickButtonPrivate;
typedef struct _GimpPickButtonClass   GimpPickButtonClass;

struct _GimpPickButton
{
  GtkButton              parent_instance;

  GimpPickButtonPrivate *priv;
};

struct _GimpPickButtonClass
{
  GtkButtonClass  parent_class;

  void (* color_picked) (GimpPickButton *button,
                         const GimpRGB  *color);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_pick_button_get_type (void) G_GNUC_CONST;
GtkWidget * gimp_pick_button_new      (void);


G_END_DECLS

#endif /* __GIMP_PICK_BUTTON_H__ */
