#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static int cmd_greet(const struct shell *sh, size_t argc, char **argv)
{
	const char *name = (argc > 1) ? argv[1] : "World";

	shell_print(sh, "Hello, %s! Greetings from Zephyr!", name);
	return 0;
}

SHELL_CMD_REGISTER(greet, NULL, "Print a greeting. Usage: greet [name]", cmd_greet);

int main(void)
{
       LOG_INF("Hello World from Zephyr!");
       while (true) {
	       LOG_INF("Sleeping for 1 second...");
	       k_sleep(K_SECONDS(1));
       }
       return 0;
}

