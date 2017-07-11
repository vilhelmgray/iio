/*
 * Counter interface
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
#ifndef _COUNTER_H_
#define _COUNTER_H_

#include <linux/device.h>
#include <linux/types.h>

struct counter_device;
struct counter_signal;

/**
 * struct counter_signal_ext - Counter Signal extensions
 * @name:	attribute name
 * @read:	read callback for this attribute; may be NULL
 * @write:	write callback for this attribute; may be NULL
 * @priv:	data private to the driver
 */
struct counter_signal_ext {
	const char	*name;
	ssize_t		(*read)(struct counter_device *counter,
				struct counter_signal *signal, void *priv,
				char *buf);
	ssize_t		(*write)(struct counter_device *counter,
				struct counter_signal *signal, void *priv,
				const char *buf, size_t len);
	void		*priv;
};

/**
 * struct counter_signal - Counter Signal node
 * @id:		unique ID used to identify signal
 * @name:	device-specific signal name
 * @ext:	optional array of Counter Signal extensions
 * @num_ext:	number of Counter Signal extensions specified in @ext
 * @priv:	optional private data supplied by driver
 */
struct counter_signal {
	int		id;
	const char	*name;

	const struct counter_signal_ext	*ext;
	size_t				num_ext;

	void	*priv;
};

/**
 * struct counter_signal_enum_ext - Signal enum extension attribute
 * @items:	Array of strings
 * @num_items:	Number of items specified in @items
 * @set:	Set callback function; may be NULL
 * @get:	Get callback function; may be NULL
 *
 * The counter_signal_enum_ext structure can be used to implement enum style
 * Signal extension attributes. Enum style attributes are those which have a set
 * of strings that map to unsigned integer values. The Generic Counter Signal
 * enum extension helper code takes care of mapping between value and string, as
 * well as generating a "_available" file which contains a list of all available
 * items. The get callback is used to query the currently active item; the index
 * of the item within the respective items array is returned via the 'item'
 * parameter. The set callback is called when the attribute is updated; the
 * 'item' parameter contains the index of the newly activated item within the
 * respective items array.
 */
struct counter_signal_enum_ext {
	const char * const	*items;
	size_t			num_items;
	int			(*get)(struct counter_device *counter,
					struct counter_signal *signal,
					size_t *item);
	int			(*set)(struct counter_device *counter,
					struct counter_signal *signal,
					size_t item);
};

extern ssize_t counter_signal_enum_read(struct counter_device *counter,
	struct counter_signal *signal, void *priv, char *buf);
extern ssize_t counter_signal_enum_write(struct counter_device *counter,
	struct counter_signal *signal, void *priv, const char *buf, size_t len);

/**
 * COUNTER_SIGNAL_ENUM() - Initialize Signal enum extension
 * @_name:	Attribute name
 * @_e:		Pointer to a counter_count_enum structure
 *
 * This should usually be used together with COUNTER_SIGNAL_ENUM_AVAILABLE()
 */
#define COUNTER_SIGNAL_ENUM(_name, _e) \
{ \
	.name = (_name), \
	.read = counter_signal_enum_read, \
	.write = counter_signal_enum_write, \
	.priv = (_e) \
}

extern ssize_t counter_signal_enum_available_read(
	struct counter_device *counter, struct counter_signal *signal,
	void *priv, char *buf);

/**
 * COUNTER_SIGNAL_ENUM_AVAILABLE() - Initialize Signal enum available extension
 * @_name:	Attribute name ("_available" will be appended to the name)
 * @_e:		Pointer to a counter_signal_enum structure
 *
 * Creates a read only attribute that lists all the available enum items in a
 * newline separated list. This should usually be used together with
 * COUNTER_SIGNAL_ENUM()
 */
#define COUNTER_SIGNAL_ENUM_AVAILABLE(_name, _e) \
{ \
	.name = (_name "_available"), \
	.read = counter_signal_enum_available_read, \
	.priv = (_e) \
}

enum synapse_action {
	SYNAPSE_ACTION_NONE = 0,
	SYNAPSE_ACTION_RISING_EDGE,
	SYNAPSE_ACTION_FALLING_EDGE,
	SYNAPSE_ACTION_BOTH_EDGES
};

/**
 * struct counter_synapse - Counter Synapse node
 * @action:		index of current action mode
 * @actions:		available action modes
 * @num_actions:	number of action modes specified in @actions
 * @signal:		pointer to associated signal
 */
struct counter_synapse {
	size_t					action;
	const enum synapse_action *const	actions;
	size_t					num_actions;

	struct counter_signal			*signal;
};

struct counter_count;

/**
 * struct counter_count_ext - Counter Count extension
 * @name:	attribute name
 * @read:	read callback for this attribute; may be NULL
 * @write:	write callback for this attribute; may be NULL
 * @priv:	data private to the driver
 */
struct counter_count_ext {
	const char	*name;
	ssize_t		(*read)(struct counter_device *counter,
				struct counter_count *count, void *priv,
				char *buf);
	ssize_t		(*write)(struct counter_device *counter,
				struct counter_count *count, void *priv,
				const char *buf, size_t len);
	void		*priv;
};

enum count_function {
	COUNT_FUNCTION_INCREASE = 0,
	COUNT_FUNCTION_DECREASE,
	COUNT_FUNCTION_PULSE_DIRECTION,
	COUNT_FUNCTION_QUADRATURE_X1,
	COUNT_FUNCTION_QUADRATURE_X2,
	COUNT_FUNCTION_QUADRATURE_X4
};

/**
 * struct counter_count - Counter Count node
 * @id:			unique ID used to identify Count
 * @name:		device-specific Count name
 * @function:		index of current function mode
 * @functions:		available function modes
 * @num_functions:	number of function modes specified in @functions
 * @synapses:		array of synapses for initialization
 * @num_synapses:	number of synapses specified in @synapses
 * @ext:		optional array of Counter Count extensions
 * @num_ext:		number of Counter Count extensions specified in @ext
 * @priv:		optional private data supplied by driver
 */
struct counter_count {
	int			id;
	const char		*name;

	size_t					function;
	const enum count_function *const	functions;
	size_t					num_functions;

	struct counter_synapse	*synapses;
	size_t			num_synapses;

	const struct counter_count_ext	*ext;
	size_t				num_ext;

	void	*priv;
};

/**
 * struct counter_count_enum_ext - Count enum extension attribute
 * @items:	Array of strings
 * @num_items:	Number of items specified in @items
 * @set:	Set callback function; may be NULL
 * @get:	Get callback function; may be NULL
 *
 * The counter_count_enum_ext structure can be used to implement enum style
 * Count extension attributes. Enum style attributes are those which have a set
 * of strings that map to unsigned integer values. The Generic Counter Count
 * enum extension helper code takes care of mapping between value and string, as
 * well as generating a "_available" file which contains a list of all available
 * items. The get callback is used to query the currently active item; the index
 * of the item within the respective items array is returned via the 'item'
 * parameter. The set callback is called when the attribute is updated; the
 * 'item' parameter contains the index of the newly activated item within the
 * respective items array.
 */
struct counter_count_enum_ext {
	const char * const	*items;
	size_t			num_items;
	int			(*get)(struct counter_device *counter,
					struct counter_count *count,
					size_t *item);
	int			(*set)(struct counter_device *counter,
					struct counter_count *count,
					size_t item);
};

extern ssize_t counter_count_enum_read(struct counter_device *counter,
	struct counter_count *count, void *priv, char *buf);
extern ssize_t counter_count_enum_write(struct counter_device *counter,
	struct counter_count *count, void *priv, const char *buf, size_t len);

/**
 * COUNTER_COUNT_ENUM() - Initialize Count enum extension
 * @_name:	Attribute name
 * @_e:		Pointer to a counter_count_enum structure
 *
 * This should usually be used together with COUNTER_COUNT_ENUM_AVAILABLE()
 */
#define COUNTER_COUNT_ENUM(_name, _e) \
{ \
	.name = (_name), \
	.read = counter_count_enum_read, \
	.write = counter_count_enum_write, \
	.priv = (_e) \
}

extern ssize_t counter_count_enum_available_read(struct counter_device *counter,
	struct counter_count *count, void *priv, char *buf);

/**
 * COUNTER_COUNT_ENUM_AVAILABLE() - Initialize Count enum available extension
 * @_name:	Attribute name ("_available" will be appended to the name)
 * @_e:		Pointer to a counter_count_enum structure
 *
 * Creates a read only attribute that lists all the available enum items in a
 * newline separated list. This should usually be used together with
 * COUNTER_COUNT_ENUM()
 */
#define COUNTER_COUNT_ENUM_AVAILABLE(_name, _e) \
{ \
	.name = (_name "_available"), \
	.read = counter_count_enum_available_read, \
	.priv = (_e) \
}

/**
 * struct counter_device_state - internal state container for a Counter device
 * @id:		unique ID used to identify the Counter
 * @dev:	internal device structure
 * @attr_list:	list to keep track of created Counter sysfs attributes
 * @attr_group:	Counter sysfs attributes group
 * @groups:	attribute groups (used internally as NULL-terminated group list)
 * @num_attr:	number of Counter sysfs attributes
 */
struct counter_device_state {
	int				id;
	struct device			dev;
	struct list_head		attr_list;
	struct attribute_group		attr_group;
	const struct attribute_group	*groups[2];
	size_t				num_attr;
};

/**
 * struct signal_read_value - Opaque Signal read value
 * @buf:	string representation of Signal read value
 * @len:	length of string in @buf
 */
struct signal_read_value{
	char	*buf;
	size_t	len;
};
typedef struct signal_read_value signal_read_value;

/**
 * struct count_read_value - Opaque Count read value
 * @buf:	string representation of Count read value
 * @len:	length of string in @buf
 */
struct count_read_value{
	char	*buf;
	size_t	len;
};
typedef struct count_read_value count_read_value;

/**
 * struct count_write_value - Opaque Count write value
 * @buf:	string representation of Count write value
 */
struct count_write_value{
	const char	*buf;
};
typedef struct count_write_value count_write_value;

/**
 * struct counter_device_ext - Counter device extension
 * @name:	attribute name
 * @read:	read callback for this attribute; may be NULL
 * @write:	write callback for this attribute; may be NULL
 * @priv:	data private to the driver
 */
struct counter_device_ext {
	const char	*name;
	ssize_t		(*read)(struct counter_device *counter, void *priv,
				char *buf);
	ssize_t		(*write)(struct counter_device *counter, void *priv,
				const char *buf, size_t len);
	void		*priv;
};

/**
 * struct counter_device_enum_ext - Counter enum extension attribute
 * @items:	Array of strings
 * @num_items:	Number of items specified in @items
 * @set:	Set callback function; may be NULL
 * @get:	Get callback function; may be NULL
 *
 * The counter_device_enum_ext structure can be used to implement enum style
 * Counter extension attributes. Enum style attributes are those which have a
 * set of strings that map to unsigned integer values. The Generic Counter enum
 * extension helper code takes care of mapping between value and string, as well
 * as generating a "_available" file which contains a list of all available
 * items. The get callback is used to query the currently active item; the index
 * of the item within the respective items array is returned via the 'item'
 * parameter. The set callback is called when the attribute is updated; the
 * 'item' parameter contains the index of the newly activated item within the
 * respective items array.
 */
struct counter_device_enum_ext {
	const char * const	*items;
	size_t			num_items;
	int			(*get)(struct counter_device *counter,
					size_t *item);
	int			(*set)(struct counter_device *counter,
					size_t item);
};

extern ssize_t counter_device_enum_read(struct counter_device *counter,
	void *priv, char *buf);
extern ssize_t counter_device_enum_write(struct counter_device *counter,
	void *priv, const char *buf, size_t len);

/**
 * COUNTER_DEVICE_ENUM() - Initialize Counter enum extension
 * @_name:	Attribute name
 * @_e:		Pointer to a counter_device_enum structure
 *
 * This should usually be used together with COUNTER_DEVICE_ENUM_AVAILABLE()
 */
#define COUNTER_DEVICE_ENUM(_name, _e) \
{ \
	.name = (_name), \
	.read = counter_device_enum_read, \
	.write = counter_device_enum_write, \
	.priv = (_e) \
}

extern ssize_t counter_device_enum_available_read(
	struct counter_device *counter, void *priv, char *buf);

/**
 * COUNTER_DEVICE_ENUM_AVAILABLE() - Initialize Counter enum available extension
 * @_name:	Attribute name ("_available" will be appended to the name)
 * @_e:		Pointer to a counter_device_enum structure
 *
 * Creates a read only attribute that lists all the available enum items in a
 * newline separated list. This should usually be used together with
 * COUNTER_DEVICE_ENUM()
 */
#define COUNTER_DEVICE_ENUM_AVAILABLE(_name, _e) \
{ \
	.name = (_name "_available"), \
	.read = counter_device_enum_available_read, \
	.priv = (_e) \
}

/**
 * struct counter_device - Counter data structure
 * @name:		name of the device
 * @parent:		optional parent device providing the counters
 * @device_state:	internal device state container
 * @signal_read:	read callback for Signal attribute; may be NULL
 * @count_read:		read callback for Count attribute; may be NULL
 * @count_write:	write callback for Count attribute; may be NULL
 * @function_get:	function to get the current count function mode. Returns
 *			0 on success and negative error code on error. The index
 *			of the respective Count's returned function mode should
 *			be passed back via the function parameter.
 * @function_set:	function to set the count function mode. function is the
 *			index of the requested function mode from the respective
 *			Count's functions array.
 * @action_get:		function to get the current action mode. Returns 0 on
 *			success and negative error code on error. The index of
 *			the respective Signal's returned action mode should be
 *			passed back via the action parameter.
 * @action_set:		function to set the action mode. action is the index of
 *			the requested action mode from the respective Synapse's
 *			actions array.
 * @signals:		array of Signals
 * @num_signals:	number of Signals specified in @signals
 * @counts:		array of Counts
 * @num_counts:		number of Counts specified in @counts
 * @ext:		optional array of Counter device extensions
 * @num_ext:		number of Counter device extensions specified in @ext
 * @priv:		optional private data supplied by driver
 */
struct counter_device {
	const char			*name;
	struct device			*parent;
	struct counter_device_state	*device_state;

	int	(*signal_read)(struct counter_device *counter,
			struct counter_signal *signal, signal_read_value *val);
	int	(*count_read)(struct counter_device *counter,
			struct counter_count *count, count_read_value *val);
	int	(*count_write)(struct counter_device *counter,
			struct counter_count *count, count_write_value *val);
	int	(*function_get)(struct counter_device *counter,
			struct counter_count *count, size_t *function);
	int	(*function_set)(struct counter_device *counter,
			struct counter_count *count, size_t function);
	int	(*action_get)(struct counter_device *counter,
			struct counter_count *count,
			struct counter_synapse *synapse, size_t *action);
	int	(*action_set)(struct counter_device *counter,
			struct counter_count *count,
			struct counter_synapse *synapse, size_t action);

	struct counter_signal	*signals;
	size_t			num_signals;
	struct counter_count	*counts;
	size_t			num_counts;

	const struct counter_device_ext	*ext;
	size_t				num_ext;

	void	*priv;
};

enum signal_level {
	SIGNAL_LEVEL_LOW = 0,
	SIGNAL_LEVEL_HIGH
};

enum signal_value_type {
	SIGNAL_LEVEL = 0
};

enum count_value_type {
	COUNT_POSITION_UNSIGNED = 0,
	COUNT_POSITION_SIGNED
};

extern void set_signal_read_value(signal_read_value *const val,
	const enum signal_value_type type, void *const data);
extern void set_count_read_value(count_read_value *const val,
	const enum count_value_type type, void *const data);
extern int get_count_write_value(void *const data,
	const enum count_value_type type, const count_write_value *const val);

extern int counter_register(struct counter_device *const counter);
extern void counter_unregister(struct counter_device *const counter);
extern int devm_counter_register(struct device *dev,
	struct counter_device *const counter);
extern void devm_counter_unregister(struct device *dev,
	struct counter_device *const counter);

#endif /* _COUNTER_H_ */
