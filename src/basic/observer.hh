#ifndef __BASIC_OBSERVER_HH
#define __BASIC_OBSERVER_HH

#include <utils_header.hh>

class observer;

class subscriber : public std::enable_shared_from_this<subscriber> {
public :
	friend observer;
	typedef std::shared_ptr<subscriber> Ptr;
	virtual ~subscriber() {}
	int subscribe(std::shared_ptr<observer> obs, std::string topic);
	int unsubscribe(std::shared_ptr<observer> obs, std::string topic);
protected :
	virtual int action (void *ctx) = 0;
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
	int create_session(std::string topic);
	int publish(std::string topic, void *ctx);
protected :
	observer() {}
private :
	std::map<std::string, std::vector<subscriber::Ptr>> _subscribers;
};

#endif /* __BASIC_OBSERVER_HH */
