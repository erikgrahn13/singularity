#include <iostream>

#include <CoreAudio/CoreAudio.h>
#include "coreAudio.h"

struct CoreHandle{
    AudioDeviceID inputDeviceID = kAudioObjectUnknown;
    AudioDeviceID outputDeviceID = kAudioObjectUnknown;
    AudioDeviceIOProcID ioProcID;
} mCoreHandle;

// Callback function for processing audio
static OSStatus AudioIOProc(
    AudioDeviceID inDevice, 
    const AudioTimeStamp* inNow, 
    const AudioBufferList* inInputData, 
    const AudioTimeStamp* inInputTime, 
    AudioBufferList* outOutputData, 
    const AudioTimeStamp* inOutputTime, 
    void* inClientData)
{
    // Check if we have valid input and output data
    if (inInputData && outOutputData) {
        for (UInt32 i = 0; i < outOutputData->mNumberBuffers; i++) {
            AudioBuffer inputBuffer = inInputData->mBuffers[i];
            AudioBuffer outputBuffer = outOutputData->mBuffers[i];
            
            // Copy input buffer to output buffer (if formats match)
            if (inputBuffer.mDataByteSize == outputBuffer.mDataByteSize) {
                memcpy(outputBuffer.mData, inputBuffer.mData, inputBuffer.mDataByteSize);
            }
        }
    }
    
    return noErr;
}

void setupCoreAudio() {
    OSStatus status;
    //AudioDeviceID deviceID = kAudioObjectUnknown;
    UInt32 propertySize;
    

    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    propertySize = sizeof(mCoreHandle.inputDeviceID);
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize, &mCoreHandle.inputDeviceID);
    if (status != noErr) {
        printf("Error getting default input device: %d\n", status);
        std::cout << "Error getting default input device" << std::endl;

    }
    std::cout << "Success getting default input device: " << mCoreHandle.inputDeviceID << std::endl;

    // Debugging
    CFStringRef deviceNameInput = NULL;
    UInt32 propertySizeInput = sizeof(deviceNameInput);
        AudioObjectPropertyAddress propertyAddressInput = {
        kAudioObjectPropertyName,    // Property to get the name
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

     status = AudioObjectGetPropertyData(mCoreHandle.inputDeviceID, &propertyAddressInput, 0, NULL, &propertySizeInput, &deviceNameInput);

    if (status == noErr && deviceNameInput != NULL)
    {
    char deviceNameCStr[256];
    if (CFStringGetCString(deviceNameInput, deviceNameCStr, sizeof(deviceNameCStr), kCFStringEncodingUTF8)) {
        printf("Device Name: %s\n", deviceNameCStr);
    }
        CFRelease(deviceNameInput); 

    }


   // Get default output device
    AudioObjectPropertyAddress outputAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    propertySize = sizeof(mCoreHandle.outputDeviceID);
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &outputAddress, 0, NULL, &propertySize, &mCoreHandle.outputDeviceID);
    if (status != noErr) {
        printf("Error getting default output device: %d\n", status);
        return;
    }
    printf("Output Device ID: %d\n", mCoreHandle.outputDeviceID);

    // Get the name of the output device for debugging
    CFStringRef outputDeviceName = NULL;
    propertySize = sizeof(outputDeviceName);
    AudioObjectPropertyAddress nameAddress = {
        kAudioObjectPropertyName,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    status = AudioObjectGetPropertyData(mCoreHandle.outputDeviceID, &nameAddress, 0, NULL, &propertySize, &outputDeviceName);
    if (status == noErr && outputDeviceName != NULL) {
        char deviceNameCStr[256];
        if (CFStringGetCString(outputDeviceName, deviceNameCStr, sizeof(deviceNameCStr), kCFStringEncodingUTF8)) {
            printf("Output Device Name: %s\n", deviceNameCStr);
        }
        CFRelease(outputDeviceName);
    }

    // Set up the IOProc for output
    status = AudioDeviceCreateIOProcID(mCoreHandle.outputDeviceID, AudioIOProc, NULL, &mCoreHandle.ioProcID);
    if (status != noErr) {
        printf("Error creating IOProc for output: %d\n", status);
        return;
    }

    // Start the output device
    status = AudioDeviceStart(mCoreHandle.outputDeviceID, mCoreHandle.ioProcID);
    if (status != noErr) {
        printf("Error starting output device: %d\n", status);
        return;
    }
    printf("Success starting output device\n");

      // Step 3: Start the audio device
    // status = AudioDeviceStart(mCoreHandle.inputDeviceID, mCoreHandle.ioProcID);
    // if (status != noErr) {
    //     printf("Error starting audio device: %d\n", status);
    // }
    // std::cout << "Success starting device" << std::endl;


}

void stopCoreAudio()
{
    OSStatus status;


    // Step 4: Stop the audio device
    status = AudioDeviceStop(mCoreHandle.inputDeviceID, mCoreHandle.ioProcID);
    if (status != noErr) {
        printf("Error stopping audio device: %d\n", status);
    }
    std::cout << "Success stopping inputdevice" << std::endl;

        status = AudioDeviceStop(mCoreHandle.outputDeviceID, mCoreHandle.ioProcID);
    if (status != noErr) {
        printf("Error stopping audio device: %d\n", status);
    }
    std::cout << "Success stopping output device" << std::endl;
    
    // Step 5: Destroy the IOProc
    status = AudioDeviceDestroyIOProcID(mCoreHandle.outputDeviceID, mCoreHandle.ioProcID);
    if (status != noErr) {
        printf("Error destroying IOProc: %d\n", status);
    }
    std::cout << "Success destroying IOProc" << std::endl;
}