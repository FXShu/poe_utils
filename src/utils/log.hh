#ifndef __UTILS_LOG_HH
#define __UTILS_LOG_HH
#include "includes.hh"

#define RED "\033[0;32;31m"
#define YOLLOW "\033[1;33m"
#define NONE "\033[m"

enum loglevel_e {
	MSG_ERROR,
	MSG_WARNING,
	MSG_INFO,
	MSG_DEBUG,
	MSG_EXCESSIVE
};

class logIt {
public:
	logIt(std::string module, loglevel_e _log = MSG_ERROR) {
		switch(_log) {
		case MSG_ERROR:
			_buffer << RED << "[ ERROR ] ";
			break;
		case MSG_WARNING:
			_buffer << YOLLOW << "[WARNING] ";
			break;
		default:
			_buffer << "[MESSAGE] ";
			break;
		}
		_buffer << "[" << module << "]";
	}
	template <typename T>
	logIt & operator<<(T const &value) {
		_buffer << value;
		return *this;
	}
	~logIt() {
		_buffer << std::endl;
		std::cerr << _buffer.str() << NONE;
	}
private:
	std::ostringstream _buffer;
};

extern loglevel_e loglevel;

#define poe_log(level, module) \
	if (level > loglevel); \
	else logIt(module, level)
#endif /* __UTILS_LOG_HH */
