#include "parser.hh"

static std::string poe_message_table[] = {
	std::string("POE_MESSAGE_TERMINAL")
};

std::string parser::get_msg(enum poe_table_type table, uint32_t enumeration) {
	if (table >= poe_table_maximum) {
		poe_log(MSG_WARNING, "Parser") << "Unexpected table type";
		return std::string("");	
	}
	switch(poe_table_type) {
	case poe_table_message:
		enumeration %= WM_USER;
		return poe_message_table[enumeration];
	}
}
