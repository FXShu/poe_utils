#ifndef __INTERCEPTOR_HH
#define __INTERCEPTOR_HH

#include "utils_header.hh"
#include "observer.hh"

#define MESSAGE_TERMINAL (WM_USER + 0x0001)
class interceptor {
public :
	typedef std::shared_ptr<interceptor> Ptr;
	static Ptr createNew() {
		Ptr instance = Ptr(new interceptor);
		return instance;
	}
	virtual ~interceptor();	
	virtual int intercept();
	int install_hook(int type,
#ifdef _WIN32
	LRESULT WINAPI(*event_handle)(int nCode, WPARAM wParam, LPARAM lParam)
#else
	void (*event_handle)(void *, void *)
#endif /* _WIN32 */
	);
private :
	interceptor(){}
	int _hook_type;
	unsigned long _id;

#ifdef _WIN32
	std::map<int, HHOOK> _hooks;
	HANDLE _handle;
#endif /* _WIN32 */
};


#endif /* __INTERCEPTOR_HH  */
