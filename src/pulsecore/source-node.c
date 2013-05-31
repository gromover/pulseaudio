/***
  This file is part of PulseAudio.

  Copyright (c) 2013 Intel Corporation
  Author: Tanu Kaskinen <tanu.kaskinen@intel.com>

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

#include "source-node.h"

pa_source_node *pa_source_node_new(pa_source *source, const char *name) {
    pa_node_new_data data;
    pa_node *node;
    pa_source_node *source_node;

    pa_assert(source);
    pa_assert(name);

    pa_node_new_data_init(&data);
    pa_node_new_data_set_name(&data, name);
    pa_node_new_data_set_description(&data, pa_source_get_description(source));
    pa_node_new_data_set_direction(&data, PA_DIRECTION_INPUT);

    node = pa_node_new(source->core, &data);
    pa_node_new_data_done(&data);

    if (!node) {
        pa_log("Failed to create a node for source %s.", source->name);
        return NULL;
    }

    source_node = pa_xnew0(pa_source_node, 1);
    source_node->core = source->core;
    source_node->node = node;
    source_node->source = source;

    node->userdata = source_node;

    /* We may need to move this to pa_source_node_put() at some point. */
    pa_node_put(node);

    return source_node;
}

void pa_source_node_free(pa_source_node *source_node) {
    pa_assert(source_node);

    if (source_node->node)
        pa_node_free(source_node->node);

    pa_xfree(source_node);
}
