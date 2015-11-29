#include "fbe/fbe_types.h"
#include "fbe/fbe_eses.h"

/*!*************************************************************************
 * @fn elem_type_to_text
 *           (fbe_u8_t elem_type)
 **************************************************************************
 *
 *  @brief
 *      This function will get int element type and convert into text.
 *
 *  @param    fbe_u8_t - elem_type
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    29-Jan-2009: Dipak Patel- created.
 *
 **************************************************************************/
char * elem_type_to_text(fbe_u8_t elem_type)
{
    char * textElemType;
        
    switch(elem_type)
    {
        case SES_ELEM_TYPE_PS:
            textElemType = (char *)("(SES_ELEM_TYPE_PS )");
	        break;	

	    case SES_ELEM_TYPE_COOLING:
	   	    textElemType = (char *)( "(SES_ELEM_TYPE_COOLING )");
	        break;

	    case SES_ELEM_TYPE_TEMP_SENSOR:
            textElemType = (char *)( "(SES_ELEM_TYPE_TEMP_SENSOR )");
	        break;	

        case SES_ELEM_TYPE_ALARM:
            textElemType = (char *)( "(SES_ELEM_TYPE_ALARM )");
	        break;

	    case SES_ELEM_TYPE_ESC_ELEC:
            textElemType = (char *)( "(SES_ELEM_TYPE_ESC_ELEC )");
	        break;

	    case SES_ELEM_TYPE_UPS:
            textElemType = (char *)( "(SES_ELEM_TYPE_UPS )");
	        break;

	    case SES_ELEM_TYPE_DISPLAY:
            textElemType = (char *)( "(SES_ELEM_TYPE_DISPLAY )");
	        break;

	    case SES_ELEM_TYPE_ENCL:
            textElemType = (char *)( "(SES_ELEM_TYPE_ENCL )");
	        break;

	    case SES_ELEM_TYPE_LANG:
            textElemType = (char *)( "(SES_ELEM_TYPE_LANG )");
	        break;	
	    case SES_ELEM_TYPE_ARRAY_DEV_SLOT:
            textElemType = (char *)( "(SES_ELEM_TYPE_ARRAY_DEV_SLOT )");
	        break;

	    case SES_ELEM_TYPE_SAS_EXP:
            textElemType = (char *)( "(SES_ELEM_TYPE_SAS_EXP )");
	        break;

	    case SES_ELEM_TYPE_SAS_CONN:
            textElemType = (char *)( "(SES_ELEM_TYPE_SAS_CONN )");
	        break;

	    case SES_ELEM_TYPE_EXP_PHY:
            textElemType = (char *)( "(SES_ELEM_TYPE_EXP_PHY )");
	        break;
    	   
        default:
            textElemType = (char *)( "(Unknown )");
	        break;	
    }

	return (textElemType);   
}
