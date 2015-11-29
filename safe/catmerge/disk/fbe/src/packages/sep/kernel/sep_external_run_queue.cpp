#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "EmcPAL.h"
#include "BasicLib/BasicCPUGroupPriorityMontitoredThreadBasicWaiterInterface.h"
#include "BasicLib/BasicWaiter.h"

extern "C" void
transport_run_queue_perform_callback(fbe_queue_element_t *queue_element);

extern "C" void
sep_external_run_queue_push_queue_element(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id);

void * __cdecl operator new( size_t   size, void  *pBuffer )
{
    return( pBuffer );
}


void
sep_external_run_queue_push_queue_element(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id)
{
    // Go from BasicWaiter back to BasicCWaiter on signal, calling handler.
    // Variants because we embed the value to return for the queueing clas sin the vtable rather
    // than using memory in every object
    class EmbeddedWaiterQC_AlternateGroup : public BasicWaiter {
    public:
        void Signaled(EMCPAL_STATUS Status) CSX_CPP_OVERRIDE {
            fbe_queue_element_t *queue_element = reinterpret_cast<fbe_queue_element_t*>(this);
            fbe_queue_element_init(queue_element);
            transport_run_queue_perform_callback(queue_element);
        }

        // SEP gets its own queue to schedule to.
        BasicWaiterQueuingClass GetQueuingClass() const CSX_CPP_OVERRIDE { return QC_AlternateGroup; }
    };    
    
    // Blow up compilation on bad assumptions.
    { CSX_ASSERT_AT_COMPILE_TIME_GLOBAL_SCOPE(sizeof(fbe_queue_element_t) >= sizeof(BasicWaiter)) }

    // This basically sets the vtable pointer in the first pointer of toQueue, leaving the second
    // pointer as the link field.
    BasicWaiter * waiter = new((void*)queue_element) EmbeddedWaiterQC_AlternateGroup();
   
    BasicCPUGroupPriorityMontitoredThreadQueueBasicWaiter(waiter, cpu_id);
}

