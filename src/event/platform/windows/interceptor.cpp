#include "interceptor.hh"
#include "informer.hh"
#include "io.hh"
#include "parser.hh"
informer::Ptr poe_informer;

int interceptor::install_hook(int type,
	LRESULT WINAPI(*event_handle)(int nCode, WPARAM wParam, LPARAM lParam)) {
	auto iter = _hooks.find(type);
	if (iter != _hooks.end()) {
		poe_log(MSG_WARNING, "Interceptor") << "Same type hook has installed";
		return -1;
	}
	HHOOK hook = SetWindowsHookEx(type, (HOOKPROC)event_handle, GetModuleHandle(NULL), 0);
	if (!hook) {
		poe_log(MSG_WARNING, "Interceptor") << "Install hook fail, error Code "
		<< GetLastError();
		return -1;
	}
	_hooks.insert(std::pair<int, HHOOK> (type, hook));
	return 0;
}

interceptor::~interceptor() {
	if (_handle) {
		if (!PostThreadMessageA(_id, POE_MESSAGE_TERMINAL, 0 , 0)) {
			poe_log(MSG_WARNING, "Interceptor")
			<< "send terminal message to thread failed, error code " << GetLastError();
		} else {
			WaitForSingleObject(_handle, INFINITE);
			_handle = nullptr;	
		}
	}
	for (auto iter = _hooks.begin(); iter != _hooks.end(); iter++) {
		UnhookWindowsHookEx(iter->second);
	}
	poe_log(MSG_DEBUG, "Interceptor") << "destory interceptor";
}

DWORD WINAPI intercept_message(void *) {
	MSG message;
	while(GetMessage(&message, NULL, 0, 0)) {
		if (message.message == POE_MESSAGE_TERMINAL) {
			poe_log(MSG_DEBUG, "Interceptor")
			<< "get terminal signal, stop intercept message";
			break;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return 0;
}

int interceptor::intercept() {
	MSG message;
	while(GetMessage(&message, NULL, 0, 0)) {
		switch(message.message) {
		case POE_MESSAGE_TERMINAL:
			poe_log(MSG_DEBUG, "Interceptor")
			<< "get terminal signal, stop intercept message";
			break;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return 0;
}

LRESULT WINAPI poe_message_handle(int nCode, WPARAM wParam, LPARAM lParam) {
	MSG *message = (MSG *)lParam;
	poe_informer->publish(parser::get_msg(poe_table_message, message->message), message);
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT WINAPI  poe_keyboard_handle(int nCode, WPARAM wParam, LPARAM lParam) {
	struct keyboard keyboard;
	
	keyboard.event = wParam;
	keyboard.info = lParam;
	
	poe_log(MSG_DEBUG, "poe_keyboard_handle") << "capture keyboard event";
	poe_informer->publish(POE_KEYBOARD_EVENT, &keyboard);
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
#define IDLE_TIME_MS 200

POINT last_point = {0,0};
DWORD last_mouse_move_time = 0;
bool is_moving = false;
CRITICAL_SECTION lock;

DWORD WINAPI mouse_movement_monitor(LPVOID arg) {
	while(1) {
		Sleep(50); //Check frequently
		DWORD now = GetTickCount();
#if 1
		//EnterCriticalSection(&lock);
		if (is_moving && now - last_mouse_move_time > IDLE_TIME_MS) {
			poe_log(MSG_DEBUG, "mouse_event") << "Mouse move END at "
				<< last_point.x << ", " << last_point.y;
			is_moving = false;
		}
		//LeaveCriticalSection(&lock);
#endif
	}
	return 0;
}

LRESULT WINAPI mouse_handle(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE) {
		MSLLHOOKSTRUCT *p = (MSLLHOOKSTRUCT *)lParam;
		DWORD now = GetTickCount();

		//EnterCriticalSection(&lock);
		if (!is_moving || (p->pt.x != last_point.x || p->pt.y != last_point.y)) {
			if (!is_moving) {
				is_moving = true;
			}

			last_point = p->pt;
			last_mouse_move_time = now;
		}
		//LeaveCriticalSection(&lock);
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

informer::Ptr informer::init() {
	poe_informer = Ptr(new informer);
	last_mouse_move_time = 0;
	CreateThread(nullptr, 0, mouse_movement_monitor, nullptr, 0, nullptr);

	return poe_informer;
} 

informer::informer() {
	/* keyboard event */
	
	if (!install_hook(WH_KEYBOARD_LL, poe_keyboard_handle)) {
		create_session(POE_KEYBOARD_EVENT);
		poe_log(MSG_DEBUG, "informer") << "install keyboard event hook success";
	}
#if 1
	/* mouse event */
	if (!install_hook(WH_MOUSE_LL, mouse_handle)) {
		create_session(POE_MOUSE_EVENT);
		poe_log(MSG_DEBUG, "informer") << "install mouse event hook success";
	}
#endif
	/* message event */
	if (!install_hook(WH_GETMESSAGE, poe_message_handle)) {
		create_session(parser::get_msg(poe_table_message, POE_MESSAGE_TERMINAL));
		create_session(parser::get_msg(poe_table_message, POE_MESSAGE_CMD));
		poe_log(MSG_DEBUG, "informer") << "install regular message hook success";
	}
}
