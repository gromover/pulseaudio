#ifndef foosourcenodehfoo
#define foosourcenodehfoo

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

typedef struct pa_source_node pa_source_node;

#include <pulsecore/core.h>
#include <pulsecore/node.h>

struct pa_source_node {
    pa_core *core;
    pa_node *node;
    pa_source *source;
};

pa_source_node *pa_source_node_new(pa_source *source, const char *name);
void pa_source_node_free(pa_source_node *source_node);

#endif
