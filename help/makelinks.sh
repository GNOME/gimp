#!/bin/sh

cd channels
    ln -sf ../dialogs/channels dialogs
cd ..

cd dialogs
cd ..

cd file
    ln -sf ../dialogs dialogs
    ln -sf ../filters filters
    ln -sf ../open open
    ln -sf ../save save
cd ..

cd filters
cd ..

cd image
    ln -sf ../dialogs dialogs
    ln -sf ../file file
    ln -sf ../filters filters
    ln -sf ../layers layers
    ln -sf ../toolbox toolbox
    ln -sf ../tools tools
    cd edit
	ln -sf ../../dialogs dialogs
    cd ..
    cd select
	ln -sf ../../dialogs dialogs
    cd ..
    cd view
	ln -sf ../../dialogs dialogs
    cd ..
    cd image
	ln -sf ../../dialogs dialogs
	cd transforms
	    ln -sf ../../../dialogs dialogs
	cd ..
    cd ..
cd ..

cd layers
    ln -sf ../dialogs/layers dialogs
cd ..

cd open
    ln -sf ../dialogs dialogs
    ln -sf ../filters filters
cd ..

cd paths
    ln -sf ../dialogs/paths dialogs
cd ..

cd save
    ln -sf ../dialogs dialogs
    ln -sf ../filters filters
cd ..

cd toolbox
    ln -sf ../dialogs dialogs
    ln -sf ../file file
    ln -sf ../filters filters
cd ..

cd tools
cd ..