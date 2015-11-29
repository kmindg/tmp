#ifndef DH_EXPORT
#define DH_EXPORT 0x00000001         /* from common dir */

/* Enumeration of the all the possible standby states
 * that a drive can be in.
 */
typedef enum _DH_STANDBY_STATE
{
    DH_DRIVE_NORMAL = 0,       /* Drive is active */
    DH_DRIVE_SPUNDOWN,         /* Drive is currently spundown */
    DH_DRIVE_ENTERING_STANDBY, /* Drive is entering a standby mode */
    DH_DRIVE_EXITING_STANDBY,  /* Drive is exiting a standby mode */
    DH_DRIVE_DEAD,             /* Drive is dead */
}
DH_STANDBY_STATE;


#endif
