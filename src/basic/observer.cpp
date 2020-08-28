#include "observer.hh"
#include <stdio.h>
int subscriber::subscribe(observer::Ptr obs, std::string topic) {
	return obs->subscribe_accept(shared_from_this(), topic);
}

int subscriber::unsubscribe(observer::Ptr obs, std::string topic) {

}

int subscriber::action(void *ctx, void *user) {
	printf("hello, here is %p\n", this);
}

int observer::subscribe_accept(subscriber::Ptr sub, std::string topic) {
	if (!sub || topic.empty()) {
		poe_log(MSG_WARNING, "Observer") << "Invalid parameter";
		return -1;
	}
	auto iter = _subscribers.find(topic);
	if (iter == _subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "Invalid topic";
		return -1;
	}

	iter->second.push_back(sub);
	return 0;
}

int observer::create_section(std::string topic) {
	auto iter = _subscribers.find(topic);
	if (iter != _subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "section already exist";
		return -1;
	}
	_subscribers.insert(std::pair<std::string, std::vector<subscriber::Ptr>>
					(topic, std::vector<subscriber::Ptr>()));
	return 0;
}
int observer::publish(std::string topic) {
	auto iter = _subscribers.find(topic);
	if (iter == _subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "Not exist section";
		return -1;
	}
	for(subscriber::Ptr sub : iter->second) {
		sub->action(NULL, NULL);
	}
}
