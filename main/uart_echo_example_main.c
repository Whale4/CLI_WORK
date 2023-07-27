/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (1)
#define ECHO_TEST_RXD (3)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (0)
#define ECHO_UART_BAUD_RATE     (115200)
#define ECHO_TASK_STACK_SIZE    (1024*5)

static const char *TAG = "UART TEST";

#define BUF_SIZE (1024)

typedef enum {
    CLI_SERVICE_RESET,                
    CLI_SERVICE_READ,           
    CLI_SERVICE_WRITE,        
    CLI_SERVICE_CONTROL,               
    CLI_SERVICE_ROUTINE,
    CLI_SERVICE_NULL 
} CLI_cmd_service;

char *const CLI_cmd_service_names[] = {
    "RESET",                
    "READ",
    "WRITE",
    "CONTROL",
    "ROUTINE"
};

typedef enum {
    CLI_SERVICE_OPT1_HARD,                
    CLI_SERVICE_OPT1_SOFT,
    CLI_SERVICE_OPT1_START,
    CLI_SERVICE_OPT1_RESULT,
    CLI_SERVICE_OPT1_NULL
} CLI_cmd_service_opt1;

char *const CLI_cmd_service_opt1_names[] = {
    "HARD",                
    "SOFT",
    "START",
    "RESULT"
}; 

typedef enum {
    CLI_SERVICE_OPT2_NO_RESP,                
    CLI_SERVICE_OPT2_RESP,
    CLI_SERVICE_OPT2_SET,
    CLI_SERVICE_OPT2_RETURN,
    CLI_SERVICE_OPT2_NULL
} CLI_cmd_service_opt2;

char *const CLI_cmd_service_opt2_names[] = {
    "NO_RESP",                
    "RESP",
    "SET",
    "RETURN"
};

typedef enum {
    CLI_RESP_STATUS_NOK,                
    CLI_RESP_STATUS_OK
} CLI_resp_status;

char *const CLI_resp_status_names[] = {
    "NOK",
    "OK"
}; 

typedef enum {
    CLI_PARAM_RADIO_MUTE_DELAY,
    CLI_PARAM_RADIO_UNMUTE_DELAY,
    CLI_PARAM_CALL_AUTO_ANSWER_TIME,
    CLI_PARAM_POST_TEST_REGISTRATION_TIME,
    CLI_PARAM_TEST_MODE_END_DISTANCE,
    CLI_PARAM_GARAGE_MODE_END_DISTANCE,
    CLI_PARAM_ECALL_TEST_NUMBER,
    CLI_PARAM_GARAGE_MODE_PIN,
    CLI_PARAM_INT_MEM_TRANSMIT_INTERVAL,
    CLI_PARAM_INT_MEM_TRANSMIT_ATTEMPTS,
    CLI_PARAM_CRASH_SIGNAL_INTERNAL,
    CLI_PARAM_CRASH_SIGNAL_EXTERNAL,
    CLI_PARAM_ASI15_TRESHOLD,
    CLI_PARAM_ECALL_MODE_PIN,
    CLI_PARAM_SOS_BUTTON_TIME,
    CLI_PARAM_CCFT,
    CLI_PARAM_MSD_MAX_TRANSMISSION_TIME,
    CLI_PARAM_NAD_DEREGISTRATION_TIME,
    CLI_PARAM_ECALL_NO_AUTOMATIC_TRIGGERING,
    CLI_PARAM_ECALL_DIAL_DURATION,
    CLI_PARAM_ECALL_AUTO_DIAL_ATTEMPTS,
    CLI_PARAM_ECALL_MANUAL_DIAL_ATTEMPTS,
    CLI_PARAM_ECALL_MANUAL_CAN_CANCEL,
    CLI_PARAM_ECALL_SMS_FALLBACK_NUMBER,
    CLI_PARAM_TEST_REGISTRATION_PERIOD,
    CLI_PARAM_IGNITION_OFF_FOLLOW_UP_TIME1,
    CLI_PARAM_IGNITION_OFF_FOLLOW_UP_TIME2,
    CLI_PARAM_CRASH_RECORD_TIME,
    CLI_PARAM_CRASH_RECORD_RESOLUTION,
    CLI_PARAM_CRASH_PRE_RECORD_TIME,
    CLI_PARAM_CRASH_PRE_RECORD_RESOLUTION,
    CLI_PARAM_GNSS_POWER_OFF_TIME,
    CLI_PARAM_GNSS_DATA_RATE,
    CLI_PARAM_GNSS_MIN_ELEVATION,
    CLI_PARAM_VIN,
    CLI_PARAM_VEHICLE_TYPE,
    CLI_PARAM_VEHICLE_PROPULSION_STORAGE_TYPE,
    CLI_PARAM_ICCID_ECALL,
    CLI_PARAM_IMEI_ECALL,
    CLI_PARAM_ESN,
    CLI_PARAM_GNSS_FIX,
    CLI_PARAM_GSM_REG_STATUS,
    CLI_PARAM_SOS_BUTTON_INPUT_STATUS,
    CLI_PARAM_FUNC_BUTTON_INPUT_STATUS,
    CLI_PARAM_IGN_INPUT_STATUS,
    CLI_PARAM_ECALL_MODE_PIN_OUPUT_STATUS,
    CLI_PARAM_GARAGE_MODE_PIN_OUTPUT_STATUS,
    CLI_PARAM_SOS_INDICATOR_OUTPUT_STATUS,
    CLI_PARAM_ECALL_EMERGENCY_DEBUG,
    CLI_PARAM_ECALL_EMERGENCY_DEBUG_NUMBER,
    CLI_PARAM_DEBUG_OUTPUT_ECALL,
    CLI_PARAM_ECALL_ON,
    CLI_PARAM_ECALL_SPEAKER_VOLUME,
    CLI_PARAM_ECALL_MICROPHONE_LEVEL,
    CLI_PARAM_FOTA_STATUS,
    CLI_PARAM_CRASH_TURNOVER_THRESHOLD,
    CLI_PARAM_CRASH_TURNOVER_DURATION,
    CLI_PARAM_MOUNT_TYPE,
    CLI_PARAM_CRASH_INPUT_PIN,
    CLI_PARAM_NUMBER_TROUBLE_CODE,
    CLI_PARAM_SIGNAL_STRENGTH,
    CLI_PARAM_SUPPLY_VOLTAGE,
    CLI_PARAM_BOOT_VERSION,
    CLI_PARAM_CLAIBRATION_NUMBER,
    CLI_PARAM_MODULE_NUMBER,
    CLI_PARAM_STRATEGY_NUMBER,
    CLI_PARAM_HARDWARE_NUMBER,
    CLI_PARAM_MODEM_SOFTWARE_NUMBER,
    CLI_PARAM_IMU_SOFTWARE_NUMBER,
    CLI_PARAM_MSD_NUMBER,
    CLI_PARAM_LAST_MSD,
    CLI_PARAM_CURRENT_GNSS_LAT,
    CLI_PARAM_CURRENT_GNSS_LONG,
    CLI_PARAM_CURRENT_GNSS_TIME,
    CLI_PARAM_NULL
} CLI_cmd_param;

const char *const CLI_cmd_param_names[] = {
	"RADIO_MUTE_DELAY",
    "RADIO_UNMUTE_DELAY",
    "CALL_AUTO_ANSWER_TIME",
    "POST_TEST_REGISTRATION_TIME",
    "TEST_MODE_END_DISTANCE",
    "GARAGE_MODE_END_DISTANCE",
    "ECALL_TEST_NUMBER",
    "GARAGE_MODE_PIN",
    "INT_MEM_TRANSMIT_INTERVAL",
    "INT_MEM_TRANSMIT_ATTEMPTS",
    "CRASH_SIGNAL_INTERNAL",
    "CRASH_SIGNAL_EXTERNAL",
    "ASI15_TRESHOLD",
    "ECALL_MODE_PIN",
    "SOS_BUTTON_TIME",
    "CCFT",
    "MSD_MAX_TRANSMISSION_TIME",
    "NAD_DEREGISTRATION_TIME",
    "ECALL_NO_AUTOMATIC_TRIGGERING",
    "ECALL_DIAL_DURATION",
    "ECALL_AUTO_DIAL_ATTEMPTS",
    "ECALL_MANUAL_DIAL_ATTEMPTS",
    "ECALL_MANUAL_CAN_CANCEL",
    "ECALL_SMS_FALLBACK_NUMBER",
    "TEST_REGISTRATION_PERIOD",
    "IGNITION_OFF_FOLLOW_UP_TIME1",
    "IGNITION_OFF_FOLLOW_UP_TIME2",
    "CRASH_RECORD_TIME",
    "CRASH_RECORD_RESOLUTION",
    "CRASH_PRE_RECORD_TIME",
    "CRASH_PRE_RECORD_RESOLUTION",
    "GNSS_POWER_OFF_TIME",
    "GNSS_DATA_RATE",
    "GNSS_MIN_ELEVATION",
    "VIN",
    "VEHICLE_TYPE",
    "VEHICLE_PROPULSION_STORAGE_TYPE",
    "ICCID_ECALL",
    "IMEI_ECALL",
    "ESN",
    "GNSS_FIX",
    "GSM_REG_STATUS",
    "SOS_BUTTON_INPUT_STATUS",
    "FUNC_BUTTON_INPUT_STATUS",
    "IGN_INPUT_STATUS",
    "ECALL_MODE_PIN_OUPUT_STATUS",
    "GARAGE_MODE_PIN_OUTPUT_STATUS",
    "SOS_INDICATOR_OUTPUT_STATUS",
    "ECALL_EMERGENCY_DEBUG",
    "ECALL_EMERGENCY_DEBUG_NUMBER",
    "DEBUG_OUTPUT_ECALL",
    "ECALL_ON",
    "ECALL_SPEAKER_VOLUME",
    "ECALL_MICROPHONE_LEVEL",
    "FOTA_STATUS",
    "CRASH_TURNOVER_THRESHOLD",
    "CRASH_TURNOVER_DURATION",
    "MOUNT_TYPE",
    "CRASH_INPUT_PIN",
    "NUMBER_TROUBLE_CODE",
    "SIGNAL_STRENGTH",
    "SUPPLY_VOLTAGE",
    "BOOT_VERSION",
    "CLAIBRATION_NUMBER",
    "MODULE_NUMBER",
    "STRATEGY_NUMBER",
    "HARDWARE_NUMBER",
    "MODEM_SOFTWARE_NUMBER",
    "IMU_SOFTWARE_NUMBER",
    "MSD_NUMBER",
    "LAST_MSD",
    "CURRENT_GNSS_LAT",
    "CURRENT_GNSS_LONG",
    "CURRENT_GNSS_TIME"
};

typedef enum {
    CLI_RESP_ERR_COMMAND_INCORRECT,
    CLI_RESP_ERR_SERVICE_INCORRECT,
    CLI_RESP_ERR_VALUE_OUT_OF_RANGE,
    CLI_RESP_ERR_WRITE_NOT_PERMIT,
    CLI_RESP_ERR_COMMON_ERROR,
    CLI_RESP_ERR_INCORRECT_VALUE_TYPE,
} CLI_resp_err;

static const char *const CLI_resp_err_names[] = {
    "COMMAND_INCORRECT",
    "SERVICE_INCORRECT",
    "VALUE_OUT_OF_RANGE",
    "WRITE_NOT_PERMIT",
    "COMMON_ERROR",
    "INCORRECT_VALUE_TYPE"
}; 

typedef struct {
	CLI_cmd_service service;
    CLI_cmd_service_opt1 service_opt1;
    CLI_cmd_service_opt2 service_opt2;
    CLI_cmd_param cmd_param;
    int int_Param_value;
    char* str_Param_value;
    float f_Param_value;
}CLI_cmd_t;

typedef struct {
	CLI_cmd_service service;
    CLI_cmd_service_opt1 service_opt1;
    CLI_cmd_service_opt2 service_opt2;
    CLI_cmd_param cmd_param;
    CLI_resp_status resp_status;
    CLI_resp_err resp_err;
    int int_Param_value;
    char* str_Param_value;
    float f_Param_value;
}CLI_cmd_response_t;

QueueHandle_t CLI_cmd_queue = NULL;
QueueHandle_t CLI_cmd_response_queue = NULL;


void CLI_cmd_listener_task(){
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    CLI_cmd_t cli_cmd = {CLI_SERVICE_NULL,CLI_SERVICE_OPT1_NULL,CLI_SERVICE_OPT2_NULL,CLI_PARAM_NULL};
    CLI_cmd_response_t cli_cmd_response = {CLI_SERVICE_NULL,CLI_SERVICE_OPT1_NULL,CLI_SERVICE_OPT2_NULL,CLI_PARAM_NULL};

    int service_number = sizeof(CLI_cmd_service_names)/sizeof(CLI_cmd_service_names[0]);
    int service_opt1_number = sizeof(CLI_cmd_service_opt1_names)/sizeof(CLI_cmd_service_opt1_names[0]);
    int service_opt2_number = sizeof(CLI_cmd_service_opt2_names)/sizeof(CLI_cmd_service_opt2_names[0]);
    int param_number = sizeof(CLI_cmd_param_names)/sizeof(CLI_cmd_param_names[0]);

    bool BAD_INPUT = false;

    for(;;){
        BAD_INPUT= false;
        cli_cmd.service = CLI_SERVICE_NULL;
        cli_cmd.service_opt1 = CLI_SERVICE_OPT1_NULL;
        cli_cmd.service_opt2 = CLI_SERVICE_OPT2_NULL;
        cli_cmd.cmd_param = CLI_PARAM_NULL;

        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        if (len) {
            data[len] = '\0';
            if(data[len-1]!=0x0A){
                BAD_INPUT = true;
                cli_cmd_response.service = CLI_SERVICE_NULL;
                cli_cmd_response.resp_err = CLI_RESP_ERR_COMMAND_INCORRECT;
            } else {
                
                int i,j,k,m;
                for(i=0; i<service_number;i++){
                    if (strstr((char *) data, CLI_cmd_service_names[i]) != NULL) {
                        cli_cmd.service = i;
                        switch (cli_cmd.service){
                            case CLI_SERVICE_RESET:
                                for(j=0; i<service_opt1_number;i++){
                                    if (strstr((char *) data, CLI_cmd_service_opt1_names[j]) != NULL) {
                                        cli_cmd.service_opt1 = j;
                                        for(k=0; i<service_opt2_number;i++){
                                            if (strstr((char *) data, CLI_cmd_service_opt2_names[k]) != NULL) {
                                                cli_cmd.service_opt2 = k;
                                                break;
                                            }
                                        }
                                    }
                                }
                                BAD_INPUT = true;
                                cli_cmd_response.service = CLI_SERVICE_NULL;
                                cli_cmd_response.resp_err = CLI_RESP_ERR_SERVICE_INCORRECT; 
                                goto exit;
                                break;
                            case CLI_SERVICE_READ:
                                break;
                            case CLI_SERVICE_WRITE:
                                break;
                            case CLI_SERVICE_CONTROL:
                                break;
                            case CLI_SERVICE_ROUTINE:
                                break;
                            default:
                                break;
                            }
                        goto exit;
                    }

                }
                BAD_INPUT = true;
                cli_cmd_response.service = CLI_SERVICE_NULL;
                cli_cmd_response.resp_err = CLI_RESP_ERR_SERVICE_INCORRECT;   
            }

            exit:
                if(!BAD_INPUT){
                    xQueueSend(CLI_cmd_queue, &cli_cmd, portMAX_DELAY);
                } else {
                    xQueueSend(CLI_cmd_response_queue, &cli_cmd_response, portMAX_DELAY);
                }
        }
    }
}    

void CLI_cmd_execute_task(){
    CLI_cmd_t cli_cmd;

    for(;;){
        xQueueReceive(CLI_cmd_queue, &cli_cmd, portMAX_DELAY);
        switch(cli_cmd.service){
            case CLI_SERVICE_READ:
                uart_write_bytes(ECHO_UART_PORT_NUM, CLI_cmd_service_names[cli_cmd.service], strlen(CLI_cmd_service_names[cli_cmd.service]));
                break;
            default:
                uart_write_bytes(ECHO_UART_PORT_NUM, CLI_cmd_service_names[CLI_SERVICE_RESET], strlen(CLI_cmd_service_names[CLI_SERVICE_RESET]));
                break;
        }		
    }
}


void CLI_cmd_response_task(){
    CLI_cmd_response_t cli_cmd_response;
    
    for(;;){
        xQueueReceive(CLI_cmd_response_queue, &cli_cmd_response, portMAX_DELAY);
        switch(cli_cmd_response.service){
            case CLI_SERVICE_READ:
                break;
            default:
                uart_write_bytes(ECHO_UART_PORT_NUM, CLI_resp_err_names[cli_cmd_response.resp_err], strlen(CLI_resp_err_names[cli_cmd_response.resp_err]));
                break;
        }	
    }
}

void CLI_init(){

    

    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    if(!CLI_cmd_queue)
		CLI_cmd_queue = xQueueCreate(1, sizeof(CLI_cmd_t));
    if(!CLI_cmd_response_queue)
		CLI_cmd_response_queue = xQueueCreate(1, sizeof(CLI_cmd_response_t));

    if(CLI_cmd_queue&&CLI_cmd_response_queue){
        xTaskCreate(CLI_cmd_listener_task, "CLI_cmd_listener_task", 1024*3, NULL, 5, NULL);
        xTaskCreate(CLI_cmd_execute_task, "CLI_cmd_execute_task", 1024*2, NULL, 5, NULL);
        xTaskCreate(CLI_cmd_response_task, "CLI_cmd_response_task", 1024*2, NULL, 5, NULL);
    }
}

void app_main(void)
{
    printf("size=%d\r\n", (sizeof(CLI_cmd_service_names) / sizeof(CLI_cmd_service_names[0])));
    vTaskDelay(1000/portTICK_PERIOD_MS);
    CLI_init();
}
