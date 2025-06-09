#ifndef __MACRO_HH
#define __MACRO_HH
#include <cstdio>
#include <boost/property_tree/ptree.hpp>
#include "utils_header.hh"
#include "observer.hh"
#ifdef _WIN32
#include "platform/windows/macro_windows_define.hh"
#endif
#define MARCO_STATUS_BOARDCAST "macro_status_boardcast"

/* macro flags bitmap */
#define MACRO_FLAGS_RECORD (1u << 0)
#define MACRO_FLAGS_ACTIVE (1u << 1)
#define MACRO_FLAGS_EXECUTE (1u << 2)

enum macro_type {
	MACRO_GENERIC,
	MACRO_PASSIVE,
	MACRO_PASSIVE_LOOP,
	MACRO_FLASK,
	MACRO_SUBSEQUENCE
};

enum instruction_type {
	INSTRUCTION_TYPE_KEYBOARD,
	INSTRUCTION_TYPE_MOUSE,
	INSTRUCTION_TYPE_MAXIMUM
};

struct macro_status {
	const char *name;
	int status;
};

class instruction {
public :
	typedef std::shared_ptr<instruction> Ptr;
	virtual int action(void *ctx) = 0;
	virtual void show(void) = 0;
	virtual ~instruction() {}
	int duration() {return _duration;}
	void set_duration(int duration) {_duration = duration;}
	virtual void descript(boost::property_tree::ptree *ptree) = 0;
	virtual bool check_token(void);
protected :
	instruction(int duration, std::string token = "", float fitness = 0.0) :
		_duration(duration), _token(token), _fitness(fitness) {}
	int _duration;
	std::string _token;
	float _fitness;
};

enum mouse_button {
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_MAX,
};

class mouse_instruction : public instruction {
public:
	typedef std::shared_ptr<mouse_instruction> Ptr;
	static Ptr createNew(enum mouse_button button, int cursor_x,
			int cursor_y, int delay,
			std::string token = "", int check_token_delay = 0, float fitness = 0.0) {
		Ptr instance = Ptr(new mouse_instruction(button, cursor_x, cursor_y,
					delay, token, check_token_delay, fitness));
		return instance;
	}
	virtual int action(void *ctx) override;
	void show(void) override;
	virtual void descript(boost::property_tree::ptree *ptree) override;
protected:
	mouse_instruction(enum mouse_button button, int cursor_x, int cursor_y,
			int delay, std::string token, int check_token_delay, float fitness) :
		instruction(delay, token, fitness), _button(button), _cursor_x(cursor_x),
		_cursor_y(cursor_y), _check_instruction_wait_time_ms(check_token_delay) {}
	enum mouse_button _button;
	int _cursor_x;
	int _cursor_y;
	int _check_instruction_wait_time_ms = 50;
};

class keyboard_instruction : public instruction {
public :
	typedef std::shared_ptr<keyboard_instruction> Ptr;
	static Ptr createNew(unsigned int code, unsigned int type,
			unsigned int duration, std::string token = "",
			int check_token_delay = 0, float fitness = 0.0) {
		Ptr instance = Ptr(new keyboard_instruction(code, type, duration,
					token, check_token_delay, fitness));
		return instance;
	}
	virtual int action(void *ctx) override;
	void show(void) override;
	virtual void descript(boost::property_tree::ptree *ptree) override;
protected :
	keyboard_instruction(unsigned int virtual_code,
		unsigned int type,unsigned int duration,
		std::string token, int check_token_delay, float fitness) :
		instruction(duration, token, fitness), 
		_type(type), _code(virtual_code),
		_check_instruction_wait_time_ms(check_token_delay) {}
	unsigned int _type;
	unsigned int _code;
	int _check_instruction_wait_time_ms = 50;
};

class flask_instruction : public keyboard_instruction {
public:
	typedef std::shared_ptr<flask_instruction> Ptr;
	static Ptr createNew(const char *name, unsigned int code,
		unsigned int type, unsigned int duration) {
		Ptr instance  = Ptr(new flask_instruction(name, code, type, duration));
		return instance;
	}
	void multiple_set(unsigned int multiple) {
		_multiple_record = multiple;
		_multiple = _multiple_record;
	}
	void multiple_reset(void) {
		_multiple = _multiple_record;
	}
	unsigned int multiple_get(void) {
		return _multiple;
	}
	void multiple_decrease(void) {
		_multiple--;
	}
	std::string get_name(void) {
		return _name;
	}
	int action(void *ctx) override;
	void descript(boost::property_tree::ptree *ptree) override;
private:
	flask_instruction(const char *name, unsigned int code,
		unsigned int type, unsigned int duration) :
		keyboard_instruction(code, type, duration, "", 0, 0.0), _name(name) {}
	unsigned int _multiple;
	unsigned int _multiple_record;
	std::string _name;
};

class macro {
public :
	typedef std::shared_ptr<macro> Ptr;
	virtual ~macro() {}
	int rename(std::string);
	std::string getname(void) {
		return _name;
	}
	int add_instruction(instruction::Ptr item);
	int remove_instruction(unsigned int order);
	int replace_instruction(unsigned int order, instruction::Ptr item);
	virtual void onboarding(void) = 0;
	virtual void statistic(boost::property_tree::ptree *tree);
protected :
	macro(const char *name) : _name(name) {}
	std::vector<instruction::Ptr> _items;
	std::string _name;
};
/***
 * macro_passive
 * class member :
 * 1. flags : bitmap, see "macro flags bitmap".
 * 2. _time : the timestamp of last message arrived.
 *   it used to calculate some specific event duration like keyboard, mouse event...etc.
 * constructure parameter:
 * 1. name : name of this macro.
 * 2. master : a observer to switch macro status.
 */
class macro_passive : public subscriber<struct instruction_event>, public macro {
public :
	typedef std::shared_ptr<macro_passive> Ptr;
	static Ptr createNew(const char *name, uint8_t hotkey,
			typename observer<struct instruction_event>::Ptr master,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool> ready) noexcept;
	void show(void);
protected :
	macro_passive(const char *name, uint8_t hotkey,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex> mtx,
			std::shared_ptr<std::condition_variable> cv,
			std::shared_ptr<bool> ready):
		subscriber<struct instruction_event>(name, queue, mtx, cv, ready),
		macro(name), _flags(0), _hotkey(hotkey) {}
	virtual int action (const char * const &topic, void *ctx) override;
	virtual int record(instruction::Ptr item, unsigned long time);
	virtual void statistic(boost::property_tree::ptree *tree) override;
	virtual void platform_sleep(int milliseconds);
	virtual void onboarding(void) override;
	int _flags;
	unsigned long _time;
	uint8_t _hotkey;
};

class macro_passive_loop : public macro_passive {
public :
	typedef std::shared_ptr<macro_passive_loop> Ptr;
	static Ptr createNew(const char *name, uint8_t start,
			uint8_t stop, int interval,
			typename observer<struct instruction_event>::Ptr master,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>>,
			std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool> ready) noexcept;
	virtual ~macro_passive_loop() {poe_log(MSG_DEBUG, "macro_passive_loop") << "discostructor";}
protected :
	macro_passive_loop(const char *name, uint8_t start, uint8_t stop, int interval,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex> mtx,
			std::shared_ptr<std::condition_variable> cv,
			std::shared_ptr<bool> ready):
		macro_passive(name, start, queue, mtx, cv, ready), _interval(interval),
		_hotkey_stop(stop), _switch(0), _timer(nullptr) {}
	virtual int action(const char * const &topic, void *ctx) override;
	virtual void statistic(boost::property_tree::ptree *tree) override;
	virtual int execute(void);
	int stop(void);
	static void __stdcall _timer_cb(void *parameter, unsigned char);
	void work(void);
	int _interval;
	uint8_t _hotkey_stop;
	uint8_t _switch;
	void *_timer;
};

class macro_flask : public macro_passive_loop {
public :
	typedef std::shared_ptr<macro_flask> Ptr;
	static Ptr createNew(const char *name, uint8_t start,
		uint8_t stop, typename observer<struct instruction_event>::Ptr master,
		std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
		std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>,
		std::shared_ptr<bool> ready) noexcept;
	void add_flask(const char *name, unsigned int code, int duration);
	void remove_flask(const char *name);
	void statistic(boost::property_tree::ptree *tree) override;
private :
	macro_flask(const char *name, uint8_t start, uint8_t stop,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex> mtx,
			std::shared_ptr<std::condition_variable> cv,
			std::shared_ptr<bool> ready):
		macro_passive_loop(name, start, stop, -1, queue, mtx, cv, ready) {}
	void cal_comm_factor(void);
	virtual int execute(void) override;
	static void __stdcall _timer_cb(void *parameter, unsigned char);
	void work(void);
	int record(instruction::Ptr item, unsigned long time) override {
		/* not support recording, do nothing */
		return 0;
	}
};

class macro_subsequence : public macro_passive_loop {
public:
	typedef std::shared_ptr<macro_subsequence> Ptr;
	static Ptr createNew(const char *name, uint8_t start,
			uint8_t stop, int interval,
			typename observer<struct instruction_event>::Ptr master,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>,
			std::shared_ptr<bool> ready) noexcept;
	virtual ~macro_subsequence() {poe_log(MSG_DEBUG, "macro_subsequence") << "disconstructor";}
protected:
	macro_subsequence(const char *name, uint8_t start, uint8_t stop, int interval,
			std::shared_ptr<ThreadsafeQueue<struct instruction_event>> queue,
			std::shared_ptr<std::mutex> mtx,
			std::shared_ptr<std::condition_variable> cv,
			std::shared_ptr<bool> ready):
		macro_passive_loop(name, start, stop, interval, queue, mtx, cv, ready) {}
	virtual int execute(void) override;
	virtual int action(const char * const &topic, void *ctx) override;
	virtual int validation(void);
	virtual void statistic(boost::property_tree::ptree *tree) override;
	static void __stdcall _timer_cb(void *parameter, unsigned char);
	void work(void);
	const int _instruction_interval_ms = 200;
	const int _repeated_wait_time_ms = 500;
	int record(instruction::Ptr item, unsigned long time) override {
		/* TODO: recording feature */
		return 0;
	}
};

class instruction_exception : public std::exception {
public:
	instruction_exception(const char *const &reason) : _reason(reason) {}
	const char *what() const throw() override {
		return _reason;
	}
private:
	const char *_reason;
};
#endif /* __MACRiO_HH */
