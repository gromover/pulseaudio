/***
  This file is part of PulseAudio.

  Copyright 2006 Lennart Poettering
  Copyright 2009 Canonical Ltd
  Copyright (C) 2012 Intel Corporation

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/xmalloc.h>

#include <pulsecore/core.h>
#include <pulsecore/modargs.h>
#include <pulsecore/source-output.h>
#include <pulsecore/source.h>
#include <pulsecore/core-util.h>

#include "module-bluetooth-policy-symdef.h"

PA_MODULE_AUTHOR("Frédéric Dalleau");
PA_MODULE_DESCRIPTION("When a bluetooth sink or source is added, load module-loopback");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(TRUE);
PA_MODULE_USAGE(
        "a2dp_source=<Handle a2dp_source card profile (sink role)?> "
        "hfgw=<Handle hfgw card profile (headset role)?>");

static const char* const valid_modargs[] = {
    "a2dp_source",
    "hfgw",
    NULL
};

struct userdata {
    bool enable_a2dp_source;
    bool enable_hfgw;
    pa_hook_slot *source_put_slot;
    pa_hook_slot *sink_put_slot;
    pa_hook_slot *port_available_changed_slot;
};

/* When a source is created, loopback it to default sink */
static pa_hook_result_t source_put_hook_callback(pa_core *c, pa_source *source, void *userdata) {
    struct userdata *u = userdata;
    const char *s;
    const char *role;
    char *args;

    pa_assert(c);
    pa_assert(source);

    /* Only consider bluetooth sinks and sources */
    s = pa_proplist_gets(source->proplist, PA_PROP_DEVICE_BUS);
    if (!s)
        return PA_HOOK_OK;

    if (!pa_streq(s, "bluetooth"))
        return PA_HOOK_OK;

    s = pa_proplist_gets(source->proplist, "bluetooth.protocol");
    if (!s)
        return PA_HOOK_OK;

    if (u->enable_a2dp_source && pa_streq(s, "a2dp_source")) /* A2DP profile (we're doing sink role) */
        role = "music";
    else if (u->enable_hfgw && pa_streq(s, "hfgw")) /* HFP profile (we're doing headset role) */
        role = "phone";
    else {
        pa_log_debug("Profile %s cannot be selected for loopback", s);
        return PA_HOOK_OK;
    }

    /* Load module-loopback */
    args = pa_sprintf_malloc("source=\"%s\" source_dont_move=\"true\" sink_input_properties=\"media.role=%s\"", source->name, role);
    (void) pa_module_load(c, "module-loopback", args);
    pa_xfree(args);

    return PA_HOOK_OK;
}

/* When a sink is created, loopback it to default source */
static pa_hook_result_t sink_put_hook_callback(pa_core *c, pa_sink *sink, void *userdata) {
    struct userdata *u = userdata;
    const char *s;
    const char *role;
    char *args;

    pa_assert(c);
    pa_assert(sink);

    /* Only consider bluetooth sinks and sources */
    s = pa_proplist_gets(sink->proplist, PA_PROP_DEVICE_BUS);
    if (!s)
        return PA_HOOK_OK;

    if (!pa_streq(s, "bluetooth"))
        return PA_HOOK_OK;

    s = pa_proplist_gets(sink->proplist, "bluetooth.protocol");
    if (!s)
        return PA_HOOK_OK;

    if (u->enable_hfgw && pa_streq(s, "hfgw")) /* HFP profile (we're doing headset role) */
        role = "phone";
    else {
        pa_log_debug("Profile %s cannot be selected for loopback", s);
        return PA_HOOK_OK;
    }

    /* Load module-loopback */
    args = pa_sprintf_malloc("sink=\"%s\" sink_dont_move=\"true\" source_output_properties=\"media.role=%s\"", sink->name, role);
    (void) pa_module_load(c, "module-loopback", args);
    pa_xfree(args);

    return PA_HOOK_OK;
}

static pa_device_port* find_best_port(pa_hashmap *ports) {
    void *state;
    pa_device_port *port;
    pa_device_port *result = NULL;

    PA_HASHMAP_FOREACH(port, ports, state) {
        if (port->available != PA_PORT_AVAILABLE_YES)
            continue;

        if (result == NULL || port->priority > result->priority)
            result = port;
    }

    return result;
}

static void set_port_profile(pa_card *card, pa_device_port *port) {
    void *state;
    pa_card_profile *profile;

    PA_HASHMAP_FOREACH(profile, port->profiles, state) {
        if (card->active_profile == profile)
            return;

        pa_log_debug("Setting card '%s' to profile '%s'", card->name, profile->name);

        if (pa_card_set_profile(card, profile->name, FALSE) != 0)
            pa_log_warn("Could not set profile '%s'", profile->name);

        return;
    }
}

static pa_hook_result_t port_available_hook_callback(pa_core *c, pa_device_port *port, void *userdata) {
    pa_card *card;
    const char *s;
    uint32_t state;
    pa_bool_t is_active_profile;
    pa_device_port *port2;

    PA_IDXSET_FOREACH(card, c->cards, state)
        if (port == pa_hashmap_get(card->ports, port->name))
            break;

    if (!card) {
        pa_log_warn("Did not find port %s in array of cards", port->name);
        return PA_HOOK_OK;
    }

    /* Only consider bluetooth cards */
    s = pa_proplist_gets(card->proplist, PA_PROP_DEVICE_BUS);
    if (!s || !pa_streq(s, "bluetooth"))
        return PA_HOOK_OK;

    is_active_profile = card->active_profile == pa_hashmap_get(port->profiles, card->active_profile->name);

    if (is_active_profile && port->available == PA_PORT_AVAILABLE_YES)
        return PA_HOOK_OK;

    if (!is_active_profile && port->available != PA_PORT_AVAILABLE_YES)
        return PA_HOOK_OK;

    if ((port2 = find_best_port(card->ports)) == NULL)
        return PA_HOOK_OK;

    set_port_profile(card, port2);

    return PA_HOOK_OK;
}

static void handle_all_ports(pa_core *core) {
    pa_card *card;
    uint32_t state;

    PA_IDXSET_FOREACH(card, core->cards, state) {
        pa_device_port *port;
        void *state2;

        PA_HASHMAP_FOREACH(port, card->ports, state2)
            port_available_hook_callback(core, port, NULL);
    }
}

int pa__init(pa_module *m) {
    pa_modargs *ma;
    struct userdata *u;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log_error("Failed to parse module arguments");
        return -1;
    }

    m->userdata = u = pa_xnew0(struct userdata, 1);

    u->enable_a2dp_source = TRUE;
    if (pa_modargs_get_value_boolean(ma, "a2dp_source", &u->enable_a2dp_source) < 0) {
        pa_log("Failed to parse a2dp_source argument.");
        goto fail;
    }

    u->enable_hfgw = TRUE;
    if (pa_modargs_get_value_boolean(ma, "hfgw", &u->enable_hfgw) < 0) {
        pa_log("Failed to parse hfgw argument.");
        goto fail;
    }

    u->source_put_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_PUT], PA_HOOK_NORMAL, (pa_hook_cb_t) source_put_hook_callback, u);

    u->sink_put_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_PUT], PA_HOOK_NORMAL, (pa_hook_cb_t) sink_put_hook_callback, u);

    u->port_available_changed_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_PORT_AVAILABLE_CHANGED],
                                       PA_HOOK_NORMAL, (pa_hook_cb_t) port_available_hook_callback, u);

    handle_all_ports(m->core);

    pa_modargs_free(ma);
    return 0;

fail:
    pa_modargs_free(ma);
    return -1;
}

void pa__done(pa_module *m) {
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata))
        return;

    if (u->source_put_slot)
        pa_hook_slot_free(u->source_put_slot);

    if (u->sink_put_slot)
        pa_hook_slot_free(u->sink_put_slot);

    if (u->port_available_changed_slot)
        pa_hook_slot_free(u->port_available_changed_slot);

    pa_xfree(u);
}
