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

#define BUF_SIZE (256)

static const char *const PIN_enum_names[] = {             
    "NONE",
    "PIN 1",
    "PIN 2",
    "PIN 3",
    "PIN 4",
    "PIN 5",
    "PIN 6",
    "PIN 7",
    "PIN 8"
};

static const char *const BOOL_names[] = {             
    "FALSE",
    "TRUE"
};

/*
static const char *const GNSS_enum_names[] = {             
    "NO_FIX",
    "FIX_2D",
    "FIX_3D"
};

static const char *const GSM_enum_names[] = {             
    "NO_REG_NO_SEARCH",
    "REG_HOME",
    "SEARCH",
    "DENIED",
    "REG_ROAMING"
};

static const char *const STATUS_enum_names[] = {             
    "ACTIVE",
    "INACTIVE"
};

static const char *const STATES_enum_names[] = {             
    "ENABLED",
    "DISABLED"
};

static const char *const FOTA_enum_names[] = {             
    "IDLE",
    "DOWLOADING",
    "WAITING_IGN_OFF",
    "FLASHING",
    "UPDATE_OK",
    "UPDATE_ERROR",
    "DOWNLOAD_CANCELLED"
};

static const char *const MOUNT_enum_names[] = {             
    "NATIVE",
    "ADDITIONAL"
};
*/
static const char *const CLI_cmd_service_names[] = {             
    "READ",
    "RESET",
    "WRITE",
    "CONTROL",
    "ROUTINE"
};

static const char *const CLI_cmd_service_opt1_names[] = {
    "HARD",                
    "SOFT",
    "START",
    "RESULT"
}; 

static const char *const CLI_cmd_service_opt2_names[] = {
    "NO_RESP",                
    "RESP",
    "SET",
    "RETURN"
};

char *const CLI_resp_status_names[] = {
    "NOK",
    "OK"
}; 

static const char *const CLI_cmd_param_names[] = {
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
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_FLOAT,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_BOOL
} CLI_param_type;

int CLI_param_value_type[] = {
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_BOOL,
    CLI_PARAM_TYPE_BOOL,
    CLI_PARAM_TYPE_FLOAT,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_BOOL,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_BOOL,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_ENUM,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_INT,
    CLI_PARAM_TYPE_STRING,
    CLI_PARAM_TYPE_FLOAT,
    CLI_PARAM_TYPE_FLOAT,
    CLI_PARAM_TYPE_INT
};

static const char *const CLI_resp_err_names[] = {
    "UNKNOWN_ERROR",
    "COMMAND_INCORRECT",
    "SERVICE_INCORRECT",
    "VALUE_OUT_OF_RANGE",
    "WRITE_NOT_PERMIT",
    "COMMON_ERROR",
    "INCORRECT_VALUE_TYPE",
    "RESET_CMD_INCORRECT",
    "TIMEOUT",
    "PARAMETER_NOT_EXIST",
    "VALUE_FORMAT_NOT_VALID"
}; 

typedef enum {
    CLI_SERVICE_READ,
    CLI_SERVICE_RESET,                          
    CLI_SERVICE_WRITE,        
    CLI_SERVICE_CONTROL,               
    CLI_SERVICE_ROUTINE,
    CLI_SERVICE_NULL 
} CLI_cmd_service;

typedef enum {
    CLI_SERVICE_OPT1_HARD,                
    CLI_SERVICE_OPT1_SOFT,
    CLI_SERVICE_OPT1_START,
    CLI_SERVICE_OPT1_RESULT,
    CLI_SERVICE_OPT1_NULL
} CLI_cmd_service_opt1;

typedef enum {
    CLI_SERVICE_OPT2_NO_RESP,                
    CLI_SERVICE_OPT2_RESP,
    CLI_SERVICE_OPT2_SET,
    CLI_SERVICE_OPT2_RETURN,
    CLI_SERVICE_OPT2_NULL
} CLI_cmd_service_opt2;

typedef enum {
    CLI_RESP_STATUS_NOK,                
    CLI_RESP_STATUS_OK
} CLI_resp_status;

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
    CLI_PARAM_NULL,
} CLI_cmd_param;

typedef enum {
    CLI_RESP_ERR_UNKNOWN_ERROR,
    CLI_RESP_ERR_COMMAND_INCORRECT,
    CLI_RESP_ERR_SERVICE_INCORRECT,
    CLI_RESP_ERR_VALUE_OUT_OF_RANGE,
    CLI_RESP_ERR_WRITE_NOT_PERMIT,
    CLI_RESP_ERR_COMMON_ERROR,
    CLI_RESP_ERR_INCORRECT_VALUE_TYPE,
    CLI_RESP_ERR_RESET_CMD_INCORRECT,
    CLI_RESP_ERR_TIMEOUT,
    CLI_RESP_ERR_PARAMETER_NOT_EXIST,
    CLI_RESP_ERR_VALUE_FORMAT_NOT_VALID,
} CLI_resp_err;

typedef struct {
	CLI_cmd_service service;
    CLI_cmd_service_opt1 service_opt1;
    CLI_cmd_service_opt2 service_opt2;
    CLI_cmd_param cmd_param;
    CLI_param_type type;
    int int_Param_value;
    char* str_Param_value;
    char string_holder[20];
    float f_Param_value;
}CLI_cmd_t;

typedef struct {
	CLI_cmd_service service;
    CLI_cmd_service_opt1 service_opt1;
    CLI_cmd_service_opt2 service_opt2;
    CLI_cmd_param cmd_param;
    CLI_resp_status resp_status;
    CLI_resp_err resp_err;
    CLI_param_type type; 
    int int_Param_value;
    char* str_Param_value;
    float f_Param_value;
}CLI_cmd_response_t;

QueueHandle_t CLI_cmd_queue = NULL;
QueueHandle_t CLI_cmd_response_queue = NULL;
int service_number;
int service_opt1_number;
int service_opt2_number;
int param_number;

void cli_service_reset(CLI_cmd_service_opt1 reset_method, CLI_cmd_service_opt2 optional_params);
void cli_service_read(CLI_cmd_param cmd_param, CLI_param_type type);
void cli_service_write(CLI_cmd_t * CLI_cmd);
int CLI_get_value(char *data, CLI_cmd_t * CLI_cmd);


//TESTING VALUES------------------------
float f_CLI_PARAM_ASI15_TRESHOLD = 0.7f;
char* test_string = "112";
char test_string_holder[20] = {0};
bool test_bool_val = false;
//--------------------------------------


void CLI_cmd_listener_task(){
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    CLI_cmd_t cli_cmd = {CLI_SERVICE_NULL,CLI_SERVICE_OPT1_NULL,CLI_SERVICE_OPT2_NULL,CLI_PARAM_NULL};
    CLI_cmd_response_t cli_cmd_response = {CLI_SERVICE_NULL,CLI_SERVICE_OPT1_NULL,CLI_SERVICE_OPT2_NULL,CLI_PARAM_NULL};

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
                
                int i,j,k;
                
                //LOOKUP FOR SERVICES
                for(i=0; i<service_number;i++){
                    if (strstr((char *) data, CLI_cmd_service_names[i]) != NULL) {
                        cli_cmd.service = i;
                        switch (cli_cmd.service){
                            //--------------------
                            //RESET SERVICE
                            //--------------------
                            case CLI_SERVICE_RESET:
                                //method chosed here: HARD or SOFT
                                for(j=0; j<service_opt1_number;j++){
                                    if (strstr((char *) data, CLI_cmd_service_opt1_names[j]) != NULL) {
                                        cli_cmd.service_opt1 = j;
                                        //searching for additional parameters  here: RESP or NO_RESP  (if no additional params: apply RESP)
                                        for(k=0; k<service_opt2_number;k++){
                                            if (strstr((char *) data, CLI_cmd_service_opt2_names[k]) != NULL) {
                                                cli_cmd.service_opt2 = k;
                                                goto exit;
                                                break;
                                            }
                                        }
                                        cli_cmd.service_opt2 = CLI_SERVICE_OPT2_RESP;
                                        goto exit;
                                    }
                                }
                                //RESET SERVICE WITH INCORRECT PARAMETERS
                                BAD_INPUT = true;
                                cli_cmd_response.service = cli_cmd.service;
                                cli_cmd_response.resp_status = CLI_RESP_STATUS_NOK;
                                cli_cmd_response.resp_err = CLI_RESP_ERR_RESET_CMD_INCORRECT; 
                                goto exit;
                                break;

                            //--------------------
                            //READ SERVICE
                            //--------------------
                            case CLI_SERVICE_READ:
                                //FIND PARAMETER HERE
                                for(j=0; j<param_number;j++){
                                    if (strstr((char *) data, CLI_cmd_param_names[j]) != NULL) {
                                        cli_cmd.cmd_param = j;
                                        cli_cmd.type = CLI_param_value_type[j];
                                        goto exit;
                                    }            
                                }
                                //NO SUCH PARAMETER IN DB
                                BAD_INPUT = true;
                                cli_cmd_response.service = cli_cmd.service;
                                cli_cmd_response.resp_status = CLI_RESP_STATUS_NOK;
                                cli_cmd_response.resp_err = CLI_RESP_ERR_PARAMETER_NOT_EXIST; 
                                goto exit;    
                                break;

                            //--------------------
                            //WRITE SERVICE
                            //--------------------
                            case CLI_SERVICE_WRITE:
                                //FIND PARAMETER HERE
                                for(j=0; j<param_number;j++){
                                    if (strstr((char *) data, CLI_cmd_param_names[j]) != NULL) {
                                        cli_cmd.cmd_param = j;
                                        cli_cmd.type = CLI_param_value_type[j];
                                        
                                        //VALIDATE INPUT VALUES HERE. OK ? GO TO COMMAND EXECUTION : BAD RESPONSE
                                        if(0==CLI_get_value((char *) data, &cli_cmd)){
                                            goto exit;
                                        } else {
                                            BAD_INPUT = true;
                                            cli_cmd_response.service = cli_cmd.service;
                                            cli_cmd_response.resp_status = CLI_RESP_STATUS_NOK;
                                            cli_cmd_response.resp_err = CLI_RESP_ERR_VALUE_FORMAT_NOT_VALID;
                                        }
                                        goto exit;
                                    }            
                                }
                                //NO SUCH PARAMETER IN DB
                                BAD_INPUT = true;
                                cli_cmd_response.service = cli_cmd.service;
                                cli_cmd_response.resp_status = CLI_RESP_STATUS_NOK;
                                cli_cmd_response.resp_err = CLI_RESP_ERR_PARAMETER_NOT_EXIST; 
                                goto exit;
                                break;

                            //--------------------
                            //CONTROL SERVICE
                            //--------------------
                            case CLI_SERVICE_CONTROL:
                                break;

                            //--------------------
                            //ROUTINE SERVICE
                            //--------------------
                            case CLI_SERVICE_ROUTINE:
                                break;
                                
                            default:
                                break;
                            }
                        goto exit;
                    }

                }
                //NO SERVICES FOUND
                BAD_INPUT = true;
                cli_cmd_response.service = CLI_SERVICE_NULL;
                cli_cmd_response.resp_err = CLI_RESP_ERR_SERVICE_INCORRECT;   
            }

            exit:
                if(!BAD_INPUT){
                    //COMMAND IS CORRECT
                    xQueueSend(CLI_cmd_queue, &cli_cmd, portMAX_DELAY);
                } else {
                    //COMMAND IS NOT CORRECT
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
            //EXECUTING RESET COMMANDS
            case CLI_SERVICE_RESET:
                cli_service_reset(cli_cmd.service_opt1, cli_cmd.service_opt2);
                break;
            //EXECUTING READ COMMANDS
            case CLI_SERVICE_READ:
                cli_service_read(cli_cmd.cmd_param, cli_cmd.type);
                break;
            //EXECUTING WRITE COMMANDS
            case CLI_SERVICE_WRITE:
                cli_service_write(&cli_cmd);
                break;
            default:
                break;
        }		
    }
}

void CLI_cmd_response_task(){
    CLI_cmd_response_t cli_cmd_response;
    char *data = (char *) malloc(BUF_SIZE);

    for(;;){
         
        xQueueReceive(CLI_cmd_response_queue, &cli_cmd_response, portMAX_DELAY);
        switch(cli_cmd_response.service){
            case CLI_SERVICE_RESET:
                if(cli_cmd_response.resp_status==CLI_RESP_STATUS_OK){
                    sprintf(data,"%s OK",
                        CLI_cmd_service_names[cli_cmd_response.service]);
                } else {
                    sprintf(data,"%s NOK %s",
                        CLI_cmd_service_names[cli_cmd_response.service],
                        CLI_resp_err_names[cli_cmd_response.resp_err]);
                }
                break;
            case CLI_SERVICE_READ:
                if(cli_cmd_response.resp_status==CLI_RESP_STATUS_OK){
                    switch (cli_cmd_response.type){
                        case CLI_PARAM_TYPE_ENUM:
                            sprintf(data,"%s OK %s %s",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.str_Param_value);
                            break;
                        case CLI_PARAM_TYPE_INT:
                            sprintf(data,"%s OK %s %d",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.int_Param_value);
                            break;
                        case CLI_PARAM_TYPE_FLOAT:
                            sprintf(data,"%s OK %s %.1f",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.f_Param_value);
                            break;
                        case CLI_PARAM_TYPE_STRING:
                            sprintf(data,"%s OK %s %s",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.str_Param_value);
                            break;
                        case CLI_PARAM_TYPE_BOOL:
                            sprintf(data,"%s OK %s %s",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.str_Param_value);
                            break;
                        default:
                            break;
                        }
                } else {
                    sprintf(data,"%s NOK %s",
                        CLI_cmd_service_names[cli_cmd_response.service],
                        CLI_resp_err_names[cli_cmd_response.resp_err]);
                }
                break;
            case CLI_SERVICE_WRITE:
                if(cli_cmd_response.resp_status==CLI_RESP_STATUS_OK){
                    switch (cli_cmd_response.type){
                        case CLI_PARAM_TYPE_ENUM:
                            sprintf(data,"%s OK %s %s",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.str_Param_value);
                            break;
                        case CLI_PARAM_TYPE_INT:
                            sprintf(data,"%s OK %s %d",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.int_Param_value);
                            break;
                        case CLI_PARAM_TYPE_FLOAT:
                            sprintf(data,"%s OK %s %.1f",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.f_Param_value);
                            break;
                        case CLI_PARAM_TYPE_STRING:
                            sprintf(data,"%s OK %s %s",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.str_Param_value);
                            break;
                        case CLI_PARAM_TYPE_BOOL:
                            sprintf(data,"%s OK %s %s",
                                CLI_cmd_service_names[cli_cmd_response.service],
                                CLI_cmd_param_names[cli_cmd_response.cmd_param],
                                cli_cmd_response.str_Param_value);
                            break;
                        default:
                            break;
                        }
                } else {
                    sprintf(data,"%s NOK %s",
                        CLI_cmd_service_names[cli_cmd_response.service],
                        CLI_resp_err_names[cli_cmd_response.resp_err]);
                }
                break;
            default:
                sprintf(data,CLI_resp_err_names[cli_cmd_response.resp_err]);
                break;
        }

        uart_write_bytes(ECHO_UART_PORT_NUM, data, strlen(data));	
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

    service_number = sizeof(CLI_cmd_service_names)/sizeof(CLI_cmd_service_names[0]);
    service_opt1_number = sizeof(CLI_cmd_service_opt1_names)/sizeof(CLI_cmd_service_opt1_names[0]);
    service_opt2_number = sizeof(CLI_cmd_service_opt2_names)/sizeof(CLI_cmd_service_opt2_names[0]);
    param_number = sizeof(CLI_cmd_param_names)/sizeof(CLI_cmd_param_names[0]);
    
    if(CLI_cmd_queue&&CLI_cmd_response_queue){
        xTaskCreate(CLI_cmd_listener_task, "CLI_cmd_listener_task", 1024*3, NULL, 5, NULL);
        xTaskCreate(CLI_cmd_execute_task, "CLI_cmd_execute_task", 1024*2, NULL, 5, NULL);
        xTaskCreate(CLI_cmd_response_task, "CLI_cmd_response_task", 1024*2, NULL, 5, NULL);
    }
}

void app_main(void)
{
    CLI_init();
}

void cli_service_reset(CLI_cmd_service_opt1 reset_method, CLI_cmd_service_opt2 optional_params) {
    CLI_cmd_response_t cli_cmd_response;

    cli_cmd_response.service = CLI_SERVICE_RESET;
    cli_cmd_response.service_opt1 = reset_method;
    cli_cmd_response.service_opt2 = optional_params;
    cli_cmd_response.resp_status = CLI_RESP_STATUS_NOK;
    cli_cmd_response.resp_err = CLI_RESP_ERR_UNKNOWN_ERROR;

    //RESET COMMAND EXECUTE
    if(true){
        cli_cmd_response.resp_status = CLI_RESP_STATUS_OK;
    }
    
    xQueueSend(CLI_cmd_response_queue, &cli_cmd_response, portMAX_DELAY);
};

void cli_service_read(CLI_cmd_param cmd_param, CLI_param_type type) {
    CLI_cmd_response_t cli_cmd_response;

    cli_cmd_response.service = CLI_SERVICE_READ;
    cli_cmd_response.cmd_param = cmd_param;
    cli_cmd_response.type = type;

    switch (cmd_param){
        case CLI_PARAM_RADIO_MUTE_DELAY:
            cli_cmd_response.int_Param_value = 15;
            break;
        case CLI_PARAM_RADIO_UNMUTE_DELAY:
            break;
        case CLI_PARAM_CALL_AUTO_ANSWER_TIME:
            break;
        case CLI_PARAM_POST_TEST_REGISTRATION_TIME:
            break;
        case CLI_PARAM_TEST_MODE_END_DISTANCE:
            break;
        case CLI_PARAM_GARAGE_MODE_END_DISTANCE:
            break;
        case CLI_PARAM_ECALL_TEST_NUMBER:
            cli_cmd_response.str_Param_value = test_string_holder;
            break;
        case CLI_PARAM_GARAGE_MODE_PIN:
            cli_cmd_response.str_Param_value = PIN_enum_names[1];
            break;
        case CLI_PARAM_INT_MEM_TRANSMIT_INTERVAL:
            break;
        case CLI_PARAM_INT_MEM_TRANSMIT_ATTEMPTS:
            break;
        case CLI_PARAM_CRASH_SIGNAL_INTERNAL:
            int bool_val = test_bool_val?1:0;
            cli_cmd_response.str_Param_value = BOOL_names[bool_val];
            break;
        case CLI_PARAM_CRASH_SIGNAL_EXTERNAL:
            break;
        case CLI_PARAM_ASI15_TRESHOLD:
            cli_cmd_response.f_Param_value = f_CLI_PARAM_ASI15_TRESHOLD;
            break;
        case CLI_PARAM_ECALL_MODE_PIN:
            break;
        case CLI_PARAM_SOS_BUTTON_TIME:
            break;
        case CLI_PARAM_CCFT:
            break;
        case CLI_PARAM_MSD_MAX_TRANSMISSION_TIME:
            break;
        case CLI_PARAM_NAD_DEREGISTRATION_TIME:
            break;
        case CLI_PARAM_ECALL_NO_AUTOMATIC_TRIGGERING:
            break;
        case CLI_PARAM_ECALL_DIAL_DURATION:
            break;
        case CLI_PARAM_ECALL_AUTO_DIAL_ATTEMPTS:
            break;
        case CLI_PARAM_ECALL_MANUAL_DIAL_ATTEMPTS:
            break;
        case CLI_PARAM_ECALL_MANUAL_CAN_CANCEL:
            break;
        case CLI_PARAM_ECALL_SMS_FALLBACK_NUMBER:
            break;
        case CLI_PARAM_TEST_REGISTRATION_PERIOD:
            break;
        case CLI_PARAM_IGNITION_OFF_FOLLOW_UP_TIME1:
            break;
        case CLI_PARAM_IGNITION_OFF_FOLLOW_UP_TIME2:
            break;
        case CLI_PARAM_CRASH_RECORD_TIME:
            break;
        case CLI_PARAM_CRASH_RECORD_RESOLUTION:
            break;
        case CLI_PARAM_CRASH_PRE_RECORD_TIME:
            break;
        case CLI_PARAM_CRASH_PRE_RECORD_RESOLUTION:
            break;
        case CLI_PARAM_GNSS_POWER_OFF_TIME:
            break;
        case CLI_PARAM_GNSS_DATA_RATE:
            break;
        case CLI_PARAM_GNSS_MIN_ELEVATION:
            break;
        case CLI_PARAM_VIN:
            break;
        case CLI_PARAM_VEHICLE_TYPE:
            break;
        case CLI_PARAM_VEHICLE_PROPULSION_STORAGE_TYPE:
            break;
        case CLI_PARAM_ICCID_ECALL:
            break;
        case CLI_PARAM_IMEI_ECALL:
            break;
        case CLI_PARAM_ESN:
            break;
        case CLI_PARAM_GNSS_FIX:
            break;
        case CLI_PARAM_GSM_REG_STATUS:
            break;
        case CLI_PARAM_SOS_BUTTON_INPUT_STATUS:
            break;
        case CLI_PARAM_FUNC_BUTTON_INPUT_STATUS:
            break;
        case CLI_PARAM_IGN_INPUT_STATUS:
            break;
        case CLI_PARAM_ECALL_MODE_PIN_OUPUT_STATUS:
            break;
        case CLI_PARAM_GARAGE_MODE_PIN_OUTPUT_STATUS:
            break;
        case CLI_PARAM_SOS_INDICATOR_OUTPUT_STATUS:
            break;
        case CLI_PARAM_ECALL_EMERGENCY_DEBUG:
            break;
        case CLI_PARAM_ECALL_EMERGENCY_DEBUG_NUMBER:
            break;
        case CLI_PARAM_DEBUG_OUTPUT_ECALL:
            break;
        case CLI_PARAM_ECALL_ON:
            break;
        case CLI_PARAM_ECALL_SPEAKER_VOLUME:
            break;
        case CLI_PARAM_ECALL_MICROPHONE_LEVEL:
            break;
        case CLI_PARAM_FOTA_STATUS:
            break;
        case CLI_PARAM_CRASH_TURNOVER_THRESHOLD:
            break;
        case CLI_PARAM_CRASH_TURNOVER_DURATION:
            break;
        case CLI_PARAM_MOUNT_TYPE:
            break;
        case CLI_PARAM_CRASH_INPUT_PIN:
            break;
        case CLI_PARAM_NUMBER_TROUBLE_CODE:
            break;
        case CLI_PARAM_SIGNAL_STRENGTH:
            break;
        case CLI_PARAM_SUPPLY_VOLTAGE:
            break;
        case CLI_PARAM_BOOT_VERSION:
            break;
        case CLI_PARAM_CLAIBRATION_NUMBER:
            break;
        case CLI_PARAM_MODULE_NUMBER:
            break;
        case CLI_PARAM_STRATEGY_NUMBER:
            break;
        case CLI_PARAM_HARDWARE_NUMBER:
            break;
        case CLI_PARAM_MODEM_SOFTWARE_NUMBER:
            break;
        case CLI_PARAM_IMU_SOFTWARE_NUMBER:
            break;
        case CLI_PARAM_MSD_NUMBER:
            break;
        case CLI_PARAM_LAST_MSD:
            break;
        case CLI_PARAM_CURRENT_GNSS_LAT:
            break;
        case CLI_PARAM_CURRENT_GNSS_LONG:
            break;
        case CLI_PARAM_CURRENT_GNSS_TIME:
            break;
        default:
            break;
    }

    cli_cmd_response.resp_status = CLI_RESP_STATUS_OK;

    xQueueSend(CLI_cmd_response_queue, &cli_cmd_response, portMAX_DELAY);
};

int float_validation(const char *str, float* f_val)
{
    int len;
    float dummy = 0.0;
    if (sscanf(str, "%f %n", &dummy, &len) == 1 && len == (int)strlen(str)) {
        *f_val = dummy;
        return 0;
    }else{
        return 1;
    }
}

int CLI_get_value(char *data, CLI_cmd_t * CLI_cmd){
    //POINTER TO THE END OF THE PARAMATER NAME
    char *start_position = strstr((char *) data, (char* )CLI_cmd_param_names[CLI_cmd->cmd_param]) + strlen(CLI_cmd_param_names[CLI_cmd->cmd_param]);

    //REMOVING ANY WHITE SPACES BEFORE VALUE ITSELF
    for (int i =0; i<strlen(start_position);i++){
        if((int)start_position[i]==32){
            start_position += sizeof(char);
        }else{
            break;
        }
    }

    //VALUE PARSING
    switch(CLI_cmd->type){
        case CLI_PARAM_TYPE_ENUM:
            break;
        case CLI_PARAM_TYPE_INT:
            break;
        case CLI_PARAM_TYPE_FLOAT:
            //FLOAT VALIDATION
            if(0 == float_validation(start_position, &CLI_cmd->f_Param_value)){
                return 0;
            } else {
                return 1;
            }
            break;
        case CLI_PARAM_TYPE_STRING:
            //STRING VALIDATION
            if(sizeof(CLI_cmd->string_holder)/sizeof(char)<strlen(start_position)){
                return 1;
            } else {
                sprintf(CLI_cmd->string_holder,"%s",start_position);
                return 0;
            }
            break;
        case CLI_PARAM_TYPE_BOOL:
            for(int i=0; i<2;i++){
                if (strstr(start_position, BOOL_names[i]) != NULL) {    
                    CLI_cmd->int_Param_value = i;
                    return 0;
                }
            }
            return 1;
            break;
        default:
            break;
    }

    return 1;

}


void cli_service_write(CLI_cmd_t * CLI_cmd) {
    CLI_cmd_response_t cli_cmd_response;

    cli_cmd_response.service = CLI_SERVICE_WRITE;
    cli_cmd_response.cmd_param = CLI_cmd->cmd_param;
    cli_cmd_response.type = CLI_cmd->type;

    switch (CLI_cmd->cmd_param){
        case CLI_PARAM_RADIO_MUTE_DELAY:
            break;
        case CLI_PARAM_RADIO_UNMUTE_DELAY:
            break;
        case CLI_PARAM_CALL_AUTO_ANSWER_TIME:
            break;
        case CLI_PARAM_POST_TEST_REGISTRATION_TIME:
            break;
        case CLI_PARAM_TEST_MODE_END_DISTANCE:
            break;
        case CLI_PARAM_GARAGE_MODE_END_DISTANCE:
            break;
        case CLI_PARAM_ECALL_TEST_NUMBER:
            sprintf(test_string_holder,"%s",CLI_cmd->string_holder);
            cli_cmd_response.str_Param_value = test_string_holder;
            break;
        case CLI_PARAM_GARAGE_MODE_PIN:
            break;
        case CLI_PARAM_INT_MEM_TRANSMIT_INTERVAL:
            break;
        case CLI_PARAM_INT_MEM_TRANSMIT_ATTEMPTS:
            break;
        case CLI_PARAM_CRASH_SIGNAL_INTERNAL:
            test_bool_val = CLI_cmd->int_Param_value;
            cli_cmd_response.str_Param_value = BOOL_names[CLI_cmd->int_Param_value];
            break;
        case CLI_PARAM_CRASH_SIGNAL_EXTERNAL:
            break;
        case CLI_PARAM_ASI15_TRESHOLD:
            f_CLI_PARAM_ASI15_TRESHOLD = CLI_cmd->f_Param_value;
            cli_cmd_response.f_Param_value = f_CLI_PARAM_ASI15_TRESHOLD;
            break;
        case CLI_PARAM_ECALL_MODE_PIN:
            break;
        case CLI_PARAM_SOS_BUTTON_TIME:
            break;
        case CLI_PARAM_CCFT:
            break;
        case CLI_PARAM_MSD_MAX_TRANSMISSION_TIME:
            break;
        case CLI_PARAM_NAD_DEREGISTRATION_TIME:
            break;
        case CLI_PARAM_ECALL_NO_AUTOMATIC_TRIGGERING:
            break;
        case CLI_PARAM_ECALL_DIAL_DURATION:
            break;
        case CLI_PARAM_ECALL_AUTO_DIAL_ATTEMPTS:
            break;
        case CLI_PARAM_ECALL_MANUAL_DIAL_ATTEMPTS:
            break;
        case CLI_PARAM_ECALL_MANUAL_CAN_CANCEL:
            break;
        case CLI_PARAM_ECALL_SMS_FALLBACK_NUMBER:
            break;
        case CLI_PARAM_TEST_REGISTRATION_PERIOD:
            break;
        case CLI_PARAM_IGNITION_OFF_FOLLOW_UP_TIME1:
            break;
        case CLI_PARAM_IGNITION_OFF_FOLLOW_UP_TIME2:
            break;
        case CLI_PARAM_CRASH_RECORD_TIME:
            break;
        case CLI_PARAM_CRASH_RECORD_RESOLUTION:
            break;
        case CLI_PARAM_CRASH_PRE_RECORD_TIME:
            break;
        case CLI_PARAM_CRASH_PRE_RECORD_RESOLUTION:
            break;
        case CLI_PARAM_GNSS_POWER_OFF_TIME:
            break;
        case CLI_PARAM_GNSS_DATA_RATE:
            break;
        case CLI_PARAM_GNSS_MIN_ELEVATION:
            break;
        case CLI_PARAM_VIN:
            break;
        case CLI_PARAM_VEHICLE_TYPE:
            break;
        case CLI_PARAM_VEHICLE_PROPULSION_STORAGE_TYPE:
            break;
        case CLI_PARAM_ICCID_ECALL:
            break;
        case CLI_PARAM_IMEI_ECALL:
            break;
        case CLI_PARAM_ESN:
            break;
        case CLI_PARAM_GNSS_FIX:
            break;
        case CLI_PARAM_GSM_REG_STATUS:
            break;
        case CLI_PARAM_SOS_BUTTON_INPUT_STATUS:
            break;
        case CLI_PARAM_FUNC_BUTTON_INPUT_STATUS:
            break;
        case CLI_PARAM_IGN_INPUT_STATUS:
            break;
        case CLI_PARAM_ECALL_MODE_PIN_OUPUT_STATUS:
            break;
        case CLI_PARAM_GARAGE_MODE_PIN_OUTPUT_STATUS:
            break;
        case CLI_PARAM_SOS_INDICATOR_OUTPUT_STATUS:
            break;
        case CLI_PARAM_ECALL_EMERGENCY_DEBUG:
            break;
        case CLI_PARAM_ECALL_EMERGENCY_DEBUG_NUMBER:
            break;
        case CLI_PARAM_DEBUG_OUTPUT_ECALL:
            break;
        case CLI_PARAM_ECALL_ON:
            break;
        case CLI_PARAM_ECALL_SPEAKER_VOLUME:
            break;
        case CLI_PARAM_ECALL_MICROPHONE_LEVEL:
            break;
        case CLI_PARAM_FOTA_STATUS:
            break;
        case CLI_PARAM_CRASH_TURNOVER_THRESHOLD:
            break;
        case CLI_PARAM_CRASH_TURNOVER_DURATION:
            break;
        case CLI_PARAM_MOUNT_TYPE:
            break;
        case CLI_PARAM_CRASH_INPUT_PIN:
            break;
        case CLI_PARAM_NUMBER_TROUBLE_CODE:
            break;
        case CLI_PARAM_SIGNAL_STRENGTH:
            break;
        case CLI_PARAM_SUPPLY_VOLTAGE:
            break;
        case CLI_PARAM_BOOT_VERSION:
            break;
        case CLI_PARAM_CLAIBRATION_NUMBER:
            break;
        case CLI_PARAM_MODULE_NUMBER:
            break;
        case CLI_PARAM_STRATEGY_NUMBER:
            break;
        case CLI_PARAM_HARDWARE_NUMBER:
            break;
        case CLI_PARAM_MODEM_SOFTWARE_NUMBER:
            break;
        case CLI_PARAM_IMU_SOFTWARE_NUMBER:
            break;
        case CLI_PARAM_MSD_NUMBER:
            break;
        case CLI_PARAM_LAST_MSD:
            break;
        case CLI_PARAM_CURRENT_GNSS_LAT:
            break;
        case CLI_PARAM_CURRENT_GNSS_LONG:
            break;
        case CLI_PARAM_CURRENT_GNSS_TIME:
            break;
        default:
            break;
    }

    cli_cmd_response.resp_status = CLI_RESP_STATUS_OK;

    xQueueSend(CLI_cmd_response_queue, &cli_cmd_response, portMAX_DELAY);
};