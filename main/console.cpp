/*
  REPL console implementation via ESP32-IDF console library
*/

#include "console.h"
#include "network.h"
#include "storage.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include <M5Core2.h>
#include <string.h>
#include "global.h"
#include "helper.h"

static esp_console_repl_t *repl;

static struct {
  struct arg_rex *ap = arg_rex0(NULL, NULL, "ap", NULL, ARG_REX_ICASE, "Configure access point");
  struct arg_rex *sta = arg_rex0(NULL, NULL, "station", NULL, ARG_REX_ICASE, "Configure station mode");
  struct arg_str *ssid = arg_str1(NULL, NULL, "<ssid>", "SSID for access point");
  struct arg_str *password = arg_str1(NULL, NULL, "<pass>", "Password for access point, must be at least 8 characters");
  struct arg_end *end = arg_end(2);
} setWifi_args;

static struct {
    struct arg_str *ip;
    struct arg_int *port;
    struct arg_end *end;
} setDB_args;

static struct {
    struct arg_int *time;
    struct arg_end *end;
} setFreq_args;

static struct {
  struct arg_str *timestamp_start;
  struct arg_str *timestamp_end;
  struct arg_end *end;
} recoverData_args;

static struct {
  struct arg_str *timestamp;
  struct arg_end *end;
} setRTC_args;


void init_console() {
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

  esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
  esp_console_register_help_command();
  esp_console_start_repl(repl);

  register_setWifi_cmd();
  register_setDB_cmd();
  register_setFreq_cmd();
  register_recoverData_cmd();
  register_setRTC_cmd();
}

/* 
  Implementation of setWifi command, relies on network.cpp
*/
static int setWifi_impl(int argc, char** argv) {
  int err = arg_parse(argc, argv, (void **) &setWifi_args);
  if (err != 0) {
      arg_print_errors(stderr, setWifi_args.end, argv[0]);
      return 1;
  }
  if ((setWifi_args.ap->count + setWifi_args.sta->count) != 1) {
    printf("Please specify either the 'ap' flag or 'station' flag.\n");
    return 1;
  }
  bool isAP = true;

  if(setWifi_args.ap->count) {
    if (strlen(setWifi_args.password->sval[0]) < 8) {
      printf("Password too short!\n");
      return 1;
    }
    printf("Setting up AP with SSID '%s' and password '%s'\n",
          setWifi_args.ssid->sval[0], setWifi_args.password->sval[0]);
  }
  else {
    isAP = false;
    printf("Enabling station mode and connecting to SSID '%s'\n",
          setWifi_args.ssid->sval[0]);
  }

  bool connected = start_wifi_cmd(setWifi_args.ssid->sval[0],
                                  setWifi_args.password->sval[0],
                                  isAP);
  if(!connected) {
    printf("Failed to start wifi mode\n");
    return 1;
  }
  printf("Configured wifi\n");
  return 0;
}

void register_setWifi_cmd() {
  esp_console_cmd_t setWifi_cmd {
    .command = "setWifi",
    .help = "Configure WiFi access point",
    .hint = NULL,
    .func = &setWifi_impl,
    .argtable = &setWifi_args
  };

  esp_console_cmd_register(&setWifi_cmd);
}

/*
  Implementation of setDB command.
*/
static int setDB_impl(int argc, char** argv) {
  int err = arg_parse(argc, argv, (void **) &setDB_args);
  if (err != 0) {
      arg_print_errors(stderr, setDB_args.end, argv[0]);
      return 1;
  }
  printf("Connecting to DB with ip '%s' and port %d\n",
           setDB_args.ip->sval[0], setDB_args.port->ival[0]);
           
  start_http_server(setDB_args.ip->sval[0], setDB_args.port->ival[0]);
  return 0;
}

void register_setDB_cmd() {
  setDB_args.ip = arg_str1(NULL, NULL, "<ip>", "IP address for InfluxDB");
  setDB_args.port = arg_int1(NULL, NULL, "<port>", "Port used for InfluxDB");
  setDB_args.end = arg_end(2);

  esp_console_cmd_t setDB_cmd {
    .command = "setDB",
    .help = "Configure InfluxDB connection",
    .hint = NULL,
    .func = &setDB_impl,
    .argtable = &setDB_args
  };

  esp_console_cmd_register(&setDB_cmd);
}

/*
  Implementation of setfreq command. Deletes old clock and starts new one.
*/
static int setFreq_impl(int argc, char** argv) {
  int err = arg_parse(argc, argv, (void **) &setFreq_args);
  if (err != 0) {
      arg_print_errors(stderr, setFreq_args.end, argv[0]);
      return 1;
  }
  printf("Changing data storage frequency to %d\n",
           setFreq_args.time->ival[0]);
           
  if(xTimerChangePeriod(threadTimer, pdMS_TO_TICKS(setFreq_args.time->ival[0] * 1000), 300) == pdFAIL) {
    printf("Failed to change timer period\n");
    return 1;
  }
  return 0;
}

void register_setFreq_cmd() {
  setFreq_args.time = arg_int1(NULL, NULL, "<time>", "Data storage frequency, in seconds");
  setFreq_args.end = arg_end(2);

  esp_console_cmd_t setFreq_cmd {
    .command = "setfreq",
    .help = "Set data storage frequency",
    .hint = NULL,
    .func = &setFreq_impl,
    .argtable = &setFreq_args
  };

  esp_console_cmd_register(&setFreq_cmd);
}

/*
  Implementation of recoverData command. Retrieves all data in SD card stored between the two dates
*/
static int recoverData_impl(int argc, char** argv) {
  int err = arg_parse(argc, argv, (void **) &recoverData_args);
  if (err != 0) {
      arg_print_errors(stderr, recoverData_args.end, argv[0]);
      return 1;
  }
  struct tm time_start = {0};
  struct tm time_end = {0};
  printf("Sending data stored between %s and %s\n",
           recoverData_args.timestamp_start->sval[0],
           recoverData_args.timestamp_end->sval[0]);
  if(strptime(recoverData_args.timestamp_start->sval[0], "%Y/%m/%d %H:%M:%S", &time_start) == NULL) {
    printf("Malformed start timestamp, could not convert to UNIX timestamp\n");
    return 1;
  }
  if(strptime(recoverData_args.timestamp_end->sval[0], "%Y/%m/%d %H:%M:%S", &time_end) == NULL) {
    printf("Malformed end timestamp, could not convert to UNIX timestamp\n");
    return 1;
  }
  time_start.tm_isdst = _daylight;
  time_end.tm_isdst = _daylight;
  readDataAndQueue(mktime(&time_start), mktime(&time_end));

  return 0;
}

void register_recoverData_cmd() {
  recoverData_args.timestamp_start = arg_str1(NULL, NULL, "<time_start>", "Beginning timestamp, in YYYY/MM/DD HH:MM:SS format");
  recoverData_args.timestamp_end = arg_str1(NULL, NULL, "<time_end>", "Ending timestamp, in YYYY/MM/DD HH:MM:SS format");
  recoverData_args.end = arg_end(2);

  esp_console_cmd_t recoverData_cmd {
    .command = "recoverData",
    .help = "Send all data stored in SD card within the timestamp range to local DB",
    .hint = NULL,
    .func = &recoverData_impl,
    .argtable = &recoverData_args
  };

  esp_console_cmd_register(&recoverData_cmd);
}

/*
  Implementation of setRTC command. Sets the real time clock to the indicated time
*/
static int setRTC_impl(int argc, char** argv) {
  int err = arg_parse(argc, argv, (void **) &setRTC_args);
  if (err != 0) {
      arg_print_errors(stderr, setRTC_args.end, argv[0]);
      return 1;
  }
  struct tm time = {0};
  printf("Setting date to %s\n",
           setRTC_args.timestamp->sval[0]);
  if(strptime(setRTC_args.timestamp->sval[0], "%Y/%m/%d %H:%M:%S", &time) == NULL) {
    printf("Malformed timestamp, could not convert to UNIX timestamp\n");
    return 1;
  }
  time.tm_isdst = _daylight;
  setRTC(time);
  return 0;
}

void register_setRTC_cmd() {
  setRTC_args.timestamp = arg_str1(NULL, NULL, "<time>", "Timestamp, in YYYY/MM/DD HH:MM:SS format");
  setRTC_args.end = arg_end(2);

  esp_console_cmd_t setRTC_cmd {
    .command = "setRTC",
    .help = "Sets the real time clock",
    .hint = NULL,
    .func = &setRTC_impl,
    .argtable = &setRTC_args
  };

  esp_console_cmd_register(&setRTC_cmd);
}