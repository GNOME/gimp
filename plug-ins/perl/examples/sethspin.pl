#!/usr/bin/perl -w

# This one's all mine.  Well, its GPL/Artisitic but I"m the author and creator. # I think you need gimp 1.1 or better for this - if  you don't, please let 
# me know

# As a fair warning, some of this code is a bit ugly.  But thats perl for ya :)
#
# Revision History:
# 1.0 - Initial (too early) release
# 1.1 - Second (still ugly) release: Made the perspective setting actually do
#       something
# 1.2 - Used some of the convienence functions, and made things a little eaiser
#       from the user's standpoint too.  Also moved it from the 
#       Filters->Animations-> menu to Xtns->Animations.  I think its 
#       clearer whats going on this way.  It also works w/ any 2 layers now.
        
# Seth Burgess
# <sjburges@gimp.org>

use Gimp 1.06;
use Gimp::Fu;
use Gimp::Util;

# Gimp::set_trace(TRACE_ALL);

sub saw {  # a sawtooth function on PI
	($val) = @_;
	if ($val < 3.14159/2.0) {
		return ($val/3.14159) ;
		}
	elsif ($val < 3.14159) {
		return (-1+$val/3.14159); 
		}
	elsif ($val < 3.14159+3.14159/2.0) {
		return ($val/3.14159) ;
		}
	else {
		return (-1+$val/3.14159); 
		}
	} 

sub spin_layer { # the function for actually spinning the layer
	my ($img, $spin, $dest, $numframes, $prp) = @_;
    # Now lets spin it!
	$stepsize = 3.14159/$numframes; # in radians 
	for ($i=0; $i<=3.14159; $i+=$stepsize) {
		# create a new layer for spinning
		$framelay = ($i < 3.14159/2.0) ?  $spin->copy(1) : $dest->copy(1);
		$img->add_layer($framelay, 0);
		# spin it a step
		$img->selection_all();
		@x = $img->selection_bounds();
		# x[1],x[2]                  x[3],x[2]
        # x[1],x[4]                  x[3],x[4]
		$floater = $framelay->perspective(1,
	$x[1]+saw($i)*$prp*$framelay->width,$x[2]+$spin->height *sin($i)/2,  
	$x[3]-saw($i)*$prp*$framelay->width,$x[2]+$spin->height *sin($i)/2,
	$x[1]-saw($i)*$prp*$framelay->width,$x[4]-$spin->height *sin($i)/2,  
	$x[3]+saw($i)*$prp*$framelay->width,$x[4]-$spin->height *sin($i)/2);  
		$floater->floating_sel_to_layer;
		# fill entire layer with background  
		$framelay->fill(1); # BG-IMAGE-FILL
	}
	for ($i=0; $i<$numframes; $i++) {
		@all_layers = $img->get_layers();
		$img->set_visible($all_layers[$i],$all_layers[$i+1]);
		$img->merge_visible_layers(0);
		}
	@all_layers = $img->get_layers();
	$destfram = $all_layers[$numframes]->copy(0);
	$img->add_layer($destfram,0);

	# clean up my temporary layers	
	$img->remove_layer($all_layers[$numframes]);
	$img->remove_layer($all_layers[$numframes+1]);
}

register "seth_spin",
         "Seth Spin",
         "Take one image.  Spin it about the horizontal axis, and end up with another image.  I made it for easy web buttons.",
         "Seth Burgess",
         "Seth Burgess <sjburges\@gimp.org>",
         "1.0.1",
         "<Toolbox>/Xtns/Animation/Seth Spin",
         "*",
         [
          [PF_DRAWABLE, "Source", "What drawable to spin from?"],
          [PF_DRAWABLE, "Destination","What drawable to spin to?"],
	  [PF_INT8, "Frames", "How many frames to use?", 16],
	  [PF_COLOR, "Background", "What color to use for background if not transparent", [0,0,0]],
	  [PF_SLIDER, "Perspective", "How much perspective effect to get", 40, [0,255,5]],
	  [PF_TOGGLE, "Spin Back", "Also spin back?" , 1],
          [PF_TOGGLE, "Convert Indexed", "Convert to indexed?", 1],
         ],
         [],
         ['gimp-1.1'],
         sub {
   my($src,$dest,$frames,$color,$psp,$spinback,$indexed) =@_;
	$maxwide = ($src->width > $dest->width) ? $src->width : $dest->width;
	$maxhigh = ($src->height > $dest->height) ? $src->height: $dest->height;
	$img = gimp_image_new($maxwide, $maxhigh, RGB);

	$tmpimglayer = $img->add_new_layer(0,3,1); 

	$oldbackground = gimp_palette_get_background();
	gimp_palette_set_background($color);
	$src->edit_copy();
	$spinlayer = $tmpimglayer->edit_paste(1);
	$spinlayer->floating_sel_to_layer();

	$dest->edit_copy();
	$destlayer = $tmpimglayer->edit_paste(1);
	$destlayer->floating_sel_to_layer();

	$tmpimglayer->remove_layer;

	$spinlayer->resize($maxwide, $maxhigh, $spinlayer->offsets);
	$destlayer->resize($maxwide, $maxhigh, $destlayer->offsets);
	# work around for PF_SLIDER when < 1
	$psp = $psp/255.0;

	# need an even number of frames for spinback
	if ($frames%2 && $spinback) {
		$frames++;
		gimp_message("An even number of frames is needed for spin back.\nAdjusted frames up to $frames");
		}

	spin_layer($img, $spinlayer, $destlayer, $spinback ? $frames/2 : $frames-1, $psp);
	# it makes ugly sounds on the next line, but no harm is done.
	$img->set_visible($img->add_new_layer(1),($img->get_layers)[0]); 
	$img->merge_visible_layers(0);
	
	if ($spinback) { 
		@layerlist = $img->get_layers();
		$img->add_layer($layerlist[$frames/2]->copy(0),0);
		@layerlist = $img->get_layers();
		spin_layer($img, $layerlist[1], $layerlist[0], $frames/2, $psp);
		$img->remove_layer(($img->get_layers)[0]);
		}	
	
	# unhide and name layers
	@all_layers = $img->get_layers;
	$img->set_visible(@all_layers);
	for ($i=1; $i<=$frames ; $i++) {
		$all_layers[$i-1]->set_name("Spin Layer $i (50ms)");
		}
	$all_layers[$frames-1]->set_name("Spin Layer SRC (250ms)");  
	
	if ($spinback) {
		$all_layers[$frames/2-1]->set_name("Spin Layer DEST (250ms)"); 
		}
	else { $all_layers[0]->set_name("Spin Layer DEST (250ms)")}
	
	$img->display_new;

	# indexed conversion wants a display for some reason
	if ($indexed) { $img->convert_indexed(1,255); } 

	gimp_palette_set_background($oldbackground);
	return();
};

exit main;

