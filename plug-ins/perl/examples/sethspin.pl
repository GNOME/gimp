#!/usr/bin/perl

# This one's all mine.  Well, its GPL but I"m the author and creator.  
# I think you need gimp 1.1 or better for this - if  you don't, please let 
# me know

# As a fair warning, some of this code is a bit ugly.  But thats perl for ya :)

# Seth Burgess
# <sjburges@gimp.org>

use Gimp;
use Gimp::Fu;

# Gimp::set_trace(TRACE_ALL);

sub hideallbut {
	($img, @butlist) = @_;
	@layers = $img->get_layers();
	foreach $layer (@layers) {
		if ($layer->get_visible()) {
			$layer->set_visible(0);
			}
		}
	foreach $but (@butlist) {
		if (! $layers[$but]->get_visible()) {
			$layers[$but]->set_visible(1);
			}
		}
	};	

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
		};
	}; 

sub spin_layer {
	my ($img, $spin, $dest, $numframes) = @_;

# Now lets spin it!
	$stepsize = 3.14159/$numframes; # in radians 
	for ($i=0; $i<=3.14159; $i+=$stepsize) {
		# create a new layer for spinning
		if ($i < 3.14159/2.0) {
			$framelay = $spin->layer_copy(1);
			}
		else {
			$framelay = $dest->layer_copy(1);
			}
		$img->add_layer($framelay, 0);
		# spin it a step
		$img->selection_all();
		@x = $img->selection_bounds();
		# x[1],x[2]                  x[3],x[2]
        # x[1],x[4]                  x[3],x[4]
		$psp = 0.2;  # The perspective amount
		$floater = $framelay->perspective(1,
	$x[1]+saw($i)*$psp*$framelay->width,$x[2]+$spin->height *sin($i)/2,  
	$x[3]-saw($i)*$psp*$framelay->width,$x[2]+$spin->height *sin($i)/2,
	$x[1]-saw($i)*$psp*$framelay->width,$x[4]-$spin->height *sin($i)/2,  
	$x[3]+saw($i)*$psp*$framelay->width,$x[4]-$spin->height *sin($i)/2);  
		$floater->floating_sel_to_layer();
		# fill entire layer with background  
		$framelay->fill(1); # BG-IMAGE-FILL
	}
	for ($i=0; $i<$numframes; $i++) {
		hideallbut($img, $i, $i+1);
		$img->merge_visible_layers(0);
		}
	@all_layers = $img->get_layers();
	$destfram = $all_layers[$numframes]->copy(0);
	$img->add_layer($destfram,0);

	# clean up my temporary layers	
	$img->remove_layer($all_layers[$numframes]);
	$img->remove_layer($all_layers[$numframes+1]);

};

register "seth_spin",
         "Seth Spin",
         "Take one image.  Spin it about the horizontal axis, and end up with another image.  I made it for easy web buttons.",
         "Seth Burgess",
         "Seth Burgess <sjburges\@gimp.org>",
         "1.0",
         "<Image>/Filters/Animation/Seth Spin",
         "RGB*, GRAY*",
         [
          [PF_DRAWABLE, "Destination","What drawable to spin to?"],
		  [PF_INT8, "Frames", "How many frames to use?", 8],
		  [PF_COLOR, "Background", "What color to use for background if not transparent", [0,0,0]],
		  [PF_SLIDER, "Perspective", "How much perspective effect to get", 40, [0,255,5]],
		  [PF_TOGGLE, "Spin Back", "Should it also spin back?  Will double the number of frames", 1],

         ],
         sub {
   my($img,$src,$dest,$frames,$color,$psp,$spinback) =@_;
  eval { $img->undo_push_group_start };

	$oldbackground = gimp_palette_get_background();
	gimp_palette_set_background($color);
# Create the new layer that the spin will occur on...
	$src->edit_copy();
	$spinlayer = $src->edit_paste(1);
	$spinlayer->floating_sel_to_layer();

	$dest->edit_copy();
	$destlayer = $dest->edit_paste(1);
	$destlayer->floating_sel_to_layer();
	
	spin_layer($img, $spinlayer, $destlayer, $frames);
	
	if ($spinback) { 
		@layerlist = $img->get_layers();
		$img->add_layer($layerlist[$frames]->copy(0),0);
		$img->remove_layer($layerlist[$frames]);
		@layerlist = $img->get_layers();
		spin_layer($img, $layerlist[1], $layerlist[0], $frames);
		$realframes = 2*$frames;
		}	
	else {
		$realframes = $frames;
		}

	# unhide and name layers
	@all_layers = $img->get_layers();
	for ($i=0; $i<$realframes ; $i++) {
		$all_layers[$i]->set_visible(1);
		$all_layers[$i]->set_name("Spin Layer $i");
		}
	gimp_palette_set_background($oldbackground);
	
  eval { $img->undo_push_group_end };
	gimp_displays_flush();
	return();
};

exit main;

