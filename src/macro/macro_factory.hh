#ifndef __POE_MACRO_FACTORY_HH
#define __POE_MACRO_FACTORY_HH

#include <memory>

#include <boost/property_tree/ptree.hpp>

#include "macro.hh"

class factory {
public:
	typedef std::shared_ptr<factory> Ptr;
	virtual macro::Ptr build_macro(typename observer<struct instruction_event>::Ptr owner,
			boost::property_tree::ptree &root,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex>,
			std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>) = 0;
};


class macro_passive_factory : public factory {
public:
	typedef std::shared_ptr<macro_passive_factory> Ptr;
	macro::Ptr build_macro(typename observer<struct instruction_event>::Ptr owner,
			boost::property_tree::ptree &root,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex>,
			std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>) override;
	virtual instruction::Ptr build_instruction(boost::property_tree::ptree &instuction); 
	int get_keyboard_event_definition(bool press);
	static macro_passive_factory *create_factory(void) {
		return _get();
	}
protected:
	macro_passive_factory(void) {}
private:
	static macro_passive_factory *_get() {
		static macro_passive_factory _macro_passive_factory;
		return &_macro_passive_factory;
	}
};

class macro_passive_loop_factory : public factory {
public:
	macro::Ptr build_macro(typename observer<struct instruction_event>::Ptr owner,
			boost::property_tree::ptree &root,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>,
			std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>) override;
};

class macro_flask_factory : public factory {
public:
	typedef std::shared_ptr<macro_flask_factory> Ptr;
	macro::Ptr build_macro(typename observer<struct instruction_event>::Ptr owner,
			boost::property_tree::ptree &root,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>,
			std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>) override;
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
	macro::Ptr build_macro(typename observer<struct instruction_event>::Ptr owner,
			boost::property_tree::ptree &rtoot,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>,
			std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool>) override;
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
