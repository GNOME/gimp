/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-parallel.h
 * Copyright (C) 2018 Ell
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PARALLEL_H__
#define __GIMP_PARALLEL_H__


typedef void (* GimpParallelRunAsyncFunc)        (GimpAsync            *async,
                                                  gpointer              user_data);

typedef void (* GimpParallelDistributeFunc)      (gint                 i,
                                                  gint                 n,
                                                  gpointer             user_data);
typedef void (* GimpParallelDistributeRangeFunc) (gsize                offset,
                                                  gsize                size,
                                                  gpointer             user_data);
typedef void (* GimpParallelDistributeAreaFunc)  (const GeglRectangle *area,
                                                  gpointer             user_data);


void        gimp_parallel_init                  (Gimp                            *gimp);
void        gimp_parallel_exit                  (Gimp                            *gimp);

GimpAsync * gimp_parallel_run_async             (GimpParallelRunAsyncFunc         func,
                                                 gpointer                         user_data);
GimpAsync * gimp_parallel_run_async_full        (gint                             priority,
                                                 GimpParallelRunAsyncFunc         func,
                                                 gpointer                         user_data,
                                                 GDestroyNotify                   user_data_destroy_func);
GimpAsync * gimp_parallel_run_async_independent (GimpParallelRunAsyncFunc         func,
                                                 gpointer                         user_data);

void        gimp_parallel_distribute            (gint                             max_n,
                                                 GimpParallelDistributeFunc       func,
                                                 gpointer                         user_data);
void        gimp_parallel_distribute_range      (gsize                            size,
                                                 gsize                            min_sub_size,
                                                 GimpParallelDistributeRangeFunc  func,
                                                 gpointer                         user_data);
void        gimp_parallel_distribute_area       (const GeglRectangle             *area,
                                                 gsize                            min_sub_area,
                                                 GimpParallelDistributeAreaFunc   func,
                                                 gpointer                         user_data);

#ifdef __cplusplus

extern "C++"
{

#include <new>

template <class ParallelRunAsyncFunc>
inline GimpAsync *
gimp_parallel_run_async (ParallelRunAsyncFunc func)
{
  ParallelRunAsyncFunc *func_copy = g_new (ParallelRunAsyncFunc, 1);

  new (func_copy) ParallelRunAsyncFunc (func);

  return gimp_parallel_run_async_full (0,
                                       [] (GimpAsync *async,
                                           gpointer   user_data)
                                       {
                                         ParallelRunAsyncFunc *func_copy =
                                           (ParallelRunAsyncFunc *) user_data;

                                         (*func_copy) (async);

                                         func_copy->~ParallelRunAsyncFunc ();
                                         g_free (func_copy);
                                       },
                                       func_copy,
                                       [] (gpointer user_data)
                                       {
                                         ParallelRunAsyncFunc *func_copy =
                                           (ParallelRunAsyncFunc *) user_data;

                                         func_copy->~ParallelRunAsyncFunc ();
                                         g_free (func_copy);
                                       });
}

template <class ParallelRunAsyncFunc,
          class DestroyFunc>
inline GimpAsync *
gimp_parallel_run_async_full (gint                 priority,
                              ParallelRunAsyncFunc func,
                              DestroyFunc          destroy_func)
{
  typedef struct
  {
    ParallelRunAsyncFunc func;
    DestroyFunc          destroy_func;
  } Funcs;

  Funcs *funcs_copy = g_new (Funcs, 1);

  new (funcs_copy) Funcs {func, destroy_func};

  return gimp_parallel_run_async_full (priority,
                                       [] (GimpAsync *async,
                                           gpointer   user_data)
                                       {
                                         Funcs *funcs_copy =
                                           (Funcs *) user_data;

                                         funcs_copy->func (async);

                                         funcs_copy->~Funcs ();
                                         g_free (funcs_copy);
                                       },
                                       funcs_copy,
                                       [] (gpointer user_data)
                                       {
                                         Funcs *funcs_copy =
                                           (Funcs *) user_data;

                                         funcs_copy->destroy_func ();

                                         funcs_copy->~Funcs ();
                                         g_free (funcs_copy);
                                       });
}

template <class ParallelRunAsyncFunc>
inline GimpAsync *
gimp_parallel_run_async_independent (ParallelRunAsyncFunc func)
{
  ParallelRunAsyncFunc *func_copy = g_new (ParallelRunAsyncFunc, 1);

  new (func_copy) ParallelRunAsyncFunc (func);

  return gimp_parallel_run_async_independent ([] (GimpAsync *async,
                                                  gpointer   user_data)
                                              {
                                                ParallelRunAsyncFunc *func_copy =
                                                  (ParallelRunAsyncFunc *) user_data;

                                                (*func_copy) (async);

                                                func_copy->~ParallelRunAsyncFunc ();
                                                g_free (func_copy);
                                              },
                                              func_copy);
}

template <class ParallelDistributeFunc>
inline void
gimp_parallel_distribute (gint                   max_n,
                          ParallelDistributeFunc func)
{
  gimp_parallel_distribute (max_n,
                            [] (gint     i,
                                gint     n,
                                gpointer user_data)
                            {
                              ParallelDistributeFunc func_copy (
                                *(const ParallelDistributeFunc *) user_data);

                              func_copy (i, n);
                            },
                            &func);
}

template <class ParallelDistributeRangeFunc>
inline void
gimp_parallel_distribute_range (gsize                       size,
                                gsize                       min_sub_size,
                                ParallelDistributeRangeFunc func)
{
  gimp_parallel_distribute_range (size, min_sub_size,
                                  [] (gsize    offset,
                                      gsize    size,
                                      gpointer user_data)
                                  {
                                    ParallelDistributeRangeFunc func_copy (
                                      *(const ParallelDistributeRangeFunc *) user_data);

                                    func_copy (offset, size);
                                  },
                                  &func);
}

template <class ParallelDistributeAreaFunc>
inline void
gimp_parallel_distribute_area (const GeglRectangle        *area,
                               gsize                       min_sub_area,
                               ParallelDistributeAreaFunc  func)
{
  gimp_parallel_distribute_area (area, min_sub_area,
                                 [] (const GeglRectangle *area,
                                     gpointer             user_data)
                                 {
                                   ParallelDistributeAreaFunc func_copy (
                                     *(const ParallelDistributeAreaFunc *) user_data);

                                   func_copy (area);
                                 },
                                 &func);
}

}

#endif /* __cplusplus */


#endif /* __GIMP_PARALLEL_H__ */
