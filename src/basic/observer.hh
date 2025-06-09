#ifndef __BASIC_OBSERVER_HH
#define __BASIC_OBSERVER_HH

#include <exception>
#include <condition_variable>
#include "utils_header.hh"
#include "notify.hh"

template <typename T>
class observer;

template <typename T>
class subscriber : public std::enable_shared_from_this<subscriber<T>> {
public :
	//friend observer;
	typedef std::shared_ptr<subscriber> Ptr;
	virtual ~subscriber() {}
	int subscribe(std::shared_ptr<observer<T>> obs, const char * const &topic) noexcept;
	int unsubscribe(std::shared_ptr<observer<T>> obs, const char * const &topic) noexcept;
	void work(void);
protected :
	subscriber(const char *name, std::shared_ptr<ThreadsafeQueue<T>> queue,
			std::shared_ptr<std::mutex> mtx,
			std::shared_ptr<std::condition_variable> cv,
			std::shared_ptr<bool> ready) :
		_name(name), queue_(queue), mtx_(mtx), cv_(cv), ready_(ready) {}
	virtual int action (const char * const &topic, void *ctx) = 0;
	std::string _name;
	std::shared_ptr<ThreadsafeQueue<T>> queue_;
	std::shared_ptr<std::mutex> mtx_;
	std::shared_ptr<std::condition_variable> cv_;
	/* To check spurious wakeup */
	std::shared_ptr<bool> ready_;
};

template<typename T>
class observer {
public :
	typedef std::shared_ptr<observer> Ptr;
	int subscribe_accept(std::shared_ptr<subscriber<T>> sub, const char * const &topic);
	int create_session(const char * const &topic);
	int publish(T &event);
	static std::shared_ptr<observer<T>>
	createNew(std::shared_ptr<ThreadsafeQueue<T>> queue,
			std::shared_ptr<std::mutex>,
			std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool> ready);

protected :
	std::map<std::string, std::vector<typename subscriber<T>::Ptr>> _subscribers;
	observer(std::shared_ptr<ThreadsafeQueue<T>> queue,
			std::shared_ptr<std::mutex> mtx,
			std::shared_ptr<std::condition_variable> cv,
			std::shared_ptr<bool> ready) :
		queue_(queue), mtx_(mtx), cv_(cv), ready_(ready) {}
	std::shared_ptr<ThreadsafeQueue<T>> queue_;
	std::shared_ptr<std::mutex> mtx_;
	std::shared_ptr<std::condition_variable> cv_;
	std::shared_ptr<bool> ready_;
};


class observer_exception : public std::exception {
public:
	observer_exception(const char * const &reason) : _reason(reason) {}
	const char *what() const throw() override {
		return _reason;
	}
private:
	const char *_reason;
};
template <class T>
int subscriber<T>::subscribe(std::shared_ptr<observer<T>> obs, const char * const &topic) noexcept {
	try {
		return obs->subscribe_accept(this->shared_from_this(), topic);
	} catch (observer_exception &e) {
		poe_log_fn(MSG_WARNING, "subscriber", __func__) << e.what() << ": " << topic;
		return -1;
	}
	poe_log_fn(MSG_EXCESSIVE, "observer", __func__) << "publish event to queue";
	return 0;
}

template <class T>
int subscriber<T>::unsubscribe(std::shared_ptr<observer<T>> obs, const char * const &topic) noexcept {
	return -1;
}

template <class T>
void subscriber<T>::work(void) {
	poe_log_fn(MSG_INFO, "subscriber", __func__) << "subscriber " << _name << " is on duty";
	while(true) {
		std::unique_lock<std::mutex> lock(*mtx_);
		poe_log_fn(MSG_EXCESSIVE, "subscriber", __func__) << "Wait for the event";
		cv_->wait(lock, [&]{ return !queue_->empty(); });
		auto event = queue_->Pop();
		*ready_ = false;
		lock.unlock();
		poe_log_fn(MSG_EXCESSIVE, "subscriber", __func__) << "receive event from queue";
		action(event.topic.c_str(), event.context.get());
	}
}

template <class T>
std::shared_ptr<observer<T>>
observer<T>::createNew(std::shared_ptr<ThreadsafeQueue<T>> queue,
		std::shared_ptr<std::mutex> mtx,
		std::shared_ptr<std::condition_variable> cv,
		std::shared_ptr<bool> ready) {
	typename observer<T>::Ptr instance;
	instance = typename observer<T>::Ptr(new observer<T>(queue, mtx, cv, ready));
	return instance;
}

template <class T>
int observer<T>::subscribe_accept(std::shared_ptr<subscriber<T>> sub, const char * const &topic) {
	if (!sub || !topic) {
		poe_log(MSG_WARNING, "Observer") << "Invalid parameter";
		return -1;
	}
	auto iter = this->_subscribers.find(topic);
	if (iter == this->_subscribers.end()) {
		throw observer_exception("No topic found");
	}

	iter->second.push_back(sub);
	return 0;
}

template <class T>
int observer<T>::create_session(const char * const &topic) {
	auto iter = this->_subscribers.find(topic);
	if (iter != this->_subscribers.end()) {
		poe_log(MSG_WARNING, "Observer") << "session already exist";
		return -1;
	}
	this->_subscribers.insert(std::pair<std::string, std::vector<std::shared_ptr<subscriber<T>>>>
					(topic, std::vector<std::shared_ptr<subscriber<T>>>()));
	return 0;
}

template <class T>
int observer<T>::publish(T &event) {
	std::lock_guard<std::mutex> lock(*mtx_);
	queue_->Push(event);
	*ready_ = true;
	cv_->notify_one();
	return 0;
}
#endif /* __BASIC_OBSERVER_HH */
