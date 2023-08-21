/**
 * Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/** @file audcalparam_api.h */
#ifndef AUDCALPARAM_API_H
#define AUDCALPARAM_API_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief audcal api session handle
 *
 * Usage example:(See example for all commands in test/testapi.c)
 * Code snippet:
 *    audcalparam_session_t *session = NULL;
 *    if (audcalparam_session_init(&session, cfg_file_name) != AUDCALPARAM_OK)
 *       return;
 *     // ... use audcal api
 *    fprintf(stdout,"***Test for Module_param***\n");
 *    mpcfg.base_cfg.cache=cache_flag;
 *    mpcfg.module_id=0x00010C37;
 *    mpcfg.param_id=0x00010C38;
 *    int32_t gain;
 *    uint8_t buf[64];
 *    uint8_t *pbuf=buf;
 *    int32_t len=sizeof(buf);
 *    r = audcalparam_cmd_module_param(h, "copp_general_playback1",  AUDCALPARAM_GET, pbuf, &len, &mpcfg);
 *    audcalparam_session_deinit(session);
 *    session = NULL;
 * End code snippet
 * Config file consists of Usecase section description
 *     "usecase_map":[
 *     {
 *       "name":"general_playback",
 *       "acdb_dev_id":41,
 *       "app_type":69936
 *     },
 * and Commnand section:
 * { "type":"Module_param",
 *        "instances":[
 *         { "name":"copp_general_playback1",
 *           "usecase":"general_playback",
 *           "topo_type":"COPP",
 *           "instance_id":"0" 
 *          },*
 * Each command instance has name field which is used as input parameter when cmd is called.
 * usecase in command instance links it to the particular usecase from usecase_map section*/

 
typedef struct audcalparam_session_t audcalparam_session_t;

/** @brief Error codes */
typedef enum {
    AUDCALPARAM_OK=0,
    AUDCALPARAM_SESINITERR,     /**< init error */
    AUDCALPARAM_SERVCONERR,     /**< calibration service connection error */
    AUDCALPARAM_CMDALLOCERR,    /**< memory allocation error */
    AUDCALPARAM_UCALLOCERR,     /**< usecase allocation error */
    AUDCALPARAM_HDLERR,         /**< handle error*/
    AUDCALPARAM_CMDTYPERR,      /**< command type error */
    AUDCALPARAM_CMDINSTERR,     /**< command instance error */
    AUDCALPARAM_CMDDIRERR,      /**< command direction error */
    AUDCALPARAM_CMDCFGERR,      /**< command config error */
    AUDCALPARAM_CMDPARAMERR,    /**< parameter error*/
    AUDCALPARAM_CMDPARSERR,     /**< parsing error */
    AUDCALPARAM_CMDCFGFILEERR,  /**< config file missing */
    AUDCALPARAM_CMDUSECASEERR   /**< use case not valid or active*/
} audcalparam_err_t;

/** @brief control command direction */
typedef enum {
    AUDCALPARAM_GET=0,      /**< read parameter */
    AUDCALPARAM_SET         /**< write parameter */
} audcalparam_cmd_dir_t;

/** @brief bmt (bass mid treble) command */
typedef struct {
    int32_t bass;   /**< setting for "bass" filter step 
                    within +/- num_steps provided in BMT calibration*/
    int32_t mid;    /**< setting for "middle" filter step
                    within +/- num_steps provided in BMT calibration*/
    int32_t treble; /**< setting for "treble" filter 
                     within +/- num_steps provided in BMT calibration*/
    int32_t filter_mask;    /**< bitmask for enabling filter:
                            bass=0x1, mid=0x2, treble=0x4 */
} audcalparam_cmd_bmt_t;


/** @brief fade & balance command */
typedef struct {
    int32_t balance;       /**< setting for "balance" step 
                     within +/- num_steps provided in FNB calibration*/
    int32_t fade;          /**< setting for "fading" step
                     within +/- num_steps provided in FNB calibration*/
    uint32_t filter_mask; /**< bitmask for enabling filter: 
                               balance=0x1, fade=0x2*/
} audcalparam_cmd_fnb_t;


/** @brief Delay command corresponds to the delay module AUDPROC_MODULE_ID_DELAY  
(Id=0x00010772) and parameter AUDPROC_PARAM_ID_MCHAN_DELAY (Id=0x00010941) */
#define AUDCALPARAM_CMD_DELAY_EL_NUM_MAX (63)
typedef struct {
    uint32_t num_el;       /**< Number of configs to apply the delay 
                           range 1..AUDCALPARAM_CMD_DELAY_EL_NUM_MAX 
                           Run always GET first with num_el= max in the range 
                           and enough memory allocated */
    uint32_t *mask_lsb;    /**< Lower 32 bits of the mask that indicates the corresponding channel
                              whose delay is to be set.
                                - Set the bits corresponding to 1 to 31 channels of standard channel
                                  mapping (channels are mapped per standard channel mapping)
                                - Position of the bit to set 1 (left shift) (channel_map) */
    uint32_t *mask_msb;	  /**< Upper 32 bits of the mask that indicates the corresponding channel
                                whose delay is to be set.
                                - Set the bits corresponding to 32 to 63 channels of standard channel
                                  mapping (channels are mapped per standard channel mapping)
                                - Position of the bit to set  1 (left shift) (channel_map - 32) */
    uint32_t *value;       /**<  The amount of delay must be greater than 0. If the value is zero, 
                           this module is disabled. The actual resolution of the delay is limited 
                           by the period of a single audio sample.*/
} audcalparam_cmd_delay_t;
#define AUDCALPARAM_CMD_MUTE_EL_NUM_MAX (32)
/** @brief Multichannel Mute command AUDPROC_PARAM_ID_MULTICHANNEL_MUTE_V2 
(Id: 0x00010948) for module AUDPROC_MODULE_ID_VOL_CTRL (Id:0x00010BFE) */
typedef struct {
    uint32_t num_el;       /**< number of channels to apply the parameter.
                           Range 1...AUDCALPARAM_CMD_MUTE_EL_NUM_MAX 
                           Run always GET first with num_el= max in the range 
                           and enough memory allocated */
    uint32_t *type;        /**< Type of channels. */
    uint32_t *value;       /**< array for num_el mute values. 
                           0: No mute, 1:Mute*/
} audcalparam_cmd_mute_t;

#define AUDCALPARAM_CMD_GAIN_EL_NUM_MAX (32)
/** @brief Multichannel Gain command for AUDPROC_MODULE_ID_VOL_CTRL (Id:0x00010BFE)
AUDPROC_PARAM_ID_MULTICHANNEL_GAIN_V2 (Id:0x00010947)*/
typedef struct {
    uint32_t num_el;       /**< number of channels to apply the parameter 
                           Range 1...AUDCALPARAM_CMD_GAIN_EL_NUM_MAX
                           Run always GET first with num_el= max in the range 
                           and enough memory allocated */
    uint32_t *type;        /**< Type of channels. */
    uint32_t *value;       /**< array for num_el gain values. Format
                           Q28: 0x10000000 corresponds to linear factor 1*/
} audcalparam_cmd_gain_t;

#define AUDCALPARAM_CMD_VOL_IDX_EL_NUM_MAX (1)
/** @brief Master volume index command (single channel)
AUDPROC_PARAM_ID_VOL_CTRL_MASTER_GAIN (Id:0x00010BFF) for 
module  AUDPROC_MODULE_ID_VOL_CTRL (Id:0x00010BFE)*/
typedef struct {
    uint32_t num_el;       /**< number of channels to apply the parameter 
                           Currently  is 1 supported*/
    uint32_t *value;       /**< array for num_el volumn-index values. 
                           Maximum index =31! An index is used to pick 
                           up value from cfg file section param for 
                           Volume_dx commands instance */
} audcalparam_cmd_volume_idx_t;

/** @brief base configuration */
typedef struct {
    bool cache;                /**< Flag to set the destination of the operation.
                                If the flag is True sets/gets parameter in ACDB cache
                                in heap ONLY not affecting the running use case. On 
                                usecase activation values from cache are applied to DSP. 
                                If the flag is False the running use case is affected, 
                                without storing to the cache in heap. 
                                The value in cache is valid across usecases until reboot */
    uint32_t sampling_rate;    /**< sampling rate (Hz) are only required
                                when stored to the acdb cache in heap. It is not required
                                for POPP type topology in acdb heap. */
} audcalparam_cmd_base_cfg_t;

/** @brief module configuration */
typedef struct{
   	audcalparam_cmd_base_cfg_t base_cfg;    /**< base configuration */
    uint32_t module_id;                     /**< Module ID */
    uint16_t instance_id;                   /**< Instance ID (reserved
                                             for future use with MDF) */
    uint32_t param_id;                      /**< Parameter ID */
} audcalparam_cmd_module_param_cfg_t;

/**
 * Initialize audio parameter calibration session:
 * - allocate memory dynamically
 * - parse the given configuration file with the commands settings.
 * - provide access to the calibration service functionality.
 *
 * @param h                 session to be initialize
 * @param cfg_file_name     config file to parse the commands configuration.
 *                          If empty string is provided default config is used.
 * @return Error code
 */
audcalparam_err_t audcalparam_session_init(audcalparam_session_t **h,
                                           char *cfg_file_name,
                                           char *snd_card_name);
/**
 * De-init audio parameter calibration session:
 * - free dynamically allocated memory.
 *
 * @param  h                session to be deinitialized
 * @return Error code
 */

audcalparam_err_t audcalparam_session_deinit(audcalparam_session_t *h);

/**
 * Setting/getting of bass-mid-treble parameter.
 *
 * @param h                     session handle
 * @param cmd_instance_name     command instance name to select
 *                              which command instance to run from config file
 * @param dir                   direction: set or get the calibration parameter
 * @param value                 BMT value to set/get
 * @param cfg                   user defined command config
 * @return Error code
 */

audcalparam_err_t audcalparam_cmd_bmt(audcalparam_session_t *h,
                                      char *cmd_instance_name,
                                      audcalparam_cmd_dir_t dir,
                                      audcalparam_cmd_bmt_t *value,
                                      audcalparam_cmd_base_cfg_t *cfg);

/**
 * Setting/getting delay parameter.
 *
 * @param h                     session handle
 * @param cmd_instance_name     command instance name to select
 *                              which command instance to run from config file
 * @param dir                   direction: set or get the calibration parameter
 * @param value                 Delay value to set/get
 * @param cfg                   User defined command config
 * @return Error code
 */

audcalparam_err_t audcalparam_cmd_delay(audcalparam_session_t *h,
                                        char *cmd_instance_name,
                                        audcalparam_cmd_dir_t dir,
                                        audcalparam_cmd_delay_t *value,
                                        audcalparam_cmd_base_cfg_t *cfg);


/**
 * Setting/getting mute paramter.
 *
 * @param h                     session handle
 * @param cmd_instance_name     command instance name to select
 *                              which command instance to run from config file
 * @param dir                   direction: set or get the calibration parameter
 * @param value                 Mute flag to set/get
 * @param cfg                   User defined command config
 * @return Error code
 */

audcalparam_err_t audcalparam_cmd_mute(audcalparam_session_t *h,
                                       char *cmd_instance_name,
                                       audcalparam_cmd_dir_t dir,
                                       audcalparam_cmd_mute_t *value,
                                       audcalparam_cmd_base_cfg_t *cfg);
/**
* Setting/getting fade and balance parameter.
*
* @param h                     session handle
* @param cmd_instance_name     command instance name to select
*                              which command instance to run from config file
* @param dir                   direction: set or get the calibration parameter
* @param value                 Fade and Balance values to set/get
* @param cfg                   User defined command config
* @return Error code
*/
audcalparam_err_t audcalparam_cmd_fnb(audcalparam_session_t *h,
                                          char* cmd_instance_name,
                                          audcalparam_cmd_dir_t dir,
                                          audcalparam_cmd_fnb_t* value,
                                          audcalparam_cmd_base_cfg_t* cfg);


/**
 * Setting/getting volume index paramter.
 *
 * @param h                     session handle
 * @param cmd_instance_name     command instance name to select
 *                              which command instance to run from config file
 * @param dir                   direction: set or get the calibration parameter
 * @param volume_idx            Volume index value to set/get
 * @param cfg                   User defined command config
 * @return Error code
 */

audcalparam_err_t audcalparam_cmd_volume_idx(audcalparam_session_t *h,
                                             char *cmd_instance_name,
                                             audcalparam_cmd_dir_t dir,
                                             audcalparam_cmd_volume_idx_t *volume_idx,
                                             audcalparam_cmd_base_cfg_t *cfg);

/**
 * Setting/getting gain paramter.
 *
 * @param h                     session handle
 * @param cmd_instance_name     command instance name to select
 *                              which command instance to run from config file
 * @param dir                   direction: set or get the calibration parameter
 * @param value                 Gain value to set/get
 * @param cfg                   User defined command config
 * @return Error code
 */

audcalparam_err_t audcalparam_cmd_gain(audcalparam_session_t *h,
                                       char *cmd_instance_name,
                                       audcalparam_cmd_dir_t dir,
                                       audcalparam_cmd_gain_t *value,
                                       audcalparam_cmd_base_cfg_t *cfg);

/**
 * Setting/getting module paramter.
 *
 * @param h                     session handle
 * @param cmd_instance_name     command instance name to select
 *                              which command instance to run from config file
 * @param dir                   direction: set or get the calibration parameter
 * @param pbuf                  Input/Output byte buffer pointer for command
 * @param buf_len               Byte buffer length. Returns output buffer length.
 * @param cfg                   Command config
 * @return Error code
 */
audcalparam_err_t audcalparam_cmd_module_param(audcalparam_session_t *h,
                                               char *cmd_instance_name,
                                               audcalparam_cmd_dir_t dir,
                                               uint8_t *pbuf,
                                               uint32_t *pbuf_len,
                                               audcalparam_cmd_module_param_cfg_t *cfg);

audcalparam_err_t audcalparam_cmd_set_print(audcalparam_session_t *h);

audcalparam_err_t audcalparam_usecase_list_print(audcalparam_session_t *h);


#endif