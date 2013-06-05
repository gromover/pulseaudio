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

#include <pulsecore/core-util.h>

#include "port-node.h"

pa_port_node *pa_port_node_new(pa_device_port *port, const char *fallback_name) {
    char *name;
    pa_node_new_data data;
    pa_node *node;
    pa_port_node *port_node;

    pa_assert(port);
    pa_assert(fallback_name);

    if (port->device_class != PA_DEVICE_CLASS_UNKNOWN)
        name = pa_sprintf_malloc("%s-%s", pa_device_class_to_string(port->device_class),
                                 port->direction == PA_DIRECTION_OUTPUT ? "output" : "input");
    else
        name = pa_xstrdup(fallback_name);

    pa_node_new_data_init(&data);
    pa_node_new_data_set_name(&data, name);
    pa_xfree(name);
    pa_node_new_data_set_description(&data, port->description);
    pa_node_new_data_set_direction(&data, port->direction);

    node = pa_node_new(port->core, &data);
    pa_node_new_data_done(&data);

    if (!node) {
        pa_log("Failed to create a node for port %s.", port->name);
        return NULL;
    }

    port_node = pa_xnew0(pa_port_node, 1);
    port_node->core = port->core;
    port_node->node = node;
    port_node->port = port;

    node->userdata = port_node;

    /* We may need to move this to pa_port_node_put() at some point. */
    pa_node_put(node);

    return port_node;
}

void pa_port_node_free(pa_port_node *port_node) {
    pa_assert(port_node);

    if (port_node->node)
        pa_node_free(port_node->node);

    pa_xfree(port_node);
}
