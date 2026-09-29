# Quickband 2.0.6: handover

## Source and changes

- Base: **Quickband 2.0.6**
- Original source: https://github.com/memmaker/quickband/tree/7ff30f1 (untouched import, commit 7ff30f1 (taken from NickMcConnell/AngbandPlus; the exact upstream commit was not recorded))
- Our changes: https://github.com/memmaker/quickband/compare/7ff30f1...main (memmaker/quickband)
- Prompt line (`RvipWM.prompt`, RVIP 5.9): `js_next_event(inkey_flag && character_generated)` in `src/main-web.c`;
  the page tracks term 0 row 0 (`row0` in `text`/`wipe`/`clear`) and sends it on
  `fresh(0)`.
