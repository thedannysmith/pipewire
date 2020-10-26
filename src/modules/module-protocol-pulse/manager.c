/* PipeWire
 *
 * Copyright © 2020 Wim Taymans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "manager.h"
#include <extensions/metadata.h>

#define manager_emit_sync(m) spa_hook_list_call(&m->hooks, struct pw_manager_events, sync, 0)
#define manager_emit_added(m,o) spa_hook_list_call(&m->hooks, struct pw_manager_events, added, 0, o)
#define manager_emit_updated(m,o) spa_hook_list_call(&m->hooks, struct pw_manager_events, updated, 0, o)
#define manager_emit_removed(m,o) spa_hook_list_call(&m->hooks, struct pw_manager_events, removed, 0, o)
#define manager_emit_metadata(m,s,k,t,v) spa_hook_list_call(&m->hooks, struct pw_manager_events, metadata,0,s,k,t,v)

struct manager {
	struct pw_manager this;

	struct spa_hook core_listener;
	struct spa_hook registry_listener;
	int sync_seq;

	struct spa_hook_list hooks;
};

struct object_info {
	const char *type;
	uint32_t version;
	const void *events;
	void (*destroy) (void *object);
};

struct object {
	struct pw_manager_object this;

	struct manager *manager;

	const struct object_info *info;

	struct spa_hook proxy_listener;
	struct spa_hook object_listener;

	unsigned int new:1;
};

static void core_sync(struct manager *m)
{
	m->sync_seq = pw_core_sync(m->this.core, PW_ID_CORE, m->sync_seq);
}

static struct object *find_object(struct manager *m, uint32_t id)
{
	struct object *o;
	spa_list_for_each(o, &m->this.object_list, this.link) {
		if (o->this.id == id)
			return o;
	}
	return NULL;
}

static void object_destroy(struct object *o)
{
	struct manager *m = o->manager;
	if (o->this.proxy)
		pw_proxy_destroy(o->this.proxy);
	free(o->this.type);
	if (o->this.props)
		pw_properties_free(o->this.props);
	spa_list_remove(&o->this.link);
	m->this.n_objects--;
        free(o);
}

/* client */
static void client_event_info(void *object, const struct pw_client_info *info)
{
        struct object *o = object;
        pw_log_debug("object %p: id:%d change-mask:%"PRIu64, o, o->this.id, info->change_mask);
        info = o->this.info = pw_client_info_update(o->this.info, info);
}

static const struct pw_client_events client_events = {
	PW_VERSION_CLIENT_EVENTS,
	.info = client_event_info,
};

static void client_destroy(void *data)
{
	struct object *o = data;
	if (o->this.info)
		pw_client_info_free(o->this.info);
}

static const struct object_info client_info = {
	.type = PW_TYPE_INTERFACE_Client,
	.version = PW_VERSION_CLIENT,
	.events = &client_events,
	.destroy = client_destroy,
};

/* module */
static void module_event_info(void *object, const struct pw_module_info *info)
{
        struct object *o = object;
        pw_log_debug("object %p: id:%d change-mask:%"PRIu64, o, o->this.id, info->change_mask);
        info = o->this.info = pw_module_info_update(o->this.info, info);
}

static const struct pw_module_events module_events = {
	PW_VERSION_MODULE_EVENTS,
	.info = module_event_info,
};

static void module_destroy(void *data)
{
	struct object *o = data;
	if (o->this.info)
		pw_module_info_free(o->this.info);
}

static const struct object_info module_info = {
	.type = PW_TYPE_INTERFACE_Module,
	.version = PW_VERSION_MODULE,
	.events = &module_events,
	.destroy = module_destroy,
};

/* device */
static void device_event_info(void *object, const struct pw_device_info *info)
{
        struct object *o = object;
        pw_log_debug("object %p: id:%d change-mask:%"PRIu64, o, o->this.id, info->change_mask);
        info = o->this.info = pw_device_info_update(o->this.info, info);
}

static const struct pw_device_events device_events = {
	PW_VERSION_DEVICE_EVENTS,
	.info = device_event_info,
};

static void device_destroy(void *data)
{
	struct object *o = data;
	if (o->this.info)
		pw_device_info_free(o->this.info);
}

static const struct object_info device_info = {
	.type = PW_TYPE_INTERFACE_Device,
	.version = PW_VERSION_DEVICE,
	.events = &device_events,
	.destroy = device_destroy,
};

/* node */
static void node_event_info(void *object, const struct pw_node_info *info)
{
	struct object *o = object;
	pw_log_debug("object %p: id:%d change-mask:%"PRIu64, o, o->this.id, info->change_mask);
	info = o->this.info = pw_node_info_update(o->this.info, info);
}

static const struct pw_node_events node_events = {
	PW_VERSION_NODE_EVENTS,
	.info = node_event_info,
};

static void node_destroy(void *data)
{
	struct object *o = data;
	if (o->this.info)
		pw_node_info_free(o->this.info);
}

static const struct object_info node_info = {
	.type = PW_TYPE_INTERFACE_Node,
	.version = PW_VERSION_NODE,
	.events = &node_events,
	.destroy = node_destroy,
};

/* link */
static const struct object_info link_info = {
	.type = PW_TYPE_INTERFACE_Link,
	.version = PW_VERSION_LINK,
};

/* metadata */
static int metadata_property(void *object,
			uint32_t subject,
			const char *key,
			const char *type,
			const char *value)
{
	struct object *o = object;
	struct manager *m = o->manager;
	manager_emit_metadata(m, subject, key, type, value);
        return 0;
}

static const struct pw_metadata_events metadata_events = {
	PW_VERSION_METADATA_EVENTS,
	.property = metadata_property,
};

static const struct object_info metadata_info = {
	.type = PW_TYPE_INTERFACE_Metadata,
	.version = PW_VERSION_METADATA,
	.events = &metadata_events,
};

static const struct object_info *objects[] =
{
	&module_info,
	&client_info,
	&device_info,
	&node_info,
	&link_info,
	&metadata_info,
};

static const struct object_info *find_info(const char *type, uint32_t version)
{
	size_t i;
	for (i = 0; i < SPA_N_ELEMENTS(objects); i++) {
		if (strcmp(objects[i]->type, type) == 0 &&
		    objects[i]->version <= version)
			return objects[i];
	}
	return NULL;
}

static uint32_t clear_params(struct spa_list *param_list, uint32_t id)
{
	struct pw_manager_param *p, *t;
	uint32_t count = 0;

	spa_list_for_each_safe(p, t, param_list, link) {
		if (id == SPA_ID_INVALID || p->id == id) {
			spa_list_remove(&p->link);
			free(p);
			count++;
		}
	}
	return count;
}

static void
destroy_removed(void *data)
{
	struct object *o = data;
	pw_proxy_destroy(o->this.proxy);
}

static void
destroy_proxy(void *data)
{
	struct object *o = data;

	clear_params(&o->this.param_list, SPA_ID_INVALID);

	if (o->info && o->info->destroy)
                o->info->destroy(o);

        o->this.proxy = NULL;
}

static const struct pw_proxy_events proxy_events = {
        PW_VERSION_PROXY_EVENTS,
        .removed = destroy_removed,
        .destroy = destroy_proxy,
};

static void registry_event_global(void *data, uint32_t id,
			uint32_t permissions, const char *type, uint32_t version,
			const struct spa_dict *props)
{
	struct manager *m = data;
	struct object *o;
	const struct object_info *info;
	struct pw_proxy *proxy;

	info = find_info(type, version);
	if (info == NULL)
		return;

	proxy = pw_registry_bind(m->this.registry,
			id, type, info->version, 0);
        if (proxy == NULL)
		return;

	o = calloc(1, sizeof(*o));
	if (o == NULL) {
		pw_log_error("can't alloc object for %u %s/%d: %m", id, type, version);
		pw_proxy_destroy(proxy);
		return;
	}
	o->this.id = id;
	o->this.permissions = permissions;
	o->this.type = strdup(type);
	o->this.version = version;
	o->this.props = props ? pw_properties_new_dict(props) : NULL;
	o->this.proxy = proxy;
	spa_list_init(&o->this.param_list);

	o->manager = m;
	o->info = info;
	o->new = true;
	spa_list_append(&m->this.object_list, &o->this.link);
	m->this.n_objects++;

	if (info->events)
		pw_proxy_add_object_listener(proxy,
				&o->object_listener,
				o->info->events, o);
	pw_proxy_add_listener(proxy,
			&o->proxy_listener,
			&proxy_events, o);

	core_sync(m);
}

static void registry_event_global_remove(void *object, uint32_t id)
{
	struct manager *m = object;
	struct object *o;

	if ((o = find_object(m, id)) == NULL)
		return;

	manager_emit_removed(m, &o->this);

	object_destroy(o);
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
	.global_remove = registry_event_global_remove,
};

static void on_core_done(void *data, uint32_t id, int seq)
{
	struct manager *m = data;
	if (id == PW_ID_CORE) {
		if (m->sync_seq == seq)
			manager_emit_sync(m);
	}
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.done = on_core_done,
};

struct pw_manager *pw_manager_new(struct pw_core *core)
{
	struct manager *m;

	m = calloc(1, sizeof(*m));
	if (m == NULL)
		return NULL;

	m->this.core = core;
	spa_hook_list_init(&m->hooks);

	spa_list_init(&m->this.object_list);

	pw_core_add_listener(m->this.core,
			&m->core_listener,
			&core_events, m);
	m->this.registry = pw_core_get_registry(m->this.core,
			PW_VERSION_REGISTRY, 0);
	pw_registry_add_listener(m->this.registry,
			&m->registry_listener,
			&registry_events, m);

	core_sync(m);

	return &m->this;
}

void pw_manager_add_listener(struct pw_manager *manager,
		struct spa_hook *listener,
		const struct pw_manager_events *events, void *data)
{
	struct manager *m = SPA_CONTAINER_OF(manager, struct manager, this);
	spa_hook_list_append(&m->hooks, listener, events, data);
}

struct pw_manager_object *pw_manager_find_object(struct pw_manager *manager,
		uint32_t id)
{
	struct manager *m = SPA_CONTAINER_OF(manager, struct manager, this);
	struct object *o;

	o = find_object(m, id);
	if (o == NULL)
		return NULL;
	return (struct pw_manager_object*)o;
}

int pw_manager_for_each_object(struct pw_manager *manager,
		int (*callback) (void *data, struct pw_manager_object *object),
		void *data)
{
	struct manager *m = SPA_CONTAINER_OF(manager, struct manager, this);
	struct object *o;
	int res;

	spa_list_for_each(o, &m->this.object_list, this.link) {
		if ((res = callback(data, &o->this)) != 0)
			return res;
	}
	return 0;
}

void  pw_manager_destroy(struct pw_manager *manager)
{
	struct manager *m = SPA_CONTAINER_OF(manager, struct manager, this);
	struct object *o;

	spa_hook_remove(&m->core_listener);

	spa_list_consume(o, &m->this.object_list, this.link)
		object_destroy(o);

	if (m->this.registry) {
		spa_hook_remove(&m->registry_listener);
		pw_proxy_destroy((struct pw_proxy*)m->this.registry);
	}
	free(m);
}