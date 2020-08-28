#ifndef __BASIC_OBSERVER_HH
#define __BASIC_OBSERVER_HH

#include <utils_header.hh>

class observer;

class subscriber : public std::enable_shared_from_this<subscriber> {
public :
	typedef std::shared_ptr<subscriber> Ptr;
/*	static Ptr createNew(void) {
		Ptr instance = Ptr(new subscriber);
		return instance;
	}*/
	virtual ~subscriber() {}
	int subscribe(std::shared_ptr<observer> obs, std::string topic);
	int unsubscribe(std::shared_ptr<observer> obs, std::string topic);
	virtual int action (void *ctx, void *user) = 0;
protected :
	subscriber() {}
};

class observer{
public :
	typedef std::shared_ptr<observer> Ptr;
	static Ptr createNew() {
		Ptr instance = Ptr(new observer);
		return instance;
	}
	int subscribe_accept(subscriber::Ptr sub, std::string topic);
	int create_section(std::string topic);
protected :
	int publish(std::string topic);
	observer() {}
private :
	std::map<std::string, std::vector<subscriber::Ptr>> _subscribers;
};

#endif /* __BASIC_OBSERVER_HH */
