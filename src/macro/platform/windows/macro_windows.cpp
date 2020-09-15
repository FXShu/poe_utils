#include "macro.hh"
#include "informer.hh"
extern informer::Ptr poe_informer;
/***
 * macro_status : notify message to macro object
 * member :
 * 1. name : name of specified macro.
 * 2. status : force macro object enter to this status,
 *    zero for recording, non-zero for execute.
 */

int keyboard_instruction::action(void *ctx) {
	HWND handle = (HWND)ctx;
	if (!PostMessage(handle, _type, _code, 0))
		return -1;
	return 0;
}

macro_passive::Ptr macro_passive::createNew(const char *name, observer::Ptr master) noexcept {
	macro_passive::Ptr instance;

	if (!master) {
		poe_log(MSG_ERROR, "Macro_passive") << "invalid parameter";
		return nullptr;
	}

	try {
		instance = macro_passive::Ptr(new macro_passive(name, master));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "Macro_passive") << e.what();
		return nullptr;
	}
	return instance;
}
