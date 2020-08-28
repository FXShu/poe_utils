#ifndef __WINDOWS_INFORMER_HH
#define __WINDOWS_INFORMER_HH

#include "interceptor.hh"
#include "observer.hh"

class informer : public interceptor, public observer {
public :
	typedef std::shared_ptr<informer> Ptr;
	static Ptr createNew() {
		Ptr instance = Ptr(new informer);
		return instance;
	}
	virtual ~informer() {}
private :
	LRESULT WINAPI message_handle(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT WINAPI keyboard_handle(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT WINAPI mouse_handle(int nCode, WPARAM wParam, LPARAM lParam);
	informer();
}
#endif /* __WINDOWS_INFORMER_HH */
