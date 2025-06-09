#ifndef __WINDOWS_INFORMER_HH
#define __WINDOWS_INFORMER_HH

#include "interceptor.hh"
#include "observer.hh"
#include "macro.hh"

class informer : public interceptor, public observer<struct instruction_event> {
public :
	typedef std::shared_ptr<informer> Ptr;
	static Ptr init(std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>);
	virtual ~informer() {}
private :
	informer(std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>);
};
#endif /* __WINDOWS_INFORMER_HH */
