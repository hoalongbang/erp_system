// Modules/TaskEngine/Service/TaskExecutorService.h
// This header is a convenience include for the ITaskExecutorService interface
// and points to the actual TaskEngine implementation header.

#ifndef MODULES_TASKENGINE_SERVICE_TASKEXECUTORSERVICE_H
#define MODULES_TASKENGINE_SERVICE_TASKEXECUTORSERVICE_H

// Include the actual TaskEngine implementation header, which provides the concrete
// class that implements ITaskExecutorService.
#include "TaskEngine.h"

// You might also include the interface explicitly for clarity, though TaskEngine.h itself should pull it in.
#include "ITaskExecutorService.h"

// No further code in this file, as it serves as a simple alias/wrapper include.

#endif // MODULES_TASKENGINE_SERVICE_TASKEXECUTORSERVICE_H