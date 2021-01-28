#ifndef __POE_SUPERVISOR_HH
#define __POE_SUPERVISOR_HH

#include <boost/property_tree/ptree.hpp>
#include "macro.hh"

template <class T>
class supervisor : public subscriber, public observer{
public:
	typedef std::shared_ptr<supervisor> Ptr;
	virtual int recruit(std::shared_ptr<T> s) = 0;
	virtual int fire(const char *name) = 0;
	virtual int deploy(void) = 0;
	virtual boost::property_tree::ptree statistic(void) = 0;
protected:
	supervisor(const char *name) : subscriber(name) {} 
	std::vector<std::shared_ptr<T>> _subordinates;
	boost::property_tree::ptree _ptree;
};

class macro_supervisor : public supervisor<macro> {
public:
	typedef std::shared_ptr<macro_supervisor> Ptr;
	static Ptr createNew(const char *name) noexcept {
		Ptr instance = Ptr(new macro_supervisor(name));
		return instance;
	}
	int recruit(macro::Ptr s) override;
	int fire(const char *name) override;
	int deploy(void) override;
	boost::property_tree::ptree statistic(void) override;
	int action(const char *const &topic, void *ctx);
private:
	macro_supervisor(const char *name) : supervisor(name) {}
};
#endif /* __POE_SUPERVISOR_HH */
