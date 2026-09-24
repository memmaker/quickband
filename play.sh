#!/bin/sh
# Quickband, X11 frontend: UT32 tiles (-g) drawn 30x30 (15x30 font + big-tile
# mode -b; tiles are always scaled nearest-neighbour) and extra windows:
# 1 inventory, 3 visible monsters, 5 visible items, 2 messages, 4 recall
# (contents set in lib/pref/user-x11.prf).
# Screen 1440x932; XQuartz adds a ~28px title bar above each window.
export ANGBAND_X11_FONT_0='-misc-fixed-medium-r-normal--32-*-*-*-c-150-iso8859-1'
export ANGBAND_X11_AT_X_0=0    ANGBAND_X11_AT_Y_0=0                                                          # main 80x24 -> 1200x720
export ANGBAND_X11_FONT_1=6x10 ANGBAND_X11_AT_X_1=1206 ANGBAND_X11_AT_Y_1=0   ANGBAND_X11_COLS_1=39  ANGBAND_X11_ROWS_1=32  # inventory
export ANGBAND_X11_FONT_3=6x10 ANGBAND_X11_AT_X_3=1206 ANGBAND_X11_AT_Y_3=356 ANGBAND_X11_COLS_3=39  ANGBAND_X11_ROWS_3=19  # monsters
export ANGBAND_X11_FONT_5=6x10 ANGBAND_X11_AT_X_5=1206 ANGBAND_X11_AT_Y_5=580 ANGBAND_X11_COLS_5=39  ANGBAND_X11_ROWS_5=19  # items
export ANGBAND_X11_FONT_2=8x13 ANGBAND_X11_AT_X_2=0    ANGBAND_X11_AT_Y_2=800 ANGBAND_X11_COLS_2=107 ANGBAND_X11_ROWS_2=7   # messages, 3/5
export ANGBAND_X11_FONT_4=8x13 ANGBAND_X11_AT_X_4=862  ANGBAND_X11_AT_Y_4=800 ANGBAND_X11_COLS_4=72  ANGBAND_X11_ROWS_4=7   # recall, 2/5
cd "$(dirname "$0")" && exec ./nppangband -mx11 "$@" -- -n6 -g -b
