/**
 * @file camera.h
 *
 * @brief This module provides a high-level API for accessing the camera.
 * @author Aramayis Orkusyan
 * @date December 9, 2019
 * @copyright GNU General Public License v3.0
 */

#ifndef SITE_MON_GMS_CAMERA_H
#define SITE_MON_GMS_CAMERA_H

#include <stdint.h>

/**
 * Initialize the camera to capture grey-scale images.
 *
 * @param device The device file of the camera e.g. /dev/video0.
 * @return On success, returns 0. Otherwise, returns -1.
 */
int camera_init(const char *device);

/**
 * Captures a fram and saves it to disk as a grey-scale pgm file.
 *
 * @param out_dir Absolute path to the directory where the image will be save.
 * @return On success, returns 0. Otherwise, returns -1.
 * @note The image will be saved as <out_dir>/<time_stamp>.pgm
 */
int camera_capture_frame(const char *out_dir);

/**
 * Detects if there is motion.
 * 
 * @param avg_pixel_diff The threshold to use when determining if motion occured.
 * @return If motion is detected, returns 1. Otherwise, returns 0.
 * @note Currently, the camera moduule captures 10 frames and computes the
 *       the average of the difference in pixel values between the 5th and 10th
 *       frames.
 */
int camera_detect_motion(uint8_t avg_pixel_diff);

#endif
