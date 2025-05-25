#ifndef __PARSER_HH
#define __PARSER_HH

#include "utils_header.hh"

enum poe_table_type {
	poe_table_message,
	poe_table_keyboard,
	poe_table_mouse,
	poe_table_maximum
};

class parser {
public:
	static const char *get_msg(enum poe_table_type table, uint32_t enumeration);
};

#endif /* __PARSER_HH */

