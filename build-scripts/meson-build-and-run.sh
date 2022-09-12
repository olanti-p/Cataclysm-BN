#!/bin/sh

mold -run meson compile -C builddir && builddir/cataclysm-tiles
