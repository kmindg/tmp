#include "terminator_virtual_phy.h"
#include "terminator_login_data.h"
#include "terminator_sas_io_api.h"
#include "terminator_enclosure.h"
#include "fbe_terminator_common.h"

#define DEFAULT_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL 0
#define DEFAULT_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL 0

/**********************************/
/*        local variables         */
/**********************************/

/****************************************************
The order of arrangement of ESES enclosure attributes
for each enclosure type.
1. encl_type
2. max_drive_count
3. max_phy_count
4. max_encl_conn_count
5. max_lcc_conn_count
6. max_port_conn_count
7. max_single_lane_port_conn_count
8. max_ps_count
9. max_cooling_elem_count
10. max_temp_sensor_elems_count
11. drive_slot_to_phy_map
12. max display character_count
13. max lccs
14. max ee lccs
15. max ext cooling elements 
16. max_conn_id_count 
17. Invidiual connector to phy mapping 
NOTE: 255 is used in the mapping array if the mapping is
      undefined. (Ex: There are no expansion port connectors
      for Magnum).
*****************************************************/

#define SELECT_ENCL_ESES_INFO_TABLE(spid)     ((!(spid)) ? a_encl_eses_info_table : b_encl_eses_info_table)

terminator_sas_encl_eses_info_t a_encl_eses_info_table[] = {
    {FBE_SAS_ENCLOSURE_TYPE_BULLET, 15,    36,     20,     10,      5,      4,      2,      4,      1, 0, 0, 0, 0, 2, 0, 0,0, 2, 0, 0},

    {FBE_SAS_ENCLOSURE_TYPE_VIPER,  15,    36,     20,     10,      5,      4,      2,      4,      1,
        {20, 22, 23, 21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9},  //drive_slot_to_phy_map[]
        3, 2, 0, 0, 0, 2,
        {   
            {4, 5, 6, 7},  // Connector Id 0 individual connector to phy map
            {0, 1, 2, 3},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_PINECONE,  12,    24,     20,     10,      5,      4,      2,      4,      0,
        {19, 20, 21, 22, 15, 18, 23, 16, 17, 14, 13, 12},  //drive_slot_to_phy_map[]
        3, 2, 0, 0, 0, 2,
        {   
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_MAGNUM, 15,    24,     10,     5,      5,      4,      0,      0,      0,
        {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}, 0, 2, 0 ,0, 0, 2,
        {   
            {0, 1, 2, 3},          // Connector Id 0 individual connector to phy map
            {255, 255, 255, 255},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
        
    {FBE_SAS_ENCLOSURE_TYPE_BUNKER, 15,    36,     20,     10,      5,      4,      0,      0,      0,
        {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, 0, 2, 0 ,0,0, 2,
        {
            {1, 0, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    }, 
        
    {FBE_SAS_ENCLOSURE_TYPE_CITADEL, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0,0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
        
    {FBE_SAS_ENCLOSURE_TYPE_DERRINGER, 25,    36,     20,     10,      5,      4,      2,      4,      1,
        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35},      
         3, 2, 0, 0, 0, 2,
         {
            {4,5,6,7},    // Connector Id 0 individual connector to phy map
            {0, 1, 2, 3}, // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },     
         
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,  0,    24,     60,     30,      5,      4,      2,      
    7,      1,
        {0},              //drive_slot_to_phy_map[]
        3, 4 , 2, 3, 0, 6,
        {
            {20, 21, 22, 23},  // Connector Id 0 individual connector to phy map
            {12, 13, 14, 15},  // Connector Id 1 individual connector to phy map
            {16, 17, 18, 19},  // Connector Id 2 individual connector to phy map
            {8, 9, 10, 11},    // Connector Id 3 individual connector to phy map
            {0, 1, 2, 7},      // Connector Id 4 individual connector to phy map
            {3, 4, 5, 6}       // Connector Id 5 individual connector to phy map
        },
        {0, 0, 0, 0, 0, 30}    // Connector id to drive start slot
    },
    
    /* Below mapping is from EE at A side only as 2 EE have different mapping for simulation purpose */ 
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE,  30,    36,     10,     5,      5,      4,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {6,  1,  2,  5, 9, 8, 3,  7,  15, 18, 22, 27, 0,  4, 12, 20, 23, 28, 11, 10, 13, 19, 24, 30, 14, 16, 17, 21, 26, 25},
        0, 2, 0, 0, 0, 1,
        {
            {31, 32, 33, 34},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },        
        
    {FBE_SAS_ENCLOSURE_TYPE_FALLBACK, 25,    36,     20,     10,      5,      4,      0,     3,      1,
        {32, 31, 29, 25, 22, 18, 17, 19, 20, 21, 23, 24, 26, 27, 28, 30, 16, 15, 14, 13, 12, 11, 10, 9, 8}, 0, 2, 0, 0, 3, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
        
    {FBE_SAS_ENCLOSURE_TYPE_BOXWOOD, 12,    36,     20,     10,      5,      4,      0,      0,      0,
        {15, 16, 17, 18, 11, 14, 19, 12, 13, 10, 9, 8}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_KNOT, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_STEELJAW, 12,    36,     20,     10,      5,      4,      0,      0,      0,
        {19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_RAMHORN, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
    {FBE_SAS_ENCLOSURE_TYPE_ANCHO,	15,    36,	   20,	   10,		5,		4,		2,		4,		1,
        {23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  5},  //drive_slot_to_phy_map[]
         3, 2, 0, 0, 0, 2,
         {	
					{0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
					{6, 7, 8, 9},  // Connector Id 1 individual connector to phy map
					{0},
					{0}, 
					{0},
					{0}
			},
		{0}
    },
    
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP,  0,    36,     88,     44,      6,      5,      4,      
    10,      2,
        {0},              //drive_slot_to_phy_map[]
        3, 2, 1, 10, 0, MAX_POSSIBLE_CONNECTOR_ID_COUNT,
        {
            {3, 2, 1, 0},           // Connector Id 0 individual connector to phy map
            {7, 6, 5, 4},           // Connector Id 1 individual connector to phy map
            {8, 9, 10, 11, 12},     // Connector Id 2 individual connector to phy map
            {13, 14, 15, 16, 17},   // Connector Id 3 individual connector to phy map
            {18, 19, 20, 21, 22},   // Connector Id 4 individual connector to phy map
            {23, 24, 25, 26, 27},   // Connector Id 5 individual connector to phy map
            {31, 30, 29, 28},       // Connector Id 6 individual connector to phy map
            {35, 34, 33, 32}        // Connector Id 7 individual connector to phy map
        },
        {0, 0, 0, 30, 60, 90, 0, 0}    // Connector id to drive start slot
    },

    /* Below mapping is from EE at A side only. B side has different mapping for simulation purpose */ 
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP,  30,    36,     12,     6,      6,      5,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {12, 14, 16, 18, 20, 13, 15, 17, 19, 21, 7, 9, 11, 23, 25, 6, 8, 10, 22, 24, 1, 3, 5, 27, 29, 0, 2, 4, 26, 28},
        0, 2, 0, 0, 0, 1,
        {
            {31, 32, 33, 34, 35},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },   
    
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP,  0,    24,     56,     28,      9,      8,      2,      
    10,      1,
        {0},              //drive_slot_to_phy_map[]
        3, 2, 1, 3, 0, MAX_POSSIBLE_CONNECTOR_ID_COUNT,
        {
            {2, 3, 1, 0},           // Connector Id 0 individual connector to phy map
            {6, 7, 5, 4},           // Connector Id 1 individual connector to phy map
            {10, 11, 9, 8},     // Connector Id 2 individual connector to phy map
            {22, 23, 21, 20},   // Connector Id 3 individual connector to phy map
            {12, 13, 14, 15, 16, 17, 18, 19},  // Connector Id 4 individual connector to phy map
            {0, 0, 0, 0},   // Connector Id 5 individual connector to phy map
        },
        {0, 0, 0, 0, 0, 0, 0, 0}    // Connector id to drive start slot
    },

    /* Below mapping is from EE at A side only. B side has different mapping for simulation purpose */ 
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP,  60,    68,     16,     8,      8,      8,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {46, 47, 42, 43, 38, 39, 32, 33, 26, 27, 22, 23, 66, 67, 62, 63, 58, 59, 14, 15, 10, 11, 6, 7, 54, 55, 50, 40, 41, 34, 35, 28, 29, 51, 18, 19, 64, 60, 56, 52, 48, 44, 24, 20, 16, 12, 8, 4, 65, 61, 57, 53, 49, 45, 25, 21, 17, 13, 9, 5},
        0, 2, 0, 0, 0, 1,
        {
            {3, 2, 1, 0, 30, 31, 36, 37},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },        
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,  0,    68,     76,     38,      9,      8,      4,      
    10,      2,
        {0},              //drive_slot_to_phy_map[]
        3, 2, 1, 10, 0, 6,
        {
            {3, 2, 1, 0},           // Connector Id 0 individual connector to phy map
            {7, 6, 5, 4},           // Connector Id 1 individual connector to phy map
            {31, 30, 29, 28},       // Connector Id 2 individual connector to phy map
            {35, 34, 33, 32},        // Connector Id 3 individual connector to phy map
            {8, 9, 10, 11, 12,13, 14, 15}, // Connector Id 4 individual connector to phy map
            {16, 17, 18, 19, 20, 21, 22, 23}   // Connector Id 5 individual connector to phy map
        },
        {0, 0, 0, 0, 0, 60}    // Connector id to drive start slot
    },

    /* Below mapping is from EE at A side only. B side has different mapping for simulation purpose */ 
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP,  60,    68,     12,     6,      9,      8,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {62, 56, 50, 44, 38, 32, 26, 20, 14, 8, 63, 57, 51, 45, 39, 33, 27, 21, 15, 9, 64, 58, 52, 46, 40, 34, 28, 22, 16, 10, 
         65, 59, 53, 47, 41, 35, 29, 23, 17, 11, 66, 60, 54, 48, 42, 36, 30, 24, 18, 12, 67, 61, 55, 49, 43, 37, 31, 25, 19, 13},
        0, 2, 0, 0, 0, 1,
        {
            {0, 1, 2, 3, 4, 5, 6, 7},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },    

    {FBE_SAS_ENCLOSURE_TYPE_RHEA, 12,    36,     20,     10,      5,      4,      0,      0,      0,
        {15, 2, 5, 32, 0, 3, 6, 31, 1, 4, 7, 30}, 0, 2, 0, 0, 0, 2,
        {
            {16, 17, 18, 19},  // Connector Id 0 individual connector to phy map
            {20, 21, 22, 23},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_MIRANDA, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {16, 17, 18, 19},  // Connector Id 0 individual connector to phy map
            {20, 21, 22, 23},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
// ENCL_CLEANUP - Moon DPE need real values        
    {FBE_SAS_ENCLOSURE_TYPE_CALYPSO, 25,    36,     20,     10,      5,      4,      0,      6,      4,
        {8, 9, 10, 11, 12, 13, 14, 15, 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_TABASCO, 25,    36,     20,     10,      5,      4,      2,      4,      1,
        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35},      
         3, 2, 0, 0, 0, 2,
         {
            {0, 1, 2, 3},    // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},    // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_LAST,   0,     0,      0,      0,      0,      0,      0,      0 }
};
// Duplicated for B side for viper enclosure other enclosure might not be correct
terminator_sas_encl_eses_info_t b_encl_eses_info_table[] = {
    {FBE_SAS_ENCLOSURE_TYPE_BULLET, 15,    36,     20,     10,      5,      4,      2,      4,      1, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0},

    {FBE_SAS_ENCLOSURE_TYPE_PINECONE,  12,    24,     20,     10,      5,      4,      2,      4,      0,
        {12, 13, 14, 17, 16, 23, 18, 15, 22, 21, 20, 19},  //drive_slot_to_phy_map[]
        3, 2, 0, 0, 0, 2,
        {   
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },


    {FBE_SAS_ENCLOSURE_TYPE_VIPER,  15,    36,     20,     10,      5,      4,      2,      4,      1,
        {9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 23, 22, 20}, 3, 2, 0, 0, 0, 2,
        {
            {4, 5, 6, 7}, // Connector Id 0 individual connector to phy map
            {0, 1, 2, 3}, // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
 
    {FBE_SAS_ENCLOSURE_TYPE_MAGNUM, 15,    24,     10,     5,      5,      4,      0,      0,      0,
        {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}, 0, 2, 0, 0, 0, 2,
        {   
            {0, 1, 2, 3},   // Connector Id 0 individual connector to phy map
            {255, 255, 255, 255},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
        
    {FBE_SAS_ENCLOSURE_TYPE_BUNKER, 15,    36,     20,     10,      5,      4,      0,      0,      0,
        {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}, 0, 2, 0, 0, 0, 2,
        {
            {1, 0, 2, 3}, // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7}, // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    }, 
        
    {FBE_SAS_ENCLOSURE_TYPE_CITADEL, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {1, 0, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
        
    {FBE_SAS_ENCLOSURE_TYPE_DERRINGER, 25,    36,     20,     10,      5,      4,      2,      4,      1,
        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35},      
         3, 2, 0, 0, 0, 2,
         {
            {4,5,6,7},     // Connector Id 0 individual connector to phy map
            {0, 1, 2, 3},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    }, 

    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,  0,    24,     60,     30,      5,      4,      2,      
    7,      1,
        {0},              //drive_slot_to_phy_map[]
        3, 4 , 2, 3, 0, 6,
        {
            {20, 21, 22, 23},  // Connector Id 0 individual connector to phy map
            {12, 13, 14, 15},  // Connector Id 1 individual connector to phy map
            {16, 17, 18, 19},  // Connector Id 2 individual connector to phy map
            {8, 9, 10, 11},    // Connector Id 3 individual connector to phy map
            {0, 1, 2, 7},      // Connector Id 4 individual connector to phy map
            {3, 4, 5, 6}       // Connector Id 5 individual connector to phy map
        },
        {0, 0, 0, 0, 0, 30}    // Connector id to drive start slot
    },
    
    /* Below mapping is from EE at B side only as 2 EE have different mapping for simulation purpose */ 
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE,  30,    36,     10,     5,      5,      4,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {12, 11, 10, 5, 0, 6, 20, 17, 14, 7,  2,  3,  19, 18, 9, 8,  4,  1,  23, 22, 21, 13, 15, 16, 27, 28, 24, 26, 25, 30},
        0, 2, 0, 0, 0, 1,
        {
            {31, 32, 33, 34},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
         
    {FBE_SAS_ENCLOSURE_TYPE_FALLBACK, 25,    36,     20,     10,      5,      4,      0,      3,      1,
        {32, 31, 30, 29, 28, 27, 26, 25, 16, 17, 19, 20, 22, 23, 24, 21, 18, 15, 14, 13, 12, 11, 10, 9, 8}, 0, 2, 0, 0, 3, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
        
    {FBE_SAS_ENCLOSURE_TYPE_BOXWOOD, 12,    36,     20,     10,      5,      4,      0,      0,      0,
        {15, 16, 17, 18, 11, 14, 19, 12, 13, 10, 9, 8}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_KNOT, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_STEELJAW, 12,    36,     20,     10,      5,      4,      0,      0,      0,
        {19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_RAMHORN, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {8, 9, 10, 11, 12, 13, 14, 15, 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_ANCHO,  15,    36,     20,     10,      5,      4,      2,      4,      1,
        {5,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23}, 3, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},    // Connector Id 0 individual connector to phy map
            {6, 7, 8, 9},    // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP,  0,    36,     88,     44,      6,      5,      4,      
    10,      2,
        {0},              //drive_slot_to_phy_map[]
        3, 2, 1, 10, 0, MAX_POSSIBLE_CONNECTOR_ID_COUNT,
        {
            {3, 2, 1, 0},           // Connector Id 0 individual connector to phy map
            {7, 6, 5, 4},           // Connector Id 1 individual connector to phy map
            {8, 9, 10, 11, 12},     // Connector Id 2 individual connector to phy map
            {13, 14, 15, 16, 17},   // Connector Id 3 individual connector to phy map
            {18, 19, 20, 21, 22},   // Connector Id 4 individual connector to phy map
            {23, 24, 25, 26, 27},   // Connector Id 5 individual connector to phy map
            {31, 30, 29, 28},       // Connector Id 6 individual connector to phy map
            {35, 34, 33, 32}        // Connector Id 7 individual connector to phy map
        },
        {0, 0, 0, 30, 60, 90, 0, 0}    // Connector id to drive start slot
    },

    {FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP,  30,    36,     12,     6,      6,      5,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {12, 14, 16, 18, 20, 13, 15, 17, 19, 21, 11, 22, 23, 25, 27, 9, 10, 24, 26, 28, 7, 8, 29, 31, 33, 5, 6, 30, 32, 34},
        0, 2, 0, 0, 0, 1,
        {
            {0, 1, 2, 3, 4},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },       

    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP,  0,    24,     56,     28,      9,      8,      2,      
    10,      1,
        {0},              //drive_slot_to_phy_map[]
        3, 2, 1, 3, 0, MAX_POSSIBLE_CONNECTOR_ID_COUNT,
        {
            {2, 3, 1, 0},           // Connector Id 0 individual connector to phy map
            {6, 7, 5, 4},           // Connector Id 1 individual connector to phy map
            {10, 11, 9, 8},     // Connector Id 2 individual connector to phy map
            {22, 23, 21, 20},   // Connector Id 3 individual connector to phy map
            {12, 13, 14, 15, 16, 17, 18, 19},  // Connector Id 4 individual connector to phy map
            {0, 0, 0, 0},   // Connector Id 5 individual connector to phy map
        },
        {0, 0, 0, 0, 0, 0, 0, 0}    // Connector id to drive start slot
    },

    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP,  60,    68,     16,     8,      8,      8,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {46, 47, 42, 43, 38, 39, 32, 33, 26, 27, 22, 23, 66, 67, 62, 63, 58, 59, 14, 15, 10, 11, 6, 7, 54, 55, 50, 40, 41, 34, 35, 28, 29, 51, 18, 19, 64, 60, 56, 52, 48, 44, 24, 20, 16, 12, 8, 4, 65, 61, 57, 53, 49, 45, 25, 21, 17, 13, 9, 5},
        0, 2, 0, 0, 0, 1,
        {
            {3, 2, 1, 0, 30, 31, 36, 37},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },        

    {FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,  0,    68,     76,     38,      9,      8,      4,      
    10,      2,
        {0},              //drive_slot_to_phy_map[]
        3, 2, 1, 10, 0, 6,
        {
            {3, 2, 1, 0},           // Connector Id 0 individual connector to phy map
            {7, 6, 5, 4},           // Connector Id 1 individual connector to phy map
            {31, 30, 29, 28},       // Connector Id 2 individual connector to phy map
            {35, 34, 33, 32},        // Connector Id 3 individual connector to phy map
            {8, 9, 10, 11, 12,13, 14, 15}, // Connector Id 4 individual connector to phy map
            {16, 17, 18, 19, 20, 21, 22, 23}   // Connector Id 5 individual connector to phy map
        },
        {0, 0, 0, 0, 0, 60}    // Connector id to drive start slot
    },

    /* Below mapping is from EE at A side only. B side has different mapping for simulation purpose */ 
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP,  60,    68,     12,     6,      9,      8,      0,      0,      0,
        // Below is drive_slot_to_phy_map[]
        {62, 56, 50, 44, 38, 32, 26, 20, 14, 8, 63, 57, 51, 45, 39, 33, 27, 21, 15, 9, 64, 58, 52, 46, 40, 34, 28, 22, 16, 10, 
         65, 59, 53, 47, 41, 35, 29, 23, 17, 11, 66, 60, 54, 48, 42, 36, 30, 24, 18, 12, 67, 61, 55, 49, 43, 37, 31, 25, 19, 13},
        0, 2, 0, 0, 0, 1,
        {
            {0, 1, 2, 3, 4, 5, 6, 7},  // Connector Id 0 individual connector to phy map
            {0},
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },    

    {FBE_SAS_ENCLOSURE_TYPE_RHEA, 12,    36,     20,     10,      5,      4,      0,      0,      0,
        {32, 7, 4, 15, 31, 6, 3, 0, 30, 5, 2, 1}, 0, 2, 0, 0, 0, 2,
        {
            {16, 17, 18, 19},  // Connector Id 0 individual connector to phy map
            {20, 21, 22, 23},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

    {FBE_SAS_ENCLOSURE_TYPE_MIRANDA, 25,    36,     20,     10,      5,      4,      0,      0,      0,
        {32, 31, 30, 29, 28, 27, 26, 25, 24, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8}, 0, 2, 0, 0, 0, 2,
        {
            {16, 17, 18, 19},  // Connector Id 0 individual connector to phy map
            {20, 21, 22, 23},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },

// ENCL_CLEANUP - Moon DPE need real values        
    {FBE_SAS_ENCLOSURE_TYPE_CALYPSO, 25,    36,     20,     10,      5,      4,      0,      0,      4,
        {8, 9, 10, 11, 12, 13, 14, 15, 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26, 27, 28, 29, 30, 31, 32}, 0, 2, 0, 0, 0, 2,
        {
            {0, 1, 2, 3},  // Connector Id 0 individual connector to phy map
            {4, 5, 6, 7},  // Connector Id 1 individual connector to phy map
            {0},
            {0}, 
            {0},
            {0}
        },
        {0}
    },
    {FBE_SAS_ENCLOSURE_TYPE_TABASCO, 25,	  36,	  20,	  10,	   5,	   4,	   2,	   4,	   1,
			{11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35},	   
			 3, 2, 0, 0, 0, 2,
			 {
				{0, 1, 2, 3},	   // Connector Id 0 individual connector to phy map
				{4, 5, 6, 7},      // Connector Id 1 individual connector to phy map
				{0},
				{0}, 
				{0},
				{0}
			},
			{0}
    }, 

    {FBE_SAS_ENCLOSURE_TYPE_LAST,   0,     0,      0,      0,      0,      0,      0,      0}
};

static terminator_sas_unsupported_eses_page_info_t unsupported_eses_page_info;


terminator_virtual_phy_t * virtual_phy_new(void)
{
    terminator_virtual_phy_t * new_virtual_phy = NULL;

    new_virtual_phy = (terminator_virtual_phy_t *)fbe_terminator_allocate_memory(sizeof(terminator_virtual_phy_t));
    if (new_virtual_phy == NULL)
    {
        /* Can't allocate the memory we need */
        return new_virtual_phy;
    }
    base_component_init(&new_virtual_phy->base);
    base_component_set_component_type(&new_virtual_phy->base, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    return new_virtual_phy;
}

/**************************************************************************
 *  sas_virtual_phy_info_new()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This interface(constructor) creates and initializes a virtual PHY object.
 *
 *  PARAMETERS:
 *      enclosure type and SAS address of the enclosure, with which the new
 *      virtual phy is to be associated.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      When Terminator Initializes--->
 *       All drive slots will be "POWERED ON"
 *           // i.e. "DEVICE OFF" bit in Array device slot status element is NOT SET.
 *       ALL Drive Slot Status will be "NOT INSTALLED".
 *       ALL PHY STATUS WILL BE "OK".
 *       Phy ready bit in PHY status is set to false
 *
 *  HISTORY:
 *      Created
 **************************************************************************/
terminator_sas_virtual_phy_info_t * sas_virtual_phy_info_new(
    fbe_sas_enclosure_type_t encl_type,
    fbe_sas_address_t sas_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t *new_info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_u32_t drive_index, phy_index, ps_index, cooling_index,
        temp_sensor_index, display_index, sas_conn_index;
    fbe_u8_t max_drive_slots, max_phys, max_ps_elems, max_cooling_elems,
        max_temp_sensor_elems, max_diplay_characters, max_sas_conn_elems_per_side;


    new_info = (terminator_sas_virtual_phy_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_virtual_phy_info_t));
    if (new_info == NULL)
        return new_info;
    /* SAFEBUG - panic on uninitialized data in sas_virtual_phy_free_allocated_memory */
    csx_p_memzero(new_info, sizeof(terminator_sas_virtual_phy_info_t));
    login_data = sas_virtual_phy_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_address(login_data, sas_address);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL);

    // Give virtual phy the enclosure type
    new_info->enclosure_type = encl_type;

    //First initialize the member pointers to NULL
    new_info->cooling_status = NULL;
    new_info->drive_slot_status = NULL;
    new_info->phy_status = NULL;
    new_info->ps_status = NULL;
    new_info->temp_sensor_status = NULL;

    // Allocate memory and initialize the drive, phy, cooling--etc eses
    // statuses as we now have the virtual phy specialized to the encl
    // type.
    // DRIVE SLOTS
    status = sas_virtual_phy_max_drive_slots(encl_type, &max_drive_slots);
    if(status != FBE_STATUS_OK)
    {
       terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:sas_virtual_phy_max_drive_slots failed, encl_type=%d\n", __FUNCTION__, encl_type);
       sas_virtual_phy_free_allocated_memory(new_info);
       return(NULL);
    }

    if (max_drive_slots == 0) { goto skip_stuff; } /* SAFEBUG - prevent allocation of 0 bytes */
    // Drive Slot ESES Status
    new_info->drive_slot_status = 
        (ses_stat_elem_array_dev_slot_struct *)fbe_terminator_allocate_memory(max_drive_slots * sizeof(ses_stat_elem_array_dev_slot_struct));
    if (new_info->drive_slot_status == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for drive slot status at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        sas_virtual_phy_free_allocated_memory(new_info);

        return NULL;
    }

    fbe_zero_memory(new_info->drive_slot_status, max_drive_slots * sizeof(ses_stat_elem_array_dev_slot_struct));

    for (drive_index = 0; drive_index < max_drive_slots; drive_index ++)
    {
        new_info->drive_slot_status[drive_index].cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
        // All drive slots are powered ON intially.
        new_info->drive_slot_status[drive_index].dev_off = FBE_FALSE;
    }

    //Drive Slot insert Count
    new_info->drive_slot_insert_count = (fbe_u8_t *)fbe_terminator_allocate_memory(max_drive_slots * sizeof(fbe_u8_t));
    if (new_info->drive_slot_insert_count == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for drive slot insert count at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        sas_virtual_phy_free_allocated_memory(new_info);

        return NULL;
    }

    fbe_zero_memory(new_info->drive_slot_insert_count, max_drive_slots * sizeof(fbe_u8_t));

    //Drive Slot power down count
    new_info->drive_power_down_count = (fbe_u8_t *)fbe_terminator_allocate_memory(max_drive_slots * sizeof(fbe_u8_t));
    if (new_info->drive_power_down_count == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for drive slot power down count at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        sas_virtual_phy_free_allocated_memory(new_info);

        return NULL;
    }

    fbe_zero_memory(new_info->drive_power_down_count, max_drive_slots * sizeof(fbe_u8_t));

    new_info->general_info_elem_drive_slot = 
        (ses_general_info_elem_array_dev_slot_struct *)fbe_terminator_allocate_memory(max_drive_slots * sizeof(ses_general_info_elem_array_dev_slot_struct));
    if (new_info->general_info_elem_drive_slot == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for general info element drive slot at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        sas_virtual_phy_free_allocated_memory(new_info);

        return NULL;
    }

    fbe_zero_memory(new_info->general_info_elem_drive_slot, max_drive_slots * sizeof(ses_general_info_elem_array_dev_slot_struct));

    for (drive_index = 0; drive_index < max_drive_slots; drive_index ++)
    {
      // currently we only have jetfire using this battery backed info. 
        if (new_info->enclosure_type == FBE_SAS_ENCLOSURE_TYPE_FALLBACK) 
        {
            new_info->general_info_elem_drive_slot[drive_index].battery_backed = 1;
        }
        else
        {
            new_info->general_info_elem_drive_slot[drive_index].battery_backed = 0;
        }
    }


    skip_stuff:;


    // PHY'S
    status = sas_virtual_phy_max_phys(encl_type, &max_phys);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }
    new_info->phy_status = 
        (ses_stat_elem_exp_phy_struct *)fbe_terminator_allocate_memory(max_phys * sizeof(ses_stat_elem_exp_phy_struct));
    if (new_info->phy_status == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for physical status at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        sas_virtual_phy_free_allocated_memory(new_info);

        return NULL;
    }

    fbe_zero_memory(new_info->phy_status, max_phys * sizeof(ses_stat_elem_exp_phy_struct));

    for(phy_index = 0; phy_index < max_phys; phy_index ++)
    {
        // As per ESES 0.16 all PHY's initially have their status code set to OK
        new_info->phy_status[phy_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        new_info->phy_status[phy_index].phy_id = phy_index;
        new_info->phy_status[phy_index].phy_rdy = FBE_TRUE;
        new_info->phy_status[phy_index].link_rdy = FBE_TRUE;
    }


    // POWER SUPPLIES
    status = sas_virtual_phy_max_ps_elems(encl_type, &max_ps_elems);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }
    if ( max_ps_elems > 0 )
    {
        new_info->ps_status = 
            (ses_stat_elem_ps_struct *)fbe_terminator_allocate_memory(max_ps_elems * sizeof(ses_stat_elem_ps_struct));
        if (new_info->ps_status == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for ps status at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            sas_virtual_phy_free_allocated_memory(new_info);

            return NULL;
        }

        fbe_zero_memory(new_info->ps_status, max_ps_elems * sizeof(ses_stat_elem_ps_struct));

        // Initialize the Power supplies. Assume all are installed & working properly.
        for(ps_index = 0; ps_index < max_ps_elems; ps_index ++)
        {
            new_info->ps_status[ps_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            new_info->ps_status[ps_index].off = 0;
            new_info->ps_status[ps_index].dc_fail = 0;
            new_info->ps_status[ps_index].rqsted_on = 1;
            new_info->ps_status[ps_index].ac_fail = 0;
        }
    }
    else
    {
        new_info->ps_status = NULL;
    }

    // SAS CONNECTOR
    status = sas_virtual_phy_max_conns_per_lcc(encl_type, &max_sas_conn_elems_per_side);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }
    if ( 2*max_sas_conn_elems_per_side > 0 )
    {
        new_info->sas_conn_status = 
            (ses_stat_elem_sas_conn_struct *)fbe_terminator_allocate_memory(2 * max_sas_conn_elems_per_side * sizeof(ses_stat_elem_sas_conn_struct));
        if (new_info->sas_conn_status == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for sas connection status at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            sas_virtual_phy_free_allocated_memory(new_info);

            return NULL;
        }

        fbe_zero_memory (new_info->sas_conn_status, 2 * max_sas_conn_elems_per_side * sizeof(ses_stat_elem_sas_conn_struct));

        // Initialize the Power supplies. Assume all are installed & working properly.
        for(sas_conn_index = 0; sas_conn_index < 2*max_sas_conn_elems_per_side; sas_conn_index ++)
        {
            new_info->sas_conn_status[sas_conn_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
        }
    }
    else
    {
        new_info->sas_conn_status = NULL;
    }

    // COOLING
    status = sas_virtual_phy_max_cooling_elems(encl_type, &max_cooling_elems);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }
    if ( max_cooling_elems > 0 )
    {
        new_info->cooling_status = 
            (ses_stat_elem_cooling_struct *)fbe_terminator_allocate_memory(max_cooling_elems * sizeof(ses_stat_elem_cooling_struct));
        if (new_info->cooling_status == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for cooling status at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            sas_virtual_phy_free_allocated_memory(new_info);

            return NULL;
        }

        fbe_zero_memory(new_info->cooling_status, max_cooling_elems * sizeof(ses_stat_elem_cooling_struct));

        // Initialize the Cooling elements(fans). Assume all are installed & working properly.
        // Internally in terminator there will be no assumption on which fan belongs to which
        // power supply. We consider them as independent . Upto the user to make the disinction.
        for(cooling_index = 0; cooling_index < max_cooling_elems; cooling_index ++)
        {
            new_info->cooling_status[cooling_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            // Fan speed not yet decided... Put to some thing for now.
            new_info->cooling_status[cooling_index].actual_fan_speed_high = 0;
            new_info->cooling_status[cooling_index].actual_fan_speed_low = 0x10;
            new_info->cooling_status[cooling_index].rqsted_on = 1;
            new_info->cooling_status[cooling_index].off = 0;
            // Relative speed code of 7 indicates the fan is at its highest speed.
            new_info->cooling_status[cooling_index].actual_speed_code = 7;
        }
    }
    else
    {
        new_info->cooling_status = NULL;
    }


    // TEMPERATURE SENSOR
    status = sas_virtual_phy_max_temp_sensor_elems(encl_type, &max_temp_sensor_elems);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }
    if ( max_temp_sensor_elems > 0 )
    {
        new_info->overall_temp_sensor_status = 
            (ses_stat_elem_temp_sensor_struct *)fbe_terminator_allocate_memory(max_temp_sensor_elems * sizeof(ses_stat_elem_temp_sensor_struct));
        if (new_info->overall_temp_sensor_status == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for overall temp sensor status at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            sas_virtual_phy_free_allocated_memory(new_info);

            return NULL;
        }

        fbe_zero_memory(new_info->overall_temp_sensor_status, max_temp_sensor_elems * sizeof(ses_stat_elem_temp_sensor_struct));

        new_info->temp_sensor_status = 
            (ses_stat_elem_temp_sensor_struct *)fbe_terminator_allocate_memory(max_temp_sensor_elems * sizeof(ses_stat_elem_temp_sensor_struct));
        if (new_info->temp_sensor_status == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for temp sensor status at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            sas_virtual_phy_free_allocated_memory(new_info);

            return NULL;
        }

        fbe_zero_memory(new_info->temp_sensor_status, max_temp_sensor_elems * sizeof(ses_stat_elem_temp_sensor_struct));

        for(temp_sensor_index = 0; temp_sensor_index < max_temp_sensor_elems; temp_sensor_index ++)
        {
            // overall temperature status element.
            new_info->overall_temp_sensor_status[temp_sensor_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            new_info->overall_temp_sensor_status[temp_sensor_index].ot_failure = 0;
            new_info->overall_temp_sensor_status[temp_sensor_index].ot_warning = 0;
            new_info->overall_temp_sensor_status[temp_sensor_index].ident = 0;
            // it was not yet decided what should be the temperature. put to room temp
            // of 22. It should be offset by 20.
            new_info->overall_temp_sensor_status[temp_sensor_index].temp = 22+42;

            new_info->temp_sensor_status[temp_sensor_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            new_info->temp_sensor_status[temp_sensor_index].ot_failure = 0;
            new_info->temp_sensor_status[temp_sensor_index].ot_warning = 0;
            new_info->temp_sensor_status[temp_sensor_index].ident = 0;
            // it was not yet decided what should be the temperature. put to room temp
            // of 22. It should be offset by 20.
            new_info->temp_sensor_status[temp_sensor_index].temp = 22+42;
        }
    }
    else
    {
        new_info->overall_temp_sensor_status = NULL;
        new_info->temp_sensor_status = NULL;
    }

    //ENCLOSURE ELEMENTS for LOCAL LCC & CHASSIS
    fbe_zero_memory(&new_info->encl_status, sizeof(ses_stat_elem_encl_struct));
    fbe_zero_memory(&new_info->chassis_encl_status, sizeof(ses_stat_elem_encl_struct));
    new_info->encl_status.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    new_info->chassis_encl_status.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    // Initially unit attention is not set on the Unit.
    new_info->unit_attention = FBE_FALSE;

    // Initialize the eses related page information stored per VPhy.
    status = terminator_initialize_eses_page_info(encl_type, &new_info->eses_page_info);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }

    //DISPLAY CHARACTERS
    status = sas_virtual_phy_max_display_characters(encl_type, &max_diplay_characters);
    if(status != FBE_STATUS_OK)
    {
        sas_virtual_phy_free_allocated_memory(new_info);
        return(NULL);
    }
    if ( max_diplay_characters > 0 )
    {
        // There is one display status element for each display character
        new_info->display_status = 
            (ses_stat_elem_display_struct *)fbe_terminator_allocate_memory(max_diplay_characters * sizeof(ses_stat_elem_display_struct));
        if (new_info->display_status == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for display status at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            sas_virtual_phy_free_allocated_memory(new_info);

            return NULL;
        }

        fbe_zero_memory(new_info->display_status , max_diplay_characters * sizeof(ses_stat_elem_display_struct));

        // Initialize all the display characters.
        for(display_index = 0; display_index < max_diplay_characters; display_index++)
        {
            new_info->display_status[display_index].cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
            new_info->display_status[display_index].display_mode_status = 0x01;
            new_info->display_status[display_index].display_char_stat = '-';
        }
    }
    else
    {
        new_info->display_status = NULL;
    }

    // EMC Enclosure Status
    new_info->emcEnclStatus.shutdown_reason = FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED;

    // EMC Power Supply Status
    new_info->emcPsInfoStatus.margining_test_mode = FBE_ESES_MARGINING_TEST_MODE_STATUS_TEST_SUCCESSFUL;
    new_info->emcPsInfoStatus.margining_test_results = FBE_ESES_MARGINING_TEST_RESULTS_SUCCESS;

    switch(new_info->enclosure_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
            break;      
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
            break;    
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
            break;     
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            break;     
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            break;     
            break; 
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            break;    
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
            break;
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            break;  
        default:
            break;
    }

    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;

    // Initialize new firmware revision queue
    fbe_queue_init(&new_info->new_firmware_rev_queue_head);

    // set time intervals to default value
    new_info->activate_time_intervel = DEFAULT_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL;
    new_info->reset_time_intervel = DEFAULT_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL;

    return new_info;
}

fbe_cpd_shim_callback_login_t * sas_virtual_phy_info_get_login_data(terminator_sas_virtual_phy_info_t * self)
{
    return (&self->login_data);
}
fbe_status_t sas_virtual_phy_info_set_enclosure_type(terminator_sas_virtual_phy_info_t * self, fbe_sas_enclosure_type_t enclosure_type)
{
    self->enclosure_type = enclosure_type;
    return FBE_STATUS_OK;
}
fbe_sas_enclosure_type_t sas_virtual_phy_info_get_enclosure_type(terminator_sas_virtual_phy_info_t * self)
{
    return self->enclosure_type;
}
fbe_status_t sas_virtual_phy_call_io_api(terminator_virtual_phy_t * device, fbe_terminator_io_t * terminator_io)
{
    if(terminator_io->payload != NULL) {
        return fbe_terminator_sas_virtual_phy_payload_io (terminator_io->payload, device);
    } else {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: missing payload data\n", __FUNCTION__);  
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

/*
 * Functions for setting and getting the ESES status
 * of a particular PHY in the given virtual Phy
 * Object, that belongs to the corresponding
 * enclosure object containing the PHY.
 */
fbe_status_t sas_virtual_phy_get_phy_eses_status(
    terminator_virtual_phy_t * self,
    fbe_u32_t phy_number,
    ses_stat_elem_exp_phy_struct *exp_phy_stat)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_phys;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_phys(encl_type, &max_phys);
    RETURN_ON_ERROR_STATUS;

    /* Should be a valid slot number */
    if (phy_number >= max_phys)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    fbe_copy_memory(exp_phy_stat, &info->phy_status[phy_number], sizeof(ses_stat_elem_exp_phy_struct));
    return status;
}

fbe_status_t sas_virtual_phy_set_phy_eses_status(
    terminator_virtual_phy_t * self,
    fbe_u32_t phy_number,
    ses_stat_elem_exp_phy_struct exp_phy_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_phys;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_phys(encl_type, &max_phys);
    RETURN_ON_ERROR_STATUS;


    /* Should be a valid phy number */
    if (phy_number >= max_phys)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    if((exp_phy_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (exp_phy_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->phy_status[phy_number], &exp_phy_stat, sizeof(ses_stat_elem_exp_phy_struct));
        info->phy_status[phy_number].phy_id = phy_number;
        status = FBE_STATUS_OK;
    }
    return status;
}
fbe_status_t sas_virtual_phy_set_sas_address(terminator_virtual_phy_t * self, fbe_sas_address_t sas_address)
{
    void * attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    attributes = base_component_get_attributes(&self->base);
    login_data = sas_virtual_phy_info_get_login_data(attributes);
    cpd_shim_callback_login_set_device_address(login_data, sas_address);
    return FBE_STATUS_OK;
}

/*
 * Functions for setting and getting the ESES status
 * of a particular physical drive slot in the given
 * virtual phy object, which belongs to the corresponding
 * enclosure object containing the drive slot physically
 */
fbe_status_t sas_virtual_phy_set_drive_eses_status(terminator_virtual_phy_t * self, fbe_u32_t slot_number, ses_stat_elem_array_dev_slot_struct drive_slot_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;

    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&self, &slot_number);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }
    if((drive_slot_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (drive_slot_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->drive_slot_status[slot_number], &drive_slot_stat, sizeof(ses_stat_elem_array_dev_slot_struct));
        status = FBE_STATUS_OK;
    }
    return status;
}
fbe_status_t sas_virtual_phy_get_drive_eses_status(terminator_virtual_phy_t * self, fbe_u32_t slot_number, ses_stat_elem_array_dev_slot_struct *drive_slot_stat)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;

    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&self, &slot_number);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    fbe_copy_memory(drive_slot_stat, &info->drive_slot_status[slot_number], sizeof(ses_stat_elem_array_dev_slot_struct));
    status = FBE_STATUS_OK;
    return status;
}
/*
 * End of functions for getting and setting drive ESES status
 */

/*
 * Functions for setting and getting the drive slot INSERT
 * count of a particular physical drive slot in the enclosure 
 */
fbe_status_t sas_virtual_phy_set_drive_slot_insert_count(terminator_virtual_phy_t * self, 
                                                         fbe_u32_t slot_number, 
                                                         fbe_u8_t insert_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;

    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&self, &slot_number);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);

    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(&info->drive_slot_insert_count[slot_number], &insert_count, sizeof(fbe_u8_t));
    status = FBE_STATUS_OK;
    return status;
}
fbe_status_t sas_virtual_phy_get_drive_slot_insert_count(terminator_virtual_phy_t * self, 
                                                         fbe_u32_t slot_number, 
                                                         fbe_u8_t *insert_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;

    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&self, &slot_number);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(insert_count, &info->drive_slot_insert_count[slot_number], sizeof(fbe_u8_t));
    status = FBE_STATUS_OK;
    return status;
}
/*
 * End of functions for getting and setting drive slot INSERT count
 */

/*
 * Functions for setting and getting the drive POWER DOWN
 * count of a particular physical drive slot in the enclosure 
 */
fbe_status_t sas_virtual_phy_set_drive_power_down_count(terminator_virtual_phy_t * self, 
                                                        fbe_u32_t slot_number, 
                                                        fbe_u8_t power_down_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;

    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&self, &slot_number);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(&info->drive_power_down_count[slot_number], &power_down_count, sizeof(fbe_u8_t));
    status = FBE_STATUS_OK;
    return status;
}
fbe_status_t sas_virtual_phy_get_drive_power_down_count(terminator_virtual_phy_t * self, 
                                                        fbe_u32_t slot_number, 
                                                        fbe_u8_t *power_down_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;

    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&self, &slot_number);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(power_down_count, &info->drive_power_down_count[slot_number], sizeof(fbe_u8_t));
    status = FBE_STATUS_OK;
    return status;
}
/*
 * End of functions for getting and setting drive slot down count
 */

/*
 * Functions for clearing the drive slot insert and drive power down
 * counts of all drive slots in the enclosure 
 */
fbe_status_t sas_virtual_phy_clear_drive_power_down_counts(terminator_virtual_phy_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_drive_slots;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    memset(info->drive_power_down_count, 0, (max_drive_slots * sizeof(fbe_u8_t)));
    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_clear_drive_slot_insert_counts(terminator_virtual_phy_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_drive_slots;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    memset(info->drive_slot_insert_count, 0, (max_drive_slots * sizeof(fbe_u8_t)));
    return(FBE_STATUS_OK);
}
/*
 * End of functions for getting and setting drive slot power cycle count
 */

/*********************************************************************
*   sas_virtual_phy_get_drive_slot_to_phy_mapping ()
*********************************************************************
*
*  Description:
*   This returns the phy identifier corresponding to given
*   physical drive slot.
*
*  Inputs:
*   drive_slot: physical drive slot on encl.
*   phy_id: Phy identifier
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_get_drive_slot_to_phy_mapping(
    fbe_u8_t drive_slot,
    fbe_u8_t *phy_id,
    fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    fbe_u8_t max_drive_slots;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    status = sas_virtual_phy_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    // to be completed based on the encl type.

    if(drive_slot < max_drive_slots)
    {
        encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
        while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
        {
            if(encl_eses_info->encl_type == encl_type)
            {
                *phy_id = encl_eses_info->drive_slot_to_phy_map[drive_slot];
                status = FBE_STATUS_OK;
                return(status);
            }
            encl_eses_info ++;
        }
    }

    status = FBE_STATUS_GENERIC_FAILURE;
    return(status);
}


/*********************************************************************
*          sas_virtual_phy_get_individual_conn_to_phy_mapping ()
*********************************************************************
*
*  Description:
*   This returns the phy identifier corresponding to given
*   connector. 
*
*  Inputs:
*   individual_lane:
*   connector_id: Connector Identifier
*   phy_id: Phy identifier (OUTPUT)
*   encl_type: 
*
*  Return Value:
*   success or failure.
*
*  History:
*    23-Sept-2011 PHE - created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_get_individual_conn_to_phy_mapping(fbe_u8_t individual_lane, fbe_u8_t connector_id, fbe_u8_t *phy_id, fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    fbe_u8_t max_single_lane_port_conns;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    status = sas_virtual_phy_max_single_lane_conns_per_port(encl_type, &max_single_lane_port_conns);
    RETURN_ON_ERROR_STATUS;

    if(individual_lane < max_single_lane_port_conns)
    {
        encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
        while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
        {
            if(encl_eses_info->encl_type == encl_type) 
            {
                *phy_id = encl_eses_info->individual_conn_to_phy_map[connector_id][individual_lane];
                terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: encl_type:%d conn_id:%d lane:%d phy_id:%d\n", __FUNCTION__, encl_type, connector_id, individual_lane, *phy_id);
                status = FBE_STATUS_OK;
                return(status);
            }
            encl_eses_info ++;
        }
    }

    status = FBE_STATUS_GENERIC_FAILURE;
    return(status);
}

/*********************************************************************
*          sas_virtual_phy_get_conn_id_to_drive_start_slot_mapping ()
*********************************************************************
*
*  Description:
*   This returns the connector id to drive start slot mapping. 
*
*  Inputs:
*   encl_type:
*   connector_id: Connector Identifier
*   drive_start_slot: (OUTPUT)
*
*  Return Value:
*   success or failure.
*
*  History:
*    23-Sept-2011 PHE - created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_get_conn_id_to_drive_start_slot_mapping(fbe_sas_enclosure_type_t encl_type,
                                                  fbe_u8_t connector_id, 
                                                  fbe_u8_t * drive_start_slot)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    fbe_u8_t max_conn_id_count = 0;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    status = terminator_max_conn_id_count(encl_type, &max_conn_id_count);
    if((status != FBE_STATUS_OK) || (connector_id >= max_conn_id_count))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type) 
        {
            *drive_start_slot = encl_eses_info->conn_id_to_drive_start_slot[connector_id];
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }
    

    status = FBE_STATUS_GENERIC_FAILURE;
    return(status);
}

/*********************************************************************
*          sas_virtual_phy_phy_corresponds_to_drive_slot ()
*********************************************************************
*
*  Description: This determines if the corresponding phy identifier corresponds
*   to a drive slot. For now assuming only for Viper.
*
*  Inputs:
*   Phy identifier
*   pointer to drive slot to be filled if phy corresponds to drive slot.
*
*  Return Value:
*   True if phy identifier corresponds to drive slot or false otherwise
*
*  History:
*    Aug08  created
*
*********************************************************************/
fbe_bool_t sas_virtual_phy_phy_corresponds_to_drive_slot(fbe_u8_t phy_id, fbe_u8_t *drive_slot, fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t returned_phy_id, i;
    fbe_u8_t max_drive_slots;

    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    for(i=0; i<max_drive_slots; i++)
    {
        status = sas_virtual_phy_get_drive_slot_to_phy_mapping(i, &returned_phy_id, encl_type);
        if(status != FBE_STATUS_OK)
        {
            break;
        }
        if(returned_phy_id == phy_id)
        {
            *drive_slot = i;
            return(FBE_TRUE);
        }
    }
    return(FBE_FALSE);
}

/*********************************************************************
*          sas_virtual_phy_phy_corresponds_to_connector ()
*********************************************************************
*
*  Description: This determines if the corresponding connector corresponds
*   to a drive slot. For now assuming only for Viper and connector is
*   a single lane connector.
*
*  Inputs:
*   Phy identifier
*   connector: pointer to connector to be filled if phy corresponds
*       to connector
*   pirmary port: pointer to primary_port that will be TRUE if the
*   connector filled is in primary port or FALSE otherwise(expansion
*   port)
*
*  Return Value:
*   True if connector corresponds to drive slot or false otherwise
*
*  History:
*    Aug08  created
*
*********************************************************************/
fbe_bool_t sas_virtual_phy_phy_corresponds_to_connector(
    fbe_u8_t phy_id,
    fbe_u8_t *connector,
    fbe_u8_t *connector_id,
    fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t i, j;
    fbe_u8_t returned_phy_id;
    fbe_u8_t max_single_lane_port_conns;
    fbe_u8_t max_conn_id_count = 0;

    status = sas_virtual_phy_max_single_lane_conns_per_port(encl_type, &max_single_lane_port_conns);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_conn_id_count(encl_type, &max_conn_id_count);
    RETURN_ON_ERROR_STATUS;

    for(i=0; i<max_single_lane_port_conns; i++)
    {
        for (j=0; j < max_conn_id_count; j++)
        {
            status =  sas_virtual_phy_get_individual_conn_to_phy_mapping(i, j, &returned_phy_id, encl_type);
            if(status != FBE_STATUS_OK)
            {
                break;
            }
            if(returned_phy_id == phy_id)
            {
                *connector = i;
                *connector_id = j; 
                return(FBE_TRUE);
            }
        }
    }
    return(FBE_FALSE);
}

/************************ NEW functions for PS - Start ******************************/

fbe_status_t sas_virtual_phy_set_ps_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_ps_id ps_id,
    ses_stat_elem_ps_struct ps_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_ps_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_ps_elems(encl_type, &max_ps_elems);
    RETURN_ON_ERROR_STATUS;

    /* Should be a supported/valid PS number */
    if (ps_id >= max_ps_elems)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    // Eventhough all status codes are not supported
    // we do want to allow all of them for testing
    // purposes.
    if((ps_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (ps_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->ps_status[ps_id], &ps_stat, sizeof(ses_stat_elem_ps_struct));
        status = FBE_STATUS_OK;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_get_ps_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_ps_id ps_id,
    ses_stat_elem_ps_struct *ps_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_ps_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_ps_elems(encl_type, &max_ps_elems);
    RETURN_ON_ERROR_STATUS;

    /* Should be a supported/valid PS number */
    if (ps_id >= max_ps_elems)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(ps_stat, &info->ps_status[ps_id], sizeof(ses_stat_elem_ps_struct));

    status = FBE_STATUS_OK;
    return(status);
}

/************************ NEW functions for PS - End ******************************/


/************************ NEW functions for EMC Encl Status - Start ******************************/
fbe_status_t sas_virtual_phy_get_emcEnclStatus(terminator_virtual_phy_t * self,
                                               ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(emcEnclStatusPtr, &info->emcEnclStatus, sizeof(ses_pg_emc_encl_stat_struct));

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t sas_virtual_phy_set_emcEnclStatus(terminator_virtual_phy_t * self,
                                               ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(&info->emcEnclStatus, emcEnclStatusPtr, sizeof(ses_pg_emc_encl_stat_struct));
    status = FBE_STATUS_OK;

    return(status);
}

/************************ NEW functions for EMC Encl Status - End ******************************/

/************************ NEW functions for EMC PS Info Status - Start ******************************/
fbe_status_t sas_virtual_phy_get_emcPsInfoStatus(terminator_virtual_phy_t * self,
                                                 ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(emcPsInfoStatusPtr, &info->emcPsInfoStatus, sizeof(ses_ps_info_elem_struct));

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t sas_virtual_phy_set_emcPsInfoStatus(terminator_virtual_phy_t * self,
                                                 ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(&info->emcPsInfoStatus, emcPsInfoStatusPtr, sizeof(ses_ps_info_elem_struct));
    status = FBE_STATUS_OK;

    return(status);
}

/************************ NEW functions for EMC PS Info Status - End ******************************/

/************************ NEW functions for EMC General Info Drive Slot Status - Start ******************************/
fbe_status_t sas_virtual_phy_get_emcGeneralInfoDirveSlotStatus(terminator_virtual_phy_t * self,
                                                 ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                                 fbe_u8_t drive_slot)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(emcGeneralInfoDirveSlotStatusPtr, &info->general_info_elem_drive_slot[drive_slot], sizeof(ses_general_info_elem_array_dev_slot_struct));

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t sas_virtual_phy_set_emcGeneralInfoDirveSlotStatus(terminator_virtual_phy_t * self,
                                                 ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                                 fbe_u8_t drive_slot)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(&info->general_info_elem_drive_slot[drive_slot], emcGeneralInfoDirveSlotStatusPtr, sizeof(ses_general_info_elem_array_dev_slot_struct));
    status = FBE_STATUS_OK;

    return(status);
}

/************************ NEW functions for EMC General Info Drive Slot Status - End ******************************/


/************************ NEW functions for SAS connector - Start ******************************/

fbe_status_t sas_virtual_phy_set_sas_conn_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct sas_conn_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_sas_conn_elems_per_side;  
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_conns_per_lcc(encl_type, &max_sas_conn_elems_per_side);
    RETURN_ON_ERROR_STATUS;

    /* Should be a supported/valid PS number */
    if (sas_conn_id >= max_sas_conn_elems_per_side * 2)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    // Eventhough all status codes are not supported
    // we do want to allow all of them for testing
    // purposes.
    if((sas_conn_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (sas_conn_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->sas_conn_status[sas_conn_id], &sas_conn_stat, sizeof(ses_stat_elem_sas_conn_struct));
        status = FBE_STATUS_OK;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_get_sas_conn_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct *sas_conn_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_sas_conn_elems_per_side;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_conns_per_lcc(encl_type, &max_sas_conn_elems_per_side);
    RETURN_ON_ERROR_STATUS;

    /* Should be a supported/valid PS number */
    if (sas_conn_id >= 2*max_sas_conn_elems_per_side)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    fbe_copy_memory(sas_conn_stat, &info->sas_conn_status[sas_conn_id], sizeof(ses_stat_elem_sas_conn_struct));

    status = FBE_STATUS_OK;
    return(status);
}

/************************ NEW functions for SAS connector - End ******************************/


/*********************************************************************
*   sas_virtual_phy_get_temp_sensor_eses_status ()
*********************************************************************
*
*  Description:
*   This gets the ESES status of the given Cooling element.
*
*  Inputs:
*   Virtual Phy object, cooling element identifier,
*   Cooling element ESES status value to set
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_set_cooling_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_cooling_id cooling_id,
    ses_stat_elem_cooling_struct cooling_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_cooling_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_cooling_elems(encl_type, &max_cooling_elems);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    /* Should be a valid phy number */
    if (cooling_id >= max_cooling_elems)
    {
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    // Eventhough all status codes are not supported
    // we do want to allow all of them for testing
    // purposes.
    if((cooling_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (cooling_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->cooling_status[cooling_id],
            &cooling_stat, sizeof(ses_stat_elem_cooling_struct));
        status = FBE_STATUS_OK;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_get_cooling_eses_status ()
*********************************************************************
*
*  Description:
*   This sets the ESES status of the given cooling(fan) element.
*
*  Inputs:
*   Virtual Phy object, Cooling element identifier,
*   Cooling element Status value to set.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_get_cooling_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_cooling_id cooling_id,
    ses_stat_elem_cooling_struct *cooling_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_cooling_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_cooling_elems(encl_type, &max_cooling_elems);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    /* Should be a valid phy number */
    if (cooling_id >= max_cooling_elems)
    {
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(cooling_stat, &info->cooling_status[cooling_id],
        sizeof(ses_stat_elem_cooling_struct));

    status = FBE_STATUS_OK;
    return(status);
}

/*********************************************************************
*   sas_virtual_phy_set_temp_sensor_eses_status ()
*********************************************************************
*
*  Description:
*   This sets the ESES status of the given temperature sensor element.
*
*  Inputs:
*   Virtual Phy object, temp sensor identifier,
*   Pointer to the Cooling element ESES status buffer to be filled.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_set_temp_sensor_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_temp_sensor_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_temp_sensor_elems(encl_type, &max_temp_sensor_elems);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    /* Should be a valid phy number */
    if (temp_sensor_id >= max_temp_sensor_elems)
    {
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    // Evnenthough all status codes are not supported
    // we do want to allow all of them for testing
    // purposes.
    if((temp_sensor_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (temp_sensor_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->temp_sensor_status[temp_sensor_id],
            &temp_sensor_stat, sizeof(ses_stat_elem_temp_sensor_struct));
        status = FBE_STATUS_OK;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_get_temp_sensor_eses_status ()
*********************************************************************
*
*  Description:
*   This gets the ESES status of the given temperature sensor element.
*
*  Inputs:
*   Virtual Phy object, temp sensor identifier,
*   Pointer to the temp sensor ESES status buffer to be filled.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_get_temp_sensor_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_temp_sensor_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_temp_sensor_elems(encl_type, &max_temp_sensor_elems);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    /* Should be a valid phy number */
    if (temp_sensor_id >= max_temp_sensor_elems)
    {
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(temp_sensor_stat, &info->temp_sensor_status[temp_sensor_id],
        sizeof(ses_stat_elem_temp_sensor_struct));

    status = FBE_STATUS_OK;
    return(status);
}

/*********************************************************************
*   sas_virtual_phy_set_overall_temp_sensor_eses_status ()
*********************************************************************
*
*  Description:
*   This sets the overall ESES status of the given temperature sensor element.
*
*  Inputs:
*   Virtual Phy object, temp sensor identifier,
*   Pointer to the Cooling element ESES status buffer to be filled.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_set_overall_temp_sensor_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_temp_sensor_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_temp_sensor_elems(encl_type, &max_temp_sensor_elems);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    /* Should be a valid phy number */
    if (temp_sensor_id >= max_temp_sensor_elems)
    {
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    // Evnenthough all status codes are not supported
    // we do want to allow all of them for testing
    // purposes.
    if((temp_sensor_stat.cmn_stat.elem_stat_code >= SES_STAT_CODE_UNSUPP) &&
       (temp_sensor_stat.cmn_stat.elem_stat_code <= SES_STAT_CODE_UNAVAILABLE))
    {
        fbe_copy_memory(&info->overall_temp_sensor_status[temp_sensor_id],
            &temp_sensor_stat, sizeof(ses_stat_elem_temp_sensor_struct));
        status = FBE_STATUS_OK;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_get_overall_temp_sensor_eses_status ()
*********************************************************************
*
*  Description:
*   This gets overall ESES status of the given temperature sensor element.
*
*  Inputs:
*   Virtual Phy object, temp sensor identifier,
*   Pointer to the temp sensor ESES status buffer to be filled.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_get_overall_temp_sensor_eses_status(
    terminator_virtual_phy_t * self,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_temp_sensor_elems;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_temp_sensor_elems(encl_type, &max_temp_sensor_elems);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    /* Should be a valid phy number */
    if (temp_sensor_id >= max_temp_sensor_elems)
    {
        return status;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }

    fbe_copy_memory(temp_sensor_stat, &info->overall_temp_sensor_status[temp_sensor_id],
        sizeof(ses_stat_elem_temp_sensor_struct));

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t sas_virtual_phy_get_enclosure_ptr(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_terminator_device_ptr_t *enclosure_ptr)
{
    base_component_t    *parent_handle = NULL;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /* get the parent enclosure */
    status = base_component_get_parent(virtual_phy_ptr, &parent_handle);
    if (parent_handle != NULL)
    {
        *enclosure_ptr = (terminator_enclosure_t *)parent_handle;
    }
    return status;
}

fbe_status_t sas_virtual_phy_get_enclosure_uid(fbe_terminator_device_ptr_t handle, fbe_u8_t ** uid)
{
    base_component_t    *parent_handle = NULL;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /* get the parent enclosure */
    status = base_component_get_parent(handle, &parent_handle);
    if (parent_handle != NULL)
    {
        *uid = sas_enclosure_get_enclosure_uid((terminator_enclosure_t *)parent_handle);
    }
    return status;
}

fbe_status_t sas_virtual_phy_get_enclosure_type(fbe_terminator_device_ptr_t handle, fbe_sas_enclosure_type_t *encl_type)
{
    base_component_t    *parent_handle = NULL;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /* get the parent enclosure */
    status = base_component_get_parent(handle, &parent_handle);
    if (parent_handle != NULL)
    {
        *encl_type = sas_enclosure_get_enclosure_type((terminator_enclosure_t *)parent_handle);
    }
    return status;
}

fbe_status_t sas_virtual_phy_check_enclosure_type(fbe_sas_enclosure_type_t encl_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            return(FBE_STATUS_OK);
        }
        encl_eses_info ++;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_max_drive_slots(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_drive_slots)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_drive_slots = encl_eses_info->max_drive_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_max_phys(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_phys)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_phys = encl_eses_info->max_phy_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);

}

fbe_status_t sas_virtual_phy_max_conns_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
     terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_conns = encl_eses_info->max_lcc_conn_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_max_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
     terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_conns = encl_eses_info->max_port_conn_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}
fbe_status_t sas_virtual_phy_max_conn_id_count(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conn_id_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_conn_id_count = encl_eses_info->max_conn_id_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(FBE_STATUS_GENERIC_FAILURE);
}


fbe_status_t sas_virtual_phy_max_single_lane_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
     terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_conns = encl_eses_info->max_single_lane_port_conn_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_max_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_cooling_elems)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_cooling_elems = encl_eses_info->max_cooling_elem_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_max_ext_cooling_elems ()
*********************************************************************
*
*  Description:
*   This gets count of ext cooling elements.
*
*  Inputs:
*   enclosure type
*   pointer containing count of ext cooling elements
*
*  Return Value:
*   success or failure.
*
*  History:
*    06-06-2011  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_max_ext_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ext_cooling_elems)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_ext_cooling_elems = encl_eses_info->max_ext_cooling_elem_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_max_bem_cooling_elems ()
*********************************************************************
*
*  Description:
*   This gets count of BEM cooling elements.
*
*  Inputs:
*   enclosure type
*   pointer containing count of ext cooling elements
*
*  Return Value:
*   success or failure.
*
*  History:
*    18-07-2012  created. Rui Chang
*
*********************************************************************/
fbe_status_t sas_virtual_phy_max_bem_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_bem_cooling_elems)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_bem_cooling_elems = encl_eses_info->max_bem_cooling_elem_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_max_lccs ()
*********************************************************************
*
*  Description:
*   This gets count of lccs.
*
*  Inputs:
*   enclosure type
*   pointer containing count of max lccs
*
*  Return Value:
*   success or failure.
*
*  History:
*    06-06-2011  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_max_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_lccs)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_lccs = encl_eses_info->max_lcc_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

/*********************************************************************
*   sas_virtual_phy_max_ee_lccs ()
*********************************************************************
*
*  Description:
*   This gets count of ee lccs.
*
*  Inputs:
*   enclosure type
*   pointer containing count of ee lccs
*
*  Return Value:
*   success or failure.
*
*  History:
*    06-06-2011  created
*
*********************************************************************/
fbe_status_t sas_virtual_phy_max_ee_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ee_lccs)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_ee_lccs = encl_eses_info->max_ee_lcc_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_max_ps_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ps_elems)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
     terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_ps_elems = encl_eses_info->max_ps_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }

    return(status);
}

fbe_status_t sas_virtual_phy_max_temp_sensor_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_temp_sensor_elems)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_temp_sensor_elems = encl_eses_info->max_temp_sensor_elems_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }
    return(status);
}


fbe_status_t sas_virtual_phy_max_display_characters(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_diplay_characters)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_encl_eses_info_t *encl_eses_info;
 terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // to be completed based on the encl type.
    encl_eses_info = SELECT_ENCL_ESES_INFO_TABLE(spid);
    while(encl_eses_info->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST)
    {
        if(encl_eses_info->encl_type == encl_type)
        {
            *max_diplay_characters = encl_eses_info->max_diplay_character_count;
            status = FBE_STATUS_OK;
            return(status);
        }
        encl_eses_info ++;
    }
    return(status);
}

fbe_status_t sas_virtual_phy_destroy(terminator_sas_virtual_phy_info_t *attributes)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if(attributes == NULL)
    {
        return(status);
    }

    sas_virtual_phy_free_allocated_memory(attributes);

    status = FBE_STATUS_OK;
    return(status);
}

/* This functions frees ANY dynamically allocated memory
 * in the Virtual Phy Object data, if any exists.
 */
void sas_virtual_phy_free_allocated_memory(terminator_sas_virtual_phy_info_t *attributes)
{
    fbe_queue_element_t * queue_element;
    terminator_enclosure_firmware_new_rev_record_t *new_rev = NULL;
    if(attributes != NULL)
    {
        terminator_free_mem_on_not_null(attributes->cooling_status);
        terminator_free_mem_on_not_null(attributes->drive_slot_status);
        terminator_free_mem_on_not_null(attributes->drive_slot_insert_count);
        terminator_free_mem_on_not_null(attributes->drive_power_down_count);
        terminator_free_mem_on_not_null(attributes->phy_status);
        terminator_free_mem_on_not_null(attributes->ps_status);
        terminator_free_mem_on_not_null(attributes->temp_sensor_status);
        terminator_free_mem_on_not_null(attributes->overall_temp_sensor_status);
        terminator_free_mem_on_not_null(attributes->display_status);
        sas_virtual_phy_free_config_page_info_memory(&attributes->eses_page_info.vp_config_diag_page_info);

        // destroy new revision record queue
        queue_element = fbe_queue_pop(&attributes->new_firmware_rev_queue_head);
        while(queue_element != NULL){
            new_rev = (terminator_enclosure_firmware_new_rev_record_t *)queue_element;
            fbe_terminator_free_memory(new_rev);
            queue_element = fbe_queue_pop(&attributes->new_firmware_rev_queue_head);
        }
        fbe_queue_destroy(&attributes->new_firmware_rev_queue_head);
    }
    terminator_free_mem_on_not_null(attributes);

    return;
}


/* this initialises a node, allocates memory for the node, and returns   */
/* a pointer to the new node. Must pass it the node details */
terminator_eses_free_mem_node_t * initnode(void)
{
   terminator_eses_free_mem_node_t *ptr;
   ptr = (terminator_eses_free_mem_node_t *) fbe_terminator_allocate_memory(sizeof(terminator_eses_free_mem_node_t) );
   if(ptr == NULL )         /* error allocating node?         */
       return NULL;         /* then return NULL, else        */
   else 
   {                               /* allocated node successfully */
       ptr->buf = 0;
       ptr->next = NULL;       
       return ptr;            /* return pointer to new node  */
   }
}

/* this adds a node to the end of the list. You must allocate a node and  */
/* then pass its address to this function                                 */
void add_to_free_mem_list( terminator_eses_free_mem_node_t ** head_ptr, fbe_u8_t *buf )  /* adding to end of list */
{
   terminator_eses_free_mem_node_t *new_node;

   new_node = initnode();
   if(new_node != NULL)
   {
       new_node->buf = buf;
   }
   else
   {
    return;
   }
   if( *head_ptr == NULL )
   {
       *head_ptr = new_node;
       (*head_ptr)->next = NULL;
       return;
   }
   else
   {
    new_node->next = (*head_ptr)->next;
    (*head_ptr)->next= new_node;
   }
}

/* search the list, and return a pointer to the found node     */
/* accepts a data(buf memory) to search for, and a pointer from which to start. If    */
/* you pass the pointer as 'head', it searches from the start of the list */
terminator_eses_free_mem_node_t * find_free_mem(terminator_eses_free_mem_node_t *head_ptr, fbe_u8_t *buf)
{
    while((head_ptr) && (head_ptr ->buf != buf ))
    {   
    head_ptr = head_ptr->next; 
    }
    return head_ptr;
}


/*Destroy the complet free memory link list for*/
void destroy_free_mem_list(terminator_eses_free_mem_node_t *head)  
{ 
    terminator_eses_free_mem_node_t *tmp; 
    if(head == NULL)
    {
        return;
    }
    while(head != NULL)
    { 
        tmp = head->next; 
        fbe_terminator_free_memory(head); 
        head = tmp; 
    } 
}

void sas_virtual_phy_free_config_page_info_memory(vp_config_diag_page_info_t *vp_config_page_info)
{
    fbe_u8_t i;
/* head points to first terminator_eses_buf_node in list, end points to last terminator_eses_buf_node in list */
/* initialise both to NULL, meaning no nodes in list yet */
    terminator_eses_free_mem_node_t *head =NULL;

    // Free memory related to version descriptor data.
    terminator_free_mem_on_not_null(vp_config_page_info->ver_desc);

    // Free memory related to buffer information.
    for(i=0; i < vp_config_page_info->num_bufs; i++)
    {
             if(vp_config_page_info->buf_info[i].buf != NULL && //only find the memoru is free or not if buf is not NULL
            !(find_free_mem(head, vp_config_page_info->buf_info[i].buf ) ))
            {
               terminator_free_mem_on_not_null(vp_config_page_info->buf_info[i].buf);
               add_to_free_mem_list(&head,vp_config_page_info->buf_info[i].buf);
            }
    }
    destroy_free_mem_list(head);
    terminator_free_mem_on_not_null(vp_config_page_info->buf_info);
}

fbe_status_t sas_virtual_phy_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t *count;

    // Check if the page is already in the unsupported table
    if(!sas_virtual_phy_is_eses_page_supported(cdb_opcode, diag_page_code))
    {
        return(FBE_STATUS_OK);
    }

    switch(cdb_opcode)
    {
        case FBE_SCSI_RECEIVE_DIAGNOSTIC:
            count = &unsupported_eses_page_info.unsupported_receive_diag_page_count;
            unsupported_eses_page_info.unsupported_receive_diag_pages[*count]
                = diag_page_code;
            (*count)++;
            break;
        case FBE_SCSI_SEND_DIAGNOSTIC:
            count = &unsupported_eses_page_info.unsupported_send_diag_page_count;
            unsupported_eses_page_info.unsupported_send_diag_pages[*count]
                = diag_page_code;
            (*count)++;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return(status);
}

fbe_bool_t sas_virtual_phy_is_eses_page_supported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    fbe_u8_t *unsupported_page_table = NULL;
    fbe_u32_t count=0, i;

    switch(cdb_opcode)
    {
        case FBE_SCSI_RECEIVE_DIAGNOSTIC:
            unsupported_page_table = unsupported_eses_page_info.unsupported_receive_diag_pages;
            count = unsupported_eses_page_info.unsupported_receive_diag_page_count;
            break;
        case FBE_SCSI_SEND_DIAGNOSTIC:
            unsupported_page_table = unsupported_eses_page_info.unsupported_send_diag_pages;
            count = unsupported_eses_page_info.unsupported_send_diag_page_count;
            break;
        default:
            return(FBE_FALSE);
    }

    for(i=0; i<count; i++)
    {
        if(unsupported_page_table[i] == diag_page_code)
        {
            return(FBE_FALSE);
        }
    }
    return(FBE_TRUE);
}


fbe_status_t sas_virtual_phy_get_eses_page_info(terminator_virtual_phy_t * self,
                                                terminator_vp_eses_page_info_t **vp_eses_page_info)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    *vp_eses_page_info = &info->eses_page_info;

    return(FBE_STATUS_OK);
}


fbe_status_t sas_virtual_phy_set_ver_desc(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      ses_ver_desc_struct ver_desc)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t ver_desc_start_index;
    fbe_u8_t num_ver_descs;
    fbe_u8_t i;
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    status = terminator_eses_get_subencl_ver_desc_position(&(info->eses_page_info),
                                                        subencl_type,
                                                        side,
                                                        &ver_desc_start_index,
                                                        &num_ver_descs);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    for(i = ver_desc_start_index; i < (num_ver_descs + ver_desc_start_index); i++)
    {
        if(info->eses_page_info.vp_config_diag_page_info.ver_desc[i].comp_type == comp_type)
        {
            fbe_copy_memory(&info->eses_page_info.vp_config_diag_page_info.ver_desc[i],
                   &ver_desc,
                   sizeof(ses_ver_desc_struct));
            status = FBE_STATUS_OK;
            break;
        }
    }

    return(status);
}


fbe_status_t sas_virtual_phy_get_ver_desc(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      ses_ver_desc_struct *ver_desc)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t ver_desc_start_index;
    fbe_u8_t num_ver_descs;
    fbe_u8_t i;
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    status = terminator_eses_get_subencl_ver_desc_position(&(info->eses_page_info),
                                                        subencl_type,
                                                        side,
                                                        &ver_desc_start_index,
                                                        &num_ver_descs);
    RETURN_ON_ERROR_STATUS;

    status = FBE_STATUS_GENERIC_FAILURE;
    for(i = ver_desc_start_index; i < (num_ver_descs + ver_desc_start_index); i++)
    {
        if(info->eses_page_info.vp_config_diag_page_info.ver_desc[i].comp_type == comp_type)
        {
            fbe_copy_memory(ver_desc,
                   &info->eses_page_info.vp_config_diag_page_info.ver_desc[i],
                   sizeof(ses_ver_desc_struct));
            status = FBE_STATUS_OK;
            break;
        }
    }

    return(status);
}

fbe_status_t sas_virtual_phy_set_ver_num(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      CHAR *ver_num)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_ver_desc_struct ver_desc;

    status = sas_virtual_phy_get_ver_desc(self, subencl_type, side, comp_type, &ver_desc);
    RETURN_ON_ERROR_STATUS;

    fbe_copy_memory(ver_desc.cdes_rev.cdes_2_0_rev.comp_rev_level, 
                    ver_num, 
                    sizeof(ver_desc.cdes_rev.cdes_2_0_rev.comp_rev_level));

    status = sas_virtual_phy_set_ver_desc(self, subencl_type, side, comp_type, ver_desc);
    RETURN_ON_ERROR_STATUS;

    return status;
}

fbe_status_t sas_virtual_phy_get_ver_num(terminator_virtual_phy_t * self,
                                      ses_subencl_type_enum subencl_type,
                                      terminator_eses_subencl_side side,
                                      fbe_u8_t  comp_type,
                                      CHAR *ver_num)
{
    fbe_status_t status = FBE_STATUS_OK;
    ses_ver_desc_struct ver_desc;

    status = sas_virtual_phy_get_ver_desc(self, subencl_type, side, comp_type, &ver_desc);
    RETURN_ON_ERROR_STATUS;

    fbe_copy_memory(ver_num, 
                    ver_desc.cdes_rev.cdes_2_0_rev.comp_rev_level, 
                    sizeof(ver_desc.cdes_rev.cdes_2_0_rev.comp_rev_level));

    return status;
}

fbe_status_t sas_virtual_phy_set_activate_time_interval(terminator_virtual_phy_t * self, fbe_u32_t time_interval)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info->activate_time_intervel = time_interval;

    return status;
}

fbe_status_t sas_virtual_phy_get_activate_time_interval(terminator_virtual_phy_t * self, fbe_u32_t *time_interval)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *time_interval = info->activate_time_intervel;

    return status;
}

fbe_status_t sas_virtual_phy_set_reset_time_interval(terminator_virtual_phy_t * self, fbe_u32_t time_interval)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info->reset_time_intervel = time_interval;

    return status;
}

fbe_status_t sas_virtual_phy_get_reset_time_interval(terminator_virtual_phy_t * self, fbe_u32_t *time_interval)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *time_interval = info->reset_time_intervel;

    return status;
}

fbe_status_t sas_virtual_phy_set_download_microcode_stat_desc(terminator_virtual_phy_t * self,
                                                              fbe_download_status_desc_t download_stat_desc)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);

    fbe_copy_memory(&info->eses_page_info.vp_download_micrcode_stat_diag_page_info.download_status_desc,
           &download_stat_desc,
           sizeof(download_stat_desc));

    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_get_download_microcode_stat_desc(terminator_virtual_phy_t * self,
                                                              fbe_download_status_desc_t *download_stat_desc)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);

    fbe_copy_memory(download_stat_desc,
           &info->eses_page_info.vp_download_micrcode_stat_diag_page_info.download_status_desc,
           sizeof(fbe_download_status_desc_t));

    return(FBE_STATUS_OK);
}

fbe_status_t 
sas_virtual_phy_get_vp_download_microcode_ctrl_page_info(
    terminator_virtual_phy_t * self,
    vp_download_microcode_ctrl_diag_page_info_t **vp_download_ctrl_page_info)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);

    *vp_download_ctrl_page_info = &info->eses_page_info.vp_download_microcode_ctrl_diag_page_info;

    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_get_firmware_download_status(terminator_virtual_phy_t * self, fbe_u8_t *status)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);

    *status = info->eses_page_info.vp_download_micrcode_stat_diag_page_info.download_status_desc.status;

    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_set_firmware_download_status(terminator_virtual_phy_t * self, fbe_u8_t status)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);

    info->eses_page_info.vp_download_micrcode_stat_diag_page_info.download_status_desc.status = status;

    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_increment_gen_code(terminator_virtual_phy_t * self)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    info->eses_page_info.vp_config_diag_page_info.gen_code++;

    // set unit attention is now set with a seperate interface
    // for flexibility

    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_get_unit_attention(terminator_virtual_phy_t * self,
                                                fbe_u8_t *unit_attention)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    *unit_attention = info->unit_attention ;
    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_set_unit_attention(terminator_virtual_phy_t * self,
                                                fbe_u8_t unit_attention)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    info->unit_attention = unit_attention;
    return(FBE_STATUS_OK);
}


fbe_status_t sas_virtual_phy_init(void)
{
    fbe_zero_memory(&unsupported_eses_page_info, sizeof(terminator_sas_unsupported_eses_page_info_t));
    return(FBE_STATUS_OK);
}


fbe_status_t sas_virtual_phy_get_encl_eses_status(terminator_virtual_phy_t * self,
                                                  ses_stat_elem_encl_struct *encl_stat)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_copy_memory(encl_stat, &info->encl_status, sizeof(ses_stat_elem_encl_struct));
    return (FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_set_encl_eses_status(terminator_virtual_phy_t * self,
                                                  ses_stat_elem_encl_struct encl_stat)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_copy_memory(&info->encl_status, &encl_stat, sizeof(ses_stat_elem_encl_struct));
    return (FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_get_chassis_encl_eses_status(terminator_virtual_phy_t * self,
                                                          ses_stat_elem_encl_struct *chassis_encl_stat)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_copy_memory(chassis_encl_stat, &info->chassis_encl_status, sizeof(ses_stat_elem_encl_struct));
    return (FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_set_chassis_encl_eses_status(terminator_virtual_phy_t * self,
                                                          ses_stat_elem_encl_struct chassis_encl_stat)
{
    terminator_sas_virtual_phy_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_copy_memory(&info->chassis_encl_status, &chassis_encl_stat, sizeof(ses_stat_elem_encl_struct));
    return (FBE_STATUS_OK);
}

/**************************************************************************
 *  sas_virtual_phy_set_buf()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the data in a particular buffer.
 *
 *  PARAMETERS:
 *    virtual phy object, buffer Identifier, buffer to be copied from,
 *    length of the buffer from which data is to be copied.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      We only allocate the required memory for the buffer when the TAPI
 *      is used. So when a request for setting data in a buffer comes and it
 *      requires different amount of memory than is currently existing in the
 *      buffer, we free the previously allocated memory and allocate the
 *      currently required memory again and start copying the data into the
 *      buffer.
 *
 *  HISTORY:
 *      Jan-09-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t sas_virtual_phy_set_buf(terminator_virtual_phy_t * self,
                                    fbe_u8_t buf_id,
                                    fbe_u8_t *buf,
                                    fbe_u32_t len)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t i;
    fbe_u8_t **buf_ptr;
    terminator_eses_buf_info_t *buf_info;
    fbe_u8_t num_bufs;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    buf_info = info->eses_page_info.vp_config_diag_page_info.buf_info;
    num_bufs = info->eses_page_info.vp_config_diag_page_info.num_bufs;
    for(i=0; i < num_bufs; i++)
    {
        if(buf_id == buf_info[i].buf_id)
        {
            buf_ptr = &buf_info[i].buf;
            if(buf_info[i].buf_len != len)
            {
                // We are asked to allocate different amount of
                // memory for the buffer than is currently existing.
                // The user can ask to deallocate all the memory by
                // setting the "len" to 0.

                // Free the currently allocated buffer memory
                if(*buf_ptr != NULL)
                {
                    fbe_terminator_free_memory(*buf_ptr);
                    buf_info[i].buf_len = 0;
                }

                // Allocate only the required memory for the buffer again.
                *buf_ptr = NULL;
                if(len != 0)
                {
                    *buf_ptr = (fbe_u8_t *)fbe_terminator_allocate_memory(len);
                    if(*buf_ptr == NULL)
                    {
                        return(FBE_STATUS_INSUFFICIENT_RESOURCES);
                    }
                }
                buf_info[i].buf_len = len;
            }

            // Copy the info into the newly allocated buffer
            if(len != 0)
            {
                fbe_copy_memory(*buf_ptr,
                    buf,
                    len);
            }

            status = FBE_STATUS_OK;
            break;
        }
    }
    return(status);
}

/**************************************************************************
 *  sas_virtual_phy_get_buf_info_by_buf_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the pointer to the "buffer information"(which contains
 *    various attributes of the buffer and the buffer itself) of a buffer.
 *
 *  PARAMETERS:
 *    virtual phy object handle, buffer Identifier, pointer to the buffer info.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jan-09-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t sas_virtual_phy_get_buf_info_by_buf_id(terminator_virtual_phy_t * self,
                                                    fbe_u8_t buf_id,
                                                    terminator_eses_buf_info_t **buf_info_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    terminator_eses_buf_info_t *buf_info;
    fbe_u8_t i, num_bufs;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    num_bufs = info->eses_page_info.vp_config_diag_page_info.num_bufs;
    buf_info = info->eses_page_info.vp_config_diag_page_info.buf_info;
    for(i=0; i < num_bufs; i++)
    {
        if(buf_id == buf_info[i].buf_id)
        {
            *buf_info_ptr = &buf_info[i];
            status = FBE_STATUS_OK;
            break;
        }
    }
    return(status);

}



/**************************************************************************
 *  sas_virtual_phy_set_bufid2_buf_area_to_bufid1()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the buffer area of buffer with buf_id2 to point to
 *    the buffer area of buffer with buf_id1. So any reads or writes to
 *    either of these two buffers operate on the SAME buffer area(memory).
 *
 *  PARAMETERS:
 *    virtual phy object handle,
 *    buf_id1 - The buffer ID of the first buffer
 *    buf_id2 - The buffer ID of the second buffer
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t  sas_virtual_phy_set_bufid2_buf_area_to_bufid1(terminator_virtual_phy_t * self,
                                                            fbe_u8_t buf_id1,
                                                            fbe_u8_t buf_id2)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    terminator_eses_buf_info_t *buf_info1 = NULL, *buf_info2 = NULL, *buf_info = NULL;
    fbe_u8_t i, num_bufs;

    if(buf_id1 == buf_id2)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: buffer id's are equal\n", __FUNCTION__);     
        return(status);
    }

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    num_bufs = info->eses_page_info.vp_config_diag_page_info.num_bufs;
    buf_info = info->eses_page_info.vp_config_diag_page_info.buf_info;
    for(i=0; i < num_bufs; i++)
    {
        if(buf_id1 == buf_info[i].buf_id)
        {
            buf_info1 = &buf_info[i];
        }
        else if(buf_id2 == buf_info[i].buf_id)
        {
            buf_info2 = &buf_info[i];
        }

        if((buf_info1 != NULL) &&
           (buf_info2 != NULL))
        {
            // If the second buffer has some buffer area(memory) already 
            // allocated then free the memory.
            //terminator_free_mem_on_not_null(buf_info2->buf);

            // Now let buffer area of second buffer point to the 
            // buffer area of first buffer
            buf_info2->buf_len = buf_info1->buf_len;
            buf_info2->buf = buf_info1->buf;
            status = FBE_STATUS_OK;
        }
    }
    return(status);
}


fbe_status_t sas_virtual_phy_get_diplay_eses_status(terminator_virtual_phy_t * self,
                                                    terminator_eses_display_character_id display_character_id,
                                                    ses_stat_elem_display_struct *display_stat_elem)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_display_characters;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_display_characters(encl_type, &max_display_characters);
    RETURN_ON_ERROR_STATUS;

    /* There is one display element for each display character */
    if (display_character_id >= max_display_characters)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }


    // There is one display status element for each display character.
    fbe_copy_memory(display_stat_elem, &info->display_status[display_character_id], sizeof(ses_stat_elem_display_struct));
    return(FBE_STATUS_OK);
}

fbe_status_t sas_virtual_phy_set_diplay_eses_status(terminator_virtual_phy_t * self,
                                                    terminator_eses_display_character_id display_character_id,
                                                    ses_stat_elem_display_struct display_stat_elem)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;
    fbe_u8_t max_display_characters;
    fbe_sas_enclosure_type_t encl_type;

    status = sas_virtual_phy_get_enclosure_type(self, &encl_type);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_max_display_characters(encl_type, &max_display_characters);
    RETURN_ON_ERROR_STATUS;

    /* There is one display element for each display character */
    if (display_character_id >= max_display_characters)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }


    // There is one display status element for each display character.
    fbe_copy_memory(&info->display_status[display_character_id], &display_stat_elem, sizeof(ses_stat_elem_display_struct));
    return(FBE_STATUS_OK);
}



/**************************************************************************
 *  sas_virtual_phy_push_enclosure_firmware_download_record()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function add an enclosure firmware download record to the queue.
 *
 *  PARAMETERS:
 *    self: virtual phy object pointer
 *    subencl_type
 *    side
 *    comp_type: component type
 *    new_rev_number: CHAR[5]
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  HISTORY:
 *      2010-8-12: Bo Gao created.
 **************************************************************************/
fbe_status_t sas_virtual_phy_push_enclosure_firmware_download_record(terminator_virtual_phy_t * self,
                                                                                    ses_subencl_type_enum subencl_type,
                                                                                    terminator_eses_subencl_side side,
                                                                                    fbe_u8_t  comp_type,
                                                                                    fbe_u32_t slot_num,
                                                                                    CHAR   *new_rev_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_virtual_phy_info_t * info = NULL;
    terminator_enclosure_firmware_new_rev_record_t *new_rev = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    new_rev = fbe_terminator_allocate_memory(sizeof(terminator_enclosure_firmware_new_rev_record_t));
    if (new_rev == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_rev->subencl_type = subencl_type;
    new_rev->side = side;
    new_rev->comp_type = comp_type;
    new_rev->slot_num = slot_num;
    fbe_copy_memory(new_rev->new_rev_number, new_rev_number, sizeof(new_rev->new_rev_number));
    fbe_queue_push(&info->new_firmware_rev_queue_head, &new_rev->queue_element);

    return status;
}

/**************************************************************************
 *  sas_virtual_phy_pop_enclosure_firmware_download_record()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function looks for an enclosure firmware download record specified by its sub-enclosure type,
 *    side and component type in the queue, and copies back the matched comp type and new revision number.
 *
 *  PARAMETERS:
 *    self: virtual phy object pointer
 *    subencl_type
 *    side
 *    comp_type: component type
 *    slot_num
 *    new_rev_number: CHAR[5]
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  HISTORY:
 *      2010-8-12: Bo Gao created.
 **************************************************************************/
fbe_status_t sas_virtual_phy_pop_enclosure_firmware_download_record(terminator_virtual_phy_t * self,
                                                                                    ses_subencl_type_enum subencl_type,
                                                                                    terminator_eses_subencl_side side,
                                                                                    fbe_u32_t slot_num,
                                                                                    ses_comp_type_enum  *comp_type_p,
                                                                                    CHAR   *new_rev_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;
    terminator_enclosure_firmware_new_rev_record_t *new_rev = NULL;
    terminator_enclosure_firmware_new_rev_record_t *next_new_rev = NULL;
    fbe_bool_t recordFound = FBE_FALSE;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_rev = (terminator_enclosure_firmware_new_rev_record_t *)fbe_queue_front(&info->new_firmware_rev_queue_head);
    while(new_rev != NULL)
    {
        next_new_rev = (terminator_enclosure_firmware_new_rev_record_t *)fbe_queue_next(&info->new_firmware_rev_queue_head,
                &new_rev->queue_element);

        if(new_rev->subencl_type == subencl_type && new_rev->side == side && new_rev->slot_num == slot_num)
        {
            /* We download init string image and FPGA image first and do the activate together with the expander image.
             * So when the terminator does the activate for expander image, we need to pop out the record for
             * init string image and FPGA image. It is Ok that saved_new_rev got overwritten by the next rev because 
             * the ESP will do the expander image (the primary image) at last. 
             */ 
            fbe_copy_memory(new_rev_number, new_rev->new_rev_number, sizeof(new_rev->new_rev_number));
            *comp_type_p = new_rev->comp_type;

            fbe_queue_remove(&new_rev->queue_element);
            fbe_terminator_free_memory(new_rev);

            recordFound = FBE_TRUE;
        }

        new_rev = next_new_rev;
    }

    if (!recordFound)
    {
        /* Did not find any record.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/**************************************************************************
 *  sas_virtual_phy_update_enclosure_firmware_download_record()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function check enclosure firmware download record queue, if already
 *    exists update the new revision number filed, else insert new record
 *
 *  PARAMETERS:
 *    self: virtual phy object pointer
 *    subencl_type
 *    side
 *    comp_type: component type
 *    slot_num
 *    new_rev_number: CHAR[5]
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  HISTORY:
 *      2010-8-12: Bo Gao created.
 **************************************************************************/
fbe_status_t sas_virtual_phy_update_enclosure_firmware_download_record(terminator_virtual_phy_t * self,
                                                                                    ses_subencl_type_enum subencl_type,
                                                                                    terminator_eses_subencl_side side,
                                                                                    ses_comp_type_enum  comp_type,
                                                                                    fbe_u32_t slot_num,
                                                                                    CHAR   *new_rev_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_virtual_phy_info_t * info = NULL;
    terminator_enclosure_firmware_new_rev_record_t *new_rev = NULL, *first_elem = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // first check if record for the same target exists
    first_elem = new_rev = (terminator_enclosure_firmware_new_rev_record_t *)fbe_queue_front(&info->new_firmware_rev_queue_head);
    while(new_rev != NULL)
    {
        // if found update the new revision number field
        if(new_rev->subencl_type == subencl_type && new_rev->side == side
                && new_rev->comp_type == comp_type && new_rev->slot_num == slot_num)
        {
            fbe_copy_memory(new_rev->new_rev_number, new_rev_number, sizeof(new_rev->new_rev_number));
            break;
        }
        new_rev = (terminator_enclosure_firmware_new_rev_record_t *)fbe_queue_next(&info->new_firmware_rev_queue_head, &new_rev->queue_element);
    }

    // if not found, allocate new record and push into the queue
    if(new_rev == NULL)
    {
        new_rev = fbe_terminator_allocate_memory(sizeof(terminator_enclosure_firmware_new_rev_record_t));
        if (new_rev == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        new_rev->subencl_type = subencl_type;
        new_rev->side = side;
        new_rev->comp_type = comp_type;
        new_rev->slot_num = slot_num;
        fbe_copy_memory(new_rev->new_rev_number, new_rev_number, sizeof(new_rev->new_rev_number));

        fbe_queue_push(&info->new_firmware_rev_queue_head, &new_rev->queue_element);
    }

    return status;
}
