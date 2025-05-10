# Custom Visual Cloud Library (VCL) Image Manipulation Functions
Starting with version 2.2, VDMS supports custom image manipulation functions when ingesting images that extend beyond the built-in functions (i.e., crop, threshold, rotate). This allows for user to perform manipulation techniques that are more specific to their application

The three components of VCL custom function are the VCL function within the VDMS server process, the local process that performs custom image manipulation, and the client code. A user invokes the AddImage command using the ”custom” operation and the “custom_operation_type” keys along with the required operands needed by this function. When the function enqueue_operations from within the ImageCommand class iterates through the operands and detects the user requests to perform a custom function, VCL performs a transaction with a local handler process that manipulates the passed image. This includes 1) performing a check to ensure the custom process is running, 2) placing the image and operands into shared memory 3) awaiting a response from local process. The client code ultimately invokes the custom function similarly to the preciously supported operands. The implementation follows the same flow as the original enque_operations function which allows users to chain together multiple image manipulation steps.

## VDMS Process
The function custom_vcl_function is responsible for passing the image and operands to another local process and awaiting the response. First, a shared memory and shared messages are initialized to facilitate the inter-process communication. The application file “vdms” is used by both the vdms process and the handler process to generate tokens used for shared memory and shared messages. This code can be found in src/vcl/CustomVCL.cc.


    key_t key_ctl_host_remote;
    key_ctl_host_remote = ftok("vdms", 60);
    int msgid_ctl_host_remote = msgget(key_ctl_host_remote, 0666 | IPC_CREAT);
    data_message message_ctl_host_remote;

    key_t key_data_host_remote;
    key_data_host_remote = ftok("vdms", 61);
    int shmid_data_host_remote = shmget(key_data_host_remote,SHARED_IMAGE_BUFFER_SIZE,0666|IPC_CREAT);
    uint8_t *image_buffer = (uint8_t*) shmat(shmid_data_host_remote,(void*)0,0);

    key_t key_ctl_remote_host;
    key_ctl_remote_host = ftok("vdms", 62);
    int msgid_ctl_remote_host = msgget(key_ctl_remote_host, 0666 | IPC_CREAT);
    data_message message_ctl_remote_host;

Up to 10 heartbeat messages are sent to the handler process to test availability. If the handler process responds, then the image and operands are place into shared memory and a message is sent to the remote process to inform the handler process that all data required has been transferred.

    //try 10 times to determine if process is running\
    while(hb_count < 10 && in_alive_msg_status < 0)
    {
        in_alive_msg_status = msgrcv(msgid_ctl_remote_host, &message_hb_remote_host,sizeof(heartbeat_message) , (long) vcl_message_type::VCL_MESSAGE_HEARTBEAT, IPC_NOWAIT);
        hb_count++;
    }


The server process then waits for a message from the handler process. This is not received until after image manipulation has completed and the data has been copied back into shared memory. The resulting image is then copied from shared memory into the VCL variable.

        //Copy image data into shared memory
        memcpy((uint8_t*) &(image_buffer[0]), (uint8_t*) &(in_image.data[0]), in_image_size);

        std::string* json_string = new std::string(ops.toStyledString());
        message_ctl_host_remote.data_json_size = json_string->size();

        //image size corresponds with first byte after the image
        memcpy(&(image_buffer[in_image_size]), json_string->c_str(), json_string->size());
        int msg_send_result = msgsnd(msgid_ctl_host_remote, &message_ctl_host_remote, sizeof(data_message), 0);
        if(msg_send_result < 0)
        {}

        int msg_recv_result = msgrcv(msgid_ctl_remote_host, &message_ctl_remote_host, sizeof(data_message), (long)vcl_message_type::VCL_MESSAGE_DATA, 0);
        if(msg_recv_result < 0)
        {}

        //Grab data back from shared memory
        cv::Mat* out_image = new cv::Mat(message_ctl_remote_host.data_rows, message_ctl_remote_host.data_cols, message_ctl_remote_host.data_type);
        memcpy(&(out_image->data[0]), &(image_buffer[0]), message_ctl_remote_host.data_image_size);

        img.deep_copy_cv(*out_image);

Neither this function or any of the vdms source code should require modification to enable custom functions. All C++ code changes take place in the handler process source code and client application code.

## Local Handler Process
Source code for a sample remoe process is provided at ext/custom_vcl/custom_vcl_process.cc. This code compiles into a separate application that interfaces with the vdms server process. As with the host process, the shared tokens point to the “vdms” application file. If a user creates a handler process in a different file directory, the parameter passed to the ftok command must be changed to reflect this relative path change.

    //create IPC structures for communicating between processes
    key_t key_ctl_host_remote;
    key_ctl_host_remote = ftok("../../vdms", 60);
    int msgid_ctl_host_remote = msgget(key_ctl_host_remote, 0666 | IPC_CREAT);
    data_message message_ctl_host_remote;

    key_t key_data_host_remote;
    key_data_host_remote = ftok("../../vdms", 61);
    int shmid_data_host_remote = shmget(key_data_host_remote,SHARED_IMAGE_BUFFER_SIZE,0666|IPC_CREAT);
    uint8_t *image_buffer = (uint8_t*) shmat(shmid_data_host_remote,(void*)0,0);

    key_t key_ctl_remote_host;
    key_ctl_remote_host = ftok("../../vdms", 62);
    int msgid_ctl_remote_host = msgget(key_ctl_remote_host, 0666 | IPC_CREAT);
    data_message message_ctl_remote_host;

After connecting to the shared memory and messages, the handler process enters a while loop that waits for a heartbeat request and responds with a message to assure the process is functioning correctly. Then the handler process receives a message from the vdms server process that indicates the image and required operands have been copied into shared memory and image manipulation can continue. The image is copied from shared memory into an OpenCV mat variable and then can be manipulated according to the user requirements.

        //Handle handshake to indicate remote process is alive
        int in_alive_msg_status = msgrcv(msgid_ctl_host_remote, &message_hb_host_remote,sizeof(heartbeat_message) , (long) vcl_message_type::VCL_MESSAGE_HEARTBEAT, 0);

        message_hb_remote_host.message_type = (long) vcl_message_type::VCL_MESSAGE_HEARTBEAT;
        message_hb_remote_host.status = 0;
        int msg_send_result = msgsnd(msgid_ctl_remote_host, &message_hb_remote_host, sizeof(heartbeat_message), 0);

        int msg_status = msgrcv(msgid_ctl_host_remote, &message_ctl_host_remote,sizeof(data_message) , (long) vcl_message_type::VCL_MESSAGE_DATA, 0);

The handler process is capable of supporting multiple custom functions using the parameter “custom_function_type”. The code bock can check for each of the custom function types and perform the image manipulation steps. With the provided example, the image is converted to HSV color space and then the function inRange performs color thresholding. After the manipulation steps are completed, the modified image is placed in shared memory and a message is sent to the vdms server process indicating the operation has completed.

            if(vcl_op.get("custom_function_type", 0).asString() == "hsv_threshold")
                {
                    cv::cvtColor(*in_image, *in_image, cv::COLOR_RGB2HSV);
                    cv::inRange(*in_image, cv::Scalar(vcl_op.get("h0", -1).asInt(), vcl_op.get("s0", -1).asInt(), vcl_op.get("v0", -1).asInt()), cv::Scalar(vcl_op.get("h1", -1).asInt(),vcl_op.get("s1", -1).asInt(),vcl_op.get("v1", -1).asInt()), *in_image);

                    size_t in_image_size = in_image->total() * in_image->elemSize();
                    memcpy(&(image_buffer[0]), &(in_image->data[0]), in_image_size);

                    //Send Response back to host
                    message_ctl_remote_host.message_type = (long) vcl_message_type::VCL_MESSAGE_DATA;
                    message_ctl_remote_host.data_rows = in_image->rows;
                    message_ctl_remote_host.data_cols = in_image->cols;
                    message_ctl_remote_host.data_type = in_image->type();
                    message_ctl_remote_host.data_image_size = in_image_size;
                    message_ctl_remote_host.data_json_size = 0;


## Client Code
Ultimately, the client code must invoke the custom VCL manipulation which require passing the manipulation operation name, the image, and required operands. To enable the handler process to accept arbitrary operands, only a check to examine correct JSON formatting Is performed on the VDMS query and no check is performed to ensure the operands required for the custom function are present. If the operands are not present, the custom function will provide an assertion and the image will not be manipulated.
The file etc/sample_query_sample_query.py provides a script that invokes the custom function discussed in the previous section. Notice the variable “type” is set to “custom, the variable “custom_function_type” is set to “hsv_threshold” and all of the required operands (h0, h1, s0, s1, v0, v1) are specified and passed into the query.


    operations = []
    operation = {}
    operation["type"] = "custom"
    operation["custom_function_type"] = "hsv_threshold"
    operation["h0"] = 10
    operation["s0"] = 250
    operation["v0"] = 175
    operation["h1"] = 20
    operation["s1"] = 255
    operation["v1"] = 185

    operations.append(operation)
    addImage["operations"] = operations
