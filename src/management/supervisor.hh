#ifndef __POE_SUPERVISOR_HH
#define __POE_SUPERVISOR_HH

#include <boost/property_tree/ptree.hpp>
#include "macro.hh"

template <class T>
class supervisor : public observer{
public:
	typedef std::shared_ptr<supervisor> Ptr;
	virtual int recruit(std::shared_ptr<T> s) = 0;
	virtual int fire(const char *name) = 0;
	virtual int deploy(boost::property_tree::ptree tree) = 0;
	virtual boost::property_tree::ptree statistic(void) = 0;
protected:
	supervisor(const char *name) : _name(name) {} 
	std::vector<std::shared_ptr<T>> _subordinates;
	boost::property_tree::ptree _ptree;
	std::string _name;
};

class macro_supervisor : public supervisor<macro> ,
	public std::enable_shared_from_this<macro_supervisor>{
public:
	typedef std::shared_ptr<macro_supervisor> Ptr;
	static Ptr createNew(const char *name) noexcept {
		Ptr instance = Ptr(new macro_supervisor(name));
		instance->create_session(MARCO_STATUS_BOARDCAST);
		return instance;
	}
	int recruit(macro::Ptr s) override;
	int fire(const char *name) override;
	int deploy(boost::property_tree::ptree tree) override;
	boost::property_tree::ptree statistic(void) override;
	int action(const char *const &topic, void *ctx);
private:
	macro_supervisor(const char *name) : supervisor(name) {}
};
#endif /* __POE_SUPERVISOR_HH */
