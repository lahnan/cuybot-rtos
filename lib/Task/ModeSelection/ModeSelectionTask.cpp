#include <ModeSelection/ModeSelectionTask.h>

extern int mode;

SemaphoreHandle_t modeChangeSemaphore;

ModeSelectionTask::ModeSelectionTask(UltrasonicTask &ultrasonicTask, IRTask &irTask, Buzzer &buzzer, LedControl &ledControl, SpinningTask &spinningTask, AutoPatrolTask &autoPatrolTask)
    : _ultrasonicTask(ultrasonicTask), _irTask(irTask), _buzzer(buzzer), _ledControl(ledControl), _spinningTask(spinningTask), _autoPatrolTask(autoPatrolTask),_lastMode(1) {
    _taskHandle = NULL;
    modeChangeSemaphore = xSemaphoreCreateBinary();
    if (modeChangeSemaphore == NULL) {
        Serial.println("Error creating modeChangeSemaphore!");
    }
}

void ModeSelectionTask::startTask() {
    if (_taskHandle == NULL) {
        xTaskCreate(modeSelectionTaskFunction, "ModeSelectionTask", _taskStackSize, this, _taskPriority, &_taskHandle);
    }
}

void ModeSelectionTask::stopTask() {
    if (_taskHandle != NULL) {
        vTaskDelete(_taskHandle);
        _taskHandle = NULL;
        Serial.println("mode selection task stopped!");
    }
}

void ModeSelectionTask::modeSelectionTaskFunction(void *parameter) {
    ModeSelectionTask *self = static_cast<ModeSelectionTask*>(parameter);
    
    while (true) {
        if (xSemaphoreTake(modeChangeSemaphore, portMAX_DELAY) == pdTRUE) {
            self->_buzzer.beep(100);
            switch (mode) {
                case 1: // WebSocketTask only
                    Serial.println("Mode 1: Manual Control");
                    self->_ledControl.setMode(mode);
                    if (self->_ultrasonicTask.getIsRunning()) {
                        self->_ultrasonicTask.stopTask();
                    }
                    if (self->_irTask.getIsRunning()) {
                        self->_irTask.stopTask();
                    }
                    break;

                case 2: // WebSocketTask + UltrasonicTask
                    Serial.println("Mode 2: Obstacle Avoidance");
                    self->_ledControl.setMode(mode);
                    if (!self->_ultrasonicTask.getIsRunning()) {
                        self->_ultrasonicTask.startTask();
                    }
                    if (self->_irTask.getIsRunning()) {
                        self->_irTask.stopTask();
                    }
                    break;

                case 3: // IRTask only (Line Following)
                    Serial.println("Mode 3: Line Following");
                    self->_ledControl.setMode(mode);
                    if (self->_ultrasonicTask.getIsRunning()) {
                        self->_ultrasonicTask.stopTask();
                    }
                    if (!self->_irTask.getIsRunning()) {
                        self->_irTask.startTask();
                    }
                    break;

                case 4: // Patrol Mode or Tuning
                    Serial.println("Mode 4: Auto Patrol");
                    self->_ledControl.setMode(mode);
                    if (self->_ultrasonicTask.getIsRunning()) {
                        self->_ultrasonicTask.stopTask();
                    }
                    if (self->_irTask.getIsRunning()) {
                        self->_irTask.stopTask();
                    }
                    break;

                default:
                    Serial.println("Unknown mode. No action taken.");
                    self->_ledControl.setMode(4);
                    if (self->_ultrasonicTask.getIsRunning()) {
                        self->_ultrasonicTask.stopTask();
                    }
                    if (self->_irTask.getIsRunning()) {
                        self->_irTask.stopTask();
                    }
                    break;
            }

            self->_lastMode = mode; 
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ModeSelectionTask::triggerModeChange(int newMode) {
    mode = newMode;
    xSemaphoreGive(modeChangeSemaphore);
}