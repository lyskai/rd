#!/bin/bash

if [ ! -d $THIRD_PARTY_DIR ]; then
        mkdir $THIRD_PARTY_DIR
fi
cd $THIRD_PARTY_DIR

# Get libsdl
if [ ! -f SDL2-2.0.14.tar.gz ]; then
        wget https://www.libsdl.org/release/SDL2-2.0.14.tar.gz . --no-check-certificate
        echo "Package SDL2-2.0.14.tar.gz downloaded"
else
        echo "Found SDL2-2.0.14.tar.gz"
fi

# Get SDL2_gfx
if [ ! -f SDL2_gfx-1.0.4.tar.gz ]; then
        wget https://versaweb.dl.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz . --no-check-certificate
        echo "Package SDL2_gfx-1.0.4.tar.gz downloaded"
else
        echo "Found SDL2_gfx-1.0.4.tar.gz"
fi

# Get SDL2_ttf
if [ ! -f SDL2_ttf-2.0.15.tar.gz ]; then
        wget https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz . --no-check-certificate
        echo "Package SDL2_ttf-2.0.15.tar.gz downloaded"
else
        echo "Found SDL2_ttf-2.0.15.tar.gz"
fi


echo "Successfully get all dependent packages in $THIRD_PARTY_DIR"

