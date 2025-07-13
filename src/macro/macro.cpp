#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#else
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif /* _WIN32_WINNT */
#endif /* _WIN32 */

#include <boost/property_tree/ptree.hpp>
#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <regex>
#define DPP_NO_DEPRECATED ON
#include <dpp/dpp.h>

#include "macro.hh"
#include "io.hh"
#include "parser.hh"
#include "utils.hh"
#include "message.hh"
#include "macro_factory.hh"

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

static std::unordered_map<std::string, std::string> variables;

static std::shared_ptr<dpp::cluster> bot;
static std::once_flag bot_init_flag;

void macro_module_deinit(void) {
	if (bot)
		bot.reset();
}

int discord_notification_instruction::action(void *ctx) {
	std::regex re(R"(\$([a-zA-Z_][a-zA-Z0-9_]*))");
	std::string result = _message;
	std::smatch match;
	std::string formatted;
	std::string ::const_iterator search_start(result.cbegin());

	while(std::regex_search(search_start, result.cend(), match, re)) {
		formatted.append(search_start, match[0].first);
		std::string var_name = match[1].str();

		auto it = variables.find(var_name);
		if (it != variables.end()) {
			formatted.append(it->second);
		} else {
			poe_object_log_fn(MSG_WARNING) << "variable " << var_name << " not existed";
			return -1;
		}
		search_start = match[0].second;
	}
	formatted.append(search_start, result.cend());

	std::call_once(bot_init_flag, [this] () {
		bot = std::make_shared<dpp::cluster>(_token,
			dpp::i_default_intents | dpp::i_message_content);
		bot->on_log(dpp::utility::cout_logger());
	});

	bot->message_create(dpp::message(_channel, formatted));

	bot->start(dpp::st_return);
	int wait_ms = 0;
	dpp::discord_client *client = nullptr;
	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(_bot_connection_check_ms));
		wait_ms += _bot_connection_check_ms;
		client = bot->get_shard(0);
	} while ((nullptr == client || !client->is_connected()) &&
		wait_ms < _bot_connection_waiting_endure);
	bot->shutdown();
	return 0;
}

void discord_notification_instruction::show(void) {
	poe_object_log(MSG_INFO) << "send message" << " to channel " << _channel <<
		" via token " << _token;
}

void discord_notification_instruction::descript(boost::property_tree::ptree *ptree) {}

int variable_obtain_instruction::action(void *ctx) {
	cv::Mat screen = utils::screenshot();
	if (screen.empty()) {
		poe_object_log_fn(MSG_WARNING) << "screen shot failed";
		return -1;
	}

	cv::Rect roi(_coordinate.start_x, _coordinate.start_y,
			_coordinate.end_x - _coordinate.start_x,
			_coordinate.end_y - _coordinate.start_y);

	screen = screen(roi);

	cv::Mat gray;
	cv::cvtColor(screen, gray, cv::COLOR_BGR2GRAY);

	// Threshold for better contrast;
	cv::Mat thresh;
	cv::threshold(gray, thresh, 130, 255, cv::THRESH_BINARY);

	// Initialize Tesseract API
	tesseract::TessBaseAPI tess;
	if (tess.Init("./tessdata", "eng", tesseract::OEM_LSTM_ONLY)) {
		poe_object_log_fn(MSG_WARNING) << "Could not initialize Tesseract";
		return -1;
	}

	tess.SetVariable("tessedit_char_whitelist", "0123456789");
	tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	tess.SetImage(thresh.data, thresh.cols, thresh.rows, 1, thresh.step);

	/* Get OCR result */
	std::string result = tess.GetUTF8Text();
	poe_object_log_fn(MSG_DEBUG) << "Obtain variable " << _variable << " = " << result;
	/* TODO: where should we store the variable? */
	variables[_variable] = result;

	return 0;
}

void variable_obtain_instruction::show(void) {
	poe_object_log(MSG_INFO) << "Try to obtain variable " << _variable <<
		" from scrren " << get_coordinate();
}

void variable_obtain_instruction::descript(boost::property_tree::ptree *ptree) {}

int condition_instruction::action(void *ctx) {
	if (check_token()) {
		/* success */
		poe_object_log_fn(MSG_DEBUG) << "execute success action";
		for (auto &item : _success_actions) {
			while (item->action(nullptr)) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(_repeated_wait_time_ms));
			}
			if (item->duration() > 0) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(item->duration()));
			} else {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(_instruction_interval_ms));
			}
		}
	} else {
		/* failed */
		poe_object_log_fn(MSG_DEBUG) << "execute failure action";
		for (auto &item : _failure_actions) {
			while (item->action(nullptr)) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(_repeated_wait_time_ms));
			}
			if (item->duration() > 0) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(item->duration()));
			} else {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(_instruction_interval_ms));
			}
		}
	}
	return 0;
}

void condition_instruction::show(void) {
	switch (_type) {
	case condition_type::IMAGE_RECOGNIZE:
		poe_object_log(MSG_INFO) << "check the image " <<  _token <<
		" presents in the screen " << get_coordinate();
		break;
	default:
		poe_object_log(MSG_INFO) << "invalid condition";
		break;
	}
}

void condition_instruction::descript(boost::property_tree::ptree *ptee) {}

condition_instruction::Ptr
condition_instruction::createNew(const boost::property_tree::ptree &config) {
	condition_instruction::Ptr instance = nullptr;
	try {
		instance = condition_instruction::Ptr(new condition_instruction());
		auto &condition = config.get_child("condition");
		auto &success = config.get_child("success");
		auto &failure = config.get_child("failure");
		if (!instance->generate_condition(condition) ||
			!instance->generate_action(instance->_success_actions, success) ||
			!instance->generate_action(instance->_failure_actions, failure)) {
			// TODO : fix memory leak.
			return nullptr;
		}

	} catch (boost::property_tree::ptree_bad_path const &e) {
		poe_log_fn(MSG_ERROR, "condition_instruction", __func__) <<
			"necessary parameter missing";
		return nullptr;
	}
	return instance;
}

bool condition_instruction::generate_action(std::vector<instruction::Ptr> &actions,
		const boost::property_tree::ptree &config) {
	for (auto iter = config.begin(); iter != config.end(); ++iter) {
		auto instruction = builder::build_instruction(iter->second);
		if (nullptr == instruction) {
			poe_object_log_fn(MSG_WARNING) << "invalid instruction detected";
			return false;
		}
		actions.push_back(instruction);
	}
	return true;
}

bool condition_instruction::generate_condition(const boost::property_tree::ptree &action) {
	try {
		_type = static_cast<enum condition_type>(action.get<int>("type"));
		switch(_type) {
		case condition_type::IMAGE_RECOGNIZE:
			_token = action.get<std::string>("token");
			if (parse_coordinate(action.get<std::string>("coordinate"))) {
				poe_object_log_fn(MSG_ERROR) << "invalid coordinate";
				return false;
			}
			_fitness = action.get<float>("token_fitness");
			break;
		default:
			poe_object_log_fn(MSG_ERROR) << "unknown condition type " << _type;
			return false;
		}
	} catch (boost::property_tree::ptree_bad_path const &e) {
		poe_object_log_fn(MSG_WARNING) << "necessary parameter missing";
		return false;
	}
	return true;
}
std::string instruction::get_coordinate(void) {
	std::stringstream ss;
	ss << "(" << _coordinate.start_x <<  ", " << _coordinate.start_y <<
		", " << _coordinate.end_x << ", " << _coordinate.end_y << ")";

	return ss.str();
}


bool instruction::parse_coordinate(const std::string &coordinate_str) {
	std::stringstream ss(coordinate_str);
	char comma;

	if ((ss >> _coordinate.start_x >> comma && comma == ',') &&
			(ss >> _coordinate.start_y >> comma && comma == ',') &&
			(ss >> _coordinate.end_x >> comma && comma == ',') &&
			(ss >> _coordinate.end_y >> comma && comma == ',')) {
		return ss.eof();
	}
	return false;
}

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

	if (_coordinate.start_x || _coordinate.start_y || _coordinate.end_x || _coordinate.end_y) {
		cv::Rect roi(_coordinate.start_x, _coordinate.start_y,
				_coordinate.end_x - _coordinate.start_x,
				_coordinate.end_y - _coordinate.start_y);
		screen = screen(roi);
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
	try {
	poe_log_fn(MSG_EXCESSIVE, macro::_name.c_str(), __func__) << "receive message topic " <<
		topic << " execute action";
	if (!strcmp(topic, MARCO_STATUS_BOARDCAST)) {
		poe_log_fn(MSG_DEBUG, macro::_name.c_str(), __func__) <<
			"receive marco status boardcast event";
		struct macro_status *status = static_cast<struct macro_status *>(ctx);
		if (status->name && strcmp(status->name, macro::_name.c_str())) {
			/* other macro, igonre */
			return 0;
		}
		poe_log_fn(MSG_DEBUG, macro::_name.c_str(), __func__) <<
			"set status to " << status->status;
		_flags = status->status;
		return 0;
	}
	/* not status change notify, must be hardware input notify event. */
	poe_log_fn(MSG_DEBUG, "macro_passive", __func__) << ": flags " << _flags;
	if (!strcmp(topic, POE_KEYBOARD_EVENT)) {
		struct keyboard *keyboard = (struct keyboard *)ctx;
		struct tagKBDLLHOOKSTRUCT *message =
			(struct tagKBDLLHOOKSTRUCT *)keyboard->info;
		if (_flags & MACRO_FLAGS_ACTIVE) {
		/* in execute status */
			if (message->vkCode != _hotkey ||
				keyboard->event != KEYBOARD_MESSAGE_KEYDOWN ||
				_flags & MACRO_FLAGS_EXECUTE)
				return 0;
			_flags |= MACRO_FLAGS_EXECUTE;
			for (auto item : _items) {
				if(item->action(ctx))
					break;
				if (item->duration() > 0)
					platform_sleep(item->duration());
			}
			poe_log_fn(MSG_EXCESSIVE, macro::_name.c_str(), __func__) << "end macro";
			_flags &= ~MACRO_FLAGS_EXECUTE;
		} else if (_flags & MACRO_FLAGS_RECORD){
		/* in record status */
			keyboard_instruction::Ptr item =
				keyboard_instruction::createNew(message->vkCode, keyboard->event, -1);
			record(item, message->time);
		}
	} else if (!strcmp(topic, POE_MOUSE_EVENT)) {
		poe_log(MSG_WARNING, "macro_passive") << "receive mouse event";
	}
	poe_log_fn(MSG_EXCESSIVE, macro::_name.c_str(), __func__) << "end action";
	} catch (std::exception &e) {
		poe_log(MSG_ERROR, "macro_passive") << e.what();
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

void macro_subsequence::_timer_cb(void *parameter, unsigned char expired) {
	macro_subsequence *self = static_cast<macro_subsequence *>(parameter);
	self->work();
}

void macro_subsequence::work() {
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

void macro_flask::_timer_cb(void *parameter, unsigned char expired) {
	macro_flask *self = static_cast<macro_flask *>(parameter);
	self->work();
}

void macro_flask::work(void) {
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

void macro_passive_loop::_timer_cb(void *parameter, unsigned char expired) {
	macro_passive_loop *self = static_cast<macro_passive_loop *>(parameter);
	self->work();
}

void macro_passive_loop::work(void) {
	for (auto item : _items) {
		if (item->action(nullptr)) {
			poe_log(MSG_WARNING, "loop_execute_macro") << "command execute fail";
		}
		if (item->duration() > 0)
			platform_sleep(item->duration());
	}
}

void macro_passive::onboarding(void) {
	work();
}
