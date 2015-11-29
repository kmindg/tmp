#ifndef FBE_STAT_H
#define FBE_STAT_H

#include "fbe/fbe_atomic.h"
#include "fbe/fbe_time.h"

enum fbe_stat_magic_numbers_e{
    FBE_STAT_MAX_INTERVAL = 0x0800000, /* 1 day of 1000 I/O per second */
};

#define FBE_STATS_FLAG_RELIABLY_CHALLENGED    0x1
typedef fbe_u32_t fbe_stats_record_flags_t;

/* Values used for fbe_control_flag_t.  These flags define the actions that a specific
   error catagory will take when the threshold is reached.  */
#define FBE_STATS_CNTRL_FLAG_EOL                    0x1UL       //0x1   End of Life
#define FBE_STATS_CNTRL_FLAG_EOL_CALL_HOME          (0x1UL << 1)  //0x2
#define FBE_STATS_CNTRL_FLAG_DRIVE_KILL             (0x1UL << 2)  //0x4
#define FBE_STATS_CNTRL_FLAG_DRIVE_KILL_CALL_HOME   (0x1UL << 3)  //0x8
#define FBE_STATS_CNTRL_FLAG_DEFAULT                (FBE_STATS_CNTRL_FLAG_EOL | FBE_STATS_CNTRL_FLAG_DRIVE_KILL)
typedef fbe_u64_t fbe_stats_control_flag_t;

#define FBE_STAT_ACTIONS_MAX 32
#define FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES 20

typedef enum fbe_stat_action_flags_e{
    FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION             = 0x00000000,
    FBE_STAT_ACTION_FLAG_FLAG_RESET                 = 0x00000001,
    FBE_STAT_ACTION_FLAG_FLAG_POWER_CYCLE           = 0x00000002,
    FBE_STAT_ACTION_FLAG_FLAG_SPINUP                = 0x00010000,
    FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE           = 0x00100000,
    FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME = 0x00200000,
    FBE_STAT_ACTION_FLAG_FLAG_FAIL                  = 0x01000000,
    FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME        = 0x02000000,
    FBE_STAT_ACTION_FLAG_FLAG_EVENT                 = 0x04000000,  /* event which clients can register callbacks for */

    FBE_STAT_ACTION_FLAG_FLAG_LAST                  = 0xffffffff,
}fbe_stat_action_flags_t;

typedef fbe_u32_t fbe_stat_action_t;



/*!****************************************************************************
 *    
 * @struct fbe_stat_drive_cfg_action_s
 *  
 * @brief 
 *   Defines a DIEH configuration action for a given drive.
 ******************************************************************************/
typedef struct fbe_stat_drive_cfg_action_s{
    fbe_stat_action_t   flag;         /* flag that represents action.*/
    fbe_u32_t           ratio;        /* ratio to execute action*/
    fbe_u32_t           reactivate_ratio;  /* low watermark to reactivate ratio*/
}fbe_stat_drive_cfg_action_t;

/*!****************************************************************************
 *    
 * @struct fbe_stat_drive_cfg_actions_s
 *  
 * @brief 
 *   Defines a set of DIEH actions for a given drive.
 ******************************************************************************/
typedef struct fbe_stat_drive_cfg_actions_s{
    fbe_u32_t                      num;
    fbe_stat_drive_cfg_action_t    entry[FBE_STAT_ACTIONS_MAX];
}fbe_stat_drive_cfg_actions_t;


/*!****************************************************************************
 *    
 * @struct fbe_stat_actions_state_s
 *  
 * @brief 
 *   Defines the state of all DIEH actions for a given drive, which will be
 *   maintained by PDO.   Each bit in the bitmap will represet an index
 *   into the configuration record fbe_stat_drive_cfg_actions_s
 ******************************************************************************/
typedef struct fbe_stat_actions_state_s{
    fbe_u32_t     tripped_bitmap;  
}fbe_stat_actions_state_t;

/*!****************************************************************************
 *    
 * @struct fbe_stat_category_state_s
 *  
 * @brief 
 *   Defines the drives DIEH action state for each error category
 ******************************************************************************/
typedef struct fbe_stat_category_state_s{
    fbe_stat_actions_state_t io;       /* cumulative */
    fbe_stat_actions_state_t recovered;
    fbe_stat_actions_state_t media;
    fbe_stat_actions_state_t hardware;
    fbe_stat_actions_state_t link;
    fbe_stat_actions_state_t data;
    fbe_stat_actions_state_t healthCheck;
} fbe_stat_category_state_t;

/*!****************************************************************************
 *    
 * @struct fbe_drive_dieh_state_s
 *  
 * @brief 
 *   Defines all of the DIEH state related info
 ******************************************************************************/
typedef struct fbe_drive_dieh_state_s{
    fbe_stat_category_state_t           category;
    fbe_u32_t                           media_weight_adjust;    /* adjust percentage multiplier.  100 means stay the same. */
    fbe_bool_t                          eol_call_home_sent;     /* indicates if call home was already sent */
    fbe_bool_t                          kill_call_home_sent;
}fbe_drive_dieh_state_t;


typedef struct fbe_stat_scsi_execption_code_s{
    fbe_u8_t		    				sense_key;
    fbe_u8_t							asc;
    fbe_u8_t							ascq_start;
    fbe_u8_t							ascq_end;
}fbe_stat_scsi_sense_code_t;

typedef enum fbe_stat_weight_exception_type_e
{
    FBE_STAT_WEIGHT_EXCEPTION_INVALID,
    FBE_STAT_WEIGHT_EXCEPTION_CC_ERROR,
    FBE_STAT_WEIGHT_EXCEPTION_PORT_ERROR,
    FBE_STAT_WEIGHT_EXCEPTION_OPCODE,

}fbe_stat_weight_exception_type_t;

#define FBE_STAT_WTX_CHANGE_INVALID 0xFFFFFFFF

/*!****************************************************************************
 *    
 * @struct fbe_stat_weight_exception_code_s
 *  
 * @brief 
 *   Defines struct for holding weight change info.  The value of a specific
 *   opcode or scsi error weight can be changed by a specific percentage.  For
 *   instance a change value of 20 means change weight to 20% value.  To increase
 *   a weight then use a value > 100
 ******************************************************************************/
typedef struct fbe_stat_weight_exception_code_s{
    fbe_stat_weight_exception_type_t  type;
    union {
        fbe_stat_scsi_sense_code_t    sense_code;
        fbe_u32_t                     port_error;
        fbe_u8_t                      opcode;
    }u;
    fbe_u32_t                         change;  /* as percentage. 100 = no change.  20 = change weight to 20% of current value*/
}fbe_stat_weight_exception_code_t;

typedef struct fbe_stat_s{
    fbe_u64_t interval;
    fbe_u64_t weight;
    fbe_u32_t threshold;
    fbe_stats_control_flag_t  control_flag;  /* obsolete. replaced by .actions */
    fbe_stat_drive_cfg_actions_t  actions;
    fbe_stat_weight_exception_code_t weight_exceptions[FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES]; /* special cases to change weight */
}fbe_stat_t;

typedef struct fbe_drive_stat_s{
    fbe_stat_t   io_stat; /* Overall drive stat */
    fbe_stat_t   recovered_stat; /* Recovered errors stat */
    fbe_stat_t   media_stat; /* Media errors stat */
    fbe_stat_t   hardware_stat; /* Hardware errors stat */
    fbe_stat_t   link_stat; /* Link errors stat */
    fbe_stat_t   healthCheck_stat; /* HealthCheck errors stat */
    fbe_stat_t   reset_stat; /* reset stat */
    fbe_stat_t   power_cycle_stat; /* power cycle stat */
    fbe_stat_t   data_stat; /* power cycle stat */
    fbe_time_t   error_burst_delta;/*delta between errors to indicate error burst*/
    fbe_u32_t    error_burst_weight_reduce;/*percent of weight reduce for error burst*/
    fbe_u32_t    recovery_tag_reduce;/* percent of interval to reduce tag after recovery */
    fbe_stats_record_flags_t record_flags;
}fbe_drive_stat_t;

typedef struct fbe_port_stat_s{
    fbe_stat_t   io_stat; /* Overall drive stat */
    fbe_stat_t   enclosure_stat;/*statistics for enclosure, there would be 8 of these for the port error itself*/
}fbe_port_stat_t;

/* This structure is directly used by Disk Collect.  Any changes here
   will directly affect the data written to disk and any post processing tools.
   Talk to Disk Collect SME before changing.
*/
typedef struct fbe_drive_error_s{
    fbe_atomic_t io_counter;
    fbe_u64_t    io_error_tag; /* Overall drive error tag */
    fbe_u64_t    recovered_error_tag; /* Recovered errors error tag */
    fbe_u64_t    media_error_tag; /* Media errors error tag */
    fbe_u64_t    hardware_error_tag; /* Hardware errors error tag */
    fbe_u64_t    link_error_tag; /* Link errors error tag */
    fbe_u64_t    healthCheck_error_tag; /* HealthCheck errors error tag*/
    fbe_u64_t    reset_error_tag; /* reset error tag */
    fbe_u64_t    power_cycle_error_tag; /* power cycle error tag */
    fbe_u64_t    data_error_tag; /* error reported by upper level (raid for example) */

    fbe_time_t   last_error_time;
}fbe_drive_error_t;



#endif /* FBE_STAT_H */
