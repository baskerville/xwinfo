#ifndef XWINFO_XWINFO_H
#define XWINFO_XWINFO_H

#define MISSING_VALUE  "N/A"

struct wm_cookies {
    xcb_get_property_cookie_t class_instance;
    xcb_get_property_cookie_t icccm_name;
    xcb_get_property_cookie_t ewmh_name;
    xcb_get_property_cookie_t window_type;
    xcb_get_property_cookie_t window_state;
};

enum {
    CLASS_NAME = 1,
    INSTANCE_NAME = 2,
    WINDOW_NAME = 4,
    WINDOW_TYPE = 8,
    WINDOW_STATE = 16,
};

xcb_connection_t *dpy;
xcb_window_t root;
xcb_ewmh_connection_t *ewmh;

void setup(void);
void process_window(xcb_window_t win, unsigned int mask, struct wm_cookies *cookies);
void print_type_atom(xcb_atom_t a);
void print_state_atom(xcb_atom_t a);

#endif
