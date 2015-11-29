/***************************************************************************
 * Copyright (C) EMC Corporation 2013-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimPmpPartition
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of the SimPmpPartition class.
 *
 * FUNCTIONS:    Just the constructor and destructor are public.
 *
 * NOTES:        The real lxPMP implemenation uses a partition on the boot
 *               device.  In simulation we implement this partition as a file.
 *               The constructor creates and initializes the file, if it didn't
 *               already exist.  The destructor deletes the file.
 *
 *
 **************************************************************************/
#ifndef SIMPMPPARTITION_H
#define SIMPMPPARTITION_H

typedef enum PMP_SP_e {
    PMP_SPA,
    PMP_SPB
} PMP_SP_t;

class SimPmpPartition {
 public:
  SimPmpPartition(PMP_SP_t sp) {
    mSPid = sp;
    createAndInitPmpPartition();
  }
  
  ~SimPmpPartition()
  {
    mut_printf(MUT_LOG_HIGH,"Deleted PMP partition for SP%c", mSPid == PMP_SPB ? 'B' : 'A');
    if(PMP_SPA == mSPid) {
      csx_p_file_delete(PMP_A_FILENAME);
    } else {
      csx_p_file_delete(PMP_B_FILENAME);
    }
  }

 static void DeletePmpFiles(void) {
    mut_printf(MUT_LOG_HIGH,"Deleted PMP files for SPA and SPB");
    csx_p_file_delete(PMP_A_FILENAME);
    csx_p_file_delete(PMP_B_FILENAME);
  }

  private:
  void createAndInitPmpPartition(void)
  {
    csx_status_e status;
    csx_size_t bytesWritten;
    csx_uoffset_t seek_pos_rv=0;    
    csx_cstring_t filename;

    if( mSPid == PMP_SPA) {
      filename = PMP_A_FILENAME;
    } else {
      filename = PMP_B_FILENAME;
    }
    // If the PMP partition file exists, use it.
    status = csx_p_file_open(EmcpalClientGetCsxModuleContext(EmcpalDriverGetCurrentClientObject()),
                             &mPartitionHandle, filename, "r");
    if( CSX_STATUS_SUCCESS == status ) {
      // The file could be opened read-only, so it exists.  No reason to create a new one.
      mut_printf(MUT_LOG_HIGH,"Using existing PMP partition %s", filename);
      status = csx_p_file_close(&mPartitionHandle);
      CSX_ASSERT_H_RDC(CSX_STATUS_SUCCESS == status);
      return;
    }
    
    // Create the partition file.  It will be the same size as a KH PMP partition.
    status = csx_p_file_open(EmcpalClientGetCsxModuleContext(EmcpalDriverGetCurrentClientObject()),
                             &mPartitionHandle, filename, "WxbD");
    CSX_ASSERT_H_RDC(CSX_STATUS_SUCCESS == status);
        
    // Seek to the end of the file
    status = csx_p_file_seek(&mPartitionHandle, CSX_P_FILE_SEEK_TYPE_SET,
                             PMP_PARTITION_SIZE, &seek_pos_rv);
    CSX_ASSERT_H_RDC(CSX_STATUS_SUCCESS == status);

    // Write
    status = csx_p_file_write(&mPartitionHandle,
                              (csx_cbytes_t)"\n",
                              (csx_size_t) strlen("\n"),
                              &bytesWritten);
    CSX_ASSERT_H_RDC(CSX_STATUS_SUCCESS == status);
    // Close
    status = csx_p_file_close(&mPartitionHandle);
    CSX_ASSERT_H_RDC(CSX_STATUS_SUCCESS == status);
    // Initialize the file
    initPmpPartition();
    mut_printf(MUT_LOG_HIGH,"Created PMP partition %s", filename);
  }

  //
  // initPmpPartition
  //
  // Description: Run lxpmp to initialize the "partition".
  //
  //    
  void initPmpPartition(void)
  {
    int status;
    int verbose = 0;
    char *argv_a_init[] = {"lxpmp", "--brief", "--force", "--file", 
                           PMP_A_FILENAME, "--init", PMP_A_FILENAME};
    char *argv_b_init[] = {"lxpmp", "--brief", "--force", "--file", 
                           PMP_B_FILENAME, "--init", PMP_B_FILENAME};
    if( PMP_SPB == mSPid ) {
      if( verbose ) {
        mut_printf(MUT_LOG_HIGH,"%s<%d>: %s %s %s %s %s", __FUNCTION__, __LINE__,
                  argv_b_init[0], argv_b_init[1], argv_b_init[2], argv_b_init[3], argv_b_init[4]);
      }
      status = lxpmp_main( sizeof(argv_b_init)/sizeof(char *), argv_b_init );
    } else {
      if( verbose ) {
        mut_printf(MUT_LOG_HIGH,"%s<%d>: %s %s %s %s %s", __FUNCTION__, __LINE__,
                  argv_a_init[0], argv_a_init[1], argv_a_init[2], argv_a_init[3], argv_a_init[4]);
      }
      status = lxpmp_main( sizeof(argv_a_init)/sizeof(char *), argv_a_init );
    }
    CSX_ASSERT_H_RDC(0 == status);
  }

  csx_p_file_t           mPartitionHandle;
  PMP_SP_t               mSPid;
};
#endif // SIMPMPPARTITION_H
