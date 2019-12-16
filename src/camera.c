#include "camera.h"
#include "debug.h"
#include "util.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>

#define CAMERA_NUM_BUFFERS 3       // number of buffers for capturing frames
#define CAMERA_VIDEO_WIDTH_PX 640  // width of captured frames in pixels
#define CAMERA_VIDEO_HEIGHT_PX 480 // height of captured frames in pixels

typedef enum camera_pixel_fmt
{
    CAMERA_PIXEL_FMT_GREY,  // greyscale images used for motion detection
    CAMERA_PIXEL_FMT_MJPEG  // color jpeg used for recording and streaming
} camera_pixel_fmt_t;

// This datastructure is used to store the infomation about shared memory
// spaces created with mmap. A shared memory buffer must be created that is
// passed to the camera device to write to and then can be read by the user.
typedef struct buffer 
{
    void *start;
    size_t length;
} buffer_t;

// Global module variables
static int                        fd;
static struct v4l2_capability     capability;
static struct v4l2_format         format;
static struct v4l2_requestbuffers bufrequest;
static buffer_t                   buffers[CAMERA_NUM_BUFFERS];


static int camera_set_pixel_fmt(camera_pixel_fmt_t pxfmt)
{
    format.type           = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width  = CAMERA_VIDEO_WIDTH_PX;
    format.fmt.pix.height = CAMERA_VIDEO_HEIGHT_PX;

    switch (pxfmt)
    {
        case CAMERA_PIXEL_FMT_GREY: 
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
            break;

        case CAMERA_PIXEL_FMT_MJPEG:
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
            break;

        default:
            return -1;
    }

    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed to set video format.\n" __FILE__);
        return -1;
    }
    return 0;
}

static int camera_allocate_buffers()
{
    // Inform the device about future buffers.
    bufrequest.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count  = CAMERA_NUM_BUFFERS;
    
    if (ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed VIDIOC_REQBUFS.\n" __FILE__);
        return -1;
    }

    for (size_t index = 0; index < bufrequest.count; ++index)
    {
        // Allocate buffers.
        static struct v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index  = index;
    
        if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0)
        {
            DEBUG_LOG(stdout, "%s: failed to allocate buffers.\n" __FILE__);
            return -1;
        }

        buffers[index].length = buffer.length;

        // Map the memory.
        buffers[index].start = mmap (
            NULL,
            buffer.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            buffer.m.offset
        );

        if(buffers[index].start == MAP_FAILED){
            DEBUG_LOG(stdout, "%s: failed to map memory with mmap.\n" __FILE__);
            return -1;
        }

        memset(buffers[index].start, 0, buffers[index].length);
    }
    return 0;
}

static int camera_release_buffers()
{
    // Unmap memory
    for (size_t index = 0; index < bufrequest.count; ++index)
    {
        if (buffers[index].start != NULL)
        {
            munmap(buffers[index].start, buffers[index].length);
        }
    }
    // Release buffers V4L2 allocated for us.
    bufrequest.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count  = 0;
    
    if (ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed VIDIOC_REQBUFS.\n" __FILE__);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 *
 * Function:    camera_init()
 *
 * Description: Initialize the camera to capture grey-scale images.
 *
 * Returns:     On success, returns 0. Otherwise, returns -1.
 *
 ******************************************************************************/
int camera_init(const char *device)
{
    // Open descriptor to camera device.
    if ((fd = open(device, O_RDWR)) == -1)
    {
        DEBUG_LOG(stdout, "%s: Failed to open camera", __FILE__);
        return -1;
    }

    // Retrieve the devices capabilities.
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0)
    {
        DEBUG_LOG(stdout, "%s: Failed VIDIOC_QUERYCAP\n", __FILE__);
        return -1;
    }
    
    // Check if camera device has single-planar video capture capability.
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        DEBUG_LOG(stdout,
                  "%s: camera does not support single-planar video capture.\n"
                  __FILE__);
        return -1;
    }

    // Check if camera device has frame streaming capability.
    if (!(capability.capabilities & V4L2_CAP_STREAMING))
    {
        DEBUG_LOG(stdout, "%s: camera does not support frame streaming.\n" __FILE__);
        return -1;
    }

    // Set video format to greyscale initially.
    if (camera_set_pixel_fmt(CAMERA_PIXEL_FMT_GREY) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed to set video format.\n" __FILE__);
        return -1;
    }

    // Allocate the buffers and memory mapped files.
    if (camera_allocate_buffers() < 0)
    {
        DEBUG_LOG(stdout, "%s: failed to allocate buffers.\n", __FILE__);
    }

    return 0;
}

/*******************************************************************************
 *
 * Function:    camera_capture_frame()
 *
 * Description: Captures a fram and saves it to disk as a grey-scale pgm file.
 *
 * Returns:     On success, returns 0. Otherwise, returns -1.
 * 
 ******************************************************************************/
int camera_capture_frame(const char *save_dir)
{
    // Set to color MJPEG mode when capturing frames for recording and streaming.
    if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG)
    {
        camera_release_buffers();
        camera_set_pixel_fmt(CAMERA_PIXEL_FMT_MJPEG);
        camera_allocate_buffers();
    }

    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;

    // Activate streaming
    int type = buffer.type;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed VIDIOC_STREAMON\n" __FILE__);
        return -1;
    }

    for (size_t index = 0; index < bufrequest.count; ++index)
    {
        buffer.index = index;
        // Put the buffer in the incoming queue.
        if (ioctl(fd, VIDIOC_QBUF, &buffer) < 0)
        {
            DEBUG_LOG(stdout, "%s: failed VIDIOC_QBUF\n" __FILE__);
            return -1;
        }
        
        // The buffer's waiting in the outgoing queue.
        if (ioctl(fd, VIDIOC_DQBUF, &buffer) < 0)
        {
            DEBUG_LOG(stdout, "%s: failed VIDIOC_DQBUF\n" __FILE__);
            return -1;
        }
    }

    // Deactivate streaming
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed VIDIOC_STREAMOFF\n" __FILE__);
        return -1;
    }

    // Write captured frame to file.
    int jpegfile;
    char filepath[256];
    sprintf(filepath, "%s/%lu.jpeg", save_dir, time(NULL));

    if ((jpegfile = open(filepath, O_WRONLY | O_CREAT)) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed to open image file\n" __FILE__);
        return -1;
    }
    write(jpegfile, buffers[bufrequest.count - 1].start, buffers[bufrequest.count - 1].length);
    close(jpegfile);

    return 0;
}

/*******************************************************************************
 *
 * Function:    camera_detect_motion()
 *
 * Description: Detects if there is motion.
 *
 * Returns:     If motion is detected, returns 1. Otherwise, returns 0.
 * 
 ******************************************************************************/
int camera_detect_motion(uint8_t avg_pixel_diff)
{
    // Set  up camera and buffers for greyscale imaging.
    if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_GREY)
    {
        camera_release_buffers();
        camera_set_pixel_fmt(CAMERA_PIXEL_FMT_GREY);
        camera_allocate_buffers();
    }

    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index  = 0;

    // Activate streaming
    DEBUG_LOG(stdout, "%s line %lu: Activate streaming\n", __FILE__, __LINE__);
    int type = buffer.type;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed VIDIOC_STREAMON\n" __FILE__);
        return -1;
    }

    for (size_t index = 0; index < bufrequest.count; ++index)
    {
        buffer.index = index;
        // Put the buffer in the incoming queue.
        if (ioctl(fd, VIDIOC_QBUF, &buffer) < 0)
        {
            DEBUG_LOG(stdout, "%s: failed VIDIOC_QBUF\n" __FILE__);
            return -1;
        }
        
        // The buffer's waiting in the outgoing queue.
        if (ioctl(fd, VIDIOC_DQBUF, &buffer) < 0)
        {
            DEBUG_LOG(stdout, "%s: failed VIDIOC_DQBUF\n" __FILE__);
            return -1;
        }

        // Capture frames 500 msecs appart for detecting motion.
        SLEEP_MSECONDS(250);
    }

    // Deactivate streaming
    DEBUG_LOG(stdout, "%s line %lu: Deactivate streaming\n", __FILE__, __LINE__);
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
    {
        DEBUG_LOG(stdout, "%s: failed VIDIOC_STREAMOFF\n" __FILE__);
        return -1;
    }

    uint8_t *first  = (uint8_t*)buffers[1].start;
    uint8_t *second = (uint8_t*)buffers[CAMERA_NUM_BUFFERS - 1].start;
    int sum_diff    = 0;
    int diff        = 0;

    for (size_t pixel = 0; pixel < buffers[0].length; ++pixel)
    {
        diff = first[pixel] - second[pixel];
        sum_diff += (diff > 0 ? diff : (-1 * diff));
    }

    return (sum_diff / (format.fmt.pix.width * format.fmt.pix.height)) > avg_pixel_diff;
}
