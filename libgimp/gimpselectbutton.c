/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaselectbutton.c
 * Copyright (C) 2003  Sven Neumann  <sven@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#include "config.h"

#include <gtk/gtk.h>

#include "ligma.h"

#include "ligmauitypes.h"
#include "ligmaselectbutton.h"


/**
 * SECTION: ligmaselectbutton
 * @title: LigmaSelectButton
 * @short_description: The base class of the data select buttons.
 *
 * The base class of the brush, pattern, gradient, palette and font
 * select buttons.
 **/


/*  local function prototypes  */

static void   ligma_select_button_dispose (GObject *object);


G_DEFINE_TYPE (LigmaSelectButton, ligma_select_button, GTK_TYPE_BOX)


static void
ligma_select_button_class_init (LigmaSelectButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_select_button_dispose;
}

static void
ligma_select_button_init (LigmaSelectButton *select_button)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (select_button),
                                  GTK_ORIENTATION_HORIZONTAL);
}

static void
ligma_select_button_dispose (GObject *object)
{
  ligma_select_button_close_popup (LIGMA_SELECT_BUTTON (object));

  G_OBJECT_CLASS (ligma_select_button_parent_class)->dispose (object);
}

/**
 * ligma_select_button_close_popup:
 * @button: A #LigmaSelectButton
 *
 * Closes the popup window associated with @button.
 *
 * Since: 2.4
 */
void
ligma_select_button_close_popup (LigmaSelectButton *button)
{
  g_return_if_fail (LIGMA_IS_SELECT_BUTTON (button));

  if (button->temp_callback)
    {
      LigmaSelectButtonClass *klass = LIGMA_SELECT_BUTTON_GET_CLASS (button);

      klass->select_destroy (button->temp_callback);

      button->temp_callback = NULL;
    }
}
