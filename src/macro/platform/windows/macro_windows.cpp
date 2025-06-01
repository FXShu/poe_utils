#include <typeinfo>
#include <exception>
#include <windows.h>
#include "macro.hh"
#include "informer.hh"
#include "macro_windows_define.hh"
#include "windows_timer.hh"
#include "io.hh"
#include "macro_factory.hh"
extern informer::Ptr poe_informer;

static DWORD WaitWithMessageLoop(DWORD milliseconds) {
    DWORD startTime = GetTickCount();
    while (true) {
        DWORD elapsed = GetTickCount() - startTime;
        if (elapsed >= milliseconds)
            break;

        DWORD wait = MsgWaitForMultipleObjects(
            0, nullptr, FALSE, milliseconds - elapsed, QS_ALLINPUT);

        if (wait == WAIT_OBJECT_0) {
            // There's a message in the queue — process it
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else {
            break; // timeout or error
        }
    }

    return 0;
}

void macro_passive::platform_sleep(int milliseconds) {
	WaitWithMessageLoop(static_cast<DWORD>(milliseconds));
}

int macro_passive_factory::get_keyboard_event_definition(bool press) {
	if (press) {
		return WM_KEYDOWN;
	} else {
		return WM_KEYUP;
	}

	return -1;
}

static int get_window_mouse_instruction(enum mouse_button button, bool press) {
	switch(button) {
	case MOUSE_BUTTON_LEFT:
		if (press)
			return MOUSEEVENTF_LEFTDOWN;
		else
			return MOUSEEVENTF_LEFTUP;
	case MOUSE_BUTTON_RIGHT:
		if (press)
			return MOUSEEVENTF_RIGHTDOWN;
		else
			return MOUSEEVENTF_RIGHTUP;
	case MOUSE_BUTTON_MIDDLE:
		if (press)
			return MOUSEEVENTF_MIDDLEDOWN;
		else
			return MOUSEEVENTF_MIDDLEUP;
	default:
		poe_log(MSG_WARNING, __func__) << "unknown mouse type " << button;
	}
	return -1;
}
int mouse_instruction::action(void *ctx) {
	SetCursorPos(_cursor_x, _cursor_y);
	INPUT input[2] = {0};

	input[0].type = INPUT_MOUSE;
	input[0].mi.dwFlags = get_window_mouse_instruction(_button, true);

	input[1].type = INPUT_MOUSE;
	input[1].mi.dwFlags = get_window_mouse_instruction(_button, false);

	if (ARRAY_SIZE(input) != SendInput(ARRAY_SIZE(input), input, sizeof(INPUT))) {
		poe_log(MSG_WARNING, "mouse_instruction") << "Send mouse event failed, "
			<< GetLastError();
		return -1;
	}
	poe_log(MSG_DEBUG, "mouse_instruction") << "Send mouse event " << _button << " {"
		<< _cursor_x << ", " << _cursor_y << "}";

	WaitWithMessageLoop(_check_instruction_wait_time_ms);
	if (!check_token()) {
		return -1;
	}
	return 0;
}

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
	poe_log(MSG_DEBUG, "keyboard_instruction") << "Post keyboard message type " <<
		_type << " button " << static_cast<char>(_code);
	WaitWithMessageLoop(_check_instruction_wait_time_ms);
	if (!check_token()) {
		if (_type == WM_KEYDOWN) {
			PostMessage(handle, WM_KEYUP, _code,
					KEYBOARD_PREVIOUSR_STATUS | KEYBOARD_TRANSITION_STATUS);
		}
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
		poe_log(MSG_ERROR, "macro_passive_loop") << "invalid parameter";
		return nullptr;
	}

	try {
		instance = macro_passive_loop::Ptr(
			new macro_passive_loop(name, start, stop, interval));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "macro_passive_loop") << e.what();
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
		} catch (std::bad_cast const &e) {
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

macro_subsequence::Ptr macro_subsequence::createNew(const char *name,
		uint8_t start, uint8_t stop, int interval, observer::Ptr master) noexcept {
	macro_subsequence::Ptr instance;

	if (!master) {
		poe_log(MSG_ERROR, "macro_subsequence") << "invalid parameter";
		return nullptr;
	}

	try {
		instance = macro_subsequence::Ptr(
			new macro_subsequence(name, start, stop, interval));
		instance->subscribe(poe_informer, POE_KEYBOARD_EVENT);
		instance->subscribe(poe_informer, POE_MOUSE_EVENT);
		instance->subscribe(master, MARCO_STATUS_BOARDCAST);
	} catch (observer_exception &e) {
		poe_log(MSG_ERROR, "macro_subsequence") << e.what();
		return nullptr;
	}
	return instance;
}

int macro_subsequence::execute(void) {
	if (_flags & MACRO_FLAGS_EXECUTE) {
		poe_log_fn(MSG_INFO, "macro_subsequence", __func__)
			<< "macro instance already execute";
		return 0;
	}
	_flags |= MACRO_FLAGS_EXECUTE;
	_timer_id = poe_timer::add_timer(
		new poe_timer::ClassCallback<macro_subsequence>(
			this, &macro_subsequence::_timer_cb),
		_interval);
	_timer_cb(0);
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
