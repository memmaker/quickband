# Quickband

Quickband is a variant of [NPPAngband](http://forum.nppangband.org/), which is itself a variant of Angband.
The original homepage is http://angband.oook.cz/quickband/ (the full upstream readme is in `readme.txt`).

This fork adds a few features, fixes and a browser port on top of Quickband 2.0.6.

**Play in the browser:** https://ruzzoli.de/roguelikes/quickband

## What's new

- **Command menu on Enter:** every command, grouped, with its key
- **Auto-explore** (`H`), plus `<` / `>` to walk to the nearest known stairs
- **X11 frontend:** UT32 tile support, big-tile mode, a six-window layout (`play.sh`) and a resize fix
- **WebAssembly port** (Emscripten + Asyncify): canvas renderer, tiling resizable windows, zoom,
  in-game help, saves and window layout kept in the browser (IndexedDB) with autosave and export/import
- **Sound effects and town music** (browser version, off by default; toggle with the Sound and Music
  buttons on the top bar): Quickband's own samples, with every event they don't cover filled from the
  Dubtrain Angband Sound Pack v3.1.0 (CC non-commercial); a music track loops while you are in town
- **Fixes:** empty subwindows for new characters, `<0x>` lines in the message window, crashes in the
  options menu and other bugs found while porting

## Building

Desktop (X11):

```sh
cd src && make -f Makefile.std
cd .. && ./play.sh
```

Browser (needs Emscripten):

```sh
web/build.sh    # output goes to web/dist
```
