#include "camera.h"
#include "gsm.h"
#include "util.h"
#include "debug.h"
#include <unistd.h>
#include <pthread.h>

#define VIDEO_DEVICE_FILE    "/dev/video0"
#define VIDEO_OUTPUT_DIR     "/home/pi/Pictures"
#define GSM_DEVICE_FILE      "/dev/ttyUSB2"
#define GSM_DESTINATION      "15599078609"
#define GSM_MESSAGE          "Motion detected"
#define AVG_PIXEL_DIFFERENCE  5
#define NUM_FRAMES_TO_CAPTURE 10 

static void *on_motion_detected(void *vargp)
{
    gsm_set_functionality_mode(GSM_FULL_FUNCTIONALITY_MODE);
    gsm_send_message(GSM_DESTINATION, GSM_MESSAGE);
    gsm_set_functionality_mode(GSM_MINIMUM_FUNCTIONALITY_MODE);
    return NULL;
}

int main()
{
    pthread_t gsm_thread;
    camera_init(VIDEO_DEVICE_FILE);
    gsm_init(GSM_DEVICE_FILE);
    gsm_set_functionality_mode(GSM_MINIMUM_FUNCTIONALITY_MODE);

    for (;;)
    {
        if (camera_detect_motion(AVG_PIXEL_DIFFERENCE))
        {
            pthread_create(&gsm_thread, NULL, on_motion_detected, NULL);
            for (int frame = 0; frame < NUM_FRAMES_TO_CAPTURE; ++frame)
            {
                camera_capture_frame(VIDEO_OUTPUT_DIR);
                SLEEP_SECONDS(1);
            }
        }
    }

    return 0;
}
