#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#endif /* _WIN32 */
#define DPP_NO_DEPRECATED ON
#include <dpp/dpp.h>


const std::string BOT_TOKEN = "";
const dpp::snowflake ANNOUNCE_ID = 1388748000137838635;

static std::shared_ptr<dpp::cluster> bot;
static std::once_flag bot_init_flag;
void bot_message(void) {
	std::call_once(bot_init_flag, [] () {
		bot = std::make_shared<dpp::cluster>(BOT_TOKEN,
			dpp::i_default_intents | dpp::i_message_content);
		bot->on_log(dpp::utility::cout_logger());
	});

	bot->message_create(dpp::message(ANNOUNCE_ID, "test for the event notificaition"));
	bot->start(dpp::st_return);
	int wait_ms = 0;
	dpp::discord_client *client = nullptr;
	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		wait_ms += 1000;
		client = bot->get_shard(0);
	} while ((nullptr == client || !client->is_connected()) &&
		wait_ms < 5000);

	bot->shutdown();
}

static std::atomic<bool> running(true);
BOOL WINAPI console_ctrl_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        std::cout << "\n[Ctrl+C received] Shutting down...\n";
        //if (bot) {
        //    bot->shutdown();
        //}
        running = false;
        return TRUE;  // suppress default termination
    }
    return FALSE;
}

int main(int argc, char **argv) {
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
	while (running) {
		std::cout << "start loop testing" << std::endl;
		bot_message();
		std::cout << "ctrl+c !" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		std::cout << "end loop testing" << std::endl;
	}

	bot.reset();
	return 0;
}
