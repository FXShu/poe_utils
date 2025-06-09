#include "supervisor.hh"
#include "macro_factory.hh"
#include "message.hh"

int macro_supervisor::work(void) {
	for (auto &employee : _subordinates) {
		auto t = std::make_shared<std::thread>(
				std::bind(&macro::onboarding, employee),
				employee);
		_threads.push_back(t);
	}
	for (auto &t : _threads) {
		t->detach();
		poe_log_fn(MSG_DEBUG, "macro_supervisor", __func__) <<
			"fork thread for employee complete";
	}
	poe_log_fn(MSG_INFO, "macro_supervisor", __func__) << "onboard all macro complete";
	return 0;
}

int macro_supervisor::recruit(macro::Ptr s) {
	_subordinates.push_back(s);
	return 0;
}

int macro_supervisor::fire(const char *name) {
	int order = -1;
	for (auto subordinate : _subordinates) {
		order++;
		if (subordinate->getname() == std::string(name)) {
			_subordinates.erase(_subordinates.begin() + order);
			break;
		}
	}
	return 0;
}

int macro_supervisor::action(const char *const &topic, void *ctx) {
	/* TODO */
	return -1;
}

int macro_supervisor::deploy(boost::property_tree::ptree tree) {
	int macro_type;
	factory *factory;
	macro::Ptr macro;
	auto it = tree.get_child("macro");
	for (auto root = it.begin(); root != it.end(); ++root) {
		macro_type = root->second.get<int>("type", -1);
		switch(macro_type) {
		case MACRO_PASSIVE:
			factory = macro_passive_factory::create_factory();
		break;
		case MACRO_FLASK:
			factory = macro_flask_factory::create_factory();
		break;
		case MACRO_SUBSEQUENCE:
			factory = macro_subsequence_factory::create_factory();
		break;
		default:
			poe_log_fn(MSG_WARNING, "MACRO_SUPERVISOR", __func__) << "Unknow macro type";
			continue;
		}
		macro = factory->build_macro(
			std::enable_shared_from_this<macro_supervisor>::shared_from_this(),
			root->second, queue_, mtx_, cv_, ready_);
		recruit(macro);
	}
	return 0;
}

boost::property_tree::ptree macro_supervisor::statistic(void) {
	boost::property_tree::ptree child;
	for (auto subordinate : _subordinates) {
		boost::property_tree::ptree description;
		subordinate->statistic(&description);
		child.push_back(std::make_pair("", description));
	}
	_ptree.put_child("macro", child);
	return _ptree;
}
