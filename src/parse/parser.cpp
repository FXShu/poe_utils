#include "parser.hh"
#include "macro.hh"
static const char *poe_message_table[] = {
	"TERMINAL",
	"COMMAND"
};

static const char *poe_keyboard_message_table[] = {
	"KEYDOWM",
	"KEYUP",
	"SYSTEM_KEYDOWN",
	"SYSTEM_KEYUP"
};

static const char *poe_mouse_message_table[] = {
	"Mouse left button",
	"Mouse right button",
	"Mouse middle button"
};

const char * parser::get_msg(enum poe_table_type table, uint32_t enumeration) {
	if (table >= poe_table_maximum) {
		poe_log(MSG_WARNING, "Parser") << "Unexpected table type";
		return nullptr;
	}
	switch((int)table) {
	case poe_table_message:
		enumeration %= WM_USER;
		return poe_message_table[enumeration];
	case poe_table_keyboard:
		enumeration %= KEYBOARD_MESSAGE_BASIC;
		return poe_keyboard_message_table[enumeration];
	case poe_table_mouse:
		if (enumeration >= MOUSE_BUTTON_MAX)
			return "Invalid mouse type";
		return poe_mouse_message_table[enumeration];
	}
	return nullptr;
}
