#ifndef __BASIC_OBSERVER_HH
#define __BASIC_OBSERVER_HH

#include <exception>
#include "utils_header.hh"

class observer;

class subscriber : public std::enable_shared_from_this<subscriber> {
public :
	friend observer;
	typedef std::shared_ptr<subscriber> Ptr;
	virtual ~subscriber() {}
	int subscribe(std::shared_ptr<observer> obs, const char * const &topic) noexcept;
	int unsubscribe(std::shared_ptr<observer> obs, const char * const &topic) noexcept;
protected :
	virtual int action (const char * const &topic, void *ctx) = 0;
	std::string _name;
	subscriber(const char *name) : _name(name) {}
};

class observer {
public :
	typedef std::shared_ptr<observer> Ptr;
	static Ptr createNew() {
		Ptr instance = Ptr(new observer);
		return instance;
	}
	int subscribe_accept(subscriber::Ptr sub, const char * const &topic);
	int create_session(const char * const &topic);
	int publish(const char * const &topic, void *ctx);
protected :
	observer() {}
private :
	std::map<std::string, std::vector<subscriber::Ptr>> _subscribers;
};


class observer_exception : public std::exception {
public:
	observer_exception(const char * const &reason) : _reason(reason) {}
	const char *what() const throw() override {
		return _reason;
	}
private:
	const char *_reason;
};
#endif /* __BASIC_OBSERVER_HH */
