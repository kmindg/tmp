#ifndef QOS_QUEUE_MANAGER_H_
#define QOS_QUEUE_MANAGER_H_

#include "QoS.h"

#include "K10HashTable.h"
#include "BasicLib/BasicTimer.h"

#define QQOS_TIMESTAMP_SCALE        10000000 


class QueueTokenBucket
{
 public:
 QueueTokenBucket():
  totalIos_(0),
    firstIo_(true)
      {}
  ULONGLONG iops_;
  ULONGLONG bandwidth_;
  bool allowThrough(ULONG outstanding)
  {
    if(firstIo_)
      {
	totalIos_++;
        firstIo_=false;
        startTime_=EmcpalQueryInterruptTime();
        return true;
      }
    ULONGLONG endTime = EmcpalQueryInterruptTime();
    if(outstanding && shouldReject(endTime,1))
      return false; 
     totalIos_++;
    if(( endTime > startTime_) && 
       (endTime - startTime_) > QQOS_TIMESTAMP_SCALE)
      startTime_ = endTime - QQOS_TIMESTAMP_SCALE;
     return true;
  }

  virtual bool shouldReject(ULONGLONG endTime, ULONG blocks)=0;
  virtual ~QueueTokenBucket(){}
 protected:
  ULONGLONG totalIos_;
  ULONGLONG startTime_;
  bool firstIo_;              //starting point of monitoring.
};
class QueueIopsTokenBucket : public QueueTokenBucket
{
 public:
 QueueIopsTokenBucket(ULONGLONG iolimit):QueueTokenBucket()
    {iops_=iolimit;}
  virtual bool shouldReject(ULONGLONG endTime, ULONG blocks)
  {
    ULONGLONG limit=iops_;
    bool reject = (endTime - startTime_) < (QQOS_TIMESTAMP_SCALE/limit);
    if(!reject)
      startTime_ += QQOS_TIMESTAMP_SCALE/limit;
    return reject;
  }
};



#endif
