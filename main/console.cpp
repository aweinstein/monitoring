#include "console.h"
#include "esp_console.h"
static esp_console_repl_t *repl;

void init_console() {
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

  esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
  esp_console_register_help_command();
  esp_console_start_repl(repl);
}