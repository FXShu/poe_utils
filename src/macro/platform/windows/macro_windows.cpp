#include <typeinfo>
#include <exception>
#include <windows.h>
#include "macro.hh"
#include "informer.hh"
#include "macro_windows_define.hh"
#include "windows_timer.hh"
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

int macro_passive_loop::execute(void) {
	if (_flags & MACRO_FLAGS_EXECUTE) {
		poe_log_fn(MSG_INFO, "MACRO_PASSIVE_LOOP", __func__)
			<< "macro instance already execute";
		return 0;
	}
	_flags |= MACRO_FLAGS_EXECUTE;
	_timer_id = poe_timer::add_timer(
		new poe_timer::ClassCallback<macro_passive_loop>(
			this, &macro_passive_loop::_timer_cb),
		_interval);
	if (!_timer_id) {
		throw windows_timer_exception(GetLastError());
	}
	return 0;
}

int macro_passive_loop::stop(void) {
	_flags &= ~MACRO_FLAGS_EXECUTE;
	poe_timer::del_timer(_timer_id);
	return 0;
}

void macro_passive_loop::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
		if (item->action(nullptr)) {
			poe_log(MSG_WARNING, "loop_execute_macro") << "command execute fail";
		}
		if (item->duration() > 0)
			Sleep(item->duration());
	}
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
			if (!_switch)
				execute();
			else
				stop();
			_switch = !_switch;
		} else {
			if ((message->vkCode == _hotkey))
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

int macro_flask::execute(void) {
	if (_flags & MACRO_FLAGS_EXECUTE) {
		poe_log_fn(MSG_INFO, "MACRO_FLASK", __func__)
			<< "macro instance already execute";
		return 0;
	}
	_flags |= MACRO_FLAGS_EXECUTE;

	/* execute instructation once at first start */
	for (auto item : _items) {
		try {
			flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
			flask->action(nullptr);
			flask->multiple_reset();
		} catch (std::bad_cast) {
			poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
		}
	}

	_timer_id = poe_timer::add_timer(
		new poe_timer::ClassCallback<macro_flask>(this, &macro_flask::_timer_cb), _interval);
	if (!_timer_id) {
		throw windows_timer_exception(GetLastError());
	}
	return 0;
}

void macro_flask::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
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
}
