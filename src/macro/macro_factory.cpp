#include "macro_factory.hh"
#include "log.hh"

macro::Ptr macro_passive_factory::build_macro(
		typename observer<struct instruction_event>::Ptr owner,
		boost::property_tree::ptree &root,
		std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
		std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv,
		std::shared_ptr<bool> ready) {
	macro_passive::Ptr instance;
	
	try {
		instance = macro_passive::createNew(
				(root.get<std::string>("name", std::string(""))).c_str(),
				root.get<char>("hotkey"),
				owner, queue, mtx, cv, ready);
		auto instructions = root.get_child("instruction");
		for (auto iter = instructions.begin(); iter != instructions.end(); ++iter) {
			auto instruction = build_instruction(iter->second);
			if (nullptr == instruction) {
				poe_log_fn(MSG_WARNING, "macro_subsequence_factory", __func__) <<
					"Invalid instruction detected";
				/* TODO: fix memory leak */
				return nullptr;
			}
			instance->add_instruction(instruction);
		}
	} catch (boost::property_tree::ptree_bad_path const &e) {
		poe_log_fn(MSG_WARNING, "macro_passive_factory", __func__) <<
			"necessary parameter missing";
		return nullptr;
	}
	return instance;
}

instruction::Ptr macro_passive_factory::build_instruction(boost::property_tree::ptree &instruction) {
	instruction::Ptr instance;
	try {
		enum instruction_type type = (enum instruction_type)instruction.get<int>("type");

		switch(type) {
		case INSTRUCTION_TYPE_KEYBOARD:
			instance = keyboard_instruction::createNew(
					instruction.get<char>("key", '0'),
					get_keyboard_event_definition(instruction.get<int>("press", 0)),
					instruction.get<int>("delay", 0),
					instruction.get<std::string>("token", ""),
					instruction.get<int>("check_token_delay", 50),
					instruction.get<float>("token_fitness", 0.8));
			break;
		case INSTRUCTION_TYPE_MOUSE:
			instance = mouse_instruction::createNew(
					(enum mouse_button)instruction.get<int>("button"),
					instruction.get<int>("cursor_x", 0),
					instruction.get<int>("cursor_y", 0),
					instruction.get<int>("delay", 0),
					instruction.get<std::string>("token", ""),
					instruction.get<int>("check_token_delay", 50),
					instruction.get<float>("token_fitness", 0.8));
			break;
		default:
			poe_log_fn(MSG_WARNING, "macro_passive_factory", __func__) <<
				"unknown instruction type " << type;
			return nullptr;
		}
	} catch (boost::property_tree::ptree_bad_path const &e) {
		poe_log_fn(MSG_WARNING, "macro_passive_factory", __func__) <<
			"neceassary parameter missing";
		return nullptr;
	}
	instance->show();
	return instance;
}

macro::Ptr macro_passive_loop_factory::build_macro(
		typename observer<struct instruction_event>::Ptr owner,
		boost::property_tree::ptree &root,
		std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
		std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv,
		std::shared_ptr<bool> ready) {
	macro_passive_loop::Ptr instance;
	return instance;
}

macro::Ptr macro_flask_factory::build_macro(
		typename observer<struct instruction_event>::Ptr owner,
		boost::property_tree::ptree &root,
		std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
		std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv,
		std::shared_ptr<bool> ready) {
	macro_flask::Ptr instance;
	try {
		instance = macro_flask::createNew(
				(root.get<std::string>("name", std::string(""))).c_str(),
				root.get<char>("hotkey", '0'),
				root.get<char>("hotkey_stop", '0'),
				owner, queue, mtx, cv, ready);
		auto it = root.get_child("instruction");
		for (auto instruction = it.begin(); instruction != it.end(); ++instruction) {
			instance->add_flask(
				(instruction->second.get<std::string>("name", std::string(""))).c_str(),
				instruction->second.get<char>("code", '0'),
				instruction->second.get<int>("duration", 0)
			);
		}
	} catch (boost::property_tree::ptree_bad_path const &e){
		poe_log_fn(MSG_WARNING, "macro_flask_factory", __func__) <<
			"necessary parameter missing";
		return nullptr;
	}
	return instance;
}



macro::Ptr macro_subsequence_factory::build_macro(
		typename observer<struct instruction_event>::Ptr owner,
		boost::property_tree::ptree &root,
		std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
		std::shared_ptr<std::mutex> mtx, std::shared_ptr<std::condition_variable> cv,
		std::shared_ptr<bool> ready) {
	macro_subsequence::Ptr macro;
	try {
		macro = macro_subsequence::createNew(
				(root.get<std::string>("name", std::string(""))).c_str(),
				root.get<char>("hotkey", 's'), // default start hotkey F4
				root.get<char>("hotkey_stop", 't'), // default stop hotkey F5
				root.get<int>("period_ms", 0),
				owner, queue, mtx, cv, ready);
		auto instructions = root.get_child("instruction");
		for (auto iter = instructions.begin(); iter != instructions.end(); ++iter) {
			auto instruction = build_instruction(iter->second);
			if (nullptr == instruction) {
				poe_log_fn(MSG_WARNING, "macro_subsequence_factory", __func__) <<
					"Invalid instruction detected";
				/* TODO: fix memory leak */
				return nullptr;
			}
			macro->add_instruction(instruction);
		}
	} catch (boost::property_tree::ptree_bad_path const &e) {
		poe_log_fn(MSG_WARNING, "macro_subsequence_factory", __func__) <<
			"necessary parameter missing";
		return nullptr;
	}
	return macro;
}
