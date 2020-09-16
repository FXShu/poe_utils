#ifndef __MACRO_HH
#define __MACRO_HH

#include "utils_header.hh"
#include "observer.hh"

#define MARCO_STATUS_BOARDCAST "macro_status_boardcast"
#define KEYBOARD_MESSAGE_BASIC 0x0100
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
	unsigned int duration() {return _duration;}
	void set_duration(unsigned int duration) {_duration = duration;}
protected :
	instruction(unsigned int duration) : _duration(duration) {}
	int _duration;
};

class keyboard_instruction : public instruction {
public :
	typedef std::shared_ptr<keyboard_instruction> Ptr;
	static Ptr createNew(unsigned int code,
		unsigned int type ,unsigned int duration) {
		Ptr instance = Ptr(new keyboard_instruction(code, type, duration));
		return instance;
	}
	int action(void *ctx) override;
	void show(void) override;
private :
	keyboard_instruction(unsigned int virtual_code,
		unsigned int type,unsigned int duration) : instruction(duration), 
		_type(type), _code(virtual_code) {}
	unsigned int _type;
	unsigned int _code;

};

class macro {
public:
	typedef std::shared_ptr<macro> Ptr;
	static Ptr createNew(const char *name) {
		Ptr instance = Ptr(new macro(name));
		return instance;
	}
	virtual ~macro() {}
	int rename(std::string);
	int add_instruction(instruction::Ptr item);
	int remove_instruction(unsigned int order);
	int replace_instruction(unsigned int order, instruction::Ptr item);

protected :
	macro(const char *name) : _name(name) {}
	std::vector<instruction::Ptr> _items;
	std::string _name;
};


/***
 * macro_passive
 * class member :
 * 1. _active : status of this macro, zero for record, non-zero for execute.
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
private :
	macro_passive(const char *name, uint8_t hotkey, observer::Ptr master) :
		macro(name), _active(0), _hotkey(hotkey) {}
	int action (const char * const &topic, void *ctx) override;
	int _active;
	unsigned long _time;
	uint8_t _hotkey;
};
#endif /* __MACRiO_HH */
