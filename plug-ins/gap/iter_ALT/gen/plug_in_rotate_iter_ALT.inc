/* ----------------------------------------------------------------------
 * p_plug_in_rotate_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_rotate_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_rotate_Vals 
    {
      long      angle;
      long      everything;
    } t_plug_in_rotate_Vals; 

    t_plug_in_rotate_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_rotate_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_rotate_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_rotate_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_rotate_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_rotate_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_rotate_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_rotate_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.angle, buf_from->angle, buf_to->angle, total_steps, current_step);
    p_delta_long(&buf.everything, buf_from->everything, buf_to->everything, total_steps, current_step);

    gimp_set_data("plug_in_rotate", &buf, sizeof(buf)); 

    return 0; /* OK */
}
