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
	poe_log(MSG_DEBUG, "macro_passive_loop") << __func__ << ": flags " << _flags;
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
	} catch (std::bad_cast const &e) {
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
		} catch (std::bad_cast const &e) {
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

void macro_flask::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
		try {
			flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
			flask->multiple_decrease();
			if (!flask->multiple_get()) {
				flask->action(nullptr);
				flask->multiple_reset();
			}
		} catch (std::bad_cast const &e) {
			poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
		}
	}
}

int macro_subsequence::validation() {
	int total_delay_time = 0;
	for (auto item : _items) {
		total_delay_time += item->duration() > 0 ? item->duration() : _instruction_interval_ms;
	}
	if (total_delay_time >= _interval) {
		poe_log(MSG_WARNING, "macro_subsequence") <<
			"total delay time of instruction exceeds over interval of macro execution";
		return -1;
	}
	return 0;
}

int macro_subsequence::action(const char *const &topic, void *ctx) {
	if (validation()) {
		return -1;
	}
	macro_passive_loop::action(topic, ctx);
	return 0;
}

void macro_subsequence::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
		if (!(_flags & MACRO_FLAGS_EXECUTE))
			break;
		poe_log_fn(MSG_DEBUG, "macro_subsequence", __func__) << "execute new inustrction";
		while ((_flags & MACRO_FLAGS_EXECUTE) && item->action(nullptr)) {
			poe_log_fn(MSG_DEBUG, "macro_subsequence", __func__) << "flags " << _flags;
			WaitWithMessageLoop(_repeated_wait_time_ms);
		}
		if (item->duration() > 0)
			WaitWithMessageLoop(item->duration());
		else
			Sleep(_instruction_interval_ms);
	}
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
