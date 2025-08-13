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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_PICK_BUTTON_H__
#define __GIMP_PICK_BUTTON_H__

G_BEGIN_DECLS


#define GIMP_TYPE_PICK_BUTTON (gimp_pick_button_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpPickButton, gimp_pick_button, GIMP, PICK_BUTTON, GtkButton)

struct _GimpPickButtonClass
{
  GtkButtonClass  parent_class;

  void (* color_picked) (GimpPickButton  *button,
                         const GeglColor *color);

  /* Padding for future expansion */
  void (* _gimp_reserved0) (void);
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
  void (* _gimp_reserved9) (void);
};


GtkWidget * gimp_pick_button_new      (void);

/*  for internal use only  */
G_GNUC_INTERNAL void _gimp_pick_button_default_pick (GimpPickButton *button);


G_END_DECLS

#endif /* __GIMP_PICK_BUTTON_H__ */
