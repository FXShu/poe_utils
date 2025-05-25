#include <stdlib.h>
#include <exception>
#include "utils_header.hh"

#ifdef _WIN32
#include "informer.hh"
#endif

#include "macro.hh"

loglevel_e loglevel;

DWORD WINAPI send_terminal(void *id) {
	Sleep(5000);
	PostThreadMessage((DWORD)id, POE_MESSAGE_TERMINAL, 0, 0);
	return 0;
}

int hardcode_instructions_for_test_purpose(macro::Ptr macro) {
	/* Using skill */
	instruction::Ptr item = keyboard_instruction::createNew('Q', WM_KEYDOWN, 60); 
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	item = keyboard_instruction::createNew('Q', WM_KEYUP, 5000);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	/* Enter to market */
	//item = mouse_instruction::createNew(MOUSE_BUTTON_LEFT, 1482, 1010, 5000);
	item = mouse_instruction::createNew(MOUSE_BUTTON_LEFT, 1960, 1350, 5000);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);
	
	/* Move to the left */
	item = keyboard_instruction::createNew('%', WM_KEYDOWN, 5000);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	item = keyboard_instruction::createNew('%', WM_KEYUP, 1000);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

#if 0
	item = mouse_instruction::createNew(MOUSE_BUTTON_LEFT, 700, 500, 1000);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);
#endif
	/* Move to the right */
	item = keyboard_instruction::createNew(0x27, WM_KEYDOWN, 1500);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	item = keyboard_instruction::createNew(0x27, WM_KEYUP, 60000);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	/* click above arrow button */
	item = keyboard_instruction::createNew('&', WM_KEYDOWN, 60);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	item = keyboard_instruction::createNew('&', WM_KEYUP, 60);
	if (nullptr == item) {
		poe_log(MSG_ERROR, "hardcode_instructions_for_test_purpose") <<
			"create instruction failed";
		return -1;
	}
	macro->add_instruction(item);

	return 0;
}

int main(int argc, char **argv) {
	try {
		/* intercepte windows system event. */
		Sleep(500);
		loglevel = loglevel_e::MSG_DEBUG;
		informer::Ptr informer = informer::init();
		poe_log(MSG_DEBUG, "Macro test") << "address of informer " << informer;

		observer::Ptr master = observer::createNew();
		poe_log(MSG_DEBUG, "Macro test") << "address of master observer " << master;
		if (master->create_session(MARCO_STATUS_BOARDCAST))
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create \"macro status\" session";
#if 1
		macro_subsequence::Ptr macro =
			macro_subsequence::createNew("macro test", 's', 't', 100000, master);
#else
		macro_passive_loop::Ptr macro =
			macro_passive_loop::createNew("macro test", 'Q', 'W', 4000, master);
		macro_passive::Ptr macro = macro_passive::createNew("macro test", 'Q', master);
#endif
		if (!macro)
			exit(EXIT_FAILURE);
		else
			poe_log(MSG_DEBUG, "Macro test") << "create new macro instace success";

		/* TODO recording feature*/
#if 0
		/* Set status of macro to recording */
		macro_status status = {
			.name = "macro test",
			.status = MACRO_FLAGS_RECORD
		};
		if (master->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		poe_log(MSG_DEBUG, "Macro test") << "force \"macro test\" macro to recording status";
		unsigned long id;
		id = GetCurrentThreadId();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)send_terminal, (void *)id, 0, &id);
		poe_log(MSG_DEBUG, "Macro test") << "start intcepte windows system message";
		
		if (informer->intercept())
			exit(EXIT_FAILURE);
		macro->show();
		/* name is nullptr for boardcast. */
#else
		/* Test field, hardcode instruction */
		if (hardcode_instructions_for_test_purpose(macro))
			exit(EXIT_FAILURE);
#endif
		macro_status status = {
			.name = nullptr,
			.status = MACRO_FLAGS_ACTIVE
		};
		if (master->publish(MARCO_STATUS_BOARDCAST, &status))
			exit(EXIT_FAILURE);
		poe_log(MSG_DEBUG, "Macro test") << "force \"macro test\" macro to execute status";
		if (informer->intercept()) {
			poe_log(MSG_ERROR, "Informer") << "intercept message failed, exit";
			exit(EXIT_FAILURE);
		}
	} catch (std::exception &e) {
		poe_log(MSG_ERROR, "macro test") << e.what();
	}
	exit(EXIT_SUCCESS);
}
