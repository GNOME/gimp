/* ----------------------------------------------------------------------
 * p_plug_in_nova_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_nova_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_nova_Vals 
    {
      gint      xcenter;
      gint      ycenter;
      t_color   color;
      gint      radius;
      gint      nspoke;
      gint      randomhue;
    } t_plug_in_nova_Vals; 

    t_plug_in_nova_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_nova_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_nova_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_nova_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_nova_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_nova_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_nova_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_nova_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.xcenter, buf_from->xcenter, buf_to->xcenter, total_steps, current_step);
    p_delta_gint(&buf.ycenter, buf_from->ycenter, buf_to->ycenter, total_steps, current_step);
    p_delta_color(&buf.color, &buf_from->color, &buf_to->color, total_steps, current_step);
    p_delta_gint(&buf.radius, buf_from->radius, buf_to->radius, total_steps, current_step);
    p_delta_gint(&buf.nspoke, buf_from->nspoke, buf_to->nspoke, total_steps, current_step);
    p_delta_gint(&buf.randomhue, buf_from->randomhue, buf_to->randomhue, total_steps, current_step);

    gimp_set_data("plug_in_nova", &buf, sizeof(buf)); 

    return 0; /* OK */
}
