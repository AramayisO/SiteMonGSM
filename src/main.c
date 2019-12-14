#include "camera.h"
#include "gsm.h"
#include "util.h"
#include "debug.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

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
    pid_t     pid;
    pthread_t gsm_thread;

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

            // Send alert message
            pthread_create(&gsm_thread, NULL, on_motion_detected, NULL);

            // Start streaming video
            if ((pid = fork()) < 0)
            {
                perror("fork");
                exit(1);
            }

            if (pid == 0)
            {
                DEBUG_LOG(stdout, "Starting video streaming server\n");
                char *args[] = {"/usr/bin/python3", "../src/server/camera_server.py", NULL};
                execv(args[0], args);
            }
            else
            {
                // Allow server to stream video for 5 minutes then kill the server
                // and check for motion again.
                SLEEP_SECONDS(300);
                DEBUG_LOG(stdout, "Killing video streaming server...");
                kill(pid, SIGQUIT);
                DEBUG_LOG(stdout, "killed\n");
            }
        }
    }

    return 0;
}
