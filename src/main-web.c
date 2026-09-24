/* File: main-web.c */

/*
 * Browser (Emscripten/WASM) front end for Quickband.
 *
 * All drawing is done by JavaScript on one <canvas> per term (see
 * web/quickband.js).  Blocking input uses Asyncify: when the game waits
 * for a key we sleep in emscripten_sleep(), which yields to the browser.
 *
 * The module registers itself as "x11" so that the same pref files
 * (keymaps, window layout, graphics) as the X11 build are used; special
 * keys are sent in the X11 keysym macro format.
 */

#include "angband.h"
#include "main.h"

#ifdef USE_WEB

#include <emscripten.h>

#define WEB_TERMS 6

static term web_term[WEB_TERMS];

/* Pending "save now" request from the page (tab hidden / closing) */
static int web_want_save = 0;

/* Last time we yielded to the browser */
static double web_last_yield = 0;


/* ---- JavaScript side (implemented in web/quickband.js) ---- */

EM_JS(void, js_text, (int t, int x, int y, int n, int a, const char *s), {
	Module.qb.text(t, x, y, n, a, s);
});

EM_JS(void, js_wipe, (int t, int x, int y, int n), {
	Module.qb.wipe(t, x, y, n);
});

EM_JS(void, js_clear, (int t), {
	Module.qb.clear(t);
});

EM_JS(void, js_curs, (int t, int x, int y, int w), {
	Module.qb.curs(t, x, y, w);
});

EM_JS(void, js_pict, (int t, int x, int y, int n, const byte *ap, const char *cp,
                      const byte *tap, const char *tcp), {
	Module.qb.pict(t, x, y, n, ap, cp, tap, tcp);
});

EM_JS(void, js_fresh, (int t), {
	Module.qb.fresh(t);
});

EM_JS(void, js_bell, (void), {
	Module.qb.bell();
});

EM_JS(void, js_color, (int i, int r, int g, int b), {
	Module.qb.color(i, r, g, b);
});

EM_JS(int, js_term_cols, (int t), {
	return Module.qb.termCols(t);
});

EM_JS(int, js_term_rows, (int t), {
	return Module.qb.termRows(t);
});

/* Layout changes after a browser resize */
EM_JS(int, js_layout_pending, (int t), {
	return Module.qb.layoutPending(t);
});

EM_JS(int, js_pending_cols, (int t), {
	return Module.qb.pendingCols(t);
});

EM_JS(int, js_pending_rows, (int t), {
	return Module.qb.pendingRows(t);
});

EM_JS(void, js_apply_layout, (int t, int cols, int rows), {
	Module.qb.applyLayout(t, cols, rows);
});

/* Next queued input: -1 none, else key; mouse events via js_mouse_* */
EM_JS(int, js_next_event, (void), {
	return Module.qb.nextEvent();
});

EM_JS(int, js_mouse_x, (void), { return Module.qb.mouseX; });
EM_JS(int, js_mouse_y, (void), { return Module.qb.mouseY; });
EM_JS(int, js_mouse_b, (void), { return Module.qb.mouseB; });

EM_JS(void, js_quit, (const char *msg), {
	Module.qb.quit(msg ? UTF8ToString(msg) : "");
});

EM_JS(void, js_plog, (const char *msg), {
	Module.qb.plog(UTF8ToString(msg));
});

EM_JS(void, js_sync, (void), {
	Module.qb.sync();
});


/* Persist the save directories (called after every save) */
void web_sync_files(void)
{
	js_sync();
}


/* Called from JS when the page is about to be hidden or closed */
EMSCRIPTEN_KEEPALIVE void web_request_save(void)
{
	web_want_save = 1;
}


/*
 * Resize the terms to the layout the page computed after a browser resize.
 * Subwindows change at once; the main window changes its size only at the
 * command prompt, where the resize event leads to a full redraw (menus and
 * prompts elsewhere don't expect one).  Returns TRUE if an event was queued.
 */
static bool web_apply_layout(void)
{
	int i;
	term *old = Term;
	bool at_prompt = (inkey_flag && character_generated);
	bool subs = FALSE, queued = FALSE;

	for (i = 0; i < WEB_TERMS; i++)
	{
		term *t = &web_term[i];
		int cols, rows;

		if (!js_layout_pending(i)) continue;

		cols = js_pending_cols(i);
		rows = js_pending_rows(i);
		if (cols < 1) cols = 1;
		if (rows < 1) rows = 1;

		if (!i)
		{
			if (cols < 80) cols = 80;
			if (rows < 24) rows = 24;

			if (((cols != t->wid) || (rows != t->hgt)) && !at_prompt) continue;
		}

		/* New canvas size and cell size (the canvas starts blank) */
		js_apply_layout(i, cols, rows);

		Term_activate(t);

		/* Queues EVT_RESIZE on this term if the size changed */
		if ((Term_resize(cols, rows) == 0) && !i) queued = TRUE;

		/* Subwindow queues are never read */
		if (i)
		{
			t->key_head = t->key_tail = 0;
			subs = TRUE;
		}

		/* Repaint the contents at the new cell size */
		Term_redraw();
	}

	Term_activate(old);

	if (subs && character_generated)
	{
		/* Refill the subwindows for their new size */
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP | PR_MESSAGE | PR_MONSTER |
		                  PR_OBJECT | PR_MONLIST | PR_ITEMLIST | PR_FEATURE);

		/* At the command prompt, redraw everything right away */
		if (at_prompt && !queued)
		{
			ui_event_data evt = EVENT_EMPTY;

			evt.type = EVT_RESIZE;
			Term_activate(&web_term[0]);
			Term_event_push(&evt);
			Term_activate(old);
			queued = TRUE;
		}
	}

	return (queued);
}


/* Move queued browser input into the main term's key queue */
static int web_pump(void)
{
	int k, got = 0;
	term *old = Term;

	if (web_apply_layout()) got = 1;

	Term_activate(&web_term[0]);

	while ((k = js_next_event()) >= 0)
	{
		if (k == 0x10000)
			Term_mousepress(js_mouse_x(), js_mouse_y(), (char)js_mouse_b());
		else
			Term_keypress(k);
		got = 1;
	}

	/* Safe autosave: only while waiting for a command */
	if (web_want_save && inkey_flag && character_generated &&
	    !p_ptr->is_dead && !got && (Term->key_head == Term->key_tail))
	{
		web_want_save = 0;
		Term_keypress(KTRL('S'));
		got = 1;
	}

	Term_activate(old);
	return got;
}

static void web_yield(int ms)
{
	emscripten_sleep(ms);
	web_last_yield = emscripten_get_now();
}

static errr web_check_events(int wait)
{
	if (web_pump()) return (0);

	if (!wait)
	{
		/* Let the browser paint now and then during long actions */
		if (emscripten_get_now() - web_last_yield > 50) web_yield(0);
		return (web_pump() ? 0 : 1);
	}

	while (1)
	{
		web_yield(10);
		if (web_pump()) return (0);
	}
}

static void web_react(void)
{
	int i;

	for (i = 0; i < MAX_COLORS; i++)
		js_color(i, angband_color_table[i][1], angband_color_table[i][2],
		         angband_color_table[i][3]);
}

static int web_idx(void)
{
	return (int)(Term - web_term);
}

static errr Term_xtra_web(int n, int v)
{
	switch (n)
	{
		case TERM_XTRA_NOISE: js_bell(); return (0);
		case TERM_XTRA_FRESH: js_fresh(web_idx()); return (0);
		case TERM_XTRA_BORED: return (web_check_events(0));
		case TERM_XTRA_EVENT: return (web_check_events(v));
		case TERM_XTRA_FLUSH:
			while (js_next_event() >= 0) ;
			return (0);
		case TERM_XTRA_CLEAR: js_clear(web_idx()); return (0);
		case TERM_XTRA_DELAY:
			js_fresh(web_idx());
			if (v > 0) web_yield(v);
			return (0);
		case TERM_XTRA_REACT: web_react(); return (0);
	}

	return (1);
}

static errr Term_curs_web(int x, int y)
{
	js_curs(web_idx(), x, y, 1);
	return (0);
}

static errr Term_bigcurs_web(int x, int y)
{
	js_curs(web_idx(), x, y, 2);
	return (0);
}

static errr Term_wipe_web(int x, int y, int n)
{
	js_wipe(web_idx(), x, y, n);
	return (0);
}

static errr Term_text_web(int x, int y, int n, byte a, cptr s)
{
	js_text(web_idx(), x, y, n, a, s);
	return (0);
}

static errr Term_pict_web(int x, int y, int n, const byte *ap, const char *cp,
                          const byte *tap, const char *tcp)
{
	js_pict(web_idx(), x, y, n, ap, cp, tap, tcp);
	return (0);
}


static void hook_plog(cptr str)
{
	if (str) js_plog(str);
}

static void hook_quit(cptr str)
{
	int i;


	for (i = 0; i < WEB_TERMS; i++) (void)term_nuke(&web_term[i]);

	js_sync();
	js_quit(str);
}


const char help_web[] = "Browser front end";

errr init_web(int argc, char **argv)
{
	int i;

	(void)argc;
	(void)argv;

	/* UT32 tiles in big-tile mode, as in the X11 build (-g -b) */
	use_graphics = GRAPHICS_DAVID_GERVAIS;
	arg_graphics = GRAPHICS_DAVID_GERVAIS;
	use_bigtile = TRUE;
	ANGBAND_GRAF = "david";

	/*
	 * Web defaults (init_angband() copies these into op_ptr->opt later;
	 * savefiles keep the player's own choice)
	 */
	options[OPT_auto_more].normal = TRUE;
	options[OPT_center_player].normal = TRUE;

	web_react();

	for (i = 0; i < WEB_TERMS; i++)
	{
		term *t = &web_term[i];
		int cols = js_term_cols(i), rows = js_term_rows(i);

		if (!i)
		{
			if (cols < 80) cols = 80;
			if (rows < 24) rows = 24;
		}

		term_init(t, cols, rows, (i == 0) ? 1024 : 16);

		t->soft_cursor = TRUE;
		t->attr_blank = TERM_WHITE;
		t->char_blank = ' ';

		t->xtra_hook = Term_xtra_web;
		t->curs_hook = Term_curs_web;
		t->bigcurs_hook = Term_bigcurs_web;
		t->wipe_hook = Term_wipe_web;
		t->text_hook = Term_text_web;
		t->pict_hook = Term_pict_web;
		t->higher_pict = TRUE;

		Term_activate(t);
		angband_term[i] = t;
	}

	Term_activate(&web_term[0]);

	web_last_yield = emscripten_get_now();

	quit_aux = hook_quit;
	plog_aux = hook_plog;

	return (0);
}

#endif /* USE_WEB */
