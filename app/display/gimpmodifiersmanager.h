/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamodifiersmanager.h
 * Copyright (C) 2022 Jehan
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

#ifndef __LIGMA_MODIFIERS_MANAGER_H__
#define __LIGMA_MODIFIERS_MANAGER_H__


#define LIGMA_TYPE_MODIFIERS_MANAGER            (ligma_modifiers_manager_get_type ())
#define LIGMA_MODIFIERS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MODIFIERS_MANAGER, LigmaModifiersManager))
#define LIGMA_MODIFIERS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MODIFIERS_MANAGER, LigmaModifiersManagerClass))
#define LIGMA_IS_MODIFIERS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MODIFIERS_MANAGER))
#define LIGMA_IS_MODIFIERS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MODIFIERS_MANAGER))
#define LIGMA_MODIFIERS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MODIFIERS_MANAGER, LigmaModifiersManagerClass))


typedef struct _LigmaModifiersManagerPrivate  LigmaModifiersManagerPrivate;
typedef struct _LigmaModifiersManagerClass    LigmaModifiersManagerClass;

/**
 * LigmaModifiersManager:
 *
 * Contains modifiers configuration for canvas interaction.
 */
struct _LigmaModifiersManager
{
  GObject                      parent_instance;

  LigmaModifiersManagerPrivate *p;
};

struct _LigmaModifiersManagerClass
{
  GObjectClass                 parent_class;
};


GType                  ligma_modifiers_manager_get_type      (void) G_GNUC_CONST;

LigmaModifiersManager * ligma_modifiers_manager_new           (void);

LigmaModifierAction     ligma_modifiers_manager_get_action    (LigmaModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers,
                                                             const gchar         **action_desc);

/* Protected functions: only use them from LigmaModifiersEditor */

GList                * ligma_modifiers_manager_get_modifiers (LigmaModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button);

void                   ligma_modifiers_manager_set           (LigmaModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers,
                                                             LigmaModifierAction    action,
                                                             const gchar          *action_desc);
void                   ligma_modifiers_manager_remove        (LigmaModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers);
void                   ligma_modifiers_manager_clear         (LigmaModifiersManager *manager);


#endif  /* __LIGMA_MODIFIERS_MANAGER_H__ */
