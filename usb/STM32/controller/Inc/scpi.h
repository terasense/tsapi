/*
 * Simple parser for
 * Standard Commands for Programmable Instruments
 */

#pragma once

#include "errors.h"
#include <ctype.h>
#include <stdbool.h>

#define SCPI_DELIM ';' 

struct scpi_node;

// User supplied callback. Process input, returns number of bytes consumed or negative error code.
typedef int (*scpi_handler_t)(const char* str, unsigned sz, struct scpi_node const*);

// The SCPI node descriptor
struct scpi_node {
	// Node name. Starts with mandatory uppercase heading followed by optional lowercase tail (see SCPI standard).
	const char*             name;
	// The next level nodes array terminated by SCPI_NODE_END. Optional, may be NULL.
	struct scpi_node const* dir;
	// The node handler processing this node itself (and any arguments passed to it)
	scpi_handler_t          handler;
	// Help text. Optional, may be NULL.
	const char*             help;
	// If non-zero, the node will be disabled unless explicitly enabled
	unsigned                disabled;
	/* If true - not listed by help */
	bool                    hidden;
	// Private parameters
	union {
		void*           param;
		int             iparam;
	};
	union {
		void*           param2;
		int             iparam2;
	};
};

#define SCPI_NODE_END {NULL}

struct scpi_tree {
	struct scpi_node const* star_nodes;
	struct scpi_node const* colon_nodes;
	unsigned                enabled;
};

// Parse command given the arrays of root nodes for '*' and ':' heading hierarchy
err_t scpi_parse(const char* str, unsigned sz, struct scpi_tree const* tree);

// Enable nodes with corresponding bits set in hidden field
static inline void scpi_enable(unsigned what, struct scpi_tree* tree)
{
	tree->enabled |= what;
}

static inline unsigned scpi_match(const char* str, unsigned sz, const char* name)
{
	unsigned matched = 0;
	if (!sz)
		return 0;
	while (*name) {
		if (!sz || toupper(*str) != toupper(*name)) {
			if (!matched)
				return 0;
			// All capital letters of the name must match
			if (isupper(*name) || isdigit(*name))
				return 0;
			// All letters from the string must match
			if (sz && isalpha(*str))
				return 0;
			// If lower case letter matched then all letters must match
			if (!isupper(name[-1]))
				return 0;
			return matched;
		}
		++matched;
		++name;
		++str;
		--sz;
	}
	// All letters from the string must match
	if (sz && isalpha(*str))
		return 0;
	return matched;
}
