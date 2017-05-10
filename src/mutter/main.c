/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Red Hat, Inc.
 * Copyright (c) 2008 Intel Corp.
 *
 * Based on plugin skeleton by:
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include <stdlib.h>
#include <string.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gjs/gjs.h>
#include <meta/main.h>
#include <meta/display.h>
#include <meta/meta-plugin.h>
#include <meta/util.h>

#include "shell-global.h"

#include "js.h"

ClutterActor *stage;
ClutterActor *scroll;

static void scroll_shell_plugin_start            (MetaPlugin          *plugin);
static void scroll_shell_plugin_minimize         (MetaPlugin          *plugin,
                                                  MetaWindowActor     *actor);
static void scroll_shell_plugin_unminimize       (MetaPlugin          *plugin,
                                                  MetaWindowActor     *actor);
static void scroll_shell_plugin_size_changed     (MetaPlugin          *plugin,
                                                  MetaWindowActor     *actor);
static void scroll_shell_plugin_size_change      (MetaPlugin          *plugin,
                                                  MetaWindowActor     *actor,
                                                  MetaSizeChange       which_change,
                                                  MetaRectangle       *old_frame_rect,
                                                  MetaRectangle       *old_buffer_rect);
static void scroll_shell_plugin_map              (MetaPlugin          *plugin,
                                                  MetaWindowActor     *actor);
static void scroll_shell_plugin_destroy          (MetaPlugin          *plugin,
                                                  MetaWindowActor     *actor);

static void scroll_shell_plugin_switch_workspace (MetaPlugin          *plugin,
                                                  gint                 from,
                                                  gint                 to,
                                                  MetaMotionDirection  direction);

static void scroll_shell_plugin_kill_window_effects   (MetaPlugin      *plugin,
                                                       MetaWindowActor *actor);
static void scroll_shell_plugin_kill_switch_workspace (MetaPlugin      *plugin);

static void scroll_shell_plugin_show_tile_preview (MetaPlugin      *plugin,
                                                   MetaWindow      *window,
                                                   MetaRectangle   *tile_rect,
                                                   int              tile_monitor);
static void scroll_shell_plugin_hide_tile_preview (MetaPlugin *plugin);
static void scroll_shell_plugin_show_window_menu  (MetaPlugin         *plugin,
                                                   MetaWindow         *window,
                                                   MetaWindowMenuType  menu,
                                                   int                 x,
                                                   int                 y);
static void scroll_shell_plugin_show_window_menu_for_rect (MetaPlugin         *plugin,
                                                           MetaWindow         *window,
                                                           MetaWindowMenuType  menu,
                                                           MetaRectangle      *rect);

static gboolean              scroll_shell_plugin_xevent_filter (MetaPlugin *plugin,
                                                                XEvent     *event);

static gboolean              scroll_shell_plugin_keybinding_filter (MetaPlugin *plugin,
                                                                    MetaKeyBinding *binding);

static void scroll_shell_plugin_confirm_display_change (MetaPlugin *plugin);

static const MetaPluginInfo *scroll_shell_plugin_plugin_info   (MetaPlugin *plugin);


#define GNOME_TYPE_SHELL_PLUGIN            (scroll_shell_plugin_get_type ())
#define SCROLL_SHELL_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_SHELL_PLUGIN, ScrollShellPlugin))
#define SCROLL_SHELL_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GNOME_TYPE_SHELL_PLUGIN, ScrollShellPluginClass))
#define GNOME_IS_SHELL_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCROLL_SHELL_PLUGIN_TYPE))
#define GNOME_IS_SHELL_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GNOME_TYPE_SHELL_PLUGIN))
#define SCROLL_SHELL_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GNOME_TYPE_SHELL_PLUGIN, ScrollShellPluginClass))

typedef struct _ScrollShellPlugin        ScrollShellPlugin;
typedef struct _ScrollShellPluginClass   ScrollShellPluginClass;

struct _ScrollShellPlugin
{
  MetaPlugin parent;

  int glx_error_base;
  int glx_event_base;
  guint have_swap_event : 1;
  CoglContext *cogl_context;

  /* ShellGlobal *global; */
};

struct _ScrollShellPluginClass
{
  MetaPluginClass parent_class;
};

GType scroll_shell_plugin_get_type (void);

G_DEFINE_TYPE (ScrollShellPlugin, scroll_shell_plugin, META_TYPE_PLUGIN)

ScrollShellPlugin *shell_plugin;


static void
scroll_shell_plugin_class_init (ScrollShellPluginClass *klass)
{
  MetaPluginClass *plugin_class  = META_PLUGIN_CLASS (klass);

  plugin_class->start            = scroll_shell_plugin_start;
  plugin_class->map              = scroll_shell_plugin_map;
  plugin_class->minimize         = scroll_shell_plugin_minimize;
  plugin_class->unminimize       = scroll_shell_plugin_unminimize;
  /* plugin_class->size_changed     = scroll_shell_plugin_size_changed; */
  plugin_class->size_change      = scroll_shell_plugin_size_change;
  plugin_class->destroy          = scroll_shell_plugin_destroy;

  plugin_class->switch_workspace = scroll_shell_plugin_switch_workspace;

  plugin_class->kill_window_effects   = scroll_shell_plugin_kill_window_effects;
  plugin_class->kill_switch_workspace = scroll_shell_plugin_kill_switch_workspace;

  plugin_class->show_tile_preview = scroll_shell_plugin_show_tile_preview;
  plugin_class->hide_tile_preview = scroll_shell_plugin_hide_tile_preview;
  plugin_class->show_window_menu = scroll_shell_plugin_show_window_menu;
  plugin_class->show_window_menu_for_rect = scroll_shell_plugin_show_window_menu_for_rect;

  plugin_class->xevent_filter     = scroll_shell_plugin_xevent_filter;
  plugin_class->keybinding_filter = scroll_shell_plugin_keybinding_filter;

  plugin_class->confirm_display_change = scroll_shell_plugin_confirm_display_change;

  plugin_class->plugin_info       = scroll_shell_plugin_plugin_info;
}

static void
scroll_shell_plugin_init (ScrollShellPlugin *shell_plugin)
{
}

static gboolean
scroll_shell_plugin_has_swap_event (ScrollShellPlugin *shell_plugin)
{
  MetaPlugin *plugin = META_PLUGIN (shell_plugin);
  CoglDisplay *cogl_display =
    cogl_context_get_display (shell_plugin->cogl_context);
  CoglRenderer *renderer = cogl_display_get_renderer (cogl_display);
  const char * (* query_extensions_string) (Display *dpy, int screen);
  Bool (* query_extension) (Display *dpy, int *error, int *event);
  MetaScreen *screen;
  Display *xdisplay;
  const char *glx_extensions;

  /* We will only get swap events if Cogl is using GLX */
  if (cogl_renderer_get_winsys_id (renderer) != COGL_WINSYS_ID_GLX)
    return FALSE;

  screen = meta_plugin_get_screen (plugin);

  xdisplay = clutter_x11_get_default_display ();

  query_extensions_string =
    (void *) cogl_get_proc_address ("glXQueryExtensionsString");
  query_extension =
    (void *) cogl_get_proc_address ("glXQueryExtension");

  query_extension (xdisplay,
                   &shell_plugin->glx_error_base,
                   &shell_plugin->glx_event_base);

  glx_extensions =
    query_extensions_string (xdisplay,
                             meta_screen_get_screen_number (screen));

  return strstr (glx_extensions, "GLX_INTEL_swap_event") != NULL;
}



static ShellWM *
get_shell_wm (void)
{
    return shell_global_get_shell_wm(shell_global_get());
}

static void
scroll_shell_plugin_start (MetaPlugin *plugin)
{
  shell_plugin = SCROLL_SHELL_PLUGIN (plugin);
  GError *error = NULL;
  int status;
  GjsContext *gjs_context;
  ClutterBackend *backend;

  _shell_global_set_plugin(plugin);

  backend = clutter_get_default_backend ();
  shell_plugin->cogl_context = clutter_backend_get_cogl_context (backend);

  shell_plugin->have_swap_event =
    scroll_shell_plugin_has_swap_event (shell_plugin);

  MetaScreen *screen = meta_plugin_get_screen(shell_plugin);
  stage = meta_get_stage_for_screen (screen);
  scroll = clutter_actor_new();
  ClutterLayoutManager *layout = clutter_box_layout_new();
  clutter_actor_set_layout_manager(scroll, layout);
  clutter_actor_show(stage);
  clutter_actor_add_child(stage, scroll);
  /* shell_perf_log_define_event (shell_perf_log_get_default (), */
  /*                              "glx.swapComplete", */
  /*                              "GL buffer swap complete event received (with timestamp of completion)", */
  /*                              "x"); */

  /* shell_plugin->global = shell_global_get (); */
  /* _shell_global_set_plugin (shell_plugin->global, META_PLUGIN (shell_plugin)); */

  /* gjs_context = _shell_global_get_gjs_context (shell_plugin->global); */

  /* if (!gjs_context_eval (gjs_context, */
  /*                        "imports.ui.environment.init();" */
  /*                        "imports.ui.main.start();", */
  /*                        -1, */
  /*                        "<main>", */
  /*                        &status, */
  /*                        &error)) */
  /*   { */
  /*     g_message ("Execution of main.js threw exception: %s", error->message); */
  /*     g_error_free (error); */
  /*     /\* We just exit() here, since in a development environment you'll get the */
  /*      * error in your shell output, and it's way better than a busted WM, */
  /*      * which typically manifests as a white screen. */
  /*      * */
  /*      * In production, we shouldn't crash =)  But if we do, we should get */
  /*      * restarted by the session infrastructure, which is likely going */
  /*      * to be better than some undefined state. */
  /*      * */
  /*      * If there was a generic "hook into bug-buddy for non-C crashes" */
  /*      * infrastructure, here would be the place to put it. */
  /*      *\/ */
  /*     g_object_unref (gjs_context); */
  /*     exit (1); */
  /*   } */
}

/* static ShellWM * */
/* get_shell_wm (void) */
/* { */
/*   ShellWM *wm; */

/*   g_object_get (shell_global_get (), */
/*                 "window-manager", &wm, */
/*                 NULL); */
/*   /\* drop extra ref added by g_object_get *\/ */
/*   g_object_unref (wm); */

/*   return wm; */
/* } */

static void
scroll_shell_plugin_minimize (MetaPlugin         *plugin,
			     MetaWindowActor    *actor)
{
  _shell_wm_minimize (get_shell_wm (),
                      actor);

}

static void
scroll_shell_plugin_unminimize (MetaPlugin         *plugin,
                               MetaWindowActor    *actor)
{
  _shell_wm_unminimize (get_shell_wm (),
                      actor);

}

static void
scroll_shell_plugin_size_changed (MetaPlugin         *plugin,
                                 MetaWindowActor    *actor)
{
  _shell_wm_size_changed (get_shell_wm (), actor);
}

static void
scroll_shell_plugin_size_change (MetaPlugin         *plugin,
                                MetaWindowActor    *actor,
                                MetaSizeChange      which_change,
                                MetaRectangle      *old_frame_rect,
                                MetaRectangle      *old_buffer_rect)
{
  _shell_wm_size_change (get_shell_wm (), actor, which_change, old_frame_rect, old_buffer_rect);
}

static void
scroll_shell_plugin_map (MetaPlugin         *plugin,
                        MetaWindowActor    *actor)
{
  _shell_wm_map (get_shell_wm (),
                 actor);
}

static void
scroll_shell_plugin_destroy (MetaPlugin         *plugin,
                            MetaWindowActor    *actor)
{
  _shell_wm_destroy (get_shell_wm (),
                     actor);
}

static void
scroll_shell_plugin_switch_workspace (MetaPlugin         *plugin,
                                     gint                from,
                                     gint                to,
                                     MetaMotionDirection direction)
{
  _shell_wm_switch_workspace (get_shell_wm(), from, to, direction);
}

static void
scroll_shell_plugin_kill_window_effects (MetaPlugin         *plugin,
                                        MetaWindowActor    *actor)
{
  _shell_wm_kill_window_effects (get_shell_wm(), actor);
}

static void
scroll_shell_plugin_kill_switch_workspace (MetaPlugin         *plugin)
{
  _shell_wm_kill_switch_workspace (get_shell_wm());
}

static void
scroll_shell_plugin_show_tile_preview (MetaPlugin      *plugin,
                                      MetaWindow      *window,
                                      MetaRectangle   *tile_rect,
                                      int              tile_monitor)
{
  _shell_wm_show_tile_preview (get_shell_wm (), window, tile_rect, tile_monitor);
}

static void
scroll_shell_plugin_hide_tile_preview (MetaPlugin *plugin)
{
  _shell_wm_hide_tile_preview (get_shell_wm ());
}

static void
scroll_shell_plugin_show_window_menu (MetaPlugin         *plugin,
                                     MetaWindow         *window,
                                     MetaWindowMenuType  menu,
                                     int                 x,
                                     int                 y)
{
  _shell_wm_show_window_menu (get_shell_wm (), window, menu, x, y);
}

static void
scroll_shell_plugin_show_window_menu_for_rect (MetaPlugin         *plugin,
                                              MetaWindow         *window,
                                              MetaWindowMenuType  menu,
                                              MetaRectangle      *rect)
{
  _shell_wm_show_window_menu_for_rect (get_shell_wm (), window, menu, rect);
}

static gboolean
scroll_shell_plugin_xevent_filter (MetaPlugin *plugin,
                                  XEvent     *xev)
{
#ifdef GLX_INTEL_swap_event
  ScrollShellPlugin *shell_plugin = SCROLL_SHELL_PLUGIN (plugin);

  if (shell_plugin->have_swap_event &&
      xev->type == (shell_plugin->glx_event_base + GLX_BufferSwapComplete))
    {
      GLXBufferSwapComplete *swap_complete_event;
      swap_complete_event = (GLXBufferSwapComplete *)xev;

      /* Buggy early versions of the INTEL_swap_event implementation in Mesa
       * can send this with a ust of 0. Simplify life for consumers
       * by ignoring such events */
      if (swap_complete_event->ust != 0)
        {
          gboolean frame_timestamps;
          g_object_get (shell_plugin->global,
                        "frame-timestamps", &frame_timestamps,
                        NULL);

          if (frame_timestamps)
            shell_perf_log_event_x (shell_perf_log_get_default (),
                                    "glx.swapComplete",
                                    swap_complete_event->ust);
        }
    }
#endif

  return FALSE;
}

static gboolean
scroll_shell_plugin_keybinding_filter (MetaPlugin     *plugin,
                                      MetaKeyBinding *binding)
{
}

static void
scroll_shell_plugin_confirm_display_change (MetaPlugin *plugin)
{
}

static const
MetaPluginInfo *scroll_shell_plugin_plugin_info (MetaPlugin *plugin)
{
  static const MetaPluginInfo info = {
    .name = "GNOME Shell",
    .version = "0.1",
    .author = "Various",
    .license = "GPLv2+",
    .description = "Provides GNOME Shell core functionality"
  };

  return &info;
}


void
main(int argc, char *argv[]) {
  GError *error = NULL;

  js_maybe_generate_gir_and_exit(argc, argv);

  js_init();

  meta_plugin_manager_set_plugin_type (scroll_shell_plugin_get_type ());
  GOptionContext *ctx = meta_get_option_context ();
  g_option_context_parse(ctx, &argc, &argv, &error);
  meta_init();

  meta_run();
}