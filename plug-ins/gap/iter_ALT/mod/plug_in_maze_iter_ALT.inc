/* ----------------------------------------------------------------------
 * p_plug_in_maze_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_maze_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
     typedef enum {
          DEPTH_FIRST,
          PRIMS_ALGORITHM
     } MazeAlgoType;

    typedef struct t_plug_in_maze_Vals 
    {
       gint width;
       gint height;
       guint seed;
       gboolean tile;
       gint multiple;
       gint offset;
       MazeAlgoType algorithm;
       /* Interface options. */
       gboolean timeseed;
    } t_plug_in_maze_Vals; 

    t_plug_in_maze_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_maze_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_maze_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_maze_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_maze_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_maze_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_maze_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_maze_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.width, buf_from->width, buf_to->width, total_steps, current_step);
    p_delta_gint(&buf.height, buf_from->height, buf_to->height, total_steps, current_step);
    p_delta_gint(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);
    p_delta_gint(&buf.multiple, buf_from->multiple, buf_to->multiple, total_steps, current_step);
    p_delta_gint(&buf.offset, buf_from->offset, buf_to->offset, total_steps, current_step);

    gimp_set_data("plug_in_maze", &buf, sizeof(buf)); 

    return 0; /* OK */
}
