#include "vcl/CustomVCL.h"
#include <opencv2/imgproc.hpp>

int main(int argc, char *argv[]) {

  // create IPC structures for communicating between processes
  key_t key_ctl_host_remote;
  key_ctl_host_remote = ftok("../../vdms", 60);
  int msgid_ctl_host_remote = msgget(key_ctl_host_remote, 0666 | IPC_CREAT);
  data_message message_ctl_host_remote;
  // need size of data message excluding message_type field for msgsnd and
  // msgrcv
  size_t data_message_size = sizeof(message_ctl_host_remote.data_rows) +
                             sizeof(message_ctl_host_remote.data_cols) +
                             sizeof(message_ctl_host_remote.data_type) +
                             sizeof(message_ctl_host_remote.data_image_size) +
                             sizeof(message_ctl_host_remote.data_json_size);

  key_t key_data_host_remote;
  key_data_host_remote = ftok("../../vdms", 61);
  int shmid_data_host_remote =
      shmget(key_data_host_remote, SHARED_IMAGE_BUFFER_SIZE, 0666 | IPC_CREAT);
  uint8_t *image_buffer =
      (uint8_t *)shmat(shmid_data_host_remote, (void *)0, 0);

  key_t key_ctl_remote_host;
  key_ctl_remote_host = ftok("../../vdms", 62);
  int msgid_ctl_remote_host = msgget(key_ctl_remote_host, 0666 | IPC_CREAT);
  ;
  data_message message_ctl_remote_host;

  heartbeat_message message_hb_host_remote;
  heartbeat_message message_hb_remote_host;
  size_t heartbeat_message_size = sizeof(message_hb_host_remote.status);

  while (true) {
    // Handle handshake to indicate remote process is alive
    int in_alive_msg_status = msgrcv(
        msgid_ctl_host_remote, &message_hb_host_remote, heartbeat_message_size,
        (long)vcl_message_type::VCL_MESSAGE_HEARTBEAT, 0);

    message_hb_remote_host.message_type =
        (long)vcl_message_type::VCL_MESSAGE_HEARTBEAT;
    message_hb_remote_host.status = 0;
    int msg_send_result = msgsnd(msgid_ctl_remote_host, &message_hb_remote_host,
                                 heartbeat_message_size, 0);

    int msg_status =
        msgrcv(msgid_ctl_host_remote, &message_ctl_host_remote,
               data_message_size, (long)vcl_message_type::VCL_MESSAGE_DATA, 0);
    if (msg_status > 0) {
      // Read image from shared memory
      cv::Mat *in_image = new cv::Mat(message_ctl_host_remote.data_rows,
                                      message_ctl_host_remote.data_cols,
                                      message_ctl_host_remote.data_type);
      memcpy((uint8_t *)&(in_image->data[0]), image_buffer,
             message_ctl_host_remote.data_image_size);

      // Read Json operands from shared memory and store into Json::Value
      char *json_string_char = new char[message_ctl_host_remote.data_json_size];
      memcpy(&(json_string_char[0]),
             &(image_buffer[message_ctl_host_remote.data_image_size]),
             message_ctl_host_remote.data_json_size);
      std::string *json_string = new std::string(json_string_char);
      Json::Value vcl_op;
      Json::Reader vcl_reader;
      bool parse_flag = vcl_reader.parse(json_string->c_str(), vcl_op);
      if (parse_flag) {
        // Manipulate Image
        if (vcl_op.get("custom_function_type", 0).asString() ==
            "hsv_threshold") {
          cv::cvtColor(*in_image, *in_image, cv::COLOR_RGB2HSV);
          cv::inRange(*in_image,
                      cv::Scalar(vcl_op.get("h0", -1).asInt(),
                                 vcl_op.get("s0", -1).asInt(),
                                 vcl_op.get("v0", -1).asInt()),
                      cv::Scalar(vcl_op.get("h1", -1).asInt(),
                                 vcl_op.get("s1", -1).asInt(),
                                 vcl_op.get("v1", -1).asInt()),
                      *in_image);

          size_t in_image_size = in_image->total() * in_image->elemSize();
          memcpy(&(image_buffer[0]), &(in_image->data[0]), in_image_size);

          // Send Response back to host
          message_ctl_remote_host.message_type =
              (long)vcl_message_type::VCL_MESSAGE_DATA;
          message_ctl_remote_host.data_rows = in_image->rows;
          message_ctl_remote_host.data_cols = in_image->cols;
          message_ctl_remote_host.data_type = in_image->type();
          message_ctl_remote_host.data_image_size = in_image_size;
          message_ctl_remote_host.data_json_size = 0;

        } else {
          // Send Response back to host in event of error
          message_ctl_remote_host.message_type =
              (long)vcl_message_type::VCL_MESSAGE_DATA;
          message_ctl_remote_host.data_rows = 0;
          message_ctl_remote_host.data_cols = 0;
          message_ctl_remote_host.data_type = 0;
          message_ctl_remote_host.data_image_size = 0;
          message_ctl_remote_host.data_json_size = 0;
        }
      }

      int msg_send_result =
          msgsnd(msgid_ctl_remote_host, &message_ctl_remote_host,
                 data_message_size, 0);
      if (msg_send_result < 0) {
      }

      // Free allocated memory
      delete in_image;
      delete[] json_string_char;
      delete json_string;
    }
  }

  // Free shared IPC components
  msgctl(msgid_ctl_host_remote, IPC_RMID, NULL);
  shmdt(image_buffer);
  shmctl(shmid_data_host_remote, IPC_RMID, NULL);
  return 0;
}