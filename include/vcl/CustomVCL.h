#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>

#include <opencv2/opencv.hpp>

#include "../ExceptionsCommand.h"
#include "Image.h"

#define SHARED_IMAGE_BUFFER_SIZE 134217728
enum class vcl_message_type { VCL_MESSAGE_HEARTBEAT = 1, VCL_MESSAGE_DATA };

// structure for message queue
// first byte of message must be non negative long
typedef struct data_msg {
  long message_type;
  unsigned int data_rows;
  unsigned int data_cols;
  unsigned int data_type;
  unsigned int data_image_size;
  unsigned int data_json_size;
} data_message;

typedef struct hb_msg {
  long message_type;
  unsigned int status;
} heartbeat_message;

int custom_vcl_function(VCL::Image &img, const Json::Value &ops);
