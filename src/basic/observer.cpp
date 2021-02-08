#include "observer.hh"
#include <stdio.h>
int subscriber::subscribe(observer::Ptr obs, const char * const &topic) noexcept {
	return obs->subscribe_accept(shared_from_this(), topic);
}

int subscriber::unsubscribe(observer::Ptr obs, const char * const &topic) noexcept {
	return -1;
}

int observer::subscribe_accept(subscriber::Ptr sub, const char * const &topic) {
	if (!sub || !topic) {
		poe_log(MSG_WARNING, "Observer") << "Invalid parameter";
		return -1;
	}
	auto iter = _subscribers.find(topic);
	if (iter == _subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "Invalid topic";
		throw observer_exception("No topic found");
	}

	iter->second.push_back(sub);
	return 0;
}

int observer::create_session(const char * const &topic) {
	auto iter = _subscribers.find(topic);
	if (iter != _subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "session already exist";
		return -1;
	}
	_subscribers.insert(std::pair<std::string, std::vector<subscriber::Ptr>>
					(topic, std::vector<subscriber::Ptr>()));
	return 0;
}

int observer::publish(const char * const &topic, void *ctx) {
	auto iter = _subscribers.find(topic);
	if (iter == _subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "Not exist session";
		return -1;
	}
	for(subscriber::Ptr sub : iter->second) {
		poe_log(MSG_DEBUG, "Observer") << "pubish topic \""
			<< topic << "\" to \"" << sub->_name << "\" scubscriber";
		sub->action(topic, ctx);
	}
	return 0;
}
