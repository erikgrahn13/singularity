#ifndef UNICODE
#define UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <iostream>
#include <windows.h>

// clang-format off
#include "asiosys.h" // This file needs to be included before asio.h
// clang-format on
#include "asio.h"
#include "asiodrivers.h"

#include "include/core/SkCanvas.h" // Include SkCanvas
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h" // Include SkPaint
#include "include/core/SkSurface.h"

extern AsioDrivers *asioDrivers;

enum
{
    // number of input and outputs supported by the host application
    // you can change these to higher or lower values
    kMaxInputChannels = 32,
    kMaxOutputChannels = 32
};

class DriverInfo
{
    // ASIOInit()
  public:
    ASIODriverInfo driverInfo;

    // ASIOGetChannels()
    long inputChannels;
    long outputChannels;

    // ASIOGetBufferSize()
    long minSize;
    long maxSize;
    long preferredSize;
    long granularity;

    // ASIOGetSampleRate()
    ASIOSampleRate sampleRate;

    // ASIOOutputReady()
    bool postOutput;

    // ASIOGetLatencies ()
    long inputLatency;
    long outputLatency;

    // ASIOCreateBuffers ()
    long inputBuffers;  // becomes number of actual created input buffers
    long outputBuffers; // becomes number of actual created output buffers
    ASIOBufferInfo bufferInfos[kMaxInputChannels + kMaxOutputChannels]; // buffer info's

    // ASIOGetChannelInfo()
    ASIOChannelInfo channelInfos[kMaxInputChannels + kMaxOutputChannels]; // channel info's
    // The above two arrays share the same indexing, as the data in them are linked together

    // Information from ASIOGetSamplePosition()
    // data is converted to double floats for easier use, however 64 bit integer can be used, too
    double nanoSeconds;
    double samples;
    double tcSamples; // time code samples

    // bufferSwitchTimeInfo()
    ASIOTime tInfo;           // time info state
    unsigned long sysRefTime; // system reference time, when bufferSwitch() was called

    // Signal the end of processing in this example
    bool stopped;
} mDriverInfo;

bool loadAsioDriver(char *name);
// static ASIODriverInfo driverInfo;
ASIOCallbacks asioCallbacks;

ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow)
{ // the actual processing callback.
    // Beware that this is normally in a seperate thread, hence be sure that you take care
    // about thread synchronization. This is omitted here for simplicity.

    // Get the buffer size in samples (usually defined by preferred buffer size)
    long bufferSize = mDriverInfo.preferredSize;

    // Find input channel 1 and output channels 1 and 2 in bufferInfos
    ASIOBufferInfo *inputBuffer = nullptr;
    ASIOBufferInfo *outputBuffer1 = nullptr;
    ASIOBufferInfo *outputBuffer2 = nullptr;

    for (int i = 0; i < mDriverInfo.inputBuffers + mDriverInfo.outputBuffers; i++)
    {
        if (mDriverInfo.bufferInfos[i].isInput && mDriverInfo.bufferInfos[i].channelNum == 0)
        {
            inputBuffer = &mDriverInfo.bufferInfos[i]; // Input channel 1 (index 0)
        }
        else if (!mDriverInfo.bufferInfos[i].isInput && mDriverInfo.bufferInfos[i].channelNum == 0)
        {
            outputBuffer1 = &mDriverInfo.bufferInfos[i]; // Output channel 1 (index 0)
        }
        else if (!mDriverInfo.bufferInfos[i].isInput && mDriverInfo.bufferInfos[i].channelNum == 1)
        {
            outputBuffer2 = &mDriverInfo.bufferInfos[i]; // Output channel 2 (index 1)
        }
    }

    if (inputBuffer && outputBuffer1 && outputBuffer2)
    {
        // Get the channel format from ASIOChannelInfo (assumed to be the same for input and output)
        ASIOSampleType sampleType = mDriverInfo.channelInfos[0].type;

        // Process according to the sample type (this example handles float32 and int16)
        switch (sampleType)
        {
        case ASIOSTFloat32LSB: {
            // 32-bit float (4 bytes per sample)
            float *in = (float *)inputBuffer->buffers[index];
            float *out1 = (float *)outputBuffer1->buffers[index];
            float *out2 = (float *)outputBuffer2->buffers[index];

            // Copy the input to both output channels
            memcpy(out1, in, bufferSize * sizeof(float)); // Send to output channel 1
            memcpy(out2, in, bufferSize * sizeof(float)); // Send to output channel 2
            break;
        }
        case ASIOSTInt16LSB: {
            // 16-bit int (2 bytes per sample)
            int16_t *in = (int16_t *)inputBuffer->buffers[index];
            int16_t *out1 = (int16_t *)outputBuffer1->buffers[index];
            int16_t *out2 = (int16_t *)outputBuffer2->buffers[index];

            // Copy the input to both output channels
            memcpy(out1, in, bufferSize * sizeof(int16_t)); // Send to output channel 1
            memcpy(out2, in, bufferSize * sizeof(int16_t)); // Send to output channel 2
            break;
        }
        case ASIOSTInt32LSB: {
            // 32-bit int (4 bytes per sample)
            int32_t *in = (int32_t *)inputBuffer->buffers[index];
            int32_t *out1 = (int32_t *)outputBuffer1->buffers[index];
            int32_t *out2 = (int32_t *)outputBuffer2->buffers[index];

            // Copy the input to both output channels
            memcpy(out1, in, bufferSize * sizeof(int32_t)); // Send to output channel 1
            memcpy(out2, in, bufferSize * sizeof(int32_t)); // Send to output channel 2
            break;
        }
        // Add more cases for other sample formats as needed
        default:
            std::cout << "Unsupported sample type." << std::endl;
            break;
        }
    }
    else
    {
        std::cout << "Input or output buffers not found." << std::endl;
    }

    // finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
    if (mDriverInfo.postOutput)
        ASIOOutputReady();

    return 0L;
}

void bufferSwitch(long index, ASIOBool processNow)
{ // the actual processing callback.
    // Beware that this is normally in a seperate thread, hence be sure that you take care
    // about thread synchronization. This is omitted here for simplicity.

    // as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
    // though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
    ASIOTime timeInfo;
    memset(&timeInfo, 0, sizeof(timeInfo));

    bufferSwitchTimeInfo(&timeInfo, index, processNow);
}

void sampleRateChanged(ASIOSampleRate sRate)
{
}

long asioMessages(long selector, long value, void *message, double *opt)
{
    // currently the parameters "value", "message" and "opt" are not used.
    long ret = 0;
    switch (selector)
    {
    case kAsioSelectorSupported:
        if (value == kAsioResetRequest || value == kAsioEngineVersion || value == kAsioResyncRequest ||
            value == kAsioLatenciesChanged
            // the following three were added for ASIO 2.0, you don't necessarily have to support them
            || value == kAsioSupportsTimeInfo || value == kAsioSupportsTimeCode || value == kAsioSupportsInputMonitor)
            ret = 1L;
        break;
    case kAsioResetRequest:
        // defer the task and perform the reset of the driver during the next "safe" situation
        // You cannot reset the driver right now, as this code is called from the driver.
        // Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
        // Afterwards you initialize the driver again.
        mDriverInfo.stopped; // In this sample the processing will just stop
        ret = 1L;
        break;
    case kAsioResyncRequest:
        // This informs the application, that the driver encountered some non fatal data loss.
        // It is used for synchronization purposes of different media.
        // Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
        // Windows Multimedia system, which could loose data because the Mutex was hold too long
        // by another thread.
        // However a driver can issue it in other situations, too.
        ret = 1L;
        break;
    case kAsioLatenciesChanged:
        // This will inform the host application that the drivers were latencies changed.
        // Beware, it this does not mean that the buffer sizes have changed!
        // You might need to update internal delay data.
        ret = 1L;
        break;
    case kAsioEngineVersion:
        // return the supported ASIO version of the host application
        // If a host applications does not implement this selector, ASIO 1.0 is assumed
        // by the driver
        ret = 2L;
        break;
    case kAsioSupportsTimeInfo:
        // informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
        // is supported.
        // For compatibility with ASIO 1.0 drivers the host application should always support
        // the "old" bufferSwitch method, too.
        ret = 1;
        break;
    case kAsioSupportsTimeCode:
        // informs the driver wether application is interested in time code info.
        // If an application does not need to know about time code, the driver has less work
        // to do.
        ret = 0;
        break;
    }
    return ret;
}

void initializeAsio()
{
    if (loadAsioDriver("Focusrite USB ASIO"))
    {
        auto result = ASIOInit(&mDriverInfo.driverInfo);
        if (result != ASE_OK)
        {
            asioDrivers->removeCurrentDriver();
            std::cout << "Error initializing driver" << std::endl;
        }

        result = ASIOGetChannels(&mDriverInfo.inputChannels, &mDriverInfo.outputChannels);
        if (result != ASE_OK)
        {
            ASIOExit();
            asioDrivers->removeCurrentDriver();
            std::cout << "Error getting channel count" << std::endl;
        }
        std::cout << "ASIOGetChannels (inputs: " << mDriverInfo.inputChannels
                  << ", outputs: " << mDriverInfo.outputChannels << ")" << std::endl;

        result = ASIOGetBufferSize(&mDriverInfo.minSize, &mDriverInfo.maxSize, &mDriverInfo.preferredSize,
                                   &mDriverInfo.granularity);
        if (result != ASE_OK)
        {
            ASIOExit();
            asioDrivers->removeCurrentDriver();
            std::cout << "Error getting buffer size" << std::endl;
        }
        std::cout << "ASIOGetBufferSize (min: " << mDriverInfo.minSize << ", max: " << mDriverInfo.maxSize
                  << ", preferred: " << mDriverInfo.preferredSize << ", granularity: " << mDriverInfo.granularity << ")"
                  << std::endl;

        // Maybe check the sample rate here later, if the device can be opened with a specified sample rate
        // result = ASIOCanSampleRate( (ASIOSampleRate) sampleRate );

        result = ASIOGetSampleRate(&mDriverInfo.sampleRate);
        if (result != ASE_OK)
        {
            ASIOExit();
            asioDrivers->removeCurrentDriver();
            std::cout << "Error getting sample rate" << std::endl;
        }
        // std::cout << "ASIOGetSampleRate(sampleRate: " << mDriverInfo.sampleRate << ")" << std::endl;

        mDriverInfo.postOutput = ASIOOutputReady() == ASE_OK ? true : false;
        std::cout << "ASIOOutputReady() - " << (mDriverInfo.postOutput ? "Supported" : "Not supported") << std::endl;

        asioCallbacks.bufferSwitch = &bufferSwitch;
        asioCallbacks.sampleRateDidChange = &sampleRateChanged;
        asioCallbacks.asioMessage = &asioMessages;
        asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

        ASIOBufferInfo *info = mDriverInfo.bufferInfos;
        mDriverInfo.inputBuffers = mDriverInfo.inputChannels;
        for (int i = 0; i < mDriverInfo.inputBuffers; ++i, ++info)
        {
            info->isInput = ASIOTrue;
            info->channelNum = i;
            info->buffers[0] = info->buffers[1] = 0;
        }

        mDriverInfo.outputBuffers = mDriverInfo.outputChannels;
        for (int i = 0; i < mDriverInfo.outputBuffers; ++i, ++info)
        {
            info->isInput = ASIOFalse;
            info->channelNum = i;
            info->buffers[0] = info->buffers[1] = 0;
        }

        result = ASIOCreateBuffers(mDriverInfo.bufferInfos, mDriverInfo.inputBuffers + mDriverInfo.outputBuffers,
                                   mDriverInfo.preferredSize, &asioCallbacks);
        if (result != ASE_OK)
        {
            ASIOExit();
            asioDrivers->removeCurrentDriver();
            std::cout << "Error creating buffers" << std::endl;
        }

        for (int i = 0; i < mDriverInfo.inputBuffers + mDriverInfo.outputBuffers; ++i)
        {
            mDriverInfo.channelInfos[i].channel = mDriverInfo.bufferInfos[i].channelNum;
            mDriverInfo.channelInfos[i].isInput = mDriverInfo.bufferInfos[i].isInput;
            result = ASIOGetChannelInfo(&mDriverInfo.channelInfos[i]);
            if (result != ASE_OK)
            {
                break;
            }
        }

        if (result == ASE_OK)
        {
            result = ASIOGetLatencies(&mDriverInfo.inputLatency, &mDriverInfo.outputLatency);
            if (result != ASE_OK)
            {
                std::cout << "Error getting latency" << std::endl;
            }
            std::cout << "ASIOGetLatencies (input: " << mDriverInfo.inputLatency
                      << ", output: " << mDriverInfo.outputLatency << ")" << std::endl;
        }

        result = ASIOStart();
        if (result != ASE_OK)
        {
            std::cout << "Error starting driver" << std::endl;
        }
        // std::cout << "Driver started. Press Enter to stop..." << std::endl;
        // std::cin.get();

        // std::this_thread::sleep_for(std::chrono::seconds(10));

        std::cout << "Driver stopped" << std::endl;
    }
    else
    {
        std::cout << "Error loading driver" << std::endl;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    SkGraphics::Init();

    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(0,                           // Optional window styles.
                               CLASS_NAME,                  // Window class
                               L"Learn to Program Windows", // Window text
                               WS_OVERLAPPEDWINDOW,         // Window style

                               // Size and position
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                               NULL,      // Parent window
                               NULL,      // Menu
                               hInstance, // Instance handle
                               NULL       // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Try to attach to the existing console (if the program is started from one)
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        std::cout << "This text is printed to the existing console." << std::endl;
    }
    else
    {
        MessageBox(NULL, L"Failed to attach console.", L"Error", MB_OK);
    }

    initializeAsio();

    // Run the message loop.

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    static sk_sp<SkSurface> skiaSurface; // Skia surface to draw on
    static BITMAPINFO bmi;               // Bitmap info for Windows GDI

    switch (uMsg)
    {
    case WM_DESTROY:
        ASIOStop();
        ASIODisposeBuffers();
        ASIOExit();
        asioDrivers->removeCurrentDriver();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (!skiaSurface)
        {
            // Create a Skia surface that matches the window dimensions
            SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600); // Set width/height accordingly
            skiaSurface = SkSurfaces::Raster(info);

            // Initialize GDI bitmap info
            memset(&bmi, 0, sizeof(bmi));
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = info.width();
            bmi.bmiHeader.biHeight = -info.height(); // Negative for top-down bitmap
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
        }

        // Get the canvas from the surface
        SkCanvas *canvas = skiaSurface->getCanvas();

        // Draw something with Skia
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        canvas->drawRect(SkRect::MakeXYWH(100, 100, 200, 200), paint); // Draw red rectangle

        SkImageInfo info;
        size_t rowBytes;
        void *pixels = canvas->accessTopLayerPixels(&info, &rowBytes);
        StretchDIBits(hdc, 0, 0, 800, 600, 0, 0, 800, 600, pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

        EndPaint(hwnd, &ps);
    }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}