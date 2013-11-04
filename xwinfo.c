#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "xwinfo.h"

int main(int argc, char *argv[])
{
    struct wm_cookies *cookies;
    cookies = malloc(sizeof(struct wm_cookies));
    if (cookies == NULL)
        err("Couldn't allocate memory for cookies.\n");
    unsigned int mask = 0;
    char opt;

    while ((opt = getopt(argc, argv, "hvcints")) != -1) {
        switch (opt) {
            case 'h':
                printf("xwinfo OPTIONS WID ...\n");
                return EXIT_SUCCESS;
                break;
            case 'v':
                printf("%s\n", VERSION);
                return EXIT_SUCCESS;
                break;
            case 'c':
                mask |= CLASS_NAME;
                break;
            case 'i':
                mask |= INSTANCE_NAME;
                break;
            case 'n':
                mask |= WINDOW_NAME;
                break;
            case 't':
                mask |= WINDOW_TYPE;
                break;
            case 's':
                mask |= WINDOW_STATE;
                break;
        }
    }

    int num = argc - optind;
    char **args = argv + optind;

    setup();

    if (num > 0) {
        char *end;
        for (int i = 0; i < num; i++) {
            errno = 0;
            long int wid = strtol(args[i], &end, 0);
            if (errno != 0 || *end != '\0')
                warn("Invalid window ID: '%s'\n", args[i]);
            else
                process_window(wid, mask, cookies);
        }
    } else {
        warn("Missing arguments.\n");
    }

    free(cookies);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(dpy);
    return EXIT_SUCCESS;
}

void setup(void)
{
    dpy = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(dpy))
        err("Can't open display.\n");
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    if (screen == NULL)
        err("Can't acquire screen.\n");
    root = screen->root;
    ewmh = malloc(sizeof(xcb_ewmh_connection_t));
    if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0)
        err("Can't initialize EWMH atoms.\n");
}

void process_window(xcb_window_t win, unsigned int mask, struct wm_cookies *cookies)
{
    if (mask & (CLASS_NAME | INSTANCE_NAME))
        cookies->class_instance = xcb_icccm_get_wm_class(dpy, win);
    if (mask & WINDOW_NAME) {
        cookies->ewmh_name = xcb_ewmh_get_wm_name(ewmh, win);
        cookies->icccm_name = xcb_icccm_get_wm_name(dpy, win);
    }
    if (mask & WINDOW_TYPE)
        cookies->window_type = xcb_ewmh_get_wm_window_type(ewmh, win);
    if (mask & WINDOW_STATE)
        cookies->window_state = xcb_ewmh_get_wm_state(ewmh, win);
    if (mask & (CLASS_NAME | INSTANCE_NAME)) {
        xcb_icccm_get_wm_class_reply_t reply;
        if (xcb_icccm_get_wm_class_reply(dpy, cookies->class_instance, &reply, NULL) == 1) {
            if (mask & CLASS_NAME)
                printf("%s\n", reply.class_name);
            if (mask & INSTANCE_NAME)
                printf("%s\n", reply.instance_name);
            xcb_icccm_get_wm_class_reply_wipe(&reply);
        } else {
            if (mask & CLASS_NAME)
                printf("%s\n", MISSING_VALUE);
            if (mask & INSTANCE_NAME)
                printf("%s\n", MISSING_VALUE);
        }
    }
    if (mask & WINDOW_NAME) {
        xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
        xcb_icccm_get_text_property_reply_t icccm_txt_prop;
        ewmh_txt_prop.strings = icccm_txt_prop.name = NULL;
        char title[MAXLEN];
        if (xcb_ewmh_get_wm_name_reply(ewmh, cookies->ewmh_name, &ewmh_txt_prop, NULL) == 1 ||
                xcb_icccm_get_wm_name_reply(dpy, cookies->icccm_name, &icccm_txt_prop, NULL) == 1) {
            if (ewmh_txt_prop.strings != NULL) {
                snprintf(title, ewmh_txt_prop.strings_len + 1, "%s", ewmh_txt_prop.strings);
                xcb_ewmh_get_utf8_strings_reply_wipe(&ewmh_txt_prop);
            } else {
                snprintf(title, icccm_txt_prop.name_len + 1, "%s", icccm_txt_prop.name);
                xcb_icccm_get_text_property_reply_wipe(&icccm_txt_prop);
            }
            printf("%s\n", title);
        } else {
            printf("%s\n", MISSING_VALUE);
        }
    }
    if (mask & WINDOW_TYPE) {
        xcb_ewmh_get_atoms_reply_t reply;
        if (xcb_ewmh_get_wm_window_type_reply(ewmh, cookies->window_type, &reply, NULL) == 1) {
            for (unsigned int i = 0; i < reply.atoms_len; i++) {
                xcb_atom_t a = reply.atoms[i];
                print_type_atom(a);
                if (i < (reply.atoms_len - 1))
                    printf("%s", " ");
            }
            printf("%s", "\n");
            xcb_ewmh_get_atoms_reply_wipe(&reply);
        } else {
            printf("%s\n", MISSING_VALUE);
        }
    }
    if (mask & WINDOW_STATE) {
        xcb_ewmh_get_atoms_reply_t reply;
        if (xcb_ewmh_get_wm_state_reply(ewmh, cookies->window_state, &reply, NULL) == 1) {
            for (unsigned int i = 0; i < reply.atoms_len; i++) {
                xcb_atom_t a = reply.atoms[i];
                print_state_atom(a);
                if (i < (reply.atoms_len - 1))
                    printf("%s", " ");
            }
            printf("%s", "\n");
            xcb_ewmh_get_atoms_reply_wipe(&reply);
        } else {
            printf("%s\n", MISSING_VALUE);
        }
    }
}

void print_type_atom(xcb_atom_t a)
{
    if (a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
        printf("%s", "desktop");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_DOCK)
        printf("%s", "dock");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR)
        printf("%s", "toolbar");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_MENU)
        printf("%s", "menu");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_UTILITY)
        printf("%s", "utility");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_SPLASH)
        printf("%s", "splash");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
        printf("%s", "dropdown_menu");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU)
        printf("%s", "popup_menu");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLTIP)
        printf("%s", "tooltip");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION)
        printf("%s", "notification");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_COMBO)
        printf("%s", "combo");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_DND)
        printf("%s", "dnd");
    else if (a == ewmh->_NET_WM_WINDOW_TYPE_NORMAL)
        printf("%s", "normal");
    else
        printf("%s", "unknown");
}

void print_state_atom(xcb_atom_t a)
{
    if (a == ewmh->_NET_WM_STATE_FULLSCREEN)
        printf("%s", "fullscreen");
    else if (a == ewmh->_NET_WM_STATE_STICKY)
        printf("%s", "sticky");
    else if (a == ewmh->_NET_WM_STATE_HIDDEN)
        printf("%s", "hidden");
    else if (a == ewmh->_NET_WM_STATE_SHADED)
        printf("%s", "shaded");
    else if (a == ewmh->_NET_WM_STATE_MODAL)
        printf("%s", "modal");
    else if (a == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION)
        printf("%s", "demands_attention");
    else if (a == ewmh->_NET_WM_STATE_ABOVE)
        printf("%s", "above");
    else if (a == ewmh->_NET_WM_STATE_BELOW)
        printf("%s", "below");
    else if (a == ewmh->_NET_WM_STATE_SKIP_TASKBAR)
        printf("%s", "skip_taskbar");
    else if (a == ewmh->_NET_WM_STATE_SKIP_PAGER)
        printf("%s", "skip_pager");
    else if (a == ewmh->_NET_WM_STATE_MAXIMIZED_VERT)
        printf("%s", "maximized_vert");
    else if (a == ewmh->_NET_WM_STATE_MAXIMIZED_HORZ)
        printf("%s", "maximized_horz");
    else
        printf("%s", "unknown");
}
