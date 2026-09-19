# Windows 11 taskbar compatibility

TrafficMonitor displays its taskbar window by parenting a window to `Shell_TrayWnd`. Windows 11 renders the task buttons through a XAML/WinUI island. Unlike the classic Windows 10 taskbar, Windows 11 does not publish an SDK contract that lets a third-party process reserve a horizontal segment inside the system taskbar.

The public Windows APIs do not provide an equivalent of the classic toolbar insertion behavior:

- `ITaskbarList3` controls an application's own taskbar button state, thumbnail toolbar, progress, and overlays. It does not reserve layout space.
- `SHAppBarMessage` registers an application desktop toolbar at a screen edge and changes the desktop work area. It cannot reserve space inside the system taskbar.
- The Windows 11 overflow menu and the newer small-button behavior are owned by Explorer. There is no public method to set their available width or force the overflow button.
- Resizing `MSTaskSwWClass` is not a supported solution on Windows 11. Explorer owns this compatibility window and immediately restores or recreates it; the XAML task button layout does not reliably follow it.
- Injecting code into Explorer or changing internal XAML objects could alter the layout, but those objects and class names are private implementation details and can change in any Windows update.

## Implemented fallback

When **Move above the taskbar when app buttons would overlap** is enabled, TrafficMonitor uses Microsoft UI Automation to read the bounding rectangles of the visible task-list and overflow buttons. The same read-only query obtains Start and notification-area bounds when Explorer exposes them, reducing dependence on private child-window classes and fixed secondary-monitor spacing. It does not invoke, resize, or modify those controls.

TrafficMonitor stays embedded while there is enough room. When the task buttons reach its intended rectangle, the same monitor window is temporarily reparented and placed immediately above the taskbar. It returns to the taskbar after enough room is available again. A small hysteresis margin prevents rapid switching at the boundary.

If UI Automation is unavailable or Explorer exposes no task-list buttons, TrafficMonitor keeps the previous positioning behavior. The compatibility option is enabled by default and can be disabled in **Options > Taskbar window settings > Windows 11 related settings**.

Windows 11's own overflow and small-button settings remain useful because they delay the point at which the fallback is needed. On supported Windows builds, see **Settings > Personalization > Taskbar > Taskbar behaviors**.

## References

- [The Taskbar — Win32 apps](https://learn.microsoft.com/windows/win32/shell/taskbar)
- [ITaskbarList3 interface](https://learn.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-itaskbarlist3)
- [SHAppBarMessage function](https://learn.microsoft.com/windows/win32/api/shellapi/nf-shellapi-shappbarmessage)
- [IUIAutomation::ElementFromHandle](https://learn.microsoft.com/windows/win32/api/uiautomationclient/nf-uiautomationclient-iuiautomation-elementfromhandle)
- [IUIAutomationElement::get_CurrentBoundingRectangle](https://learn.microsoft.com/windows/win32/api/uiautomationclient/nf-uiautomationclient-iuiautomationelement-get_currentboundingrectangle)
- [Microsoft's Windows 11 taskbar overflow announcement](https://blogs.windows.com/windows-insider/2022/07/20/announcing-windows-11-insider-preview-build-25163/)
- [Customize the taskbar in Windows](https://support.microsoft.com/windows/experience/personalization/customize-the-taskbar-in-windows)
