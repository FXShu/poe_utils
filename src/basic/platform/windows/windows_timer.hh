#ifndef __WINDOWS_TIMER_HH__
#define __WINDOWS_TIMER_HH__

#include <map>
#include <exception>

#include <winuser.h>

class poe_timer {
public:
	typedef void CALLBACK (*timer_cb)(PVOID parameter, BOOLEAN timer_or_wait_fired);
	static PVOID add_timer(timer_cb cb, PVOID object, DWORD due_timer, DWORD interval) {
		HANDLE timer = nullptr;
		if (!CreateTimerQueueTimer(&timer, nullptr, cb, object,
					due_timer, interval, WT_EXECUTELONGFUNCTION)) {
			poe_log_fn(MSG_ERROR, "poe_timer", __func__) <<
				"create timer queue timer failed";
			return nullptr;
		}
		return timer;
	}
	static void del_timer(PVOID timer) {
		if (nullptr != timer)
			DeleteTimerQueueTimer(nullptr, timer, nullptr);
	}
};

class windows_timer_exception : public std::exception {
public:
	windows_timer_exception(DWORD code) : _error_code(code) {}
	const char *what() const throw() override {
		std::ostringstream os;
		os << "timer_exception " << _error_code;
		std::string s = os.str();
		return s.c_str();
	}
private:
	DWORD _error_code;
};

#endif /* __WINDOWS_TIMER_HH__ */
