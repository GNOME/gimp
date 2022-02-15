This file details the logic for item locks (on layers, channels, paths,
etc.)

[TOC]

## The 4 types of locks
### Lock pixels

Locking pixels means you cannot change any value in any of the channels
of the image. In other words, you cannot paint, erase, apply any
filters, neither can you apply most transformations.

The only transformation which you can apply is moving the whole layer at
once (not moving a selection contents), because then you only change the
offsets. The contents of each channel stay the same.

### Lock position

Locking position is the opposite of the "Lock pixels" as you can do
anything on your item except moving it, i.e. changing its offsets.

### Lock alpha

Locking alpha is in a way a subcase of "Lock pixels" as the only channel
which cannot change is the alpha channel, i.e. how transparent is each
pixel. You can still change the colors of pixels, but not their
transparency. In other words, if you paint over a layer with "Lock
alpha", this would only affect the color.

This use case can sometimes be frustrating/confusing if you paint over
full transparent pixels as it would look it's not working, yet you would
get neither statusbar error nor blinking locks to explain why you see no
changes on your layer.

This lock is useless over an item with no alpha channel, though it is
currently still allowed to set such a lock. It just does nothing useful.

Note that you can still change the opacity of the item as a whole
through the Opacity slider (for instance in Layers or Channels
dockables) or through actions affecting the layer's opacity. It's only
individual pixel alpha values which are locked.

### Lock visibility

Locking visibility means you cannot change the item's visibility
(typically the "eye" icon on dockables).

This is especially useful for when you [shift-click or
alt-click](#exclusive-visibility-locks-switch) the eye icons to
massively toggle item visibility, yet you want to exclude some items.
For instance, say you have a background you want to always show and
several foreground object you want to conditionally show alone or
together. You could lock the background to keep it always visible and
shift-click foreground objects.

## Effects on layer groups

Layers in particular can be grouped in "Layer groups". Though the effect
of locks is obvious on non-group items, we tried to specify their
behaviour when set on layer groups to be the most useful and also
hopefully expected possible.

Below are what the effects are for various locks.

### Lock pixels and alpha

Since it involves painting, it means nothing on layer groups as you
can't paint on them. There are 2 exceptions:

#### Layer group masks

A layer group can have a mask. A "Lock pixels" would prevent from
drawing in it. "Lock alpha" is meaningless there as masks don't have
alpha channels (well they are kind of separate alpha value themselves,
but as said, we use the "Lock pixels" to block this type of editing).

#### Child layers

These 2 locks when set on a group apply on all children. In other words,
you can lock painting and drawing on many layers organized in groups by
locking an ancestor.

### Lock position

Moving the group as a whole is equivalent to moving each and every of
its children layers (recursively if there are sub-groups). Locking the
layer group forbid this.

Locking a group also forbid moving any child layer individually, as do
"Lock pixels" and "Lock alpha".

Note that locking the position of a child layer also prevents the whole
group contents to be moved.

So "Lock position" is really working both ways: any lock in the tree
prevents the whole direct genealogy of layers to be moved. Sister layers
are fine though.

### Lock visibility

Locking a layer group visibility has the exact semantic as locking a
non-group layer since this is the only feature which is actually
normally applicable to layer groups.

Also it means that unlike all other lock features, locking the
visibility of the layer group does not forbid changing visibility of a
child layer.

## Layer mask

The layer mask is also its own particular case. Right now, the "Lock
alpha" and "Lock visibility" don't affect them (masks are somehow their
own alpha channel, but in the same time, we use "Lock pixels" to forbid
painting; as for visibility, they have their own feature for this).

"Lock pixels" prevents from painting in a mask.

Note that both "Lock pixels" and "Lock position" prevent from moving a
mask. Indeed masks dimensions are currently tied to their associated
layer (or layer group) dimensions. It means than moving a mask does not
just change the offset. You actually also delete some of the mask
pixels/data and create new ones. Therefore moving changes the contents
of the mask, which is why "Lock pixels" also prevent moving the mask.

## Exclusive visibility/locks switch

GIMP provides a way to massively toggles visibility and locks.

### Exclusive switch on same level

When `Shift-clicking` either a visibility icon or a lock icon, all other
visibility or locks respectively on the same level will be switched OFF
(leaving only the clicked switch ON). `Shift-clicking` again, all
visibility or locks on same level will be back on.

Say the state is:

```
* Layer 1
* Layer group
  * Layer 2
  * Layer 3
  * Layer 4
* Layer 5
```

Then `Shift-click` the eye icon of Layer 3: Layer 2 and Layer 4's
visibility will be turned OFF. Shift-click again and it goes back to
initial state.

### Exclusive switch of selected items

When `Alt-clicking` a visibility or lock icon, the same happens as
`Shift-click` except among selected items. The level in the item tree
doesn't matter.

### Exclusive visibility switch with a "Lock visibility" set

If any "Lock visibility" is set on an item, its visibility status won't
be switched along other items when `Shift-clicking` or `Alt-clicking`.
Being able to exclude some items from such massive switch is one of the
main use case of the "Lock visibility".
