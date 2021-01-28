#include "supervisor.hh"

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

int macro_supervisor::deploy(void) {
	/* TODO */
	return -1;
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
