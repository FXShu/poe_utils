#ifndef __WINDOWS_TIMER_HH__
#define __WINDOWS_TIMER_HH__

#include <map>
#include <exception>

#include <winuser.h>

class poe_timer {
public:
	class Callback {
	public:
		virtual ~Callback() {}
		virtual void operator() (DWORD dwTime) = 0;
	};
	template<class T>
	class ClassCallback : public Callback {
	private:
		T* _classPtr;
		typedef void (T::*timer_cb)(DWORD dwTime);
		timer_cb _cb;
	public:
		ClassCallback(T* classPtr, timer_cb cb) : _classPtr(classPtr), _cb(cb) {}
		~ClassCallback() {}
		virtual void operator()(DWORD dwTime) override {
			(_classPtr->*_cb)(dwTime);
		}

	};
	static long add_timer(Callback *object, DWORD interval) {
		UINT_PTR id = SetTimer(NULL, 0, interval, TimerProc);
		_timers[id] = object;
		return id;
	}
	static void del_timer(long timer_id) {
		KillTimer(NULL, timer_id);
		delete _timers[timer_id];
	}
	static void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR timerId, DWORD dwTime) {
		_timers[timerId]->operator()(dwTime);
	}
private:
	static std::map<UINT_PTR, Callback *> _timers;
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
std::map<UINT_PTR, poe_timer::Callback *>poe_timer::_timers;

#endif /* __WINDOWS_TIMER_HH__ */
