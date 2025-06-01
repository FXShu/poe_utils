#include <boost/property_tree/ptree.hpp>
#include <opencv2/opencv.hpp>
#include "macro.hh"
#include "io.hh"
#include "parser.hh"
#include "utils.hh"
#include "message.hh"

bool instruction::check_token(void) {
	poe_log_fn(MSG_DEBUG, "instruction", __func__) << "recognize token " <<
		_token << " from screen.";
	if (_token.empty())
		return true;
	cv::Mat screen = utils::screenshot();
	cv::Mat token = cv::imread(_token.c_str());

	if (screen.empty()) {
		poe_log_fn(MSG_WARNING, "instruction", __func__) <<
			"screenshot failed";
		throw instruction_exception("screenshot failed");
	}

	if (token.empty()) {
		poe_log_fn(MSG_WARNING, "instruction", __func__) <<
			"image " << _token << "load failed";
		std::stringstream reason;
		reason << "image " << _token << " load failed";
		throw instruction_exception(reason.str().c_str());

	}
	cv::Mat result;
	cv::matchTemplate(screen, token, result, cv::TM_CCOEFF_NORMED);
	double min_val, max_val;
	cv::minMaxLoc(result, &min_val, &max_val);

	poe_log_fn(MSG_DEBUG, "instruction", __func__) << "maximum match value " << max_val;
	return max_val >= _fitness;
}

void mouse_instruction::show(void) {
	poe_log(MSG_INFO, "mouse_instruction")
		<< "click " << parser::get_msg(poe_table_mouse, _button) <<
		" at {" << _cursor_x << ", " << _cursor_y << "}";
}

void mouse_instruction::descript(boost::property_tree::ptree *ptree) {
	ptree->put("type", INSTRUCTION_TYPE_MOUSE);
	ptree->put("event", _button);
	ptree->put("cursor_x", _cursor_x);
	ptree->put("cursor_y", _cursor_y);
}

void keyboard_instruction::show(void) {
	poe_log(MSG_INFO, "keyboard_instruction")
		<< parser::get_msg(poe_table_keyboard, _type)
		<< " "<< (char)_code << " duration : " << _duration;
}

void keyboard_instruction::descript(boost::property_tree::ptree *ptree) {
	ptree->put("type", INSTRUCTION_TYPE_KEYBOARD);
	ptree->put("event", _type);
	ptree->put("code", (char)_code);
	ptree->put("duration", _duration);
}

void flask_instruction::descript(boost::property_tree::ptree *ptree) {
	ptree->put("name", _name);
	ptree->put("event", _type);
	ptree->put("code", (char)_code);
	ptree->put("duration", _duration);
}

int macro::rename(std::string name) {
	if (name.empty()) {
		poe_log(MSG_WARNING, "Macro") << "Inavliad parameter";
		return -1;		
	}
	_name = name;
	return 0;
}

int macro::add_instruction(instruction::Ptr item) {
	_items.push_back(item);
	return 0;
}

int macro::remove_instruction(unsigned int order) {
	if (order < 0 || order > _items.size()) {
		poe_log(MSG_WARNING, "Macro") << "Invalid parameter";
		return -1;
	}
	_items.erase(_items.begin() + order);
	return 0;
}

int macro::replace_instruction(unsigned int order, instruction::Ptr item) {
	if (order < 0 || order > _items.size() || item->duration() < 0) {
		poe_log(MSG_WARNING, "Macro") << "Invalid parameter";
		return -1;
	}
	_items[order] = item;
	return 0;
}

void macro::statistic(boost::property_tree::ptree *tree) {
	tree->put("type", MACRO_GENERIC);
	tree->put("name", _name);
	boost::property_tree::ptree child;
	for (auto item : _items) {
		boost::property_tree::ptree description;
		item->descript(&description);
		child.push_back(std::make_pair("", description));
	}
	tree->put_child("instruction", child);
}

int macro_passive::record(instruction::Ptr item, unsigned long time) {
	if (_items.size() == 0) {
		_time = time;
	} else {
		_items.back()->set_duration(time - _time);
		_time = time;
	}
	add_instruction(item);
	return 0;
}

int macro_passive::action(const char * const &topic, void *ctx) {
	if (!strcmp(topic, MARCO_STATUS_BOARDCAST)) {
		struct macro_status *status = (struct macro_status *)ctx;
		if (status->name && strcmp(status->name, macro::_name.c_str())) {
			/* other macro, igonre */
			return 0;
		}
		_flags = status->status;
		return 0;
	}
	/* not status change notify, must be hardware input notify event. */
	if (!strcmp(topic, POE_KEYBOARD_EVENT)) {
		struct keyboard *keyboard = (struct keyboard *)ctx;
		struct tagKBDLLHOOKSTRUCT *message =
			(struct tagKBDLLHOOKSTRUCT *)keyboard->info;
		if (_flags & MACRO_FLAGS_ACTIVE) {
		/* in execute status */
			if (message->vkCode != _hotkey ||
				keyboard->event != KEYBOARD_MESSAGE_KEYDOWN)
				return 0;
			for (auto item : _items) {
				if(item->action(ctx))
					return -1;
				if (item->duration() > 0)
					Sleep(item->duration());
			}
		} else if (_flags & MACRO_FLAGS_RECORD){
		/* in record status */
			keyboard_instruction::Ptr item =
				keyboard_instruction::createNew(message->vkCode, keyboard->event, -1);
			record(item, message->time);
		}
	} else if (!strcmp(topic, POE_MOUSE_EVENT)) {
		poe_log(MSG_WARNING, "macro_passive") << "receive mouse event";
	}
	return 0;
}

void macro_passive::show(void) {
	if (_items.size() <= 0) {
		poe_log(MSG_DEBUG, "MACRO") << "empty macro";
		return;
	}
	for (auto item : _items) {
		item->show();
	}
}

void macro_passive::statistic(boost::property_tree::ptree *tree) {
	tree->put("type", MACRO_PASSIVE);
	tree->put("name", macro::_name);
	tree->put("hotkey", (char)_hotkey);
	boost::property_tree::ptree child;
	for (auto item : _items) {
		boost::property_tree::ptree description;
		item->descript(&description);
		child.push_back(std::make_pair("", description));
	}
	tree->put_child("instruction", child);
}

void macro_passive_loop::statistic(boost::property_tree::ptree *tree) {
	tree->put("type", MACRO_PASSIVE_LOOP);
	tree->put("name", macro::_name);
	tree->put("hotkey", (char)_hotkey);
	tree->put("hotkey_stop", (char)_hotkey_stop);
	tree->put("execute_interval", _interval);
	boost::property_tree::ptree child;
	for (auto item : _items) {
		boost::property_tree::ptree description;
		item->descript(&description);
		child.push_back(std::make_pair("", description));
	}
	tree->put_child("instruction", child);
}

void macro_flask::statistic(boost::property_tree::ptree *tree) {
	tree->put("type", MACRO_FLASK);
	tree->put("name", macro::_name);
	tree->put("hotkey", (char)_hotkey);
	tree->put("hotkey_stop", (char)_hotkey_stop);
	tree->put("GCD", _interval);
	boost::property_tree::ptree child;
	for (auto item : _items) {
		boost::property_tree::ptree description;
		item->descript(&description);
		child.push_back(std::make_pair("", description));
	}
	tree->put_child("instruction", child);
}

void macro_subsequence::statistic(boost::property_tree::ptree *tree) {
	tree->put("type", MACRO_SUBSEQUENCE);
	tree->put("name", macro::_name);
	tree->put("hotkey", (char)_hotkey);
	tree->put("hotkey_stop", (char)_hotkey_stop);
	boost::property_tree::ptree child;
	for (auto item : _items) {
		boost::property_tree::ptree description;
		item->descript(&description);
		description.put("delay", item->duration());
		child.push_back(std::make_pair("", description));
	}
	tree->put_child("instruction", child);
}
int macro_passive_loop::action(const char * const &topic, void *ctx) {
	if (!strcmp(topic, MARCO_STATUS_BOARDCAST)) {
		struct macro_status *status = static_cast<struct macro_status *>(ctx);
		if (status->name && strcmp(status->name, macro::_name.c_str())) {
			_flags ^= MACRO_FLAGS_ACTIVE;
			return 0;
		}
		_flags = status->status;
		return 0;
	}
	/* not status change notify, must be hardware input notify event. */
	struct keyboard *keyboard = (struct keyboard *)ctx;
	struct tagKBDLLHOOKSTRUCT *message =
		reinterpret_cast<struct tagKBDLLHOOKSTRUCT *>(keyboard->info);
	poe_log(MSG_DEBUG, "macro_passive_loop") << __func__ << ": flags " << _flags;
	if ((_flags & MACRO_FLAGS_ACTIVE) && keyboard->event == KEYBOARD_MESSAGE_KEYDOWN) {
		if (_hotkey == _hotkey_stop && message->vkCode == _hotkey) {
			if (!_switch)
				execute();
			else
				stop();
			_switch = !_switch;
		} else {
			if ((message->vkCode == _hotkey))
				execute();
			else if (message->vkCode == _hotkey_stop)
				stop();
		}
	} else if (_flags & MACRO_FLAGS_RECORD){
		keyboard_instruction::Ptr item =
			keyboard_instruction::createNew(message->vkCode, keyboard->event, -1);
		macro_passive::record(item, message->time);
	}
	return 0;
}

void macro_flask::add_flask(const char *name, unsigned int code, int duration) {
	flask_instruction::Ptr flask = flask_instruction::createNew(name, code, WM_KEYDOWN, duration);
	macro::add_instruction(flask);
	cal_comm_factor();
}

void macro_flask::remove_flask(const char *name) {
	int index;
	try {
		for (auto item : _items) {
			flask_instruction* flask = dynamic_cast<flask_instruction *>(item.get());
			if (flask->get_name() == std::string(name))
				break;
			index++;
		}
	} catch (std::bad_cast const &e) {
		poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
	}
	macro::remove_instruction(index);
}

void macro_flask::cal_comm_factor(void) {
	if (_items.empty())
		return;
	int result = _items.front()->duration();
	for (auto item : _items) {
		result = std::__gcd(result, item->duration());
	}
	for (auto item : _items) {
		try {
			flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
			flask->multiple_set(flask->duration() / result);
		} catch (std::bad_cast const &e) {
			poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
		}
	}
	macro_passive_loop::_interval = result;
	poe_log_fn(MSG_DEBUG, "macro_flask", __func__) << "GDC of Flask :" << result;
}

void macro_subsequence::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
		if (!(_flags & MACRO_FLAGS_EXECUTE))
			break;
		poe_log_fn(MSG_DEBUG, "macro_subsequence", __func__) << "execute new inustrction";
		while ((_flags & MACRO_FLAGS_EXECUTE) && item->action(nullptr)) {
			poe_log_fn(MSG_DEBUG, "macro_subsequence", __func__) << "flags " << _flags;
			platform_sleep(_repeated_wait_time_ms);
		}
		if (item->duration() > 0)
			platform_sleep(item->duration());
		else
			platform_sleep(_instruction_interval_ms);
	}
}

int macro_subsequence::validation() {
	int total_delay_time = 0;
	for (auto item : _items) {
		total_delay_time += item->duration() > 0 ? item->duration() : _instruction_interval_ms;
	}
	if (total_delay_time >= _interval) {
		poe_log(MSG_WARNING, "macro_subsequence") <<
			"total delay time of instruction exceeds over interval of macro execution";
		return -1;
	}
	return 0;
}

int macro_subsequence::action(const char *const &topic, void *ctx) {
	if (validation()) {
		return -1;
	}
	macro_passive_loop::action(topic, ctx);
	return 0;
}

void macro_flask::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
		try {
			flask_instruction *flask = dynamic_cast<flask_instruction *>(item.get());
			flask->multiple_decrease();
			if (!flask->multiple_get()) {
				flask->action(nullptr);
				flask->multiple_reset();
			}
		} catch (std::bad_cast const &e) {
			poe_log_fn(MSG_WARNING, "macro_flask", __func__) << "dynamic cast fail";
		}
	}
}

void macro_passive_loop::_timer_cb(long unsigned int dwTime) {
	for (auto item : _items) {
		if (item->action(nullptr)) {
			poe_log(MSG_WARNING, "loop_execute_macro") << "command execute fail";
		}
		if (item->duration() > 0)
			platform_sleep(item->duration());
	}
}
