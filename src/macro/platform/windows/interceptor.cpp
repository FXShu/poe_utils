#include "interceptor.hh"

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
}

interceptor::~interceptor() {
	if (_handle) {
		if (!PostThreadMessageA(_id, MESSAGE_TERMINAL, 0 , 0)) {
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
		if (message.message == MESSAGE_TERMINAL) {
			poe_log(MSG_DEBUG, "Interceptor")
			<< "get terminal signal, stop intercept message";
			break;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

int interceptor::intercept() {
	_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)intercept_message,
			NULL, 0, &_id);
	if (!_handle) {
		poe_log(MSG_WARNING, "Interceptor")
		<< "Create new thread fail, error code "<< GetLastError();
		return -1;
	}
	return 0;
}

LRESULT WINAPI informer::message_handle(int nCode, WPARAM wParam, LPARAM lParam) {
	MSG *message = (MSG *)lParam;
	publish(parser::get_msg(message.message));
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT WINAPI keyboard_handle(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT *kbdStruct;

	kbdStruct = (KBDLLHOOKSTRUCT *)lParam;
	
}

informer::informer() {
	/* keyboard event */
	if (!install_hook(WH_KEYBOARD_LL, Keyboard_handle))
		create_section(std::string(POE_KEYBOARD_EVENT));
	/* mouse event */
	if (!install_hook(WH_MOUSE_LL, mouse_handle))
		create_section(std::string(POE_MOUSE_EVENT));
	/* message event */
	if (!install_hook(WH_GETMESSAGE, message_handle))
		create_section(std::string(POE_MESSAGE_EVENT));
}
