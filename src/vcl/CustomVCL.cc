#include "vcl/CustomVCL.h"

int custom_vcl_function(VCL::Image &img, const Json::Value &ops) {
  int return_value = 0;
  // create IPC structures for communicating between processes
  key_t key_ctl_host_remote;
  key_ctl_host_remote = ftok("vdms", 60);
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
  key_data_host_remote = ftok("vdms", 61);
  int shmid_data_host_remote =
      shmget(key_data_host_remote, SHARED_IMAGE_BUFFER_SIZE, 0666 | IPC_CREAT);
  uint8_t *image_buffer =
      (uint8_t *)shmat(shmid_data_host_remote, (void *)0, 0);

  key_t key_ctl_remote_host;
  key_ctl_remote_host = ftok("vdms", 62);
  int msgid_ctl_remote_host = msgget(key_ctl_remote_host, 0666 | IPC_CREAT);
  data_message message_ctl_remote_host;

  heartbeat_message message_hb_host_remote;
  heartbeat_message message_hb_remote_host;
  size_t heartbeat_message_size = sizeof(message_hb_host_remote.status);

  // Pass messages to ensure the remote process is functional
  message_hb_host_remote.message_type =
      (long)vcl_message_type::VCL_MESSAGE_HEARTBEAT;
  message_hb_host_remote.status = 0;
  int out_alive_msg_status =
      msgsnd(msgid_ctl_host_remote, &message_hb_host_remote,
             heartbeat_message_size, 0);

  int hb_count = 0;
  int in_alive_msg_status = -1;

  // try 10 times to determine if process is running
  while (hb_count < 10 && in_alive_msg_status < 0) {
    in_alive_msg_status = msgrcv(
        msgid_ctl_remote_host, &message_hb_remote_host, heartbeat_message_size,
        (long)vcl_message_type::VCL_MESSAGE_HEARTBEAT, IPC_NOWAIT);
    hb_count++;
  }

  if (in_alive_msg_status > -1) {
    // Read image from file and obtain image information to calculate size
    cv::Mat in_image = img.get_cvmat(true);

    size_t in_image_size = in_image.total() * in_image.elemSize();
    message_ctl_host_remote.message_type =
        (long)vcl_message_type::VCL_MESSAGE_DATA;
    message_ctl_host_remote.data_rows = in_image.rows;
    message_ctl_host_remote.data_cols = in_image.cols;
    message_ctl_host_remote.data_type = in_image.type();
    message_ctl_host_remote.data_image_size = in_image_size;

    // Copy image data into shared memory
    memcpy((uint8_t *)&(image_buffer[0]), (uint8_t *)&(in_image.data[0]),
           in_image_size);

    std::string *json_string = new std::string(ops.toStyledString());
    message_ctl_host_remote.data_json_size = json_string->size();
    // image size corresponds with first byte after the image
    memcpy(&(image_buffer[in_image_size]), json_string->c_str(),
           json_string->size());
    int msg_send_result = msgsnd(
        msgid_ctl_host_remote, &message_ctl_host_remote, data_message_size, 0);
    if (msg_send_result < 0) {
      delete json_string;
      return -1;
    }

    int msg_recv_result =
        msgrcv(msgid_ctl_remote_host, &message_ctl_remote_host,
               data_message_size, (long)vcl_message_type::VCL_MESSAGE_DATA, 0);

    if (msg_recv_result < 0) {
    }

    // Grab data back from shared memory
    cv::Mat *out_image = new cv::Mat(message_ctl_remote_host.data_rows,
                                     message_ctl_remote_host.data_cols,
                                     message_ctl_remote_host.data_type);
    memcpy(&(out_image->data[0]), &(image_buffer[0]),
           message_ctl_remote_host.data_image_size);

    img.deep_copy_cv(*out_image);

    // Free allocated memory
    delete out_image;
    delete json_string;

    // Free shared IPC components
    shmdt(image_buffer);
    // msgctl(msgid_ctl_remote_host, IPC_RMID, NULL);
    return_value = 0;
  } else {
    return_value = -1;
    throw VDMS::ExceptionCommand(ImageError, "Error in custom Function");
  }

  return return_value;
}
