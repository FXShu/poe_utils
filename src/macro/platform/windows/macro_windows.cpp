#include <windows.h>
#include "macro.hh"
#include "informer.hh"
#include "macro_windows_define.hh"
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
		instance = macro_passive::Ptr(new macro_passive(name, hotkey, master));
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
		uint8_t start, uint8_t stop, observer::Ptr master)
	noexcept {
	macro_passive_loop::Ptr instance;

	if (!master) {
		poe_log(MSG_ERROR, "Macro_passive") << "invalid parameter";
		return nullptr;
	}

	try {
		instance = macro_passive_loop::Ptr(new macro_passive_loop(name, start, stop, master));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "Macro_passive") << e.what();
		return nullptr;
	}
	return instance;
}
