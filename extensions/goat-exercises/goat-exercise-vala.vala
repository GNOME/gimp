/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * hello-world.vala
 * Copyright (C) Niels De Graef <nielsdegraef@gmail.com>
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

private const string PLUG_IN_PROC = "plug-in-goat-exercise-vala";
private const string PLUG_IN_ROLE = "goat-exercise-vala";
private const string PLUG_IN_BINARY = "goat-exercise-vala";
private const string PLUG_IN_SOURCE = PLUG_IN_BINARY + ".vala";
private const string URL = "https://gitlab.gnome.org/GNOME/ligma/blob/master/extensions/goat-exercises/goat-exercise-vala.vala";

public int main(string[] args) {
  return Ligma.main(typeof(Goat), args);
}

public class Goat : Ligma.PlugIn {

  public override GLib.List<string> query_procedures() {
    GLib.List<string> procs = null;
    procs.append(PLUG_IN_PROC);
    return procs;
  }

  public override Ligma.Procedure create_procedure(string name) {
    assert(name == PLUG_IN_PROC);

    var procedure = new Ligma.ImageProcedure(this, name, Ligma.PDBProcType.PLUGIN, this.run);
    procedure.set_image_types("RGB*, INDEXED*, GRAY*");
    procedure.set_sensitivity_mask(Ligma.ProcedureSensitivityMask.DRAWABLE);
    procedure.set_menu_label(N_("Exercise a Vala goat"));
    procedure.set_documentation(N_("Exercise a goat in the Vala language"),
                                N_("Takes a goat for a walk in Vala"),
                                PLUG_IN_PROC);
    procedure.add_menu_path("<Image>/Filters/Development/Goat exercises/");
    procedure.set_attribution("Niels De Graef", "Niels De Graef", "2020");
    procedure.set_icon_name(LigmaUi.ICON_GEGL);

    return procedure;
  }

  public Ligma.ValueArray run(Ligma.Procedure procedure,
                             Ligma.RunMode run_mode,
                             Ligma.Image image,
                             Ligma.Drawable[] drawables,
                             Ligma.ValueArray args) {
    var drawable = drawables[0];

    if (run_mode == Ligma.RunMode.INTERACTIVE) {
      LigmaUi.init(PLUG_IN_BINARY);

      var dialog =
          new LigmaUi.Dialog(_("Exercise a goat (Vala)"),
                          PLUG_IN_ROLE,
                          null,
                          Gtk.DialogFlags.USE_HEADER_BAR,
                          LigmaUi.standard_help_func,
                          PLUG_IN_PROC,
                          _("_Cancel"), Gtk.ResponseType.CANCEL,
                          _("_Source"), Gtk.ResponseType.APPLY,
                          _("_Run"), Gtk.ResponseType.OK,
                          null);

      var geometry = Gdk.Geometry();
      geometry.min_aspect = 0.5;
      geometry.max_aspect = 1.0;
      dialog.set_geometry_hints(null, geometry, Gdk.WindowHints.ASPECT);

      var box = new Gtk.Box(Gtk.Orientation.VERTICAL, 12);
      box.border_width = 12;
      dialog.get_content_area().add(box);
      box.show();

      var head_text =
        _("This plug-in is an exercise in '%s' to demo plug-in creation.\nCheck out the last version of the source code online by clicking the \"Source\" button.")
        .printf("Vala");

      var label = new Gtk.Label(head_text);
      box.pack_start(label, false, false, 1);
      label.show();


      string file = Path.build_filename(Ligma.PlugIn.directory(), "extensions", "org.ligma.extension.goat-exercises", PLUG_IN_SOURCE);
      string contents;
      try {
        FileUtils.get_contents(file, out contents);
      } catch (Error err) {
        contents = "Couldn't get file contents: %s".printf(err.message);
      }

      var scrolled = new Gtk.ScrolledWindow(null, null);
      scrolled.vexpand = true;
      box.pack_start(scrolled, true, true, 1);
      scrolled.show();

      var view = new Gtk.TextView();
      view.wrap_mode = Gtk.WrapMode.WORD;
      view.editable = false;
      view.buffer.text = contents;
      scrolled.add(view);
      view.show();

      while (true) {
        var response = dialog.run();

        if (response == Gtk.ResponseType.OK) {
          dialog.destroy();
          break;
        } else if (response == Gtk.ResponseType.APPLY) {
          try {
            Gtk.show_uri_on_window(dialog, URL, Gdk.CURRENT_TIME);
          } catch (Error err) {
            warning("Couldn't launch browser for %s: %s", URL, err.message);
          }
          continue;
        } else {
          dialog.destroy();
          return procedure.new_return_values(Ligma.PDBStatusType.CANCEL, null);
        }
      }
    }

    int x, y, width, height;
    if (!drawable.mask_intersect(out x, out y, out width, out height)) {
      var error = new GLib.Error.literal(GLib.Quark.from_string("goat-error-quark"), 0,
                         "No pixels to process in the selected area.");
      return procedure.new_return_values(Ligma.PDBStatusType.CALLING_ERROR, error);
    }

    unowned string[]? argv = null;
    Gegl.init(ref argv);

    {
      var buffer = drawable.get_buffer();
      var shadow_buffer = drawable.get_shadow_buffer();
      Gegl.render_op(buffer, shadow_buffer, "gegl:invert", null);
      // We don't need this line, since shadow_buffer is unreffed
      // at the end of this block.
      // No block? Then you still need to uncomment the following line
      // shadow_buffer.flush();
    }

    drawable.merge_shadow(true);
    drawable.update(x, y, width, height);
    Ligma.displays_flush();
    Gegl.exit();

    return procedure.new_return_values(Ligma.PDBStatusType.SUCCESS, null);
  }
}
