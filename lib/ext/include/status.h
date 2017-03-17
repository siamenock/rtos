#ifndef __APP_STATUS_H__
#define __APPP_STATUS_H__

/**
 * @file
 * Network application status information (This maybe deprecated)
 */

/**
 * Net App is stopped
 */
#define APP_STATUS_STOPPED 	0

/**
 * Net App is paused
 */
#define APP_STATUS_PAUSED	1

/**
 * Net App is started
 */
#define APP_STATUS_STARTED	2

/**
 * Status of Net App.
 */
extern int __app_status;

#endif /* __APP_STATUS_H__ */
