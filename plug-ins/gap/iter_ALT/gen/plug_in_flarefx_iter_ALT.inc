/* ----------------------------------------------------------------------
 * p_plug_in_flarefx_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_flarefx_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_flarefx_Vals 
    {
      long      posx;
      long      posy;
    } t_plug_in_flarefx_Vals; 

    t_plug_in_flarefx_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_flarefx_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_flarefx_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_flarefx_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_flarefx_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_flarefx_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_flarefx_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_flarefx_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.posx, buf_from->posx, buf_to->posx, total_steps, current_step);
    p_delta_long(&buf.posy, buf_from->posy, buf_to->posy, total_steps, current_step);

    gimp_set_data("plug_in_flarefx", &buf, sizeof(buf)); 

    return 0; /* OK */
}
