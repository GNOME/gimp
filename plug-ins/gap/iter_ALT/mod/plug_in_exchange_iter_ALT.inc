/* ----------------------------------------------------------------------
 * p_plug_in_exchange_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_exchange_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_exchange_Vals 
    {
      guchar      fromred;
      guchar      fromgreen;
      guchar      fromblue;
      guchar      tored;
      guchar      togreen;
      guchar      toblue;
      guchar      red_threshold;
      guchar      green_threshold;
      guchar      blue_threshold;
      gint32      image;
      gint32      drawable;
    } t_plug_in_exchange_Vals; 

    t_plug_in_exchange_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_exchange_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_exchange_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_exchange_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_exchange_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_exchange_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_exchange_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_exchange_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_guchar(&buf.fromred, buf_from->fromred, buf_to->fromred, total_steps, current_step);
    p_delta_guchar(&buf.fromgreen, buf_from->fromgreen, buf_to->fromgreen, total_steps, current_step);
    p_delta_guchar(&buf.fromblue, buf_from->fromblue, buf_to->fromblue, total_steps, current_step);
    p_delta_guchar(&buf.tored, buf_from->tored, buf_to->tored, total_steps, current_step);
    p_delta_guchar(&buf.togreen, buf_from->togreen, buf_to->togreen, total_steps, current_step);
    p_delta_guchar(&buf.toblue, buf_from->toblue, buf_to->toblue, total_steps, current_step);
    p_delta_guchar(&buf.red_threshold, buf_from->red_threshold, buf_to->red_threshold, total_steps, current_step);
    p_delta_guchar(&buf.green_threshold, buf_from->green_threshold, buf_to->green_threshold, total_steps, current_step);
    p_delta_guchar(&buf.blue_threshold, buf_from->blue_threshold, buf_to->blue_threshold, total_steps, current_step);

    /* exchange takes FROM colors from foreground when called in mode RUN_WITH_LAST_VALS */
    gimp_palette_set_foreground(buf.fromred, buf.fromgreen, buf.fromblue);
    gimp_set_data("plug_in_exchange", &buf, sizeof(buf)); 

    return 0; /* OK */
}
