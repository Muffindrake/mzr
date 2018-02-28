#define CNV(x) ((x) / 255.0)
#define GRGB(hex) \
        {CNV(hex >> 16 & 0xFF), CNV(hex >> 8 & 0xFF), CNV(hex & 0xFF), 1.0}

/* edit colors to your liking */
#define MZR_PALETTE_SZ 16
const GdkRGBA mzr_palette[MZR_PALETTE_SZ] = {
        GRGB(0x000000),         /* black */
        GRGB(0xcc0000),         /* red */
        GRGB(0x00cc00),         /* green */
        GRGB(0xcccc00),         /* yellow */
        GRGB(0x0000cc),         /* blue */
        GRGB(0xcc00cc),         /* magenta */
        GRGB(0x00cccc),         /* cyan */
        GRGB(0xcccccc),         /* grey */
        GRGB(0xa8a8a8),         /* dark grey */
        GRGB(0xff0000),         /* bright red */
        GRGB(0x00ff00),         /* bright green */
        GRGB(0xffff00),         /* bright yellow */
        GRGB(0x0000ff),         /* bright blue */
        GRGB(0xff00ff),         /* bright magenta */
        GRGB(0x00ffff),         /* bright cyan */
        GRGB(0xffffff)          /* white */
};

const GdkRGBA mzr_color_cursor_fg = GRGB(0xff7e00);
const GdkRGBA mzr_color_cursor_bg = GRGB(0x000000);
const GdkRGBA mzr_color_fg = GRGB(0xff7e00);
const GdkRGBA mzr_color_bg = GRGB(0x000000);
#undef GRGB
#undef CNV

/* general configuration*/
#define MZR_FONT                "Liberation Mono 12"
#define MZR_DEFAULT_COL         80
#define MZR_DEFAULT_ROW         24
/* default value of $TERM variable */
#define MZR_TERM                "xterm-256color"

/* vte-specific below, caveat emptor */
/* urgency bell */
#define MZR_VTE_BELL_URGENT     1
/* audible bell */
#define MZR_VTE_BELL_SOUND      1
/* allow bold text */
#define MZR_VTE_BOLD_ALLOW      1
/* auto-hide mouse*/
#define MZR_VTE_MOUSE_HIDE      0
/* legal choices:
 * VTE_ERASE_AUTO, VTE_ERASE_ASCII_BACKSPACE, VTE_ERASE_ASCII_DELETE,
 * VTE_ERASE_DELETE_SEQUENCE, VTE_ERASE_TTY
 * (see enum VteEraseBinding docs)*/
#define MZR_VTE_BACKSPACE_CHAR  VTE_ERASE_ASCII_BACKSPACE
/* VTE_CURSOR_BLINK_SYSTEM (global GTK+), VTE_CURSOR_BLINK_ON,
 * VTE_CURSOR_BLINK_OFF */
#define MZR_VTE_CURSOR_BLINK    VTE_CURSOR_BLINK_OFF
/* VTE_CURSOR_SHAPE_BLOCK, VTE_CURSOR_SHAPE_IBEAM, VTE_CURSOR_SHAPE_UNDERLINE */
#define MZR_VTE_CURSOR_SHAPE    VTE_CURSOR_SHAPE_BLOCK
#define MZR_VTE_WORD_CHARS      "-,./?%&#_~:"
