/*
 * Generic Counter interface
 * Copyright (C) 2017 William Breathitt Gray
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/types.h>

#include <linux/counter.h>

ssize_t counter_signal_enum_read(struct counter_device *counter,
	struct counter_signal *signal, void *priv, char *buf)
{
	const struct counter_signal_enum_ext *const e = priv;
	int err;
	size_t index;

	if (!e->get)
		return -EINVAL;

	err = e->get(counter, signal, &index);
	if (err)
		return err;

	if (index >= e->num_items)
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%s\n", e->items[index]);
}
EXPORT_SYMBOL(counter_signal_enum_read);

ssize_t counter_signal_enum_write(struct counter_device *counter,
	struct counter_signal *signal, void *priv, const char *buf, size_t len)
{
	const struct counter_signal_enum_ext *const e = priv;
	ssize_t index;
	int err;

	if (!e->set)
		return -EINVAL;

	index = __sysfs_match_string(e->items, e->num_items, buf);
	if (index < 0)
		return index;

	err = e->set(counter, signal, index);
	if (err)
		return err;

	return len;
}
EXPORT_SYMBOL(counter_signal_enum_write);

ssize_t counter_signal_enum_available_read(struct counter_device *counter,
	struct counter_signal *signal, void *priv, char *buf)
{
	const struct counter_signal_enum_ext *const e = priv;
	size_t i;
	size_t len = 0;

	if (!e->num_items)
		return 0;

	for (i = 0; i < e->num_items; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
			e->items[i]);

	return len;
}
EXPORT_SYMBOL(counter_signal_enum_available_read);

ssize_t counter_count_enum_read(struct counter_device *counter,
	struct counter_count *count, void *priv, char *buf)
{
	const struct counter_count_enum_ext *const e = priv;
	int err;
	size_t index;

	if (!e->get)
		return -EINVAL;

	err = e->get(counter, count, &index);
	if (err)
		return err;

	if (index >= e->num_items)
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%s\n", e->items[index]);
}
EXPORT_SYMBOL(counter_count_enum_read);

ssize_t counter_count_enum_write(struct counter_device *counter,
	struct counter_count *count, void *priv, const char *buf, size_t len)
{
	const struct counter_count_enum_ext *const e = priv;
	ssize_t index;
	int err;

	if (!e->set)
		return -EINVAL;

	index = __sysfs_match_string(e->items, e->num_items, buf);
	if (index < 0)
		return index;

	err = e->set(counter, count, index);
	if (err)
		return err;

	return len;
}
EXPORT_SYMBOL(counter_count_enum_write);

ssize_t counter_count_enum_available_read(struct counter_device *counter,
	struct counter_count *count, void *priv, char *buf)
{
	const struct counter_count_enum_ext *const e = priv;
	size_t i;
	size_t len = 0;

	if (!e->num_items)
		return 0;

	for (i = 0; i < e->num_items; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
			e->items[i]);

	return len;
}
EXPORT_SYMBOL(counter_count_enum_available_read);

ssize_t counter_device_enum_read(struct counter_device *counter, void *priv,
	char *buf)
{
	const struct counter_device_enum_ext *const e = priv;
	int err;
	size_t index;

	if (!e->get)
		return -EINVAL;

	err = e->get(counter, &index);
	if (err)
		return err;

	if (index >= e->num_items)
		return -EINVAL;

	return scnprintf(buf, PAGE_SIZE, "%s\n", e->items[index]);
}
EXPORT_SYMBOL(counter_device_enum_read);

ssize_t counter_device_enum_write(struct counter_device *counter, void *priv,
	const char *buf, size_t len)
{
	const struct counter_device_enum_ext *const e = priv;
	ssize_t index;
	int err;

	if (!e->set)
		return -EINVAL;

	index = __sysfs_match_string(e->items, e->num_items, buf);
	if (index < 0)
		return index;

	err = e->set(counter, index);
	if (err)
		return err;

	return len;
}
EXPORT_SYMBOL(counter_device_enum_write);

ssize_t counter_device_enum_available_read(struct counter_device *counter,
	void *priv, char *buf)
{
	const struct counter_device_enum_ext *const e = priv;
	size_t i;
	size_t len = 0;

	if (!e->num_items)
		return 0;

	for (i = 0; i < e->num_items; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
			e->items[i]);

	return len;
}
EXPORT_SYMBOL(counter_device_enum_available_read);

static const char *const signal_level_str[] = {
	[SIGNAL_LEVEL_LOW] = "low",
	[SIGNAL_LEVEL_HIGH] = "high"
};

/**
 * set_signal_read_value - set signal_read_value data
 * @val:	signal_read_value structure to set
 * @type:	property Signal data represents
 * @data:	Signal data
 *
 * This function sets an opaque signal_read_value structure with the provided
 * Signal data.
 */
void set_signal_read_value(signal_read_value *const val,
	const enum signal_value_type type, void *const data)
{
	if (type == SIGNAL_LEVEL)
		val->len = scnprintf(val->buf, PAGE_SIZE, "%s\n",
			signal_level_str[*(enum signal_level *)data]);
	else
		val->len = 0;
}
EXPORT_SYMBOL(set_signal_read_value);

/**
 * set_count_read_value - set count_read_value data
 * @val:	count_read_value structure to set
 * @type:	property Count data represents
 * @data:	Count data
 *
 * This function sets an opaque count_read_value structure with the provided
 * Count data.
 */
void set_count_read_value(count_read_value *const val,
	const enum count_value_type type, void *const data)
{
	switch (type) {
	case COUNT_POSITION_UNSIGNED:
		val->len = scnprintf(val->buf, PAGE_SIZE, "%lu\n",
			*(unsigned long *)data);
		break;
	case COUNT_POSITION_SIGNED:
		val->len = scnprintf(val->buf, PAGE_SIZE, "%ld\n",
			*(long *)data);
		break;
	default:
		val->len = 0;
	}
}
EXPORT_SYMBOL(set_count_read_value);

/**
 * get_count_write_value - get count_write_value data
 * @data:	Count data
 * @type:	property Count data represents
 * @val:	count_write_value structure containing data
 *
 * This function extracts Count data from the provided opaque count_write_value
 * structure and stores it at the address provided by @data.
 *
 * RETURNS:
 * 0 on success, negative error number on failure.
 */
int get_count_write_value(void *const data,
	const enum count_value_type type, const count_write_value *const val)
{
	int err;

	switch (type) {
	case COUNT_POSITION_UNSIGNED:
		err = kstrtoul(val->buf, 0, data);
		if (err)
			return err;
		break;
	case COUNT_POSITION_SIGNED:
		err = kstrtol(val->buf, 0, data);
		if (err)
			return err;
		break;
	}

	return 0;
}
EXPORT_SYMBOL(get_count_write_value);

struct counter_device_attr {
	struct device_attribute		dev_attr;
	struct list_head		l;
	void				*component;
};

static int counter_attribute_create(const char *const prefix,
	const char *const name,
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
		char *buf),
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len),
	void *const component, struct counter_device_state *const device_state)
{
	struct counter_device_attr *counter_attr;
	struct device_attribute *dev_attr;
	int err;
	struct list_head *const attr_list = &device_state->attr_list;

	/* Allocate a Counter device attribute */
	counter_attr = kzalloc(sizeof(*counter_attr), GFP_KERNEL);
	if (!counter_attr)
		return -ENOMEM;
	dev_attr = &counter_attr->dev_attr;

	sysfs_attr_init(&dev_attr->attr);

	/* Configure device attribute */
	dev_attr->attr.name = kasprintf(GFP_KERNEL, "%s%s", prefix, name);
	if (!dev_attr->attr.name) {
		err = -ENOMEM;
		goto err_free_counter_attr;
	}
	if (show) {
		dev_attr->attr.mode |= 0444;
		dev_attr->show = show;
	}
	if (store) {
		dev_attr->attr.mode |= 0200;
		dev_attr->store = store;
	}

	/* Store associated Counter component with attribute */
	counter_attr->component = component;

	/* Keep track of the attribute for later cleanup */
	list_add(&counter_attr->l, attr_list);
	device_state->num_attr++;

	return 0;

err_free_counter_attr:
	kfree(counter_attr);
	return err;
}

#define to_counter_attr(_dev_attr) \
	container_of(_dev_attr, struct counter_device_attr, dev_attr)

struct signal_comp_t {
	struct counter_signal	*signal;
};

static ssize_t counter_signal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct counter_device *const counter = dev_get_drvdata(dev);
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct signal_comp_t *const component = devattr->component;
	struct counter_signal *const signal = component->signal;
	int err;
	signal_read_value val = { .buf = buf };

	err = counter->signal_read(counter, signal, &val);
	if (err)
		return err;

	return val.len;
}

struct name_comp_t {
	const char	*name;
};

static ssize_t counter_device_attr_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct name_comp_t *const comp = to_counter_attr(attr)->component;

	return scnprintf(buf, PAGE_SIZE, "%s\n", comp->name);
}

struct signal_ext_comp_t {
	struct counter_signal		*signal;
	const struct counter_signal_ext	*ext;
};

static ssize_t counter_signal_ext_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct signal_ext_comp_t *const component = devattr->component;
	const struct counter_signal_ext *const ext = component->ext;
	struct counter_device *const counter = dev_get_drvdata(dev);
	struct counter_signal *const signal = component->signal;

	return ext->read(counter, signal, ext->priv, buf);
}

static ssize_t counter_signal_ext_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct signal_ext_comp_t *const component = devattr->component;
	const struct counter_signal_ext *const ext = component->ext;
	struct counter_device *const counter = dev_get_drvdata(dev);
	struct counter_signal *const signal = component->signal;

	return ext->write(counter, signal, ext->priv, buf, len);
}

static int counter_signal_ext_register(const char *const prefix,
	struct counter_signal *const signal,
	struct counter_device_state *const device_state)
{
	const size_t num_ext = signal->num_ext;
	size_t i;
	const struct counter_signal_ext *ext;
	struct signal_ext_comp_t *signal_ext_comp;
	int err;

	/* Return early if no extensions */
	if (!signal->ext || !num_ext)
		return 0;

	/* Create an attribute for each extension */
	for (i = 0 ; i < num_ext; i++) {
		ext = signal->ext + i;

		/* Allocate signal_ext attribute component */
		signal_ext_comp = kmalloc(sizeof(*signal_ext_comp), GFP_KERNEL);
		if (!signal_ext_comp)
			return -ENOMEM;
		signal_ext_comp->signal = signal;
		signal_ext_comp->ext = ext;

		/* Allocate a Counter device attribute */
		err = counter_attribute_create(prefix, ext->name,
			(ext->read) ? counter_signal_ext_show : NULL,
			(ext->write) ? counter_signal_ext_store : NULL,
			signal_ext_comp, device_state);
		if (err) {
			kfree(signal_ext_comp);
			return err;
		}
	}

	return 0;
}

static int counter_signal_attributes_create(const char *const signal_attr_name,
	const struct counter_device *const counter,
	struct counter_signal *const signal)
{
	struct signal_comp_t *signal_comp;
	struct counter_device_state *const device_state = counter->device_state;
	int err;
	const char *prefix;
	struct name_comp_t *name_comp;

	/* Allocate Signal attribute component */
	signal_comp = kmalloc(sizeof(*signal_comp), GFP_KERNEL);
	if (!signal_comp)
		return -ENOMEM;
	signal_comp->signal = signal;

	/* Create main Signal attribute */
	err = counter_attribute_create("", signal_attr_name,
		(counter->signal_read) ? counter_signal_show : NULL, NULL,
		signal_comp, device_state);
	if (err) {
		kfree(signal_comp);
		return err;
	}

	prefix = kasprintf(GFP_KERNEL, "%s_", signal_attr_name);
	if (!prefix)
		return -ENOMEM;

	/* Create Signal name attribute */
	if (signal->name) {
		/* Allocate name attribute component */
		name_comp = kmalloc(sizeof(*name_comp), GFP_KERNEL);
		if (!name_comp) {
			err = -ENOMEM;
			goto err_free_prefix;
		}
		name_comp->name = signal->name;

		/* Allocate Signal name attribute */
		err = counter_attribute_create(prefix, "name",
			counter_device_attr_name_show, NULL, name_comp,
			device_state);
		if (err) {
			kfree(name_comp);
			goto err_free_prefix;
		}
	}

	/* Register Signal extension attributes */
	err = counter_signal_ext_register(prefix, signal, device_state);
	if (err)
		goto err_free_prefix;

	kfree(prefix);

	return 0;

err_free_prefix:
	kfree(prefix);
	return err;
}

static int counter_signals_register(const struct counter_device *const counter)
{
	const size_t num_signals = counter->num_signals;
	struct counter_device_state *const device_state = counter->device_state;
	struct device *const dev = &device_state->dev;
	size_t i;
	struct counter_signal *signal;
	const char *name;
	int err;

	/* At least one Signal must be defined */
	if (!counter->signals || !num_signals) {
		dev_err(dev, "Signals undefined\n");
		return -EINVAL;
	}

	/* Register each Signal */
	for (i = 0; i < num_signals; i++) {
		signal = counter->signals + i;

		/* Generate Signal attribute name */
		name = kasprintf(GFP_KERNEL, "signal%d", signal->id);
		if (!name)
			return -ENOMEM;

		/* Create all attributes associated with Signal */
		err = counter_signal_attributes_create(name, counter, signal);
		if (err)
			goto err_free_name;

		kfree(name);
	}

	return 0;

err_free_name:
	kfree(name);
	return err;
}

static const char *const synapse_action_str[] = {
	[SYNAPSE_ACTION_NONE] = "none",
	[SYNAPSE_ACTION_RISING_EDGE] = "rising edge",
	[SYNAPSE_ACTION_FALLING_EDGE] = "falling edge",
	[SYNAPSE_ACTION_BOTH_EDGES] = "both edges"
};

struct action_comp_t {
	struct counter_synapse	*synapse;
	struct counter_count	*count;
};

static ssize_t counter_action_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	int err;
	struct counter_device *const counter = dev_get_drvdata(dev);
	const struct action_comp_t *const component = devattr->component;
	struct counter_count *const count = component->count;
	struct counter_synapse *const synapse = component->synapse;
	size_t action_index;
	enum synapse_action action;

	err = counter->action_get(counter, count, synapse, &action_index);
	if (err)
		return err;

	synapse->action = action_index;

	action = synapse->actions[action_index];
	return scnprintf(buf, PAGE_SIZE, "%s\n", synapse_action_str[action]);
}

static ssize_t counter_action_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct action_comp_t *const component = devattr->component;
	struct counter_synapse *const synapse = component->synapse;
	size_t action_index;
	const size_t num_actions = synapse->num_actions;
	enum synapse_action action;
	int err;
	struct counter_device *const counter = dev_get_drvdata(dev);
	struct counter_count *const count = component->count;

	/* Find requested action mode */
	for (action_index = 0; action_index < num_actions; action_index++) {
		action = synapse->actions[action_index];
		if (sysfs_streq(buf, synapse_action_str[action]))
			break;
	}
	/* If requested action mode not found */
	if (action_index >= num_actions)
		return -EINVAL;

	err = counter->action_set(counter, count, synapse, action_index);
	if (err)
		return err;

	synapse->action = action_index;

	return len;
}

struct action_avail_comp_t {
	const enum synapse_action	*actions;
	size_t				num_actions;
};

static ssize_t counter_synapse_action_available_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct action_avail_comp_t *const component = devattr->component;
	const enum synapse_action *const actions = component->actions;
	const size_t num_actions = component->num_actions;
	size_t i;
	enum synapse_action action;
	ssize_t len = 0;

	for (i = 0; i < num_actions; i++) {
		action = actions[i];
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
			synapse_action_str[action]);
	}

	return len;
}

static int counter_synapses_register(const struct counter_device *const counter,
	struct counter_count *const count, const char *const count_attr_name)
{
	struct counter_device_state *const device_state = counter->device_state;
	const size_t num_synapses = count->num_synapses;
	struct device *const dev = &device_state->dev;
	size_t i;
	struct counter_synapse *synapse;
	const char *prefix;
	struct action_comp_t *action_comp;
	int err;
	struct action_avail_comp_t *avail_comp;

	/* At least one Synapse must be defined */
	if (!count->synapses || !num_synapses) {
		dev_err(dev, "Count '%d' Synapses undefined\n",	count->id);
		return -EINVAL;
	}

	/* Register each Synapse */
	for (i = 0; i < num_synapses; i++) {
		synapse = count->synapses + i;

		/* Ensure all Synapses have a defined Signal */
		if (!synapse->signal) {
			dev_err(dev,
				"Count '%d' Synapse '%zu' Signal undefined\n",
				count->id, i);
			return -EINVAL;
		}

		/* At least one action mode must be defined for each Synapse */
		if (!synapse->actions || !synapse->num_actions) {
			dev_err(dev,
				"Count '%d' Signal '%d' action modes undefined\n",
				count->id, synapse->signal->id);
			return -EINVAL;
		}

		/* Generate attribute prefix */
		prefix = kasprintf(GFP_KERNEL, "%s_signal%d_", count_attr_name,
			synapse->signal->id);
		if (!prefix)
			return -ENOMEM;

		/* Allocate action attribute component */
		action_comp = kmalloc(sizeof(*action_comp), GFP_KERNEL);
		if (!action_comp) {
			err = -ENOMEM;
			goto err_free_prefix;
		}
		action_comp->synapse = synapse;
		action_comp->count = count;

		/* Create action attribute */
		err = counter_attribute_create(prefix, "action",
			(counter->action_get) ? counter_action_show : NULL,
			(counter->action_set) ? counter_action_store : NULL,
			action_comp, device_state);
		if (err) {
			kfree(action_comp);
			goto err_free_prefix;
		}

		/* Allocate action available attribute component */
		avail_comp = kmalloc(sizeof(*avail_comp), GFP_KERNEL);
		if (!avail_comp) {
			err = -ENOMEM;
			goto err_free_prefix;
		}
		avail_comp->actions = synapse->actions;
		avail_comp->num_actions = synapse->num_actions;

		/* Create action_available attribute */
		err = counter_attribute_create(prefix, "action_available",
			counter_synapse_action_available_show, NULL,
			avail_comp, device_state);
		if (err) {
			kfree(avail_comp);
			goto err_free_prefix;
		}

		kfree(prefix);
	}

	return 0;

err_free_prefix:
	kfree(prefix);
	return err;
}

struct count_comp_t {
	struct counter_count	*count;
};

static ssize_t counter_count_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct counter_device *const counter = dev_get_drvdata(dev);
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_comp_t *const component = devattr->component;
	struct counter_count *const count = component->count;
	int err;
	count_read_value val = { .buf = buf };

	err = counter->count_read(counter, count, &val);
	if (err)
		return err;

	return val.len;
}

static ssize_t counter_count_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	struct counter_device *const counter = dev_get_drvdata(dev);
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_comp_t *const component = devattr->component;
	struct counter_count *const count = component->count;
	int err;
	count_write_value val = { .buf = buf };

	err = counter->count_write(counter, count, &val);
	if (err)
		return err;

	return len;
}

static const char *const count_function_str[] = {
	[COUNT_FUNCTION_INCREASE] = "increase",
	[COUNT_FUNCTION_DECREASE] = "decrease",
	[COUNT_FUNCTION_PULSE_DIRECTION] = "pulse-direction",
	[COUNT_FUNCTION_QUADRATURE_X1] = "quadrature x1",
	[COUNT_FUNCTION_QUADRATURE_X2] = "quadrature x2",
	[COUNT_FUNCTION_QUADRATURE_X4] = "quadrature x4"
};

static ssize_t counter_function_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int err;
	struct counter_device *const counter = dev_get_drvdata(dev);
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_comp_t *const component = devattr->component;
	struct counter_count *const count = component->count;
	size_t func_index;
	enum count_function function;

	err = counter->function_get(counter, count, &func_index);
	if (err)
		return err;

	count->function = func_index;

	function = count->functions[func_index];
	return scnprintf(buf, PAGE_SIZE, "%s\n", count_function_str[function]);
}

static ssize_t counter_function_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_comp_t *const component = devattr->component;
	struct counter_count *const count = component->count;
	const size_t num_functions = count->num_functions;
	size_t func_index;
	enum count_function function;
	int err;
	struct counter_device *const counter = dev_get_drvdata(dev);

	/* Find requested Count function mode */
	for (func_index = 0; func_index < num_functions; func_index++) {
		function = count->functions[func_index];
		if (sysfs_streq(buf, count_function_str[function]))
			break;
	}
	/* Return error if requested Count function mode not found */
	if (func_index >= num_functions)
		return -EINVAL;

	err = counter->function_set(counter, count, func_index);
	if (err)
		return err;

	count->function = func_index;

	return len;
}

static ssize_t counter_count_synapses_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	size_t i;
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_comp_t *const component = devattr->component;
	struct counter_count *const count = component->count;
	const struct counter_synapse *synapse;
	enum synapse_action action;
	ssize_t len = 0;

	/* Print out a list of every Signal association to Count */
	for (i = 0; i < count->num_synapses; i++) {
		synapse = count->synapses + i;
		action = synapse->actions[synapse->action];
		len += snprintf(buf + len, PAGE_SIZE - len, "%d\t%s\t%s\n",
			synapse->signal->id, synapse->signal->name,
			synapse_action_str[action]);
		if (len >= PAGE_SIZE)
			return -ENOMEM;
	}

	return len;
}

struct count_ext_comp_t {
	struct counter_count		*count;
	const struct counter_count_ext	*ext;
};

static ssize_t counter_count_ext_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_ext_comp_t *const comp = devattr->component;
	const struct counter_count_ext *const ext = comp->ext;
	struct counter_device *const counter = dev_get_drvdata(dev);
	struct counter_count *const count = comp->count;

	return ext->read(counter, count, ext->priv, buf);
}

static ssize_t counter_count_ext_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct count_ext_comp_t *const comp = devattr->component;
	const struct counter_count_ext *const ext = comp->ext;
	struct counter_device *const counter = dev_get_drvdata(dev);
	struct counter_count *const count = comp->count;

	return ext->write(counter, count, ext->priv, buf, len);
}

static int counter_count_ext_register(const char *const prefix,
	struct counter_count *const count,
	struct counter_device_state *const device_state)
{
	const size_t num_ext = count->num_ext;
	size_t i;
	const struct counter_count_ext *ext;
	struct count_ext_comp_t *count_ext_comp;
	int err;

	/* Return early if no extensions */
	if (!count->ext || !num_ext)
		return 0;

	/* Create an attribute for each extension */
	for (i = 0 ; i < num_ext; i++) {
		ext = count->ext + i;

		/* Allocate count_ext attribute component */
		count_ext_comp = kmalloc(sizeof(*count_ext_comp), GFP_KERNEL);
		if (!count_ext_comp)
			return -ENOMEM;
		count_ext_comp->count = count;
		count_ext_comp->ext = ext;

		/* Allocate count_ext attribute */
		err = counter_attribute_create(prefix, ext->name,
			(ext->read) ? counter_count_ext_show : NULL,
			(ext->write) ? counter_count_ext_store : NULL,
			count_ext_comp, device_state);
		if (err) {
			kfree(count_ext_comp);
			return err;
		}
	}

	return 0;
}

struct func_avail_comp_t {
	const enum count_function	*functions;
	size_t				num_functions;
};

static ssize_t counter_count_function_available_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct func_avail_comp_t *const component = devattr->component;
	const enum count_function *const functions = component->functions;
	const size_t num_functions = component->num_functions;
	size_t i;
	enum count_function function;
	ssize_t len = 0;

	for (i = 0; i < num_functions; i++) {
		function = functions[i];
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
			count_function_str[function]);
	}

	return len;
}

static int counter_count_attributes_create(const char *const count_attr_name,
	const struct counter_device *const counter,
	struct counter_count *const count)
{
	struct count_comp_t *count_comp;
	struct counter_device_state *const device_state = counter->device_state;
	int err;
	const char *prefix;
	struct count_comp_t *func_comp;
	struct func_avail_comp_t *avail_comp;
	struct count_comp_t *synapses_comp;
	struct name_comp_t *name_comp;

	/* Allocate count attribute component */
	count_comp = kmalloc(sizeof(*count_comp), GFP_KERNEL);
	if (!count_comp)
		return -ENOMEM;
	count_comp->count = count;

	/* Create main Count attribute */
	err = counter_attribute_create("", count_attr_name,
		(counter->count_read) ? counter_count_show : NULL,
		(counter->count_write) ? counter_count_store : NULL,
		count_comp, device_state);
	if (err) {
		kfree(count_comp);
		return err;
	}

	prefix = kasprintf(GFP_KERNEL, "%s_", count_attr_name);
	if (!prefix)
		return -ENOMEM;

	/* Allocate function attribute component */
	func_comp = kmalloc(sizeof(*func_comp), GFP_KERNEL);
	if (!func_comp) {
		err = -ENOMEM;
		goto err_free_prefix;
	}
	func_comp->count = count;

	/* Create Count function attribute */
	err = counter_attribute_create(prefix, "function",
		(counter->function_get) ? counter_function_show : NULL,
		(counter->function_set) ? counter_function_store : NULL,
		func_comp, device_state);
	if (err) {
		kfree(func_comp);
		goto err_free_prefix;
	}

	/* Allocate function available attribute component */
	avail_comp = kmalloc(sizeof(*avail_comp), GFP_KERNEL);
	if (!avail_comp) {
		err = -ENOMEM;
		goto err_free_prefix;
	}
	avail_comp->functions = count->functions;
	avail_comp->num_functions = count->num_functions;

	/* Create Count function_available attribute */
	err = counter_attribute_create(prefix, "function_available",
		counter_count_function_available_show, NULL, avail_comp,
		device_state);
	if (err) {
		kfree(avail_comp);
		goto err_free_prefix;
	}

	/* Allocate synapses attribute component */
	synapses_comp = kmalloc(sizeof(*synapses_comp), GFP_KERNEL);
	if (!synapses_comp) {
		err = -ENOMEM;
		goto err_free_prefix;
	}
	synapses_comp->count = count;

	/* Create Count synapses attribute */
	err = counter_attribute_create(prefix, "synapses",
		counter_count_synapses_show, NULL, synapses_comp, device_state);
	if (err) {
		kfree(synapses_comp);
		goto err_free_prefix;
	}

	/* Allocate name attribute component */
	name_comp = kmalloc(sizeof(*name_comp), GFP_KERNEL);
	if (!name_comp) {
		err = -ENOMEM;
		goto err_free_prefix;
	}
	name_comp->name = count->name;

	/* Create Count name attribute */
	if (count->name) {
		err = counter_attribute_create(prefix, "name",
			counter_device_attr_name_show,	NULL, name_comp,
			device_state);
		if (err) {
			kfree(name_comp);
			goto err_free_prefix;
		}
	}

	/* Register Count extension attributes */
	err = counter_count_ext_register(prefix, count, device_state);
	if (err)
		goto err_free_prefix;

	kfree(prefix);

	return 0;

err_free_prefix:
	kfree(prefix);
	return err;
}

static int counter_counts_register(const struct counter_device *const counter)
{
	const size_t num_counts = counter->num_counts;
	struct device *const dev = &counter->device_state->dev;
	size_t i;
	struct counter_count *count;
	const char *name;
	int err;

	/* At least one Count must be defined */
	if (!counter->counts || !num_counts) {
		dev_err(dev, "Counts undefined\n");
		return -EINVAL;
	}

	/* Register each Count */
	for (i = 0; i < num_counts; i++) {
		count = counter->counts + i;

		/* At least one function mode must be defined for each Count */
		if (!count->functions || !count->num_functions) {
			dev_err(dev, "Count '%d' function modes undefined\n",
				count->id);
			return -EINVAL;
		}

		/* Generate attribute name */
		name = kasprintf(GFP_KERNEL, "count%d", count->id);
		if (!name)
			return -ENOMEM;

		/* Register the Synapses associated with each Count */
		err = counter_synapses_register(counter, count, name);
		if (err)
			goto err_free_name;

		/* Create all attributes associated with Count */
		err = counter_count_attributes_create(name, counter, count);
		if (err)
			goto err_free_name;

		kfree(name);
	}

	return 0;

err_free_name:
	kfree(name);
	return err;
}

static struct bus_type counter_bus_type = {
	.name = "counter"
};

static int __init counter_init(void)
{
	return bus_register(&counter_bus_type);
}

static void __exit counter_exit(void)
{
	bus_unregister(&counter_bus_type);
}

static void free_counter_device_state_attr_list(struct list_head *attr_list)
{
	struct counter_device_attr *p, *n;

	list_for_each_entry_safe(p, n, attr_list, l) {
		kfree(p->dev_attr.attr.name);
		kfree(p->component);
		list_del(&p->l);
		kfree(p);
	}
}

/* Provides a unique ID for each counter device */
static DEFINE_IDA(counter_ida);

static void counter_device_release(struct device *dev)
{
	struct counter_device *const counter = dev_get_drvdata(dev);
	struct counter_device_state *const device_state = counter->device_state;

	kfree(device_state->attr_group.attrs);
	free_counter_device_state_attr_list(&device_state->attr_list);
	ida_simple_remove(&counter_ida, device_state->id);
	kfree(device_state);
}

static struct device_type counter_device_type = {
	.name = "counter_device",
	.release = counter_device_release
};

struct ext_comp_t {
	const struct counter_device_ext	*ext;
};

static ssize_t counter_device_ext_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct ext_comp_t *const component = devattr->component;
	const struct counter_device_ext *const ext = component->ext;
	struct counter_device *const counter = dev_get_drvdata(dev);

	return ext->read(counter, ext->priv, buf);
}

static ssize_t counter_device_ext_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	const struct counter_device_attr *const devattr = to_counter_attr(attr);
	const struct ext_comp_t *const component = devattr->component;
	const struct counter_device_ext *const ext = component->ext;
	struct counter_device *const counter = dev_get_drvdata(dev);

	return ext->write(counter, ext->priv, buf, len);
}

static int counter_device_ext_register(struct counter_device *const counter)
{
	const size_t num_ext = counter->num_ext;
	struct ext_comp_t *ext_comp;
	size_t i;
	const struct counter_device_ext *ext;
	int err;

	/* Return early if no extensions */
	if (!counter->ext || !num_ext)
		return 0;

	/* Create an attribute for each extension */
	for (i = 0 ; i < num_ext; i++) {
		ext = counter->ext + i;

		/* Allocate extension attribute component */
		ext_comp = kmalloc(sizeof(*ext_comp), GFP_KERNEL);
		if (!ext_comp)
			return -ENOMEM;
		ext_comp->ext = ext;

		/* Allocate extension attribute */
		err = counter_attribute_create("", ext->name,
			(ext->read) ? counter_device_ext_show : NULL,
			(ext->write) ? counter_device_ext_store : NULL,
			ext_comp, counter->device_state);
		if (err) {
			kfree(ext_comp);
			return err;
		}
	}

	return 0;
}

static ssize_t counter_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const struct counter_device *const counter = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n", counter->name);
}

static DEVICE_ATTR_RO(counter_name);

/**
 * counter_register - register Counter to the system
 * @counter:	pointer to Counter to register
 *
 * This function registers a Counter to the system. A sysfs "counter" directory
 * will be created and populated with sysfs attributes correlating with the
 * Counter Signals, Synapses, and Counts respectively.
 */
int counter_register(struct counter_device *const counter)
{
	int err;
	struct counter_device_state *device_state;
	struct counter_device_attr *p;
	size_t i = 0;

	if (!counter)
		return -EINVAL;

	/* Allocate internal state container for Counter device */
	device_state = kzalloc(sizeof(*device_state), GFP_KERNEL);
	if (!device_state)
		return -ENOMEM;
	counter->device_state = device_state;

	/* Acquire unique ID */
	device_state->id = ida_simple_get(&counter_ida, 0, 0, GFP_KERNEL);
	if (device_state->id < 0) {
		err = device_state->id;
		goto err_free_device_state;
	}

	/* Configure device structure for Counter */
	device_state->dev.type = &counter_device_type;
	device_state->dev.bus = &counter_bus_type;
	if (counter->parent) {
		device_state->dev.parent = counter->parent;
		device_state->dev.of_node = counter->parent->of_node;
	}
	dev_set_name(&device_state->dev, "counter%d", device_state->id);
	device_initialize(&device_state->dev);
	dev_set_drvdata(&device_state->dev, counter);

	/* Initialize attribute list */
	INIT_LIST_HEAD(&device_state->attr_list);

	/* Verify Signals are valid and register */
	err = counter_signals_register(counter);
	if (err)
		goto err_free_attributes;

	/* Verify Counts and respective Synapses are valid and register */
	err = counter_counts_register(counter);
	if (err)
		goto err_free_attributes;

	/* Register Counter device extension attributes */
	err = counter_device_ext_register(counter);
	if (err)
		goto err_free_attributes;

	/* Account for name attribute */
	if (counter->name)
		device_state->num_attr++;

	/* Allocate space for attribute pointers in attribute group */
	device_state->attr_group.attrs = kcalloc(device_state->num_attr + 1,
		sizeof(*device_state->attr_group.attrs), GFP_KERNEL);
	if (!device_state->attr_group.attrs) {
		err = -ENOMEM;
		goto err_free_attributes;
	}

	/* Add attribute pointers to attribute group */
	list_for_each_entry(p, &device_state->attr_list, l)
		device_state->attr_group.attrs[i++] = &p->dev_attr.attr;
	if (counter->name)
		device_state->attr_group.attrs[i] = &dev_attr_counter_name.attr;

	/* Associate attributes with device */
	device_state->groups[0] = &device_state->attr_group;
	device_state->dev.groups = device_state->groups;

	/* Add device to system */
	err = device_add(&device_state->dev);
	if (err)
		goto err_free_attr_group_attrs;

	return 0;

err_free_attr_group_attrs:
	kfree(counter->device_state->attr_group.attrs);
err_free_attributes:
	free_counter_device_state_attr_list(&counter->device_state->attr_list);
	ida_simple_remove(&counter_ida, counter->device_state->id);
err_free_device_state:
	kfree(counter->device_state);
	return err;
}
EXPORT_SYMBOL(counter_register);

/**
 * counter_unregister - unregister Counter from the system
 * @counter:	pointer to Counter to unregister
 *
 * The Counter is unregistered from the system; all allocated memory is freed.
 */
void counter_unregister(struct counter_device *const counter)
{
	if (counter)
		device_del(&counter->device_state->dev);
}
EXPORT_SYMBOL(counter_unregister);

static void devm_counter_unreg(struct device *dev, void *res)
{
	counter_unregister(*(struct counter_device **)res);
}

/**
 * devm_counter_register - Resource-managed counter_register
 * @dev:	device to allocate counter_device for
 * @counter:	pointer to Counter to register
 *
 * Managed counter_register. The Counter registered with this function is
 * automatically unregistered on driver detach. This function calls
 * counter_register internally. Refer to that function for more information.
 *
 * If an Counter registered with this function needs to be unregistered
 * separately, devm_counter_unregister must be used.
 *
 * RETURNS:
 * 0 on success, negative error number on failure.
 */
int devm_counter_register(struct device *dev,
	struct counter_device *const counter)
{
	struct counter_device **ptr;
	int ret;

	ptr = devres_alloc(devm_counter_unreg, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	ret = counter_register(counter);
	if (!ret) {
		*ptr = counter;
		devres_add(dev, ptr);
	} else
		devres_free(ptr);

	return ret;
}
EXPORT_SYMBOL(devm_counter_register);

static int devm_counter_match(struct device *dev, void *res, void *data)
{
	struct counter_device **r = res;

	if (!r || !*r) {
		WARN_ON(!r || !*r);
		return 0;
	}

	return *r == data;
}

/**
 * devm_counter_unregister - Resource-managed counter_unregister
 * @dev:	device this counter_device belongs to
 * @counter:	pointer to Counter associated with the device
 *
 * Unregister Counter registered with devm_counter_register.
 */
void devm_counter_unregister(struct device *dev,
	struct counter_device *const counter)
{
	int rc;

	rc = devres_release(dev, devm_counter_unreg,
		devm_counter_match, counter);
	WARN_ON(rc);
}
EXPORT_SYMBOL(devm_counter_unregister);

subsys_initcall(counter_init);
module_exit(counter_exit);

MODULE_AUTHOR("William Breathitt Gray <vilhelm.gray@gmail.com>");
MODULE_DESCRIPTION("Generic Counter interface");
MODULE_LICENSE("GPL v2");
