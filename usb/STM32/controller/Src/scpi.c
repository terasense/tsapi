/*
 * Simple parser for
 * Standard Commands for Programmable Instruments
 */

#include "scpi.h"
#include "cli.h"
#include "str_util.h"
#include <stddef.h>

static inline int scpi_parse_node_value(const char* str, unsigned sz, struct scpi_node const* node)
{
	// We should have a handler
	if (!node->handler)
		return -err_cmd;
	// Call handler
	int rc = node->handler(str, sz, node);
	// Check parsing result
	if (rc < 0)
		return rc;
	if (rc > sz)
		return -err_internal;
	// Advance buffer pointer
	str += rc;
	sz  -= rc;
	return rc;
}

static inline err_t scpi_help_value(struct scpi_node const* node)
{
	if (node->dir)
		return cli_printf("%s is not a value", node->name);
	if (!node->help)
		return cli_printf("value %s has no help", node->name);
	else
		return cli_printf("%s%s", node->name, node->help);
}

static err_t scpi_list_dir(struct scpi_node const* node, struct scpi_tree const* tree)
{
	err_t err;
	struct scpi_node const *n = NULL;
	unsigned const not_enabled = ~tree->enabled;

	if (node->help && (err = cli_printf("%s%s\n", node->name, node->help)))
		return err;

	for (n = node->dir; n->name; ++n) {
		if (n->hidden)
			continue;
		if (n->disabled & not_enabled)
			continue;
		if ((err = cli_printf("%s", n->name))
			|| (n->dir && (err = cli_printf(":")))
			|| (err = cli_printf(" "))
		)
			return err;
	}
	return err_ok;
}

static int scpi_parse_node(const char* str, unsigned sz, struct scpi_node const* node, bool help_mode, struct scpi_tree const* tree)
{
	unsigned const sz_in = sz;
	if (!node->dir) {
		if (help_mode)
			return cli_printf("%s is a value", node->name);
		return -err_cmd;
	}

	if (!sz) {
		if (help_mode)
			return -scpi_list_dir(node, tree);
		else if (!node->handler)
			return -err_cmd;
	}

	while (sz && *str != ':' && *str != '*')
	{
		int rc = 0;
		unsigned matched_subnode = 0;
		struct scpi_node const *n = NULL;
		unsigned const not_enabled = ~tree->enabled;
		// Lookup subnode
		for (n = node->dir; n->name; ++n) {
			if (n->disabled & not_enabled)
				continue;
			if ((matched_subnode = scpi_match(str, sz, n->name)))
				break;
		}
		if (!matched_subnode)
			return -err_cmd;			
		// Parse subnode
		str += matched_subnode;
		sz  -= matched_subnode;
		if (sz && *str == ':') {
			++str;
			--sz;
			rc = scpi_parse_node(str, sz, n, help_mode, tree);
		} else if (help_mode) {
			if (!sz) {
				err_t const err = scpi_help_value(n);
				return err ? -err : sz_in;
			} else
				return -err_cmd;
		} else {
			rc = scpi_parse_node_value(str, sz, n);
		}
		// Check parsing result
		if (rc < 0)
			return rc;
		if (rc > sz)
			return -err_internal;
		// Advance buffer pointer
		str += rc;
		sz  -= rc;
		// Consume delimiter if any
		while (sz && *str == SCPI_DELIM) {
			++str;
			--sz;
		}
	}
	if (node->handler) {
		// Always call handler if provided
		int rc = node->handler(NULL, 0, node);
		if (rc < 0)
			return rc;
	}
	return sz_in - sz;
}

static inline err_t scpi_default_help(void)
{
	return cli_printf("use\n?* or ?: to list top level tags,\n?<path>: to list tags rooting at given path,\n?<path>  to get help about particular parameter");
}

err_t scpi_parse(const char* str, unsigned sz, struct scpi_tree const* tree)
{
	struct scpi_node star_root  = {NULL, tree->star_nodes, NULL};
	struct scpi_node colon_root = {NULL, tree->colon_nodes, NULL};
	struct scpi_node const* root = &colon_root;
	bool help_mode = false;

	if (!sz)
		return err_proto;

	if (*str == '?') {
		help_mode = true;
		str += 1;
		sz  -= 1;
	}

	if (!sz)
		return scpi_default_help();

	while (sz) {
		switch (*str) {
		case '*':
			root = &star_root;
			++str;
			--sz;
			break;
		case ':':
			root = &colon_root;
			++str;
			--sz;
			break;
		}
		int rc = scpi_parse_node(str, sz, root, help_mode, tree);
		if (rc < 0)
			return (err_t)(-rc);
		if (rc > sz)
			return err_internal;
		str += rc;
		sz  -= rc;
	}

        return err_ok;
}

//
// Generic value handlers
//

int scpi_u16_rw_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (!sz)
		return -err_cmd;

	if (*str == '?')
		return scpi_u16_r_handler(str, sz, n);
	else
		return scpi_u16_w_handler(str, sz, n);
}

int scpi_u16_r_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (sz != 1 || *str != '?')
		return -err_cmd;

	err_t const err = cli_printf("%u", *(uint16_t const*)n->param);
	if (err)
		return -err;
	return sz;
}

int scpi_u16_w_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	uint32_t val;
	unsigned const rc = scan_u(str, sz, &val);
	if (!rc || val > (uint16_t)~0)
		return -err_param;
	*(uint16_t*)n->param = val;
	return rc;
}

int scpi_u32_rw_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (!sz)
		return -err_cmd;

	if (*str == '?')
		return scpi_u32_r_handler(str, sz, n);
	else
		return scpi_u32_w_handler(str, sz, n);
}

int scpi_u32_r_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (sz != 1 || *str != '?')
		return -err_cmd;

	err_t const err = cli_printf("%u", *(uint32_t const*)n->param);
	if (err)
		return -err;
	return sz;
}

int scpi_u32_w_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	uint32_t val;
	unsigned const rc = scan_u(str, sz, &val);
	if (!rc)
		return -err_param;
	*(uint32_t*)n->param = val;
	return rc;
}

int scpi_bool_rw_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (!sz)
		return -err_cmd;

	if (*str == '?')
		return scpi_bool_r_handler(str, sz, n);
	else
		return scpi_bool_w_handler(str, sz, n);
}

int scpi_bool_r_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (sz != 1 || *str != '?')
		return -err_cmd;

	err_t const err = cli_printf("%d", *(bool const*)n->param ? 1 : 0);
	if (err)
		return -err;
	return sz;
}

int scpi_bool_w_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	unsigned const skip = skip_spaces(str, sz);
	sz  -= skip;
	str += skip;

	if (sz >= 1 && str[0] == '1') {
		*(bool*)n->param = true;
		return skip + 1;
	}
	if (sz >= 1 && str[0] == '0') {
		*(bool*)n->param = false;
		return skip + 1;
	}
	if (has_prefix_casei(str, sz, "ON")) {
		*(bool*)n->param = true;
		return skip + 2;
	}
	if (has_prefix_casei(str, sz, "OFF")) {
		*(bool*)n->param = false;
		return skip + 3;
	}

	return -err_cmd;
}

int scpi_bool_rw_handler2(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (!sz)
		return -err_cmd;

	if (*str == '?')
		return scpi_bool_r_handler2(str, sz, n);
	else
		return scpi_bool_w_handler2(str, sz, n);
}

int scpi_bool_r_handler2(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (sz != 1 || *str != '?')
		return -err_cmd;

	err_t const err = cli_printf("%d", ((bool_get_fn)n->param)() ? 1 : 0);
	if (err)
		return -err;
	return sz;
}

int scpi_bool_w_handler2(const char* str, unsigned sz, struct scpi_node const* n)
{
	unsigned const skip = skip_spaces(str, sz);
	sz  -= skip;
	str += skip;

	if (sz >= 1 && str[0] == '1') {
		((bool_set_fn)n->param2)(true);
		return skip + 1;
	}
	if (sz >= 1 && str[0] == '0') {
		((bool_set_fn)n->param2)(false);
		return skip + 1;
	}
	if (has_prefix_casei(str, sz, "ON")) {
		((bool_set_fn)n->param2)(true);
		return skip + 2;
	}
	if (has_prefix_casei(str, sz, "OFF")) {
		((bool_set_fn)n->param2)(false);
		return skip + 3;
	}

	return -err_cmd;
}
