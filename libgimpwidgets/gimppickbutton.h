/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapickbutton.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_PICK_BUTTON_H__
#define __LIGMA_PICK_BUTTON_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_PICK_BUTTON            (ligma_pick_button_get_type ())
#define LIGMA_PICK_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PICK_BUTTON, LigmaPickButton))
#define LIGMA_PICK_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PICK_BUTTON, LigmaPickButtonClass))
#define LIGMA_IS_PICK_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PICK_BUTTON))
#define LIGMA_IS_PICK_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PICK_BUTTON))
#define LIGMA_PICK_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PICK_BUTTON, LigmaPickButtonClass))


typedef struct _LigmaPickButtonPrivate LigmaPickButtonPrivate;
typedef struct _LigmaPickButtonClass   LigmaPickButtonClass;

struct _LigmaPickButton
{
  GtkButton              parent_instance;

  LigmaPickButtonPrivate *priv;
};

struct _LigmaPickButtonClass
{
  GtkButtonClass  parent_class;

  void (* color_picked) (LigmaPickButton *button,
                         const LigmaRGB  *color);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_pick_button_get_type (void) G_GNUC_CONST;
GtkWidget * ligma_pick_button_new      (void);


G_END_DECLS

#endif /* __LIGMA_PICK_BUTTON_H__ */
