#ifndef foonodehfoo
#define foonodehfoo

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

typedef struct pa_node_new_data pa_node_new_data;
typedef struct pa_node pa_node;

#include <pulsecore/core.h>

struct pa_node_new_data {
    char *name;
    char *description;

    pa_direction_t direction;
};

struct pa_node {
    uint32_t index;
    pa_core *core;

    char *name;
    char *description;

    pa_direction_t direction;
    pa_proplist *proplist;

    void *userdata;
};

pa_node_new_data *pa_node_new_data_init(pa_node_new_data *data);
void pa_node_new_data_set_name(pa_node_new_data *data, const char *name);
void pa_node_new_data_set_description(pa_node_new_data *data, const char *description);
void pa_node_new_data_set_direction(pa_node_new_data *data, pa_direction_t direction);
void pa_node_new_data_done(pa_node_new_data *data);

pa_node *pa_node_new(pa_core *core, pa_node_new_data *data);
void pa_node_free(pa_node *node);

void pa_node_put(pa_node *node);
/* No need for pa_node_unlink() currently, just use pa_node_free(). */

#endif
