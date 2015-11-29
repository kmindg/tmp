#ifndef FBE_POWER_SAVE_INTERFACE_H
#define FBE_POWER_SAVE_INTERFACE_H

/*!*******************************************************************
 * @enum fbe_power_save_state_t
 *********************************************************************
 * @brief
 *  This structure is used to show what stage of power saving the objetc is in
 *
 * @ingroup fbe_api_power_save_interface
 *********************************************************************/
typedef enum fbe_power_save_state_e{
	FBE_POWER_SAVE_STATE_INVALID,/*!<Invalid state */
	FBE_POWER_SAVE_STATE_NOT_SAVING_POWER,/*!< Object is up and running and not saving power */
	FBE_POWER_SAVE_STATE_SAVING_POWER,/*!< Object is in hibernation, saving power */
	FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE,/*!< Objects is starting to save power, but is not saving yet */
	FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE/*!< Object starts to get out of power saving*/
}fbe_power_save_state_t;

#endif /*FBE_POWER_SAVE_INTERFACE_H*/
