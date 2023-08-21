/*============================================================================
  @file sns_hal_batch_fmq.cpp

  @brief
  Test the QTI Hybrid approach at the HAL layer.

  Copyright (c) 2013, 2015-2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ==========================================================================*/

#include <dlfcn.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include <hidl/MQDescriptor.h>
#include <unordered_map>
#include "convert.h"
#include <fmq/MessageQueue.h>
#include <thread>
#include "sensors_hw_module.h"
#include "android/hardware/sensors/2.0/ISensorsCallback.h"
#include "android/hardware/sensors/2.0/types.h"
#include <sensors_qti.h>

#define  MAX_RECEIVE_BUFFER_EVENT_COUNT 256

using android::hardware::sensors::V2_0::EventQueueFlagBits;
using android::hardware::sensors::V2_0::WakeLockQueueFlagBits;
using ::android::hardware::sensors::V1_0::Event;

using android::hardware::sensors::V1_0::SensorInfo;
using android::hardware::sensors::V2_0::ISensors;
using android::hardware::sensors::V2_0::ISensorsCallback;
using android::hardware::sensors::V1_0::implementation::convertToSensorEvent;
using android::hardware::sensors::V2_0::implementation::sensors_hw_module;

using Result = ::android::hardware::sensors::V1_0::Result;
typedef android::hardware::MessageQueue<Event, android::hardware::kSynchronizedReadWrite> EventMessageQueue;
typedef android::hardware::MessageQueue<uint32_t, android::hardware::kSynchronizedReadWrite> WakeLockQueue;

std::unique_ptr<EventMessageQueue> mEventQueue = nullptr;
std::unique_ptr<WakeLockQueue> mWakeLockQueue = nullptr;
android::hardware::EventFlag* mEventQueueFlag = nullptr;
android::hardware::EventFlag*  mWakeLockQueueFlag = nullptr;

template<typename EnumType>
constexpr typename std::underlying_type<EnumType>::type asBaseType(EnumType value) {
    return static_cast<typename std::underlying_type<EnumType>::type>(value);
}

std::array<Event, MAX_RECEIVE_BUFFER_EVENT_COUNT> mEventBuffer;

#define OPTSTRING "h:"
#define COMMAND_CHARS "adsfbe12345678"

#define DEFAULT_SENSOR_TYPE       1
#define DEFAULT_WAKEUP_FLAG      'd'
#define DEFAULT_SAMPLING_RATE_HZ  5
#define DEFAULT_REPORT_RATE_HZ    0
#define DEFAULT_OUTPUT_FILE       "/data/local/sns_hal_batch.out"
#define OUTPUT_TO_STDOUT          ""
#define DEFAULT_DURATION_SECS     10

#define NSEC_PER_MS         1000000
#define NSEC_PER_S          1000000000
#define HZ_TO_NSEC(hz)      (1000000000LL/(hz))

struct sensor_t *list = NULL;
int sensor_count;

typedef enum
  {
    ACTIVATE = 0,
    DEACTIVATE,
    BATCH,
    EXIT,
    NUM_COMMANDS
  } testapp_command_e;

typedef enum
  {
    INACTIVE = 0,
    STREAMING,
    BATCHING
  } sensor_operating_state_e;

static const char *handle_to_string( struct sensor_t const* list, int handle )
{
  const struct sensor_t *list_p = list;
  if(list_p!=nullptr){
      while( list_p->name ) {
        if( list_p->handle == handle ) {
          return (char*)list_p->name;
        }
        list_p++;
      }
    }
  return "no_name";
}

static const char *type_to_string( int type )
{
  return (type == SENSOR_TYPE_ACCELEROMETER) ? SENSOR_STRING_TYPE_ACCELEROMETER :
    (type == SENSOR_TYPE_GEOMAGNETIC_FIELD) ? SENSOR_STRING_TYPE_MAGNETIC_FIELD :
    (type == SENSOR_TYPE_ORIENTATION) ? SENSOR_STRING_TYPE_ORIENTATION :
    (type == SENSOR_TYPE_GYROSCOPE) ? SENSOR_STRING_TYPE_GYROSCOPE :
    (type == SENSOR_TYPE_LIGHT) ? SENSOR_STRING_TYPE_LIGHT :
    (type == SENSOR_TYPE_PRESSURE) ? SENSOR_STRING_TYPE_PRESSURE :
    (type == SENSOR_TYPE_TEMPERATURE) ? SENSOR_STRING_TYPE_TEMPERATURE :
    (type == SENSOR_TYPE_PROXIMITY) ? SENSOR_STRING_TYPE_PROXIMITY :
    (type == SENSOR_TYPE_GRAVITY) ? SENSOR_STRING_TYPE_GRAVITY :
    (type == SENSOR_TYPE_LINEAR_ACCELERATION) ? SENSOR_STRING_TYPE_LINEAR_ACCELERATION :
    (type == SENSOR_TYPE_ROTATION_VECTOR) ? SENSOR_STRING_TYPE_ROTATION_VECTOR :
    (type == SENSOR_TYPE_RELATIVE_HUMIDITY) ? SENSOR_STRING_TYPE_RELATIVE_HUMIDITY :
    (type == SENSOR_TYPE_AMBIENT_TEMPERATURE) ? SENSOR_STRING_TYPE_AMBIENT_TEMPERATURE :
    (type == SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED) ? SENSOR_STRING_TYPE_MAGNETIC_FIELD_UNCALIBRATED :
    (type == SENSOR_TYPE_GAME_ROTATION_VECTOR) ? SENSOR_STRING_TYPE_GAME_ROTATION_VECTOR :
    (type == SENSOR_TYPE_GYROSCOPE_UNCALIBRATED) ? SENSOR_STRING_TYPE_GYROSCOPE_UNCALIBRATED :
    (type == SENSOR_TYPE_SIGNIFICANT_MOTION) ? SENSOR_STRING_TYPE_SIGNIFICANT_MOTION :
    (type == SENSOR_TYPE_STEP_DETECTOR) ? SENSOR_STRING_TYPE_STEP_DETECTOR :
    (type == SENSOR_TYPE_STEP_COUNTER) ? SENSOR_STRING_TYPE_STEP_COUNTER :
    (type == SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR) ? SENSOR_STRING_TYPE_GEOMAGNETIC_ROTATION_VECTOR :
    (type == SENSOR_TYPE_HEART_RATE) ? SENSOR_STRING_TYPE_HEART_RATE :
    (type == SENSOR_TYPE_TILT_DETECTOR) ? SENSOR_STRING_TYPE_TILT_DETECTOR :
    (type == SENSOR_TYPE_WAKE_GESTURE) ? SENSOR_STRING_TYPE_WAKE_GESTURE :
    (type == SENSOR_TYPE_GLANCE_GESTURE) ? SENSOR_STRING_TYPE_GLANCE_GESTURE :
    (type == SENSOR_TYPE_PICK_UP_GESTURE) ? SENSOR_STRING_TYPE_PICK_UP_GESTURE :
    (type == SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED) ? SENSOR_STRING_TYPE_ACCELEROMETER_UNCALIBRATED :
    (type == SENSOR_TYPE_ADDITIONAL_INFO) ? SENSOR_STRING_TYPE_ADDITIONAL_INFO :
    (type == SENSOR_TYPE_META_DATA) ? "Meta Data (FLUSH_CMPLT)" :
    (type == QTI_SENSOR_TYPE_BASE) ? "com.qti.sensor.basic_gestures" :
    (type == QTI_SENSOR_TYPE_BASE + 1) ? "com.qti.sensor.tap" :
    (type == QTI_SENSOR_TYPE_BASE + 2) ? "com.qti.sensor.facing" :
    (type == QTI_SENSOR_TYPE_BASE + 3) ? "com.qti.sensor.tilt" :
    (type == QTI_SENSOR_TYPE_BASE + 4) ? "com.qti.sensor.fns" :
    (type == QTI_SENSOR_TYPE_BASE + 5) ? "com.qti.sensor.bte" :
    (type == QTI_SENSOR_TYPE_BASE + 6) ? "com.qti.sensor.amd" :
    (type == QTI_SENSOR_TYPE_BASE + 7) ? "com.qti.sensor.rmd" :
    (type == QTI_SENSOR_TYPE_BASE + 8) ? "com.qti.sensor.vmd" :
    (type == QTI_SENSOR_TYPE_BASE + 9) ? "com.qti.sensor.pedometer" :
    (type == QTI_SENSOR_TYPE_BASE + 10) ? "com.qti.sensor.pam" :
    (type == QTI_SENSOR_TYPE_BASE + 11) ? "com.qti.sensor.motion_accel" :
    (type == QTI_SENSOR_TYPE_BASE + 12) ? "com.qti.sensor.cmc" :
    (type == QTI_SENSOR_TYPE_BASE + 13) ? "com.qti.sensor.rgb" :
    (type == QTI_SENSOR_TYPE_BASE + 14) ? "com.qti.sensor.ir_gesture" :
    (type == QTI_SENSOR_TYPE_BASE + 15) ? "com.qti.sensor.sar" :
    (type == QTI_SENSOR_TYPE_BASE + 16) ? "com.qti.sensor.hall_effect" :
    (type == QTI_SENSOR_TYPE_BASE + 17) ? "com.qti.sensor.fast_amd" :
    (type == QTI_SENSOR_TYPE_BASE + 18) ? "com.qti.sensor.uv" :
    (type == QTI_SENSOR_TYPE_BASE + 19) ? "com.qti.sensor.thermopile" :
    (type == QTI_SENSOR_TYPE_BASE + 20) ? "com.qti.sensor.cct" :
    "Unknown";
}

static const char *sensor_to_stringType (struct sensor_t const* sensor)
{
  if(sensor!=nullptr)
    return *(sensor->stringType) != 0 ? sensor->stringType : type_to_string(sensor->type);
  else
    return "Unknown";
}


// Print out the usage statement in interactive mode
void print_usage_msg( struct sensor_t const *list )
{
  int count = 0;
  struct sensor_t const *list_p = list;

  fprintf(stderr,"Sensors HAL TEST APP, version 1\n");
  fprintf(stderr,"Usage:\n");
  fprintf(stderr,"\tChoose a sensor to interact with by inputting the sensor's type and whether or not is the wakeup version as shown in the 'Sensors list'\n");
  fprintf(stderr,"\tNext, choose a command, by inputting one of the following characters:\n");
  fprintf(stderr,"\t\ta - Activate the sensor that was previously chosen.\n");
  fprintf(stderr,"\t\td - Deactivate the sensor.\n");
  fprintf(stderr,"\t\tf - Flush the sensor.\n");
  fprintf(stderr,"\t\tb - Batch. The program will prompt for additional information.\n");
  fprintf(stderr,"\t\te - Exit\n");
  fprintf(stderr,"Sensors list:\n");
  for (count = 0; count < sensor_count; count++ ) {
    if(list_p!=nullptr){
       fprintf(stderr,"\t[Type:%2d] (%s) Name:%s Vendor:%s Version:%d Handle:%d\n",
            list_p->type, type_to_string(list_p->type), list_p->name, list_p->vendor, list_p->version, list_p->handle);
    list_p++;
    }
  }
  fprintf(stderr,"\n");
}

void print_sensor( struct sensor_t const* sensor)
{
  fprintf(stderr, "[Type: %2d] %s (%s)\n", sensor->type, sensor_to_stringType(sensor), (sensor->flags & SENSOR_FLAG_WAKE_UP) != 0 ? "wakeup" : "non-wakeup");
  fprintf(stderr, "\tName:%s Vendor:%s Version:%d Handle:%d\n", sensor->name, sensor->vendor, sensor->version, sensor->handle);
  fprintf(stderr, "\tmaxRange: %f resolution: %f power: %f mA\n", sensor->maxRange, sensor->resolution, sensor->power);
  fprintf(stderr, "\tminDelay: %d us maxDelay: %ld us\n", sensor->minDelay, sensor->maxDelay);
  fprintf(stderr, "\tfifoReservedEventCount: %d fifoMaxEventCount: %d\n", sensor->fifoReservedEventCount, sensor->fifoMaxEventCount);
  fprintf(stderr, "\trequiredPermission: %s\n", sensor->requiredPermission);
}

void print_sensor_list( struct sensor_t const* list )
{
  struct sensor_t const* list_p = list;
  fprintf(stderr, "Sensor list:\n");
  if(list_p!=nullptr){
      while ( list_p->name )
      {
        print_sensor(list_p);
        list_p++;
      }
  }
}

void print_help_msg(int argc, char * const argv[]) {
  fprintf(stderr, "Usage: %s [OPTIONS]...\n\n", argc > 0 ? argv[0]: "");
  fprintf(stderr, "-h --help\t\t Print this message\n");
  fprintf(stderr, "-l --listsensors\t List all available sensors and their attributes\n");
  fprintf(stderr, "-o --output\t\tthe output file to write the sensor values to\n\t\t\tdefault: %s\n\n", DEFAULT_OUTPUT_FILE);
  fprintf(stderr, "Providing no parameter options runs the interactive command line interface\n");
  fprintf(stderr, "Providing 1 or more parameters to sns_hal_batch will run the following sequence:\n");
  fprintf(stderr, "\t- set batching parameters for the sensor: (sampling rate, report rate)\n");
  fprintf(stderr, "\t- activate the sensor\n");
  fprintf(stderr, "\t- wait for the specified duration\n");
  fprintf(stderr, "\t- deactivate the sensor\n\n");
  fprintf(stderr, "The parameters, as well as their default values, are as follows:\n");
  fprintf(stderr, "\t-s  --sensor\t\tthe android sensor type enumeration value\n\t\t\t\tdefault: %d for %s\n", DEFAULT_SENSOR_TYPE, type_to_string(DEFAULT_SENSOR_TYPE));
  fprintf(stderr, "\t-w  --wakeup\t\tflag for wakeup or non-wakeup sensor\n\t\t\t\t\tw for wakeup\n\t\t\t\t\tn for non-wakeup\n\t\t\t\t\td for don't care or default\n\t\t\t\tdefault: %c\n", DEFAULT_WAKEUP_FLAG);
  fprintf(stderr, "\t-sr --samplingrate\tthe sampling rate (in Hz)\n\t\t\t\tdefault: %d Hz\n", DEFAULT_SAMPLING_RATE_HZ);
  fprintf(stderr, "\t-rr --reportrate\tthe report rate (in Hz)\n\t\t\t\tuse 0 for no batching\n\t\t\t\t(report events as available)\n\t\t\t\tdefault: %d\n", DEFAULT_REPORT_RATE_HZ);
  fprintf(stderr, "\t-d  --duration\t\tthe duration (in seconds) to run the sensor for\n\t\t\t\tdefault: %d seconds\n", DEFAULT_DURATION_SECS);
}

int find_sensor_by_type(int sensor_type, char wakeup_flag, struct sensor_t const* list, const struct sensor_t **ret)
{
  const struct sensor_t *list_p = list;
  while( list_p->name ) {
    if( list_p->type == sensor_type ) {
      // now that we found a sensor of the correct type, find out if it is wakeup or not
      if (wakeup_flag == 'd') {
        break;
      } else if (wakeup_flag == 'w' && (list_p->flags & SENSOR_FLAG_WAKE_UP) != 0) {
        break;
      } else if (wakeup_flag == 'n' && (~(list_p->flags) & SENSOR_FLAG_WAKE_UP) != 0) {
        break;
      }
    }
    list_p++;
  }

  *ret = list_p;

  if ( list_p-> name ) {
    return 0;
  } else {
    return -1;
  }
}

static void print_sns_event(sensors_event_t buffer){

    uint64_t now_ns;
    struct timespec now_rt;
    clock_gettime( CLOCK_BOOTTIME, &now_rt );
    now_ns = (uint64_t)now_rt.tv_sec * 1000000000LL + (uint64_t)now_rt.tv_nsec;
    if( buffer.type == SENSOR_TYPE_META_DATA ) {
        printf("\n%lu.%06lu, %s/%s, %ld.%06ld, %d latency: %lu.%06lu\n\n",
        now_ns/NSEC_PER_MS, now_ns%NSEC_PER_MS, "META_DATA",
        (buffer.meta_data.what == META_DATA_FLUSH_COMPLETE) ?"FLUSH_COMPLETE":"Unk",
        buffer.timestamp/NSEC_PER_MS, buffer.timestamp%NSEC_PER_MS,
        buffer.meta_data.sensor,
        (now_ns - buffer.timestamp) / NSEC_PER_MS,
        (now_ns - buffer.timestamp) % NSEC_PER_MS);
    } else{
        if( buffer.type == SENSOR_TYPE_ADDITIONAL_INFO) {
            printf("%lu.%06lu, %s/%s, %ld.%06ld, TYPE_ADDITIONAL_INFO type:%d, value: %f \n\n",
            now_ns/NSEC_PER_MS, now_ns%NSEC_PER_MS,
            type_to_string( buffer.type ), handle_to_string(list, buffer.sensor),
            buffer.timestamp/NSEC_PER_MS, buffer.timestamp%NSEC_PER_MS,
            (int)buffer.additional_info.type,
            buffer.additional_info.data_float[0]);
        }else{
            printf("%lu.%06lu, %s/%s, %ld.%06ld, %f, %f, %f, %f, %lld, latency(ms): %lu.%06lu\n",
            (unsigned long)now_ns/NSEC_PER_MS, (unsigned long)now_ns%NSEC_PER_MS,
            type_to_string( buffer.type ), handle_to_string(list, buffer.sensor),
            (long int)buffer.timestamp/NSEC_PER_MS, (long int)buffer.timestamp%NSEC_PER_MS,
            buffer.data[0], buffer.data[1], buffer.data[2], buffer.data[3],
            (long long int)buffer.u64.step_counter,
            (unsigned long)(now_ns - buffer.timestamp) / NSEC_PER_MS,
            (unsigned long)(now_ns - buffer.timestamp) % NSEC_PER_MS);
        }
    }
}

// Constantly polls data from HAL
static void hal_pollFMQ( sensors_event_t buffer[],size_t maxNumEventsToRead, struct sensor_t const* list){

    while(1){
        ssize_t eventsRead = 0;
        size_t available_events = mEventQueue->availableToRead();
        uint32_t event_flag_state = 0;
        if (available_events == 0) {
            mEventQueueFlag->wait(asBaseType(EventQueueFlagBits::READ_AND_PROCESS),&event_flag_state);
            available_events = mEventQueue->availableToRead();
                if (available_events == 0) {
                    ALOGW("Event FMQ wake without any events");
                }
        }
        size_t eventsToRead = std::min({available_events, maxNumEventsToRead, mEventBuffer.size()});
        if (eventsToRead > 0) {
            if (mEventQueue->read(mEventBuffer.data(), eventsToRead)) {
                mEventQueueFlag->wake(asBaseType(EventQueueFlagBits::EVENTS_READ));
                for (size_t i = 0; i < eventsToRead; i++) {
                    convertToSensorEvent(mEventBuffer[i], &buffer[i]);
                }
                eventsRead = eventsToRead;
            } else {
                ALOGW("Failed to read %zu events, currently %zu events available",
                eventsToRead, available_events);
            }
        }
        for( int i = 0; i <  eventsRead ; i++ ) {
            print_sns_event(buffer[i]);
        }
        //wakelock written
        uint32_t wakeEvents = 0;
        for (int i = 0; i < eventsRead; i++) {
            const struct sensor_t *ret_sensor;
            find_sensor_by_type(buffer[i].type,'w', list, &ret_sensor );
            if ( (ret_sensor->flags & SENSOR_FLAG_WAKE_UP) !=0 ){
                wakeEvents++;
            }
        }
        if (wakeEvents > 0) {
            if (mWakeLockQueue->write(&wakeEvents)) {
                mWakeLockQueueFlag->wake(asBaseType(WakeLockQueueFlagBits::DATA_WRITTEN));
            }else{
                ALOGW("Failed to write wake lock handled");
            }
        }
    }
}


int parseArgs(  int argc,
                char * const argv[],
                bool *is_interactive,
                bool *list_sensors,
                int *sensor_type,
                char *wakeup_flag,
                double *sampling_rate_hz,
                int64_t *sampling_rate_nsecs,
                double *report_rate_hz,
                int64_t *report_rate_nsecs,
                int *duration_secs,
                char *output_filename,
                size_t max_output_file_length)
{

  static struct option long_options[] =
        {
          /* These options set a flag. */
          {"sensor", required_argument, 0, 's'},
          {"wakeup", required_argument, 0, 'w'},
          {"samplingrate", required_argument, 0, 0},
          {"sr", required_argument, 0, 0},
          {"reportrate", required_argument, 0, 'r'},
          {"rr", required_argument, 0, 'r'},
          {"duration", required_argument, 0, 'd'},
          {"outputfile", optional_argument, 0, 'o'},
          {"help", no_argument, 0, 'h'},
          {"listsensors", no_argument, 0, 'l'},
          {0, 0, 0, 0}
        };

  *is_interactive = true;
  *list_sensors = false;
  *sensor_type = DEFAULT_SENSOR_TYPE;
  *wakeup_flag = DEFAULT_WAKEUP_FLAG;
  *sampling_rate_hz = DEFAULT_SAMPLING_RATE_HZ;
  *report_rate_hz = DEFAULT_REPORT_RATE_HZ;
  *duration_secs = DEFAULT_DURATION_SECS;
  *output_filename = 0;

  *is_interactive = true;

  int num_scanned;
  int option_index = 0;
  int opt;
  while( (opt = getopt_long_only (argc, argv, ":s:w:r:d:o:hl", long_options, &option_index)) != -1 ) {
    switch(opt)
    {
      // special handling for sampling rate (sr) because two characters
      case 0:
        if (strncmp("sr", long_options[option_index].name, 80) == 0
          || strncmp("samplingrate", long_options[option_index].name, 80) == 0)
        {
          num_scanned = sscanf(optarg, "%lf", sampling_rate_hz);
          if (num_scanned != 1) {
            fprintf(stderr, "ERROR: Invalid argument for %s option\n", long_options[option_index].name);
            exit(1);
          }
          *is_interactive = false;
        }
        break;

      // sensor type
      case 's':
        num_scanned = sscanf(optarg, "%d", sensor_type);
        if (num_scanned != 1) {
          fprintf(stderr, "ERROR: Invalid argument for %s option\n", long_options[option_index].name);
          exit(1);
        }
        *is_interactive = false;
        break;

      // wakeup flag
      case 'w':
        num_scanned = sscanf(optarg, "%c", wakeup_flag);
        if (num_scanned != 1) {
          fprintf(stderr, "ERROR: Invalid argument for %s option\n", long_options[option_index].name);
          exit(1);
        }
        *is_interactive = false;

        break;

      // report rate
      case 'r':
        num_scanned = sscanf(optarg, "%lf", report_rate_hz);
        if (num_scanned != 1) {
          fprintf(stderr, "ERROR: Invalid argument for %s option\n", long_options[option_index].name);
          exit(1);
        }
        *is_interactive = false;
        break;

      // duration
      case 'd':
        num_scanned = sscanf(optarg, "%d", duration_secs);
        if (num_scanned != 1) {
          fprintf(stderr, "ERROR: Invalid argument for %s option\n", long_options[option_index].name);
          exit(1);
         }
        *is_interactive = false;
        break;

      // output file
      case 'o':
          // this parameter is optional so make sure we are not using one of the other options
          // as our argument
          if(*optarg == '-') {
            optind -= 1;
            strlcpy(output_filename, DEFAULT_OUTPUT_FILE, max_output_file_length);
          } else {
            strlcpy(output_filename, optarg, max_output_file_length);
          }
        break;

      case 'l':
        *list_sensors = true;
        return 0;

      // help
      case 'h':
        print_help_msg(argc, argv);
        exit(0);

      // unknown argument
      case '?':
      default:
        fprintf(stderr, "Unknown option \'%c\'.\n", optopt);
        print_help_msg(argc, argv);
        exit(1);
    }
  }


  *sampling_rate_nsecs = *sampling_rate_hz == 0 ? 0 : (uint64_t) HZ_TO_NSEC(*sampling_rate_hz);
  *report_rate_nsecs = *report_rate_hz == 0 ? 0 : (uint64_t) HZ_TO_NSEC(*report_rate_hz);

  return 0;
}

int validateParams(
                    int *sensor_type,
                    char *wakeup_flag,
                    double *sampling_rate_hz,
                    int64_t *sampling_rate_nsecs,
                    double *report_rate_hz,
                    int64_t *report_rate_nsecs,
                    int *duration_secs)
{
  // validate sensor type
  if (*sensor_type < 0) {
    fprintf(stderr, "Invalid sensor_type (sensor out of bounds): %d\n", *sensor_type);
    exit(1);
  }

  if (*wakeup_flag != 'w' && *wakeup_flag != 'n' && *wakeup_flag != 'd') {
    fprintf(stderr, "Invalid wakeup_flag (must be w, n, or d): %c\n", *wakeup_flag);
    exit(1);
  }

  if (*sampling_rate_hz <= 0) {
    fprintf(stderr, "Invalid sampling_rate_hz (must be non-negative): %f\n", *sampling_rate_hz);
    exit(1);
  }

  if (*sampling_rate_nsecs < 0) {
    fprintf(stderr, "Invalid sampling_rate_nsecs (must be non-negative): %" PRId64 "\n", *sampling_rate_nsecs);
    exit(1);
  }

  if (*report_rate_hz < 0) {
    fprintf(stderr, "Invalid report_rate_hz (must be non-negative): %f\n", *report_rate_hz);
    exit(1);
  }

  if (*report_rate_nsecs < 0) {
    fprintf(stderr, "Invalid report_rate_nsecs (must be non-negative): %" PRId64 "\n", *report_rate_nsecs);
    exit(1);
  }

  if (*duration_secs <= 0) {
    fprintf(stderr, "Invalid duration_secs (must be non-negative): %d\n", *duration_secs);
    exit(1);
  }

  return 0;
}

void clearFMQVariables(){
  if(nullptr != mEventQueueFlag) {
    android::hardware::EventFlag::deleteEventFlag(&mEventQueueFlag);
    mEventQueueFlag = nullptr;
  }
    if(nullptr != mEventQueueFlag) {
    android::hardware::EventFlag::deleteEventFlag(&mWakeLockQueueFlag);
    mEventQueueFlag = nullptr;
  }
    mEventQueue.reset(nullptr);
    mWakeLockQueue.reset(nullptr);
}

int main( int argc, char * const argv[] )
{
  struct sensor_t const *list_p;
  int num_scanned;
  double sampling_rate_hz;
  int64_t sampling_rate_nsecs;
  double report_rate_hz;
  int64_t report_rate_nsecs;
  int error;
  char command=' ';
  char prev_command='z';
  char wakeup_flag;
  bool is_interactive;
  bool list_sensors;
  bool valid_input = false;
  char input_buffer[80];
  char output_filename[80];
  int sensor_type;
  int duration_secs;
  struct timespec before_rt;
  struct timespec after_rt;
  uint64_t before_ns;
  uint64_t after_ns;
  uint64_t time_taken_ns;
  Result result;

  error = parseArgs(argc, argv, &is_interactive, &list_sensors, &sensor_type, &wakeup_flag, &sampling_rate_hz, &sampling_rate_nsecs, &report_rate_hz, &report_rate_nsecs, &duration_secs, output_filename, 80);
  if (error)
    exit(1);

  error = validateParams(&sensor_type, &wakeup_flag, &sampling_rate_hz, &sampling_rate_nsecs, &report_rate_hz, &report_rate_nsecs, &duration_secs);
  if (error)
    exit(1);

  FILE* fp = NULL;
  // if the output parameter was set, then redirect stdout
  if (*output_filename != 0) {
    fprintf(stderr, "redirecting stdout to %s\n", output_filename);
    fp = freopen (output_filename, "w", stdout);
  }

  // ------------- FMQ -------- Initialization

  mEventQueue = std::make_unique<EventMessageQueue>(
      MAX_RECEIVE_BUFFER_EVENT_COUNT,
      true /* configureEventFlagWord */);

  mWakeLockQueue = std::make_unique<WakeLockQueue>(
      MAX_RECEIVE_BUFFER_EVENT_COUNT,
      true /* configureEventFlagWord */);

  android::hardware::EventFlag::deleteEventFlag(&mEventQueueFlag);
  android::hardware::EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag);
  android::hardware::EventFlag::deleteEventFlag(&mWakeLockQueueFlag);
  android::hardware::EventFlag::createEventFlag(mWakeLockQueue->getEventFlagWord(),
      &mWakeLockQueueFlag);
  sensors_event_t buffer[MAX_RECEIVE_BUFFER_EVENT_COUNT];

  sensors_hw_module *sensor_module = new sensors_hw_module;
  if(nullptr==sensor_module){
    fprintf(stderr, "ERROR: Unable to initialize sensors HAL!");
    clearFMQVariables();
    exit(1);
  }
  if( mEventQueue!=nullptr && mEventQueue!=nullptr )
  {
  sensor_module->initialize(
      *mEventQueue->getDesc(),
      *mWakeLockQueue->getDesc(),
      nullptr );
    }
  else
    {
    fprintf(stderr, "ERROR: Unable to initialize sensors HAL!");
    delete sensor_module;
    sensor_module = nullptr;
    clearFMQVariables();
    exit(1);
  }
  sensors_hal* _hal = sensors_hal::get_instance();
  if(nullptr == _hal){
    fprintf(stderr, "ERROR: Unable to initialize sensors HAL!");
    clearFMQVariables();
    delete sensor_module;
    sensor_module = nullptr;
    exit(1);
  }
  clock_gettime( CLOCK_BOOTTIME, &before_rt );
  // get time in ms and print
  before_ns = (uint64_t)before_rt.tv_sec * 1000000000LL + (uint64_t)before_rt.tv_nsec;
  sensor_count=_hal->get_sensors_list( (struct sensor_t const**)&list );
  clock_gettime( CLOCK_BOOTTIME, &after_rt );
  after_ns = (uint64_t)after_rt.tv_sec * 1000000000LL + (uint64_t)after_rt.tv_nsec;
  time_taken_ns = (after_ns-before_ns);
  fprintf(stderr, "get_sensors_list took %" PRId64 " nanoseconds\n and count %d", time_taken_ns, (int)sensor_count);

  std::thread t1(hal_pollFMQ, buffer, MAX_RECEIVE_BUFFER_EVENT_COUNT, list);

  if (list_sensors)
  {
    print_sensor_list(list);
    clearFMQVariables();
    delete sensor_module;
    sensor_module = nullptr;
    exit(1);
  }

  if(!is_interactive)
  {
    const struct sensor_t *sensor_to_use;
    error = find_sensor_by_type(sensor_type, wakeup_flag, list, &sensor_to_use);
    if ( error ) {
      fprintf(stderr, "ERROR: sensor not found!");
      clearFMQVariables();
      print_sensor_list(list);
      clearFMQVariables();
      delete sensor_module;
      sensor_module = nullptr;
      exit(1);
    }

    // ACTIVATE
    result  = sensor_module->activate(sensor_to_use->handle, true );

    if( result != Result::OK) {
      fprintf(stderr,"Error %d in batch\n", error );
      clearFMQVariables();
      print_sensor_list(list);
      clearFMQVariables();
      delete sensor_module;
      sensor_module = nullptr;
      exit(1);
    } else {
      fprintf(stderr,"batch success\n");
    }

    // BATCH
    result = sensor_module->batch(sensor_to_use->handle, sampling_rate_nsecs, report_rate_nsecs );

    if( result != Result::OK ) {
      fprintf(stderr,"Error %d activating sensor\n", error );
      clearFMQVariables();
      delete sensor_module;
      sensor_module = nullptr;
      exit(1);
    } else {
      fprintf(stderr, "activate success\n");
    }

    // print out a confirmation for what we just did
    fprintf(stderr, "Activated sensor [Type: %d] %s (%c) for %d seconds, sampling at %f Hz and reporting at %f Hz\n",
        sensor_to_use->type, sensor_to_use->name, wakeup_flag, duration_secs,
        sampling_rate_hz, report_rate_hz);

    // print out more information on the sensor we used
    print_sensor(sensor_to_use);

    // wait to deactivate and exit
    fprintf(stderr, "Sleeping for %d seconds before deactivating and exiting\n", duration_secs);
    sleep(duration_secs);
    fprintf(stderr, "Deactivating and exiting...\n");

    //DEACTIVATE;
    result  = sensor_module->activate(sensor_to_use->handle, false );
    if( result != Result::OK) {
      fprintf(stderr,"Error %d deactivating sensor\n", error );
      clearFMQVariables();
      delete sensor_module;
      sensor_module = nullptr;
      exit(1);
    }

    fprintf(stderr, "deactivate success\n");
    if (fp) fclose(fp);
    fprintf(stderr,"Exiting ESP+ test app...\n");
    clearFMQVariables();
    delete sensor_module;
    sensor_module = nullptr;
    exit(1);
  }

  do
  {
    if((prev_command=='b' && command=='a') || (prev_command=='a' && command=='b'));
    else
        print_usage_msg(list);

    // Get the sensor
    do
    {
      fprintf(stderr,"Please choose a listed sensor type to interact with> ");
      if( NULL == fgets(input_buffer,80,stdin) ) {
        while(1) {
          fprintf(stderr,"Will exit after 600 seconds of EOF\n");
          fprintf(stdout,"Will exit after 600 seconds of EOF\n");
          sleep(600);
          fprintf(stderr,"Exiting after 600 seconds of EOF\n");
          fprintf(stdout,"Exiting after 600 seconds of EOF\n");
          clearFMQVariables();
          delete sensor_module;
          sensor_module = nullptr;
          exit(1);
        }
      }
      fflush(stdin);
      num_scanned = sscanf(input_buffer, "%d", &sensor_type);
      valid_input = true;
      if ( num_scanned != 1 )
      {
        valid_input = false;
        fprintf(stderr,"ERROR: Unknown input.\n");
      }
      else
      {
        fprintf(stderr, "Do you want the wakeup sensor or the non-wakeup sensor? (w for wakeup, n for non-wakeup, d for don't care)> ");
        fgets(input_buffer,80,stdin);
        num_scanned = sscanf(input_buffer, "%c", &wakeup_flag);
        valid_input = true;
        if ( num_scanned != 1 || (wakeup_flag != 'w' && wakeup_flag != 'n' && wakeup_flag != 'd'))
        {
          valid_input = false;
          fprintf(stderr,"ERROR: Unknown input for wakeup_flag.\n");
        } else {
          list_p = list;
          while( list_p->name ) {
            if( list_p->type == sensor_type ) {
              // now that we found a sensor of the correct type, find out if it is wakeup or not
              if (wakeup_flag == 'd') {
                break;
              } else if (wakeup_flag == 'w' && (list_p->flags & SENSOR_FLAG_WAKE_UP) != 0) {
                break;
              } else if (wakeup_flag == 'n' && (~(list_p->flags) & SENSOR_FLAG_WAKE_UP) != 0) {
                break;
              }
            }
            list_p++;
          }
          if ( !list_p->name ) {
            valid_input = false;
            fprintf(stderr,"ERROR: No such sensor exists\n");
          }
        }
      }
    } while ( !valid_input );

    // Get the command
    do
    {
      fprintf(stderr,"Please choose a command (a,s,d,b,f,e)> ");
      fgets(input_buffer,80,stdin);
      fflush(stdin);
      prev_command=command;
      num_scanned = sscanf(input_buffer, "%c", &command);
      valid_input = true;
      if ( num_scanned != 1 )
      {
        valid_input = false;
        fprintf(stderr,"ERROR: Unknown input.\n");
      }
      else if ( NULL == strchr(COMMAND_CHARS, command) )
      {
        valid_input = false;
        fprintf(stderr,"ERROR: Invalid command.\n");
      }
    } while ( !valid_input );

    switch(command)
    {
      case 'A':
      case 'a':
        //Activate;
        fprintf(stderr,"Activating sensor %s @ handle %d ...\n", list_p->name, list_p->handle);
        result = sensor_module->activate(list_p->handle, true );
        if( result != Result::OK) {
          fprintf(stderr,"Error %d activating sensor\n", error );
        }
        command='a';
      break;
      case 'D':
      case 'd':
        //DEACTIVATE;
        fprintf(stderr,"Deactivating sensor %s @ handle %d ...\n", list_p->name, list_p->handle);
        printf("Deactivating sensor %s @ handle %d ...\n", list_p->name, list_p->handle);
            result  = sensor_module->activate(list_p->handle, false );
        if( result != Result::OK) {
          fprintf(stderr,"Error %d deactivating sensor\n", error );
        }
        command='d';
      break;
      case 'B':
      case 'b':
        //BATCH;
        do
        {
          fprintf(stderr,"Please specify a delay period (in Hz)> ");
          fgets(input_buffer,80,stdin);
          fflush(stdin);
          num_scanned = sscanf(input_buffer, "%lf", &sampling_rate_hz);
          valid_input = true;
          if ( num_scanned != 1 )
          {
            valid_input = false;
            fprintf(stderr,"ERROR: Unknown input.\n");
          } else if (sampling_rate_hz <= 0) {
            valid_input = false;
            fprintf(stderr,"ERROR: sampling_rate_hz must be positive\n");
          } else {
            sampling_rate_nsecs = (uint64_t) HZ_TO_NSEC(sampling_rate_hz);
          }
        } while ( !valid_input );
        do
        {
          fprintf(stderr,"Please specify a timeout (in Hz)> ");
          fgets(input_buffer,80,stdin);
          fflush(stdin);
          num_scanned = sscanf(input_buffer, "%lf", &report_rate_hz);
          valid_input = true;
          if ( num_scanned != 1 )
          {
            valid_input = false;
            fprintf(stderr,"ERROR: Unknown input.\n");
          } else if (report_rate_hz < 0) {
            valid_input = false;
            fprintf(stderr,"ERROR: report_rate_hz must be non-negative\n");
          } else {
            report_rate_nsecs = report_rate_hz == 0 ? 0 : (uint64_t) HZ_TO_NSEC(report_rate_hz);
          }
        } while ( !valid_input );

        fprintf(stderr,"Setting batch mode for sensor %s @ handle %d ...\n", list_p->name, list_p->handle);
        result  = sensor_module->batch(list_p->handle, sampling_rate_nsecs, report_rate_nsecs );
        if( result != Result::OK) {
          fprintf(stderr,"Error %d in batch\n", error );
        } else {
          fprintf(stderr,"batch success\n");
        }
        command='b';
        break;

      case 'F':
      case 'f':
        //Flush;
        fprintf(stderr,"flushing sensor %s @ handle %d ...\n", list_p->name, list_p->handle);
        result  = sensor_module->flush(list_p->handle );
        if( result != Result::OK) {
          fprintf(stderr,"Error %d in flush\n", error );
        } else {
          fprintf(stderr,"flush success\n");
        }
        command='f';
        break;
      case 'E':
      case 'e':
        //EXIT;
        break;
    }
  } while( command != 'e' && command != 'E' );
  fprintf(stderr,"Exiting ESP+ test app...\n");
  t1.join();
  clearFMQVariables();
  delete sensor_module;
  sensor_module = nullptr;
  return 0;
}
