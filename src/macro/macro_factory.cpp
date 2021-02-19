#include "macro_factory.hh"
#include "log.hh"

macro::Ptr macro_passive_factory::build_macro(observer::Ptr owner, boost::property_tree::ptree &root) {
	macro_passive::Ptr instance;
	
	return instance;
}

macro::Ptr macro_passive_loop_factory::build_macro(observer::Ptr owner,
	boost::property_tree::ptree &root) {
	macro_passive_loop::Ptr instance;
	return instance;
}

macro::Ptr macro_flask_factory::build_macro(observer::Ptr owner, boost::property_tree::ptree &root) {
	macro_flask::Ptr instance;
	try {
		instance = macro_flask::createNew(
				(root.get<std::string>("name", std::string(""))).c_str(),
				root.get<char>("hotkey", '0'),
				root.get<char>("hotkey_stop", '0'),
				owner);
		auto it = root.get_child("instruction");
		for (auto instruction = it.begin(); instruction != it.end(); ++instruction) {
			instance->add_flask(
				(instruction->second.get<std::string>("name", std::string(""))).c_str(),
				instruction->second.get<char>("code", '0'),
				instruction->second.get<int>("duration", 0)
			);
		}
	} catch (boost::property_tree::ptree_bad_path){
		poe_log_fn(MSG_WARNING, "macro_flask_factory", __func__) <<
			"necessary parameter missing";
		return nullptr;
	}
	return instance;
}
