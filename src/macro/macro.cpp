#include "macro.hh"
#include "io.hh"
#include "parser.hh"
void keyboard_instruction::show(void) {
	poe_log(MSG_INFO, "keyboard_instruction")
		<< parser::get_msg(poe_table_keyboard, _type)
		<< " "<< (char)_code << " duration : " << _duration;
}

int macro::rename(std::string name) {
	if (name.empty()) {
		poe_log(MSG_WARNING, "Macro") << "Inavliad parameter";
		return -1;		
	}
	_name = name;
	return 0;
}

int macro::add_instruction(instruction::Ptr item) {
	if(item->duration() < 0) {
		poe_log(MSG_WARNING, "Macro") << "Invalid parameter";
		return -1;
	}
	_items.push_back(item);
	return 0;
}

int macro::remove_instruction(unsigned int order) {
	if (order < 0 || order > _items.size()) {
		poe_log(MSG_WARNING, "Macro") << "Invalid parameter";
		return -1;
	}
	_items.erase(_items.begin() + order);
	return 0;
}

int macro::replace_instruction(unsigned int order, instruction::Ptr item) {
	if (order < 0 || order > _items.size() || item->duration() < 0) {
		poe_log(MSG_WARNING, "Macro") << "Invalid parameter";
		return -1;
	}
	_items[order] = item;
	return 0;
}

#ifndef _WIN32
macro_passive::Ptr macro_passive::createNew(const char *name, uint8_t hotkey,
		observer::Ptr master) : _hotkey(hotkey) noexcept {
	macro_passive::Ptr instance;
	if (!master) {
		poe_log(MSG_ERROR, "Macro_passive") << "invalid parameter";
		return nullptr;
	}
	try {
		instance = macro_passive::Ptr(new macro_passive(name, master));
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "Macro_passive") << e.what();
		return nullptr;
	}

	return instance;
}
#endif

int macro_passive::action(const char * const &topic, void *ctx) {
	if (!strcmp(topic, MARCO_STATUS_BOARDCAST)) {
		struct macro_status *status = (struct macro_status *)ctx;
		if (status->name && strcmp(status->name, _name.c_str())) {
			_flags ^= MACRO_FLAGS_ACTIVE;
			return 0;
		}
		_flags = status->status;
		return 0;
	}
	/* not status change notify, must be hardware input notify event. */
	struct keyboard *keyboard = (struct keyboard *)ctx;
	struct tagKBDLLHOOKSTRUCT *message =
		(struct tagKBDLLHOOKSTRUCT *)keyboard->info;
	if (_flags & MACRO_FLAGS_ACTIVE) {
	/* in execute status */
		if (message->vkCode != _hotkey || 
			keyboard->event != 0x0100)
			return 0;
		for (auto item : _items) {
			if(item->action(ctx))
				return -1;
			if (item->duration() < 0)
				Sleep(item->duration());
		}
	} else if (_flags & MACRO_FLAGS_RECORD){
	/* in record status */
		if (_items.size() == 0)
			_time = message->time;
		else {
			_items.back()->set_duration(message->time - _time);
			_time = message->time;
		}
		keyboard_instruction::Ptr item =
			keyboard_instruction::createNew(message->vkCode,
					keyboard->event, -1);
		add_instruction(item);
	}
	return 0;
}

void macro_passive::show(void) {
	if (_items.size() <= 0) {
		poe_log(MSG_DEBUG, "MACRO") << "empty macro";
		return;
	}
	for (auto item : _items) {
		item->show();
	}
}
