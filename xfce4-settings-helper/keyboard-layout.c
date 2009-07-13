/* $Id$ */
/*
 *  Copyright (c) 2008 Olivier Fourdan <olivier@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>

#ifdef HAVE_LIBXKLAVIER
#include <libxklavier/xklavier.h>
#endif /* HAVE_LIBXKLAVIER */

#include "keyboard-layout.h"

static void xfce_keyboard_layout_helper_finalize                  (GObject                       *object);
static void xfce_keyboard_layout_helper_set_model                 (XfceKeyboardLayoutHelper      *helper);
static void xfce_keyboard_layout_helper_set_layout                (XfceKeyboardLayoutHelper      *helper);
static void xfce_keyboard_layout_helper_set_variant               (XfceKeyboardLayoutHelper      *helper);
static void xfce_keyboard_layout_helper_channel_property_changed  (XfconfChannel                 *channel,
                                                                   const gchar                   *property_name,
                                                                   const GValue                  *value,
                                                                   XfceKeyboardLayoutHelper      *helper);

struct _XfceKeyboardLayoutHelperClass
{
    GObjectClass __parent__;
};

struct _XfceKeyboardLayoutHelper
{
    GObject  __parent__;

    /* xfconf channel */
    XfconfChannel     *channel;
    gboolean xkb_disable_settings;

#ifdef HAVE_LIBXKLAVIER
    /* libxklavier */
    XklEngine         *engine;
    XklConfigRegistry *registry;
    XklConfigRec      *config;
#endif /* HAVE_LIBXKLAVIER */
};

G_DEFINE_TYPE (XfceKeyboardLayoutHelper, xfce_keyboard_layout_helper, G_TYPE_OBJECT);

static void
xfce_keyboard_layout_helper_class_init (XfceKeyboardLayoutHelperClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = xfce_keyboard_layout_helper_finalize;
}

static void
xfce_keyboard_layout_helper_init (XfceKeyboardLayoutHelper *helper)
{
    /* init */
    helper->channel = NULL;

    /* open the channel */
    helper->channel = xfconf_channel_new ("keyboard-layout");

    /* monitor channel changes */
    g_signal_connect (G_OBJECT (helper->channel), "property-changed", G_CALLBACK (xfce_keyboard_layout_helper_channel_property_changed), helper);

#ifdef HAVE_LIBXKLAVIER
    helper->engine = xkl_engine_get_instance (GDK_DISPLAY ());
    helper->config = xkl_config_rec_new ();
    xkl_config_rec_get_from_server (helper->config, helper->engine);
#endif /* HAVE_LIBXKLAVIER */

    /* load settings */
    helper->xkb_disable_settings = xfconf_channel_get_bool (helper->channel, "/Default/XkbDisable", TRUE);
    xfce_keyboard_layout_helper_set_model (helper);
    xfce_keyboard_layout_helper_set_layout (helper);
    xfce_keyboard_layout_helper_set_variant (helper);
}

static void
xfce_keyboard_layout_helper_finalize (GObject *object)
{
    XfceKeyboardLayoutHelper *helper = XFCE_KEYBOARD_LAYOUT_HELPER (object);

    /* release the channel */
    if (G_LIKELY (helper->channel))
        g_object_unref (G_OBJECT (helper->channel));

    (*G_OBJECT_CLASS (xfce_keyboard_layout_helper_parent_class)->finalize) (object);
}

static void
xfce_keyboard_layout_helper_set_model (XfceKeyboardLayoutHelper *helper)
{
#ifdef HAVE_LIBXKLAVIER
    gchar *xkbmodel;

    if (!helper->xkb_disable_settings)
    {
        xkbmodel = xfconf_channel_get_string (helper->channel, "/Default/XkbModel", helper->config->model);
        g_free (helper->config->model);
        helper->config->model = xkbmodel;
        xkl_config_rec_activate (helper->config, helper->engine);
    }
#endif /* HAVE_LIBXKLAVIER */
}

static void
xfce_keyboard_layout_helper_set_layout (XfceKeyboardLayoutHelper *helper)
{
#ifdef HAVE_LIBXKLAVIER
    gchar *default_layouts, *val_layout;
    gchar **layouts;

    if (!helper->xkb_disable_settings)
    {
        default_layouts  = g_strjoinv(",", helper->config->layouts);
        val_layout  = xfconf_channel_get_string (helper->channel, "/Default/XkbLayout",  default_layouts);
        layouts = g_strsplit_set (val_layout, ",", 0);
        g_strfreev(helper->config->layouts);
        helper->config->layouts = layouts;
        xkl_config_rec_activate (helper->config, helper->engine);
        g_free (val_layout);
        g_free (default_layouts);
    }
#endif /* HAVE_LIBXKLAVIER */
}

static void
xfce_keyboard_layout_helper_set_variant (XfceKeyboardLayoutHelper *helper)
{
#ifdef HAVE_LIBXKLAVIER
    gchar *default_variants, *val_variant;
    gchar **variants;

    if (!helper->xkb_disable_settings)
    {
        default_variants  = g_strjoinv(",", helper->config->variants);
        val_variant  = xfconf_channel_get_string (helper->channel, "/Default/XkbVariant",  default_variants);
        variants = g_strsplit_set (val_variant, ",", 0);
        g_strfreev(helper->config->variants);
        helper->config->variants = variants;
        xkl_config_rec_activate (helper->config, helper->engine);
        g_free (val_variant);
        g_free (default_variants);
    }
#endif /* HAVE_LIBXKLAVIER */
}

static void
xfce_keyboard_layout_helper_channel_property_changed (XfconfChannel      *channel,
                                               const gchar               *property_name,
                                               const GValue              *value,
                                               XfceKeyboardLayoutHelper  *helper)
{
    g_return_if_fail (helper->channel == channel);

    if (strcmp (property_name, "/Default/XkbDisable") == 0)
    {
        helper->xkb_disable_settings = g_value_get_boolean (value);
        /* Apply all settings */
        xfce_keyboard_layout_helper_set_model (helper);
        xfce_keyboard_layout_helper_set_layout (helper);
        xfce_keyboard_layout_helper_set_variant (helper);
    }
    else if (strcmp (property_name, "/Default/XkbModel") == 0)
    {
        xfce_keyboard_layout_helper_set_layout (helper);
    }
    else if (strcmp (property_name, "/Default/XkbLayout") == 0)
    {
        xfce_keyboard_layout_helper_set_layout (helper);
    }
    else if (strcmp (property_name, "/Default/XkbVariant") == 0)
    {
        xfce_keyboard_layout_helper_set_variant (helper);
    }
}
