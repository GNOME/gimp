/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodifiersmanager.h
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

#ifndef __GIMP_MODIFIERS_MANAGER_H__
#define __GIMP_MODIFIERS_MANAGER_H__


#define GIMP_TYPE_MODIFIERS_MANAGER            (gimp_modifiers_manager_get_type ())
#define GIMP_MODIFIERS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MODIFIERS_MANAGER, GimpModifiersManager))
#define GIMP_MODIFIERS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MODIFIERS_MANAGER, GimpModifiersManagerClass))
#define GIMP_IS_MODIFIERS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MODIFIERS_MANAGER))
#define GIMP_IS_MODIFIERS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MODIFIERS_MANAGER))
#define GIMP_MODIFIERS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MODIFIERS_MANAGER, GimpModifiersManagerClass))


typedef struct _GimpModifiersManagerPrivate  GimpModifiersManagerPrivate;
typedef struct _GimpModifiersManagerClass    GimpModifiersManagerClass;

/**
 * GimpModifiersManager:
 *
 * Contains modifiers configuration for canvas interaction.
 */
struct _GimpModifiersManager
{
  GObject                      parent_instance;

  GimpModifiersManagerPrivate *p;
};

struct _GimpModifiersManagerClass
{
  GObjectClass                 parent_class;
};


GType                  gimp_modifiers_manager_get_type      (void) G_GNUC_CONST;

GimpModifiersManager * gimp_modifiers_manager_new           (void);

GimpModifierAction     gimp_modifiers_manager_get_action    (GimpModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers,
                                                             const gchar         **action_desc);

/* Protected functions: only use them from GimpModifiersEditor */

GList                * gimp_modifiers_manager_get_modifiers (GimpModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button);

void                   gimp_modifiers_manager_set           (GimpModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers,
                                                             GimpModifierAction    action,
                                                             const gchar          *action_desc);
void                   gimp_modifiers_manager_remove        (GimpModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers);
void                   gimp_modifiers_manager_clear         (GimpModifiersManager *manager);


#endif  /* __GIMP_MODIFIERS_MANAGER_H__ */
