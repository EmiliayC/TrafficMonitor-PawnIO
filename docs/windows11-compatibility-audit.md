# Windows 11 compatibility audit

This audit covers the desktop window, taskbar integration, display/DPI handling, notification area, startup, and shell dependencies in the PawnIO branch. It was performed against Windows 11 build 26200 and the public Win32 documentation.

## Completed in this iteration

- The Windows 11 taskbar path now reads the Start button and notification-area bounds through read-only UI Automation as well as the legacy HWND fallback. This removes the normal dependency on `Start` and `TrayNotifyWnd` child classes and avoids the fixed 88-pixel secondary-monitor estimate when Explorer exposes automation bounds.
- All taskbar, Start, and notification-area HWND uses are validated before their rectangles are read. Missing or recreated Explorer windows now produce a safe edge placement instead of using stale rectangle data.
- NOTIFYICONDATA is now zero-initialized so its icon ID, state, and balloon fields never contain indeterminate values.
- `WM_DISPLAYCHANGE` schedules a delayed taskbar-window rebuild. Docking, undocking, display removal, and secondary-taskbar recreation can therefore acquire fresh Explorer HWNDs and DPI data.
- The overlap fallback continues to use visible application and overflow button bounds and leaves Explorer controls unchanged.

## Recommended next iterations

### 1. Per-Monitor V2 DPI migration

The executable manifest currently contains only `dpiAware=true`, so the process is System-DPI aware. Windows can bitmap-scale windows on monitors whose DPI differs from the sign-in primary monitor. Microsoft recommends Per-Monitor V2, but MFC itself does not provide complete automatic migration support and the project caches fonts, bitmaps, sizes, and DPI values in several classes.

This must be handled as a dedicated change: add a `dpiAwareness=PerMonitorV2` manifest, process the suggested `WM_DPICHANGED` rectangle, update every top-level dialog and cached asset, replace DPI-virtualized APIs, and run a mixed-DPI test matrix. The taskbar embedding path needs special testing because Microsoft documents that cross-process `SetParent` calls between different DPI-awareness modes can reset the child process's awareness context.

### 2. Remove the always-elevated runtime if PawnIO permits it

The standard build still embeds `requireAdministrator`. An elevated, always-running desktop app has weaker integration with medium-integrity Explorer and ordinary applications, complicates startup, and broadens the effect of plugin or update defects. PawnIO is installed as a system component, so the next step is to test whether its device ACL permits sensor reads from an `asInvoker` client. If installation or one-time configuration needs elevation, isolate that operation rather than elevating the monitor for its entire lifetime.

### 3. Persist a monitor identity instead of a secondary-display index

The selected secondary taskbar is stored by index. Display enumeration order can change after docking, Remote Desktop, GPU reset, or rearranging monitors, so the same index can refer to another display. Persist the monitor device name or display path and resolve it to the current `HMONITOR`/taskbar HWND, retaining the index only as a compatibility fallback.

### 4. Adopt notification-area protocol version 4

The application adds `NOTIFYICONDATA` but does not call `NIM_SETVERSION`. The current legacy callback still works, but version 4 provides the current notification-area event contract and keyboard selection/context-menu semantics. Migrating requires updating `OnNotifyIcon` to decode `LOWORD(lParam)`, handle `NIN_SELECT`, `NIN_KEYSELECT`, and `WM_CONTEXTMENU`, and reapply the version after `TaskbarCreated`.

### 5. Reduce private Shell and registry assumptions

The app still detects the Windows 11 taskbar through `Windows.UI.Composition.DesktopWindowContentBridge` and reads `TaskbarAl`, `TaskbarDa`, and `MMTaskbarEnabled` directly. These are useful compatibility hints rather than stable application APIs. Keep feature detection and safe defaults, and avoid making program startup depend on any one class name or registry value.

### 6. Validate native dark mode and accessibility

Custom colors cover the taskbar display, but dialogs and menus do not use a complete supported dark-mode path, and the custom taskbar surface exposes little semantic information to screen readers. Audit contrast at Windows text-scaling settings, keyboard access, high-contrast themes, and UI Automation names before describing the app as fully Windows 11 accessible.

### 7. Modernize path handling and warning hygiene

Several code paths still use `MAX_PATH`, and both x86 and x64 builds contain existing narrowing and source-encoding warnings. Enable `longPathAware` only after replacing fixed buffers with length-aware APIs. Then reduce the warnings so new 64-bit truncation defects are visible in CI.

## Validation performed

- x64 Debug build: passed.
- x86 Debug build: passed.
- UI Automation probe on the current Windows 11 taskbar found `StartButton`, task-list peers, overflow-compatible identifiers, and `SystemTray` peers with usable screen bounds.
- ARM64/ARM64EC was intentionally excluded from this branch's validation scope.

## Microsoft references

- [High DPI desktop application development](https://learn.microsoft.com/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)
- [Application manifests and dpiAwareness](https://learn.microsoft.com/windows/win32/sbscs/application-manifests)
- [SetParent and DPI-awareness behavior](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setparent)
- [WM_DISPLAYCHANGE](https://learn.microsoft.com/windows/win32/gdi/wm-displaychange)
- [Shell_NotifyIcon and NIM_SETVERSION](https://learn.microsoft.com/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw)
- [Windows 11 settings reference](https://learn.microsoft.com/windows/apps/develop/settings/settings-windows-11)