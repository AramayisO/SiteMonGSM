#include "camera.h"
#include "gsm.h"
#include "util.h"
#include "debug.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define VIDEO_DEVICE_FILE    "/dev/video0"
#define VIDEO_OUTPUT_DIR     "/home/pi/Pictures"
#define GSM_DEVICE_FILE      "/dev/ttyUSB2"
#define GSM_DESTINATION      "15599078609"
#define GSM_MESSAGE          "Motion detected"
#define AVG_PIXEL_DIFFERENCE  8
#define NUM_FRAMES_TO_CAPTURE 10 

int main()
{
    pid_t pid;

    // Initialize modules
    camera_init(VIDEO_DEVICE_FILE);
    gsm_init(GSM_DEVICE_FILE);
    gsm_set_functionality_mode(GSM_MINIMUM_FUNCTIONALITY_MODE);
    for (;;)
    {
        DEBUG_LOG(stdout, "Waiting for motion\n");
        if (camera_detect_motion(AVG_PIXEL_DIFFERENCE))
        {
            DEBUG_LOG(stdout, "Motion detected\n");

            // Start streaming video
            if ((pid = fork()) < 0)
            {
                perror("fork");
                exit(1);
            }

            if (pid == 0)
            {
                DEBUG_LOG(stdout, "Starting video streaming server\n");
                gsm_set_functionality_mode(GSM_FULL_FUNCTIONALITY_MODE);
                gsm_send_message(GSM_DESTINATION, GSM_MESSAGE;
                // Give the modem a couple of seconds to finish transmitting
                // the sms before putting back in low power mode.
                SLEEP_SECONDS(2);
                gsm_set_functionality_mode(GSM_MINIMUM_FUNCTIONALITY_MODE);
                return 0;
            }
            else
            {
                DEBUG_LOG(stdout, "starting capture\n");
                for (int count = 0; count < 5; ++ count)
                {
                    camera_capture_frame("/home/pi/Pictures");
                    SLEEP_SECONDS(1);
                }
                DEBUG_LOG(stdout, "end capture\n");
            }
        }
    }
    return 0;
}
