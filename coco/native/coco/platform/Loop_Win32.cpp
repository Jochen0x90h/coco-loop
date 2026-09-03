#include <coco/platform/WindowsDef.hpp>
#include <Windows.h>
#include <Dbt.h>
#include <initguid.h> // DEFINE_GUID needed for GUID_DEVINTERFACE_USB_DEVICE and GUID_DEVINTERFACE_COMPORT
#include <usbiodef.h> // GUID_DEVINTERFACE_USB_DEVICE
#include <ntddser.h> // GUID_DEVINTERFACE_COMPORT
#include <coco/platform/WindowsUndef.hpp>

#include "Loop_Win32.hpp"
#include <iterator>
#include <iostream>


namespace coco {

Loop_Win32::Loop_Win32(bool noWindowMessages)
    : noWindowMessages_(noWindowMessages)
{
    // get frequency of QueryPerformanceCounter()
    // https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
    // http://www.geisswerks.com/ryan/FAQS/timing.html
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    frequency_ = frequency.QuadPart / 1000;

    // create io completion port
    port_ = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, // FileHandle,
        nullptr, // ExistingCompletionPort,
        NULL, // CompletionKey,
        1 // NumberOfConcurrentThreads
    );
    if (port_ == INVALID_HANDLE_VALUE) {
        auto e = GetLastError();
        std::cerr << "CreateIoCompletionPort: " << e << std::endl;
    }
}

Loop_Win32::~Loop_Win32() {
    CloseHandle(port_);
}

void Loop_Win32::run() {
    while (!exitFlag_) {
        handleEvents();
    }
    exitFlag_ = false;
}

Loop::Time Loop_Win32::now() {
    // todo: handle overflow
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return Time(time.QuadPart / frequency_);
}

Awaitable<CoroutineTimedTask> Loop_Win32::sleep(Time time) {
    return {sleepTasks2_, time};
}

void Loop_Win32::addDeviceHandler(DeviceHandler &handler) {
    if (window_ == nullptr) {
        // create window to receive device events
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            bool add = wParam == DBT_DEVICEARRIVAL;
            if (uMsg == WM_DEVICECHANGE && (add || wParam == DBT_DEVICEREMOVECOMPLETE)) {
                PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
                if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    PDEV_BROADCAST_DEVICEINTERFACE_W pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE_W)pHdr;

                    // get device type
                    DeviceType type = DeviceType::UNKNOWN;
                    if (IsEqualGUID(pDevInf->dbcc_classguid, GUID_DEVINTERFACE_COMPORT))
                        type = DeviceType::COM;
                    else if (IsEqualGUID(pDevInf->dbcc_classguid, GUID_DEVINTERFACE_USB_DEVICE))
                        type = DeviceType::USB;

                    // call handlers
                    auto &list = *reinterpret_cast<IntrusiveList<DeviceHandler> *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                    DevicePath path(pDevInf->dbcc_name);
                    for (auto &handler : list) {
                        handler.onDeviceChange(type, add, path);
                    }
                }
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"coco-loop";
        RegisterClassExW(&wc);

        // create window
        window_ = CreateWindowExW(
            0, // no extended style
            wc.lpszClassName,
            L"", // no title
            0, // no styles (no WS_VISIBLE!)
            0, 0, 0, 0, // position/size
            HWND_MESSAGE, // message only window
            nullptr,
            wc.hInstance,
            nullptr
        );
        SetWindowLongPtr(window_, GWLP_USERDATA, (LONG_PTR)&deviceHandlers_);

        // register device notifications
        DEV_BROADCAST_DEVICEINTERFACE_W filter = {};
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
        RegisterDeviceNotificationW(
            window_,
            &filter,
            DEVICE_NOTIFY_WINDOW_HANDLE
        );
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = GUID_DEVINTERFACE_COMPORT;
        RegisterDeviceNotificationW(
            window_,
            &filter,
            DEVICE_NOTIFY_WINDOW_HANDLE
        );
    }

    this->deviceHandlers_.add(handler);
}

void Loop_Win32::handleEvents(int wait) {
    // determine timeout, only sleep if there are no coroutines waiting on yield()
    int timeout = 0;
    {
        Time currentTime = now();
        Time sleepTime = sleepTasks2_.nextValue(sleepTasks1_.nextValue(currentTime + wait * 1ms));
        int t = (sleepTime - currentTime).value;
        timeout = t > 0 ? t : 0;
    }

    if (noWindowMessages_ || window_ == nullptr) {
        // no window messages: only wait for io completion
        ULONG entryCount;
        OVERLAPPED_ENTRY entries[16];
        if (GetQueuedCompletionStatusEx(
            port_,
            entries,
            std::size(entries),
            &entryCount,
            timeout,
            false))
        {
            // call handler of completed operations
            for (int i = 0; i < entryCount; ++i) {
                auto &entry = entries[i];
                auto handler = (CompletionHandler *)(entry.lpCompletionKey);
                handler->onCompletion(entry.lpOverlapped);
            }
        }

        // timeout
        //auto e = GetLastError();
        //if (e != WAIT_TIMEOUT)
        //    std::cerr << "GetQueuedCompletionStatusEx: " << e << std::endl;
    } else {
        // with window messages
        DWORD result = MsgWaitForMultipleObjects(
            1, // one handle (the io completion port)
            &port_,
            FALSE, // wait any
            timeout,
            QS_ALLINPUT | QS_ALLPOSTMESSAGE
        );
        if (result == WAIT_OBJECT_0) {
            // IOCP
            ULONG entryCount;
            OVERLAPPED_ENTRY entries[16];
            if (GetQueuedCompletionStatusEx(
                port_,
                entries,
                std::size(entries),
                &entryCount,
                0,
                false))
            {
                // call handler of completed operations
                for (int i = 0; i < entryCount; ++i) {
                    auto &entry = entries[i];
                    auto handler = (CompletionHandler *)(entry.lpCompletionKey);
                    handler->onCompletion(entry.lpOverlapped);
                }
            }
        } else if (result == WAIT_OBJECT_0 + 1) {
            // window message
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT)
                    exitFlag_ = true;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else if (result == WAIT_TIMEOUT) {
            // timeout
        }
    }
    // resume coroutines waiting on yield() and activate yield handlers
    //yieldTasks1.doAll();
    //yieldTasks2.doAll();

    // resume coroutines waiting on sleep() and activate time handlers
    {
        Time currentTime = now();
        sleepTasks1_.doUntil(currentTime);//, [](TimeoutHandler &handler) {handler.onTimeout();});
        sleepTasks2_.doUntil(currentTime);
    }

    //return result;
}

} // namespace coco
