#ifndef __MACRO_HH
#define __MACRO_HH
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
	MACRO_PASSIVE_LOOP
};

enum instruction_type {
	INSTRUCTION_TYPE_KEYBOARD,
	INSTRUCTION_TYPE_MAXIMUM,
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
protected :
	instruction(int duration) : _duration(duration) {}
	int _duration;
};

class keyboard_instruction : public instruction {
public :
	typedef std::shared_ptr<keyboard_instruction> Ptr;
	static Ptr createNew(unsigned int code, unsigned int type ,unsigned int duration) {
		Ptr instance = Ptr(new keyboard_instruction(code, type, duration));
		return instance;
	}
	virtual int action(void *ctx) override;
	void show(void) override;
	void descript(boost::property_tree::ptree *ptree) override;
protected :
	keyboard_instruction(unsigned int virtual_code,
		unsigned int type,unsigned int duration) : instruction(duration), 
		_type(type), _code(virtual_code) {}
	unsigned int _type;
	unsigned int _code;
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
private:
	flask_instruction(const char *name, unsigned int code,
		unsigned int type, unsigned int duration) :
		keyboard_instruction(code, type, duration), _name(name) {}
	unsigned int _multiple;
	unsigned int _multiple_record;
	std::string _name;
};

class macro {
public :
	typedef std::shared_ptr<macro> Ptr;
	static Ptr createNew(const char *name) {
		Ptr instance = Ptr(new macro(name));
		return instance;
	}
	virtual ~macro() {}
	int rename(std::string);
	std::string getname(void) {
		return _name;
	}
	int add_instruction(instruction::Ptr item);
	int remove_instruction(unsigned int order);
	int replace_instruction(unsigned int order, instruction::Ptr item);
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
class macro_passive : public subscriber, public macro {
public :
	typedef std::shared_ptr<macro_passive> Ptr;
	static Ptr createNew(const char *name, uint8_t hotkey,observer::Ptr master) noexcept;
	void show(void);
protected :
	macro_passive(const char *name, uint8_t hotkey) :
		subscriber(name), macro(name), _flags(0), _hotkey(hotkey) {}
	virtual int action (const char * const &topic, void *ctx) override;
	virtual int record(instruction::Ptr item, unsigned long time);
	virtual void statistic(boost::property_tree::ptree *tree) override;
	int _flags;
	unsigned long _time;
	uint8_t _hotkey;
};

DWORD WINAPI loop_execute_macro(LPVOID lpParam);

class macro_passive_loop : public macro_passive {
friend DWORD WINAPI loop_execute_macro(LPVOID lpParam);
public :
	typedef std::shared_ptr<macro_passive_loop> Ptr;
	static Ptr createNew(const char *name, uint8_t start,
			uint8_t stop, int interval, observer::Ptr master) noexcept;
	virtual ~macro_passive_loop() {poe_log(MSG_DEBUG, "loop_execute_macro") << "discostructor";}
protected :
	macro_passive_loop(const char *name, uint8_t start, uint8_t stop, int interval) :
		macro_passive(name, start), _interval(interval), _hotkey_stop(stop), _switch(0) {}
	virtual int action(const char * const &topic, void *ctx) override;
	virtual int execute(void) noexcept;
	virtual int stop(void) noexcept;
	void statistic(boost::property_tree::ptree *tree) override;
	int _interval;
	uint8_t _hotkey_stop;
	uint8_t _switch;
};

class macro_flask : public macro_passive_loop {
friend DWORD WINAPI loop_execute_flask_macro(LPVOID lpParam);
public :
	typedef std::shared_ptr<macro_flask> Ptr;
	static Ptr createNew(const char *name, uint8_t start,
		uint8_t stop, observer::Ptr master) noexcept;
	void add_flask(const char *name, unsigned int code, int duration);
	void remove_flask(const char *name);
private :
	macro_flask(const char *name, uint8_t start, uint8_t stop) :
		macro_passive_loop(name, start, stop, -1) {}
	void cal_comm_factor(void);
	int execute(void) noexcept override;
	int record(instruction::Ptr item, unsigned long time) override {
		/* not support recording, do nothing */
		return 0;
	}
};

#endif /* __MACRiO_HH */
