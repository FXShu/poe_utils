#include <typeinfo>
#include <exception>
#include <windows.h>
#include "macro.hh"
#include "informer.hh"
#include "macro_windows_define.hh"
#include "io.hh"
extern informer::Ptr poe_informer;

int keyboard_instruction::action(void *ctx) {
	int ret;
	HWND handle = (HWND)GetCurrentProcess();
	if (!handle)
		return -1;
	switch(_type) {
	case WM_KEYDOWN:
		ret = PostMessage(handle, _type, _code, 0);
		break;
	case WM_KEYUP:
		ret = PostMessage(handle, _type, _code,
			KEYBOARD_PREVIOUSR_STATUS | KEYBOARD_TRANSITION_STATUS);
		break;
	}
	if (!ret) {
		poe_log(MSG_WARNING, "keyboard_instruction") << "post message fail" << GetLastError();
		return -1;
	}
	return 0;
}

int flask_instruction::action(void *ctx) {
	int ret;
	HWND handle = (HWND)GetCurrentProcess();
	if (!handle)
		return -1;
	poe_log_fn(MSG_DEBUG, "keyboard_instruction", __func__) << "post keyboard event " << (char)_code;
	ret = PostMessage(handle, WM_KEYDOWN, _code, 0);
	Sleep(60);
	ret = PostMessage(handle, WM_KEYUP, _code, KEYBOARD_PREVIOUSR_STATUS | KEYBOARD_TRANSITION_STATUS);
	return ret;
}

macro_passive::Ptr macro_passive::createNew(const char *name, uint8_t hotkey,observer::Ptr master)
	noexcept {
	macro_passive::Ptr instance;

	if (!master) {
		poe_log(MSG_ERROR, "Macro_passive") << "invalid parameter";
		return nullptr;
	}

	try {
		instance = macro_passive::Ptr(new macro_passive(name, hotkey));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "Macro_passive") << e.what();
		return nullptr;
	}
	return instance;
}

macro_passive_loop::Ptr macro_passive_loop::createNew(const char *name,
		uint8_t start, uint8_t stop, int interval, observer::Ptr master) noexcept {
	macro_passive_loop::Ptr instance;

	if (!master) {
		poe_log(MSG_ERROR, "Macro_passive") << "invalid parameter";
		return nullptr;
	}

	try {
		instance = macro_passive_loop::Ptr(
			new macro_passive_loop(name, start, stop, interval));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "Macro_passive") << e.what();
		return nullptr;
	}
	return instance;
}

DWORD WINAPI loop_execute_macro(LPVOID lpParam) {
	if (!lpParam) {
		poe_log(MSG_ERROR, "loop_execute_macro") << "invalid parameter";
		return -1;
	} 
	macro_passive_loop *instance = reinterpret_cast<macro_passive_loop *>(lpParam);
	for(;;) {
		/* TODO : lock */
		if (!(instance->_flags & MACRO_FLAGS_EXECUTE))
			break;
		for (auto item : instance->_items) {
			if (item->action(nullptr)) {
				poe_log(MSG_WARNING, "loop_execute_macro") << "command execute fail";
				return -1;
			}
			if (item->duration() > 0)
				Sleep(item->duration());
		}
		Sleep(instance->_interval);
	}
	return 0;
}

int macro_passive_loop::execute(void) noexcept {
	try {
		_flags |= MACRO_FLAGS_EXECUTE;
		CreateThread(nullptr, 0, loop_execute_macro, shared_from_this().get(), 0, nullptr);
	} catch (std::exception &e) {
		poe_log_fn(MSG_WARNING, "macr_passive_loop", __func__) << e.what();
	}
	return 0;
}

int macro_passive_loop::stop(void) noexcept {
	poe_log_fn(MSG_DEBUG, "macro_passive_loop", __func__) << "looping macro: "
		<<  macro::getname() << " turn off";
	_flags &= ~MACRO_FLAGS_EXECUTE;
	return 0;
}

int macro_passive_loop::action(const char * const &topic, void *ctx) {
	if (!strcmp(topic, MARCO_STATUS_BOARDCAST)) {
		struct macro_status *status = static_cast<struct macro_status *>(ctx);
		if (status->name && strcmp(status->name, macro::_name.c_str())) {
			_flags ^= MACRO_FLAGS_ACTIVE;
			return 0;
		}
		_flags = status->status;
		return 0;
	}
	/* not status change notify, must be hardware input notify event. */
	struct keyboard *keyboard = (struct keyboard *)ctx;
	struct tagKBDLLHOOKSTRUCT *message =
		reinterpret_cast<struct tagKBDLLHOOKSTRUCT *>(keyboard->info);
	if ((_flags & MACRO_FLAGS_ACTIVE) && keyboard->event == KEYBOARD_MESSAGE_KEYDOWN) {
		if (_hotkey == _hotkey_stop && message->vkCode == _hotkey) {
			if (!_switch && !(_flags & MACRO_FLAGS_EXECUTE))
				execute();
			else
				stop();
			_switch = !_switch;
		} else {
			if ((message->vkCode == _hotkey) && !(_flags & MACRO_FLAGS_EXECUTE))
				execute();
			else if (message->vkCode == _hotkey_stop)
				stop();
		}
	} else if (_flags & MACRO_FLAGS_RECORD){
		keyboard_instruction::Ptr item =
			keyboard_instruction::createNew(message->vkCode, keyboard->event, -1);
		macro_passive::record(item, message->time);
	}
	return 0;
}

macro_flask::Ptr macro_flask::createNew(const char *name, uint8_t start,
	uint8_t stop, observer::Ptr master) noexcept {
	macro_flask::Ptr instance;

	if (!master) {
		poe_log_fn(MSG_ERROR, "Macro_flask", "constructor") << "invalid parameter";
		return nullptr;
	}
	try {
		instance = macro_flask::Ptr(new macro_flask(name, start, stop));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log_fn(MSG_ERROR, "Macro_flask", "constructor") << e.what();
		return nullptr;
	}
	return instance;
}

void macro_flask::add_flask(const char *name, unsigned int code, int duration) {
	flask_instruction::Ptr flask = flask_instruction::createNew(name, code, WM_KEYDOWN, duration);
	macro::add_instruction(flask);
	cal_comm_factor();
}

void macro_flask::remove_flask(const char *name) {
	int index;
	try {
		for (auto item : _items) {
			flask_instruction* flask = dynamic_cast<flask_instruction *>(item.get());
			if (flask->get_name() == std::string(name))
				break;
			index++;
		}
	} catch (std::bad_cast) {
		poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
	}
	macro::remove_instruction(index);
}

DWORD WINAPI loop_execute_flask_macro(LPVOID lpParam) {
	if (!lpParam) {
		poe_log(MSG_ERROR, "loop_execute_macro") << "invalid parameter";
		return -1;
	}
	macro_flask *instance = reinterpret_cast<macro_flask *>(lpParam);
	/* execute instructation once at first start */
	for (auto item : instance->_items) {
		try {
			flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
			flask->action(nullptr);
			flask->multiple_reset();
		} catch (std::bad_cast) {
			poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
		}
	}
	for(;;) {
		/* TODO : lock */
		if (!(instance->_flags & MACRO_FLAGS_EXECUTE))
			break;
		for (auto item : instance->_items) {
			try {
				flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
				flask->multiple_decrease();
				if (!flask->multiple_get()) {
					flask->action(nullptr);
					flask->multiple_reset();
				}
			} catch (std::bad_cast) {
				poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
			}
		}
		Sleep(instance->_interval);
	}
	return 0;
}
void macro_flask::cal_comm_factor(void) {
	if (_items.empty())
		return;
	int result = _items.front()->duration();
	for (auto item : _items) {
		result = std::__gcd(result, item->duration());
	}
	for (auto item : _items) {
		try {
			flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
			flask->multiple_set(flask->duration() / result);
		} catch (std::bad_cast) {
			poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
		}
	}
	macro_passive_loop::_interval = result;
	poe_log_fn(MSG_DEBUG, "macro_flask", __func__) << "GDC of Flask :" << result;
}

int macro_flask::execute(void) noexcept {
	try {
		_flags |= MACRO_FLAGS_EXECUTE;
		CreateThread(nullptr, 0, loop_execute_flask_macro, shared_from_this().get(), 0, nullptr);
	} catch (std::exception &e) {
		poe_log_fn(MSG_WARNING, "macr_passive_loop", __func__) << e.what();
	}
	return 0;
}
