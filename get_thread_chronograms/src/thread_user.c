#include <zephyr/shell/shell.h>

extern uint8_t ram_tracing[];

static int cmd_dump_trace_ram(const struct shell *sh, size_t argc, char **argv)
{
    // Dumps the RAM trace buffer chunk directly to the shell terminal
    shell_hexdump(sh, ram_tracing, CONFIG_RAM_TRACING_BUFFER_SIZE);
    return 0;
}

SHELL_CMD_REGISTER(dump_trace_ram, NULL, "Dump tracing RAM buffer", cmd_dump_trace_ram);

