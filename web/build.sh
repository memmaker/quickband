#!/bin/sh
# Build Quickband for the browser (Emscripten + Asyncify).
# Output goes to web/dist; deploy with web/deploy.sh.
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" web/stage && mkdir -p "$OUT" web/stage/lib

# Game files: everything but the X11 fonts, sounds and BMP tiles
# (the browser draws from 32x32.png, served next to the page)
for d in edit file help pref; do cp -R lib/$d web/stage/lib/; done
mkdir -p web/stage/lib/info web/stage/lib/save web/stage/lib/user web/stage/lib/apex web/stage/lib/bone
# Tiles on by default (Tiles button switches to text); keep the X11 layout prefs
cp lib/xtra/graf/32x32.png "$OUT/"

SRCS=$(tr -d '\r' < src/Makefile.src | sed -n '/^ANGFILES/,/^$/p;/^ZFILES/,/^MAINFILES/p' \
	| grep -o '[a-z0-9_-]*\.o' | grep -v '^main' | sed 's/\.o$/.c/;s|^|src/|' | sort -u)

mkdir -p web/stage/lib/xtra/sound && cp lib/xtra/sound/sound.cfg web/stage/lib/xtra/sound/sound.cfg
emcc -O2 -fcommon -std=gnu99 -DUSE_WEB -Isrc \
	$SRCS src/main.c src/main-web.c \
	-o "$OUT/quickband-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=64MB \
	-sEXPORTED_FUNCTIONS=_main,_web_request_save,_web_set_tiles,_web_set_tile_mult \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web \
	--preload-file web/stage/lib@/quickband/lib

cp web/index.html web/quickband.js "$OUT/"
# Sound effects and town music are fetched by the page, not preloaded
cp -R lib/xtra/sound "$OUT/sound"
mkdir -p "$OUT/music" && cp web/music/new_town.ogg "$OUT/music/"
# Game guide for the Help button, from ~/Desktop/Games/Roguelikes/Docs
# Font choosers: the index page's fonts/*.woff (loaded from ../fonts/)
FONTS="${FONTS:-$HOME/Games/roguelikes-index/fonts}"
(ls "$FONTS" 2>/dev/null | sed -n 's/\.woff$//p') | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read().split()))' > "$OUT/fonts.json"
python3 web/make-help.py > "$OUT/help.html"
rm -rf web/stage
ls -la "$OUT"
