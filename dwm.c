/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))

/* LASTEvent is 35 in X11 (GenericEvent + 1) */
#define LASTEvent               35


/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
	int monitor;
} Rule;

/* Xlib stuff */
typedef struct {
	int ascent;
	int descent;
	int height;
	XFontSet set;
	XFontStruct *xfont;
} Fnt;

typedef struct {
	unsigned long pix;
} Clr;

typedef struct {
	Cursor cursor;
} Cur;

typedef struct {
	Display *dpy;
	int screen;
	Window root;
	Fnt *fonts;
	Clr **scheme;
	Cur *cursor;
	int w, h;
	GC gc;
} Drw;

/* function declarations */
static void applyrules(Client *c);
static void arrange(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static void die(const char *fmt, ...);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);

/* variables */
static const char broken[] = "broken";
static char stext[256];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh, blw = 0;      /* bar geometry */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static Layout layouts[] = {
	{ "[T]", tile },
	{ "[F]", NULL },
	{ "[M]", monocle },
};

/* configuration, allows nested code to access above variables */
#include "config.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* function implementations - minimal stubs */
void applyrules(Client *c) { c->tags = c->mon->tagset[c->mon->seltags]; }
void arrange(Monitor *m) { showhide(m->stack); if (layouts[m->sellt].arrange) layouts[m->sellt].arrange(m); restack(m); }
void attach(Client *c) { c->next = c->mon->clients; c->mon->clients = c; }
void attachstack(Client *c) { c->snext = c->mon->stack; c->mon->stack = c; }
void buttonpress(XEvent *e) {}
void cleanup(void) {}
void cleanupmon(Monitor *mon) { free(mon); }
void clientmessage(XEvent *e) {}
void configure(Client *c) { XConfigureEvent ce; ce.type = ConfigureNotify; ce.display = dpy; ce.event = c->win; ce.window = c->win; ce.x = c->x; ce.y = c->y; ce.width = c->w; ce.height = c->h; ce.border_width = c->bw; ce.above = None; ce.override_redirect = False; XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce); }
void configurenotify(XEvent *e) { updategeom(); updatebars(); }
void configurerequest(XEvent *e) { XConfigureRequestEvent *ev = &e->xconfigurerequest; XWindowChanges wc; wc.x = ev->x; wc.y = ev->y; wc.width = ev->width; wc.height = ev->height; wc.border_width = ev->border_width; wc.sibling = ev->above; wc.stack_mode = ev->detail; XConfigureWindow(dpy, ev->window, ev->value_mask, &wc); }
void createmon(void) { if (!mons) { mons = calloc(1, sizeof(Monitor)); mons->tagset[0] = mons->tagset[1] = 1; mons->mfact = mfact; mons->nmaster = nmaster; mons->showbar = showbar; mons->topbar = topbar; selmon = mons; } }
void destroynotify(XEvent *e) { Client *c; XDestroyWindowEvent *ev = &e->xdestroywindow; if ((c = wintoclient(ev->window))) unmanage(c, 1); }
void detach(Client *c) { Client **tc; for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next); *tc = c->next; }
void detachstack(Client *c) { Client **tc, *t; for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext); *tc = c->snext; if (c == c->mon->sel) { for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext); c->mon->sel = t; } }
void die(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); exit(1); }
Monitor *dirtomon(int dir) { return selmon; }
void drawbar(Monitor *m) {}
void drawbars(void) {}
void enternotify(XEvent *e) {}
void expose(XEvent *e) {}
void focus(Client *c) { if (!c || !ISVISIBLE(c)) for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext); selmon->sel = c; }
void focusin(XEvent *e) {}
void focusmon(const Arg *arg) {}
void focusstack(const Arg *arg) { Client *c = NULL; if (!selmon->sel) return; if (arg->i > 0) { for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next); if (!c) for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next); } else { Client *i; for (i = selmon->clients; i != selmon->sel; i = i->next) if (ISVISIBLE(i)) c = i; if (!c) for (; i; i = i->next) if (ISVISIBLE(i)) c = i; } if (c) { focus(c); restack(selmon); } }
Atom getatomprop(Client *c, Atom prop) { return None; }
int getrootptr(int *x, int *y) { int di; unsigned int dui; Window dummy; return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui); }
long getstate(Window w) { return WithdrawnState; }
int gettextprop(Window w, Atom atom, char *text, unsigned int size) { return 0; }
void grabbuttons(Client *c, int focused) {}
void grabkeys(void) { XUngrabKey(dpy, AnyKey, AnyModifier, root); unsigned int i; KeyCode code; for (i = 0; i < LENGTH(keys); i++) if ((code = XKeysymToKeycode(dpy, keys[i].keysym))) XGrabKey(dpy, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync); }
void keypress(XEvent *e) { unsigned int i; KeySym keysym; XKeyEvent *ev; ev = &e->xkey; keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0); for (i = 0; i < LENGTH(keys); i++) if (keysym == keys[i].keysym && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state) && keys[i].func) keys[i].func(&(keys[i].arg)); }
void killclient(const Arg *arg) { if (!selmon->sel) return; XKillClient(dpy, selmon->sel->win); }
void manage(Window w, XWindowAttributes *wa) { Client *c = calloc(1, sizeof(Client)); c->win = w; c->x = c->oldx = wa->x; c->y = c->oldy = wa->y; c->w = c->oldw = wa->width; c->h = c->oldh = wa->height; c->oldbw = wa->border_width; c->bw = borderpx; c->mon = selmon; c->tags = 0; applyrules(c); attach(c); attachstack(c); XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask); grabbuttons(c, 0); XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h); setclientstate(c, NormalState); configure(c); XMapWindow(dpy, c->win); focus(c); arrange(c->mon); }
void mappingnotify(XEvent *e) { XRefreshKeyboardMapping(&e->xmapping); if (e->xmapping.request == MappingKeyboard) grabkeys(); }
void maprequest(XEvent *e) { XWindowAttributes wa; XMapRequestEvent *ev = &e->xmaprequest; if (!XGetWindowAttributes(dpy, ev->window, &wa)) return; if (wa.override_redirect) return; if (!wintoclient(ev->window)) manage(ev->window, &wa); }
void monocle(Monitor *m) { unsigned int n = 0; Client *c; for (c = m->clients; c; c = c->next) if (ISVISIBLE(c)) n++; if (n > 0) for (c = nexttiled(m->clients); c; c = nexttiled(c->next)) resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0); }
void motionnotify(XEvent *e) {}
void movemouse(const Arg *arg) {}
Client *nexttiled(Client *c) { for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next); return c; }
void pop(Client *c) {}
void propertynotify(XEvent *e) {}
void quit(const Arg *arg) { running = 0; }
Monitor *recttomon(int x, int y, int w, int h) { return selmon; }
void resize(Client *c, int x, int y, int w, int h, int interact) { if (w <= 0 || h <= 0) return; c->x = x; c->y = y; c->w = w; c->h = h; XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h); configure(c); }
void resizeclient(Client *c, int x, int y, int w, int h) { resize(c, x, y, w, h, 0); }
void resizemouse(const Arg *arg) {}
void restack(Monitor *m) {}
void run(void) { XEvent ev; XSync(dpy, False); while (running && !XNextEvent(dpy, &ev)) if (handler[ev.type]) handler[ev.type](&ev); }
void scan(void) {}
int sendevent(Client *c, Atom proto) { return 0; }
void sendmon(Client *c, Monitor *m) {}
void setclientstate(Client *c, long state) { long data[] = { state, None }; XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32, PropModeReplace, (unsigned char *)data, 2); }
void setfocus(Client *c) { XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime); }
void setfullscreen(Client *c, int fullscreen) {}
void setlayout(const Arg *arg) { if (!arg || !arg->v) { selmon->sellt ^= 1; } else { selmon->sellt = (Layout *)arg->v - layouts; } arrange(selmon); }
void setmfact(const Arg *arg) { float f; if (!arg || !selmon) return; f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0; if (f < 0.1 || f > 0.9) return; selmon->mfact = f; arrange(selmon); }
void setup(void) { sigchld(0); screen = DefaultScreen(dpy); root = RootWindow(dpy, screen); sw = DisplayWidth(dpy, screen); sh = DisplayHeight(dpy, screen); wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False); wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False); wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False); wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False); XSetErrorHandler(xerror); XSync(dpy, False); XSetErrorHandler(xerror); XSelectInput(dpy, root, SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask); grabkeys(); updategeom(); createmon(); }
void seturgent(Client *c, int urg) {}
void showhide(Client *c) { if (!c) return; if (ISVISIBLE(c)) { XMoveWindow(dpy, c->win, c->x, c->y); if (c->mon->sel && c->mon->sel->isfloating) resize(c, c->x, c->y, c->w, c->h, 0); showhide(c->snext); } else { showhide(c->snext); XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y); } }
void sigchld(int unused) { if (signal(SIGCHLD, sigchld) == SIG_ERR) die("can't install SIGCHLD handler"); while (0 < waitpid(-1, NULL, WNOHANG)); }
void spawn(const Arg *arg) { if (fork() == 0) { if (dpy) close(ConnectionNumber(dpy)); setsid(); execvp(((char **)arg->v)[0], (char **)arg->v); fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]); perror(" failed"); exit(EXIT_FAILURE); } }
void tag(const Arg *arg) { if (selmon->sel && arg->ui & TAGMASK) { selmon->sel->tags = arg->ui & TAGMASK; focus(NULL); arrange(selmon); } }
void tagmon(const Arg *arg) {}
void tile(Monitor *m) { unsigned int i, n, h, mw, my, ty; Client *c; for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++); if (n == 0) return; if (n > m->nmaster) mw = m->nmaster ? m->ww * m->mfact : 0; else mw = m->ww; for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) if (i < m->nmaster) { h = (m->wh - my) / (MIN(n, m->nmaster) - i); resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0); my += HEIGHT(c); } else { h = (m->wh - ty) / (n - i); resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0); ty += HEIGHT(c); } }
void togglebar(const Arg *arg) {}
void togglefloating(const Arg *arg) { if (!selmon->sel) return; selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed; if (selmon->sel->isfloating) resize(selmon->sel, selmon->sel->x, selmon->sel->y, selmon->sel->w, selmon->sel->h, 0); arrange(selmon); }
void toggletag(const Arg *arg) { unsigned int newtags; if (!selmon->sel) return; newtags = selmon->sel->tags ^ (arg->ui & TAGMASK); if (newtags) { selmon->sel->tags = newtags; focus(NULL); arrange(selmon); } }
void toggleview(const Arg *arg) { unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK); if (newtagset) { selmon->tagset[selmon->seltags] = newtagset; focus(NULL); arrange(selmon); } }
void unfocus(Client *c, int setfocus) {}
void unmanage(Client *c, int destroyed) { Monitor *m = c->mon; detach(c); detachstack(c); if (!destroyed) { XGrabServer(dpy); XUngrabButton(dpy, AnyButton, AnyModifier, c->win); setclientstate(c, WithdrawnState); XSync(dpy, False); XUngrabServer(dpy); } free(c); focus(NULL); updateclientlist(); arrange(m); }
void unmapnotify(XEvent *e) { Client *c; XUnmapEvent *ev = &e->xunmap; if ((c = wintoclient(ev->window))) { if (ev->send_event) setclientstate(c, WithdrawnState); else unmanage(c, 0); } }
void updatebarpos(Monitor *m) {}
void updatebars(void) {}
void updateclientlist(void) {}
int updategeom(void) { if (!mons) createmon(); selmon = mons; selmon->mx = selmon->wx = 0; selmon->my = selmon->wy = 0; selmon->mw = selmon->ww = sw; selmon->mh = selmon->wh = sh; return 1; }
void updatenumlockmask(void) { unsigned int i, j; XModifierKeymap *modmap; numlockmask = 0; modmap = XGetModifierMapping(dpy); for (i = 0; i < 8; i++) for (j = 0; j < modmap->max_keypermod; j++) if (modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock)) numlockmask = (1 << i); XFreeModifiermap(modmap); }
void updatesizehints(Client *c) {}
void updatestatus(void) {}
void updatetitle(Client *c) { if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name)) gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name); if (c->name[0] == '\0') strcpy(c->name, broken); }
void updatewindowtype(Client *c) {}
void updatewmhints(Client *c) {}
void view(const Arg *arg) { if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags]) return; selmon->seltags ^= 1; if (arg->ui & TAGMASK) selmon->tagset[selmon->seltags] = arg->ui & TAGMASK; focus(NULL); arrange(selmon); }
Client *wintoclient(Window w) { Client *c; Monitor *m; for (m = mons; m; m = m->next) for (c = m->clients; c; c = c->next) if (c->win == w) return c; return NULL; }
Monitor *wintomon(Window w) { return selmon; }
int xerror(Display *dpy, XErrorEvent *ee) { if (ee->error_code == BadWindow || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch) || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable) || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable) || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable) || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch) || (ee->request_code == X_GrabButton && ee->error_code == BadAccess) || (ee->request_code == X_GrabKey && ee->error_code == BadAccess) || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)) return 0; fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n", ee->request_code, ee->error_code); return xerrorxlib(dpy, ee); }
int xerrordummy(Display *dpy, XErrorEvent *ee) { return 0; }
int xerrorstart(Display *dpy, XErrorEvent *ee) { die("dwm: another window manager is already running"); return -1; }
void zoom(const Arg *arg) {}

/* drawing stubs */
int drw_fontset_getwidth(Drw *drw, const char *text) { return 0; }

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-minimal-1.0\n");
	else if (argc != 1)
		die("usage: dwm [-v]\n");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display\n");
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
