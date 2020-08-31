#ifndef __WINDOWS_INFORMER_HH
#define __WINDOWS_INFORMER_HH

#include "interceptor.hh"
#include "observer.hh"

class informer : public interceptor, public observer {
public :
	typedef std::shared_ptr<informer> Ptr;
	static Ptr init();
	virtual ~informer() {}
private :
	informer();
};
#endif /* __WINDOWS_INFORMER_HH */
