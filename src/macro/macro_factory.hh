#ifndef __POE_MACRO_FACTORY_HH
#define __POE_MACRO_FACTORY_HH

#include <memory>

#include <boost/property_tree/ptree.hpp>

#include "macro.hh"

class factory {
public:
	typedef std::shared_ptr<factory> Ptr;
	virtual macro::Ptr build_macro(observer::Ptr owner, boost::property_tree::ptree &root) = 0;
};


class macro_passive_factory : public factory {
public:
	macro::Ptr build_macro(observer::Ptr owner, boost::property_tree::ptree &root) override;
	virtual instruction::Ptr build_instruction(boost::property_tree::ptree &instuction); 
	int get_keyboard_event_definition(bool press);
};

class macro_passive_loop_factory : public factory {
public:
	macro::Ptr build_macro(observer::Ptr owner, boost::property_tree::ptree &root) override;
};

class macro_flask_factory : public factory {
public:
	typedef std::shared_ptr<macro_flask_factory> Ptr;
	macro::Ptr build_macro(observer::Ptr owner, boost::property_tree::ptree &root) override;
	static macro_flask_factory *create_factory(void) {
		return _get();
	}
private:
	macro_flask_factory(void) {}
	static macro_flask_factory *_get() {
		static macro_flask_factory _macro_flask_factory;
		return &_macro_flask_factory;
	}
};

class macro_subsequence_factory : public macro_passive_factory {
public:
	typedef std::shared_ptr<macro_subsequence_factory> Ptr;
	macro::Ptr build_macro(observer::Ptr owner, boost::property_tree::ptree &rtoot) override;
	static macro_subsequence_factory *create_factory(void) {
		return _get();
	}
private:
	macro_subsequence_factory(void) {}
	static macro_subsequence_factory *_get() {
		static macro_subsequence_factory _macro_subsequence_factory;
		return &_macro_subsequence_factory;
	}
};
#endif /* __POE_MACRO_FACTORY_HH */
