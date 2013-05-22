/***
  This file is part of PulseAudio.

  Copyright (c) 2012 Intel Corporation
  Janos Kovacs <jankovac503@gmail.com>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pulsecore/core-util.h>
#include <pulsecore/namereg.h>

#include "node.h"

pa_node_new_data *pa_node_new_data_init(pa_node_new_data *data) {
    pa_assert(data);

    pa_zero(*data);
    data->direction = PA_DIRECTION_OUTPUT;

    return data;
}

void pa_node_new_data_set_name(pa_node_new_data *data, const char *name) {
    pa_assert(data);

    pa_xfree(data->name);
    data->name = pa_xstrdup(name);
}

void pa_node_new_data_set_description(pa_node_new_data *data, const char *description) {
    pa_assert(data);

    pa_xfree(data->description);
    data->description = pa_xstrdup(description);
}

void pa_node_new_data_set_direction(pa_node_new_data *data, pa_direction_t direction) {
    pa_assert(data);

    data->direction = direction;
}

void pa_node_new_data_done(pa_node_new_data *data) {
    pa_assert(data);

    pa_xfree(data->description);
    pa_xfree(data->name);
}

pa_node *pa_node_new(pa_core *core, pa_node_new_data *data) {
    pa_node *n = NULL;
    const char *name = NULL;

    pa_assert(core);
    pa_assert(data);
    pa_assert(data->name);
    pa_assert(data->description);

    n = pa_xnew0(pa_node, 1);

    if (!(name = pa_namereg_register(core, data->name, PA_NAMEREG_NODE, n, false))) {
        pa_log("Failed to register name %s.", data->name);
        goto fail;
    }

    pa_node_new_data_set_name(data, name);

    n->core = core;
    n->name = pa_xstrdup(name);
    n->description = pa_xstrdup(data->description);
    n->direction = data->direction;

    return n;

fail:
    if (name)
        pa_namereg_unregister(core, name);

    pa_node_free(n);

    return NULL;
}

void pa_node_free(pa_node *n) {
    pa_assert(n);

    pa_log_debug("Freeing node %s.", pa_strnull(n->name));

    if (n->core)
        pa_idxset_remove_by_data(n->core->nodes, n, NULL);

    if (n->proplist)
        pa_proplist_free(n->proplist);

    pa_xfree(n->description);

    if (n->name) {
        pa_namereg_unregister(n->core, n->name);
        pa_xfree(n->name);
    }

    pa_xfree(n);
}

void pa_node_put(pa_node *n) {
    pa_assert(n);

    pa_assert_se(pa_idxset_put(n->core->nodes, n, &n->index) >= 0);

    pa_log_debug("Created node %s.", n->name);
}
