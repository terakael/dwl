/* Taken from https://github.com/djpohly/dwl/issues/466 */
#include <xkbcommon/xkbcommon-keysyms.h>
#define COLOR(hex)    { ((hex >> 24) & 0xFF) / 255.0f, \
                        ((hex >> 16) & 0xFF) / 255.0f, \
                        ((hex >> 8) & 0xFF) / 255.0f, \
                        (hex & 0xFF) / 255.0f }
/* appearance */
static const int sloppyfocus               = 1;  /* focus follows mouse */
static const int bypass_surface_visibility = 0;  /* 1 means idle inhibitors will disable idle tracking even if it's surface isn't visible  */
static const int smartgaps                 = 1;  /* 1 means no outer gap when there is only one window */
static const int hide_cursor_when_typing   = 1;
static const unsigned int gappih           = 10; /* horiz inner gap between windows */
static const unsigned int gappiv           = 10; /* vert inner gap between windows */
static const unsigned int gappoh           = 10; /* horiz outer gap between windows and screen edge */
static const unsigned int gappov           = 10; /* vert outer gap between windows and screen edge */
static const unsigned int borderpx         = 3;  /* border pixel of windows */
static const unsigned int snap             = 32; /* snap pixel */
static const float rootcolor[]             = COLOR(0x222222ff);
static const float bordercolor[]           = COLOR(0x25232201);
static const float focuscolor[]            = COLOR(0x96ad73ff);
static const float urgentcolor[]           = COLOR(0xff0000ff);
/* This conforms to the xdg-protocol. Set the alpha to zero to restore the old behavior */
static const float fullscreen_bg[]         = {0.0f, 0.0f, 0.0f, 1.0f}; /* You can also use glsl colors */

/* tagging - TAGCOUNT must be no greater than 31 */
#define TAGCOUNT (9)

/* logging */
static int log_level = WLR_ERROR;

static const Rule rules[] = {
	/* app_id             title       tags mask     isfloating   monitor */
	{ "Gimp_EXAMPLE",     NULL,       0,            1,           -1 }, /* Start on currently visible tags floating, not tiled */
	{ "firefox_EXAMPLE",  NULL,       1 << 8,       0,           -1 }, /* Start on ONLY tag "9" */
    /* default/example rule: can be changed but cannot be eliminated; at least one rule must exist */
};

static const Layout layouts[] = {
	{ "[\\]", dwindle },
};

/* monitors */
/* (x=-1, y=-1) is reserved as an "autoconfigure" monitor position indicator
 * WARNING: negative values other than (-1, -1) cause problems with Xwayland clients due to
 * https://gitlab.freedesktop.org/xorg/xserver/-/issues/899 */
static const MonitorRule monrules[] = {
   /* name        mfact  nmaster scale layout       rotate/reflect                x    y
    * example of a HiDPI laptop monitor:
    { "eDP-1",    0.5f,  1,      2,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL,   -1,  -1 }, */
	{ NULL,       0.55f, 1,      1,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL,   -1,  -1 },
	/* default monitor rule: can be changed but cannot be eliminated; at least one monitor rule must exist */
};

/* keyboard */
static const struct xkb_rule_names xkb_rules = {
	/* can specify fields: rules, model, layout, variant, options */
	/* example:
	.options = "ctrl:nocaps",
	*/
	.options = NULL,
};

static const int repeat_rate = 25;
static const int repeat_delay = 600;

/* Trackpad */
static const int tap_to_click = 1;
static const int tap_and_drag = 1;
static const int drag_lock = 1;
static const int natural_scrolling = 0;
static const int disable_while_typing = 1;
static const int left_handed = 0;
static const int middle_button_emulation = 0;
/* You can choose between:
LIBINPUT_CONFIG_SCROLL_NO_SCROLL
LIBINPUT_CONFIG_SCROLL_2FG
LIBINPUT_CONFIG_SCROLL_EDGE
LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN
*/
static const enum libinput_config_scroll_method scroll_method = LIBINPUT_CONFIG_SCROLL_2FG;

/* You can choose between:
LIBINPUT_CONFIG_CLICK_METHOD_NONE
LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS
LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER
*/
static const enum libinput_config_click_method click_method = LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

/* You can choose between:
LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE
*/
static const uint32_t send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;

/* You can choose between:
LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT
LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE
*/
static const enum libinput_config_accel_profile accel_profile = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
static const double accel_speed = 0.0;

/* You can choose between:
LIBINPUT_CONFIG_TAP_MAP_LRM -- 1/2/3 finger tap maps to left/right/middle
LIBINPUT_CONFIG_TAP_MAP_LMR -- 1/2/3 finger tap maps to left/middle/right
*/
static const enum libinput_config_tap_button_map button_map = LIBINPUT_CONFIG_TAP_MAP_LRM;

/* If you want to use the windows key for MODKEY, use WLR_MODIFIER_LOGO */
#define MODKEY WLR_MODIFIER_LOGO

#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                    KEY,            view,            {.ui = 1 << TAG} }, \
	{ MODKEY|WLR_MODIFIER_CTRL,  KEY,            toggleview,      {.ui = 1 << TAG} }, \
	{ MODKEY|WLR_MODIFIER_SHIFT, KEY,            tag,             {.ui = 1 << TAG} }, \
	{ MODKEY|WLR_MODIFIER_CTRL|WLR_MODIFIER_SHIFT,KEY,toggletag,  {.ui = 1 << TAG} }

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *tmuxcmd[] = { "footclient", "tmux", "-u", "new", "-A", "-s", "main", NULL };
static const char *devcmd[] = { "footclient", "ssh", "dan@dev", "-t", "tmux", "-u", "new", "-A", "-s", "main", NULL };
static const char *termcmd[] = { "footclient", NULL };
static const char *bluetoothcmd[] = { "footclient", "bluetui", NULL };
static const char *screenshotcmd[] = { "/bin/sh", "-c", "grim -g \"$(slurp)\" - | wl-copy", NULL };
static const char *menucmd[] = { "fuzzel", NULL };
static const char *filecmd[] = { "nautilus", NULL };
static const char *browsercmd[] = { "brave", NULL };
static const char *mailcmd[] = { "brave", "--app=https://mail.proton.me", NULL };
static const char *photocmd[] = { "brave", "--app=https://photos.intur.nl", NULL };
static const char *whatsappcmd[] = { "brave", "--app=https://web.whatsapp.com", NULL };
static const char *browserprivcmd[] = { "brave", "--incognito", NULL };
static const char *lockcmd[] = { "/bin/sh", "-c", "$HOME/.local/bin/lock", NULL };

enum { NORMAL = -1, VOLUME };
static const char *volupcmd[] = { "/bin/sh", "-c", "$HOME/.local/bin/volume up -t 0", NULL };
static const char *voldowncmd[] = { "/bin/sh", "-c", "$HOME/.local/bin/volume down -t 0", NULL };
static const char *volmutecmd[] = { "/bin/sh", "-c", "$HOME/.local/bin/volume mute -t 0", NULL };
static const char *volshowcmd[] = { "/bin/sh", "-c", "$HOME/.local/bin/volume show -t 0", NULL };
static const char *voldismisscmd[] = { "/bin/sh", "-c", "makoctl list -j | jq '.[] | select(.category == \"volume-osd\") | .id' | xargs -r -I {} makoctl dismiss -n {}", NULL };

static void onentermode(int mode) {
	if (mode == VOLUME)
		spawn(&(Arg){.v = volshowcmd});
}

static void onexitmode(int mode) {
	if (mode == VOLUME)
		spawn(&(Arg){.v = voldismisscmd});
}

static const Key keys[] = {
	/* modifier                  key                  function          argument */
	{ MODKEY,                    XKB_KEY_space,       spawn,            {.v = menucmd} },
	{ MODKEY,                    XKB_KEY_Return,      spawn,            {.v = tmuxcmd} },
	{ MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_Return,      spawn,            {.v = termcmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Return,      spawn,            {.v = devcmd} },
	{ MODKEY,                    XKB_KEY_e,           spawn,            {.v = filecmd} },
	{ MODKEY,                    XKB_KEY_v,           entermode,        {.i = VOLUME} },
	{ MODKEY,                    XKB_KEY_b,           spawn,            {.v = bluetoothcmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_b,           spawn,            {.v = browsercmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_m,           spawn,            {.v = mailcmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_p,           spawn,            {.v = photocmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_w,           spawn,            {.v = whatsappcmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_s,           spawn,            {.v = screenshotcmd} },
	{ MODKEY,                    XKB_KEY_Escape,      spawn,            {.v = lockcmd} },
	{ MODKEY|WLR_MODIFIER_SHIFT|WLR_MODIFIER_CTRL, XKB_KEY_b,           spawn,            {.v = browserprivcmd} },
	{ MODKEY,                    XKB_KEY_h,           focusdir,         {.ui = 0} },
	{ MODKEY,                    XKB_KEY_l,           focusdir,         {.ui = 1} },
	{ MODKEY,                    XKB_KEY_k,           focusdir,         {.ui = 2} },
	{ MODKEY,                    XKB_KEY_j,           focusdir,         {.ui = 3} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_h,           swapdir,          {.ui = 0} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_l,           swapdir,          {.ui = 1} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_k,           swapdir,          {.ui = 2} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_j,           swapdir,          {.ui = 3} },
	{ MODKEY,                    XKB_KEY_Tab,         view,             {0} },
	{ MODKEY,                    XKB_KEY_w,           killclient,       {0} },
	{ MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_space,       togglefloating,   {0} },
	{ MODKEY,                    XKB_KEY_f,           togglefullscreen, {0} },
	TAGKEYS(                     XKB_KEY_1,           0),
	TAGKEYS(                     XKB_KEY_2,           1),
	TAGKEYS(                     XKB_KEY_3,           2),
	TAGKEYS(                     XKB_KEY_4,           3),
	TAGKEYS(                     XKB_KEY_5,           4),
	TAGKEYS(                     XKB_KEY_6,           5),
	TAGKEYS(                     XKB_KEY_7,           6),
	TAGKEYS(                     XKB_KEY_8,           7),
	TAGKEYS(                     XKB_KEY_9,           8),

	/* Ctrl-Alt-Backspace and Ctrl-Alt-Fx used to be handled by X server */
	{ WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT,XKB_KEY_BackSpace, quit, {0} },
	/* Ctrl-Alt-Fx is used to switch to another VT, if you don't know what a VT is
	 * do not remove them.
	 */
#define CHVT(n) { WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT,XKB_KEY_F##n, chvt, {.ui = (n)} }
	CHVT(1), CHVT(2), CHVT(3), CHVT(4), CHVT(5), CHVT(6),
	CHVT(7), CHVT(8), CHVT(9), CHVT(10), CHVT(11), CHVT(12),
};

static const Modekey modekeys[] = {
	/* mode    modifier  key             function  argument */
	{ VOLUME, { 0,       XKB_KEY_k,      spawn,    {.v = volupcmd} } },
	{ VOLUME, { 0,       XKB_KEY_l,      spawn,    {.v = volupcmd} } },
	{ VOLUME, { 0,       XKB_KEY_j,      spawn,    {.v = voldowncmd} } },
	{ VOLUME, { 0,       XKB_KEY_h,      spawn,    {.v = voldowncmd} } },
	{ VOLUME, { 0,       XKB_KEY_m,      spawn,    {.v = volmutecmd} } },
	{ VOLUME, { 0,       XKB_KEY_Escape, entermode,{.i = NORMAL} } },
	{ VOLUME, { 0,       XKB_KEY_Return, entermode,{.i = NORMAL} } },
	{ VOLUME, { MODKEY,  XKB_KEY_v,      entermode,{.i = NORMAL} } },
};

static const Button buttons[] = {
	{ MODKEY, BTN_LEFT,   moveresize,     {.ui = CurMove} },
	{ MODKEY, BTN_MIDDLE, togglefloating, {0} },
	{ MODKEY, BTN_RIGHT,  moveresize,     {.ui = CurResize} },
};

static const Axis axes[] = {
	{ MODKEY, AxisUp,   spawn, SHCMD("volume-up_EXAMPLE") },
	{ MODKEY, AxisDown, spawn, SHCMD("volume-down_EXAMPLE") },
};
