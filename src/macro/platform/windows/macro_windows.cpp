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

static DWORD WINAPI loop_execute_macro(LPVOID lpParam) {
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
		if ((message->vkCode == _hotkey) && !(_flags & MACRO_FLAGS_EXECUTE)) {
			_flags |= MACRO_FLAGS_EXECUTE;
			CreateThread(nullptr, 0, loop_execute_macro,
				shared_from_this().get(), 0, nullptr);
		} else if (message->vkCode == _hotkey_stop) {
			/* TODO : lock */
			_flags &= ~MACRO_FLAGS_EXECUTE;
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
