#include "stdafx.h"
#include "Win11TaskbarDlg.h"
#include "WindowsSettingHelper.h"
#include <UIAutomation.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace
{
    enum class TaskbarElementKind
    {
        None,
        ApplicationButton,
        StartButton,
        NotificationArea
    };

    TaskbarElementKind GetTaskbarElementKind(IUIAutomationElement* element)
    {
        BSTR automation_id{};
        BSTR class_name{};
        const HRESULT id_result = element->get_CurrentAutomationId(&automation_id);
        const HRESULT class_result = element->get_CurrentClassName(&class_name);

        const bool has_id = SUCCEEDED(id_result) && automation_id != nullptr;
        const bool has_class = SUCCEEDED(class_result) && class_name != nullptr;
        TaskbarElementKind kind{ TaskbarElementKind::None };
        if ((has_id && wcsncmp(automation_id, L"Appid:", 6) == 0)
            || (has_class && wcsstr(class_name, L"TaskListButton") != nullptr)
            || (has_id && wcsstr(automation_id, L"Overflow") != nullptr)
            || (has_class && wcsstr(class_name, L"Overflow") != nullptr))
        {
            kind = TaskbarElementKind::ApplicationButton;
        }
        else if (has_id && wcscmp(automation_id, L"StartButton") == 0)
        {
            kind = TaskbarElementKind::StartButton;
        }
        else if ((has_id && (wcscmp(automation_id, L"SystemTrayIcon") == 0
            || wcscmp(automation_id, L"NotifyItemIcon") == 0))
            || (has_class && wcsstr(class_name, L"SystemTray") != nullptr))
        {
            kind = TaskbarElementKind::NotificationArea;
        }

        ::SysFreeString(automation_id);
        ::SysFreeString(class_name);
        return kind;
    }

    void AddToBounds(CRect& bounds, const CRect& element_rect)
    {
        if (bounds.IsRectEmpty())
            bounds = element_rect;
        else
            bounds.UnionRect(&bounds, &element_rect);
    }
}

bool CWin11TaskbarDlg::GetTaskbarLayout(CRect& button_bounds, CRect& start_bounds, CRect& notify_bounds) const
{
    button_bounds.SetRectEmpty();
    start_bounds.SetRectEmpty();
    notify_bounds.SetRectEmpty();

    ComPtr<IUIAutomation> automation;
    if (FAILED(::CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation))))
        return false;

    ComPtr<IUIAutomationElement> root;
    if (FAILED(automation->ElementFromHandle(m_hTaskbar, &root)) || root == nullptr)
        return false;

    VARIANT control_type;
    ::VariantInit(&control_type);
    control_type.vt = VT_I4;
    control_type.lVal = UIA_ButtonControlTypeId;

    ComPtr<IUIAutomationCondition> button_condition;
    const HRESULT condition_result =
        automation->CreatePropertyCondition(UIA_ControlTypePropertyId, control_type, &button_condition);
    ::VariantClear(&control_type);
    if (FAILED(condition_result) || button_condition == nullptr)
        return false;

    ComPtr<IUIAutomationElementArray> buttons;
    if (FAILED(root->FindAll(TreeScope_Descendants, button_condition.Get(), &buttons)) || buttons == nullptr)
        return false;

    int count{};
    if (FAILED(buttons->get_Length(&count)))
        return false;

    for (int index = 0; index < count; ++index)
    {
        ComPtr<IUIAutomationElement> button;
        if (FAILED(buttons->GetElement(index, &button)) || button == nullptr)
            continue;

        const TaskbarElementKind kind = GetTaskbarElementKind(button.Get());
        if (kind == TaskbarElementKind::None)
            continue;

        BOOL is_offscreen{};
        if (SUCCEEDED(button->get_CurrentIsOffscreen(&is_offscreen)) && is_offscreen)
            continue;

        RECT rect{};
        if (FAILED(button->get_CurrentBoundingRectangle(&rect)) || ::IsRectEmpty(&rect))
            continue;

        CRect element_rect(rect);
        CRect intersection;
        if (!intersection.IntersectRect(&element_rect, &m_rcTaskbar))
            continue;

        switch (kind)
        {
        case TaskbarElementKind::ApplicationButton:
            AddToBounds(button_bounds, element_rect);
            break;
        case TaskbarElementKind::StartButton:
            AddToBounds(start_bounds, element_rect);
            break;
        case TaskbarElementKind::NotificationArea:
            AddToBounds(notify_bounds, element_rect);
            break;
        default:
            break;
        }
    }
    return true;
}
void CWin11TaskbarDlg::SetFloatingAboveTaskbar(bool floating)
{
    if (floating == m_floating_above_taskbar)
        return;

    ::SetLastError(ERROR_SUCCESS);
    HWND previous_parent = ::SetParent(m_hWnd, floating ? nullptr : m_hTaskbar);
    if (previous_parent == nullptr && ::GetLastError() != ERROR_SUCCESS)
        return;

    ::SetWindowPos(m_hWnd, floating ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    m_floating_above_taskbar = floating;
}

CRect CWin11TaskbarDlg::GetFloatingRect(int notify_x_pos) const
{
    const int notify_screen_x = m_rcTaskbar.left + notify_x_pos;

    CRect rect;
    rect.right = min(notify_screen_x + DPI(2), m_rcTaskbar.right);
    rect.left = max(m_rcTaskbar.left, rect.right - m_window_width);

    HMONITOR monitor = ::MonitorFromWindow(m_hTaskbar, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{ sizeof(MONITORINFO) };
    ::GetMonitorInfo(monitor, &monitor_info);
    const int taskbar_center_y = (m_rcTaskbar.top + m_rcTaskbar.bottom) / 2;
    const int monitor_center_y = (monitor_info.rcMonitor.top + monitor_info.rcMonitor.bottom) / 2;
    if (taskbar_center_y >= monitor_center_y)
    {
        rect.bottom = m_rcTaskbar.top - DPI(2);
        rect.top = rect.bottom - m_window_height;
    }
    else
    {
        rect.top = m_rcTaskbar.bottom + DPI(2);
        rect.bottom = rect.top + m_window_height;
    }
    return rect;
}

bool CWin11TaskbarDlg::ShouldFloatAboveTaskbar(const CRect& docked_rect, const CRect& taskbar_button_bounds) const
{
    if (!theApp.m_taskbar_data.avoid_overlap_with_taskbar_buttons || taskbar_button_bounds.IsRectEmpty())
        return false;

    CRect docked_screen_rect(docked_rect);
    docked_screen_rect.OffsetRect(m_rcTaskbar.left, m_rcTaskbar.top);
    const int clearance = docked_screen_rect.left - taskbar_button_bounds.right;
    return clearance < DPI(m_floating_above_taskbar ? 16 : 6);
}

void CWin11TaskbarDlg::AdjustTaskbarWndPos(bool force_adjust)
{
    m_rcNotify.SetRectEmpty();
    m_rcStart.SetRectEmpty();
    if (::IsWindow(m_hNotify))
        ::GetWindowRect(m_hNotify, m_rcNotify);
    if (::IsWindow(m_hStart))
        ::GetWindowRect(m_hStart, m_rcStart);

    CRect taskbar_button_bounds;
    CRect automation_start_bounds;
    CRect automation_notify_bounds;
    const bool button_query_succeeded =
        GetTaskbarLayout(taskbar_button_bounds, automation_start_bounds, automation_notify_bounds);
    if (!automation_start_bounds.IsRectEmpty())
        m_rcStart = automation_start_bounds;
    if (!automation_notify_bounds.IsRectEmpty())
        m_rcNotify = automation_notify_bounds;
    if (!m_rcStart.IsRectEmpty())
        m_rcStart.MoveToXY(m_rcStart.left - m_rcTaskbar.left, m_rcStart.top - m_rcTaskbar.top);

    //设置窗口大小
    m_rect.right = m_rect.left + m_window_width;
    m_rect.bottom = m_rect.top + m_window_height;

    const int taskbar_button_right = taskbar_button_bounds.IsRectEmpty() ? 0 : taskbar_button_bounds.right;
    if (force_adjust || m_rcNotify.Width() != m_last_notify_width || m_rcStart.left != m_last_start_pos
        || taskbar_button_right != m_last_taskbar_button_right
        || button_query_succeeded != m_last_taskbar_button_query_succeeded)
    {
        m_last_notify_width = m_rcNotify.Width();
        m_last_start_pos = m_rcStart.left;
        m_last_taskbar_button_right = taskbar_button_right;
        m_last_taskbar_button_query_succeeded = button_query_succeeded;
        //任务窗口显示在右侧时，或者Windows11下任务栏左对齐时
        //（Windows11下，如果任务栏设置为左对齐，即使在“任务栏窗口设置”中设置了任务窗口显示在左边，窗口仍然显示在右边）
        int notify_x_pos{};
        if (!theApp.m_taskbar_data.tbar_wnd_on_left || !CWindowsSettingHelper::IsTaskbarCenterAlign())
        {
            //通知区窗口的水平位置
            notify_x_pos = m_rcNotify.left;
            if (notify_x_pos != 0)
                notify_x_pos -= m_rcTaskbar.left;
            //没有获取到通知区位置的情况
            if (notify_x_pos == 0)
            {
                //Win11副屏没有通知区窗口，这里使用固定的值（88像素的系统时间区域）
                if (m_is_secondary_display)
                    notify_x_pos = m_rcTaskbar.Width() - DPI(88);
                //如果不是副屏，但是仍然没有获取到通知区域的位置，使用配置文件中taskbar_right_space_win11指定的值
                else
                    notify_x_pos = m_rcTaskbar.Width() - DPI(theApp.m_taskbar_data.taskbar_right_space_win11);
            }
            //如果显示了小组件，并且任务栏靠左显示，则留出小组件的位置
            if (theApp.m_taskbar_data.avoid_overlap_with_widgets && CWindowsSettingHelper::IsTaskbarWidgetsBtnShown() && !CWindowsSettingHelper::IsTaskbarCenterAlign())
                m_rect.MoveToX(notify_x_pos - m_rect.Width() + 2 - DPI(theApp.m_taskbar_data.taskbar_left_space_win11));
            else
                m_rect.MoveToX(notify_x_pos - m_rect.Width() + 2);
        }
        //任务栏窗口显示在左侧时
        else
        {
            //靠近“开始”按钮
            if (theApp.m_taskbar_data.tbar_wnd_snap && !m_rcStart.IsRectEmpty())
            {
                m_rect.MoveToX(m_rcStart.left - m_rect.Width() - 2);
            }
            else if (theApp.m_taskbar_data.tbar_wnd_snap)
            {
                // UI Automation and the legacy Start HWND may both be unavailable on
                // a future Explorer build. Keep the window at a safe taskbar edge.
                m_rect.MoveToX(2);
            }
            //靠近最左侧
            else
            {
                if (CWindowsSettingHelper::IsTaskbarWidgetsBtnShown())
                    m_rect.MoveToX(2 + DPI(theApp.m_taskbar_data.taskbar_left_space_win11));
                else
                    m_rect.MoveToX(2);
            }
        }
        //水平偏移
        m_rect.MoveToX(m_rect.left + DPI(theApp.m_taskbar_data.window_offset_left));

        //设置任务栏窗口的垂直位置
        //注：这里加上(m_rcTaskbar.Height() - rcStart.Height())用于修正Windows11 build 22621版本后触屏设备任务栏窗口位置不正确的问题。
        //在这种情况下m_rcTaskbar的高度要大于m_rcBar的高度，正常情况下，它们的高度相同
        //但是当任务栏上没有任何图标时，m_rcBar的高度会变为0，因此使用rcStart代替
        const int taskbar_content_height = m_rcStart.IsRectEmpty() ? m_rcTaskbar.Height() : m_rcStart.Height();
        m_rect.MoveToY((taskbar_content_height - m_rect.Height()) / 2 + (m_rcTaskbar.Height() - taskbar_content_height) + DPI(theApp.m_taskbar_data.window_offset_top));

        const bool right_side_layout =
            !theApp.m_taskbar_data.tbar_wnd_on_left || !CWindowsSettingHelper::IsTaskbarCenterAlign();
        const bool should_float = right_side_layout && button_query_succeeded
            && ShouldFloatAboveTaskbar(m_rect, taskbar_button_bounds);
        SetFloatingAboveTaskbar(should_float);
        if (m_floating_above_taskbar)
            MoveWindow(GetFloatingRect(notify_x_pos));
        else
            MoveWindow(m_rect);
    }
}

void CWin11TaskbarDlg::InitTaskbarWnd()
{
    m_hNotify = ::FindWindowEx(m_hTaskbar, 0, L"TrayNotifyWnd", NULL);
    m_hStart = ::FindWindowEx(m_hTaskbar, nullptr, L"Start", NULL);
    m_rcNotify.SetRectEmpty();
    m_rcStart.SetRectEmpty();
    if (::IsWindow(m_hNotify))
        ::GetWindowRect(m_hNotify, m_rcNotify);
    if (::IsWindow(m_hStart))
        ::GetWindowRect(m_hStart, m_rcStart);
}

void CWin11TaskbarDlg::ResetTaskbarPos()
{
}

HWND CWin11TaskbarDlg::GetParentHwnd()
{
    return m_hTaskbar;
}

void CWin11TaskbarDlg::CheckTaskbarOnTopOrBottom()
{
    m_taskbar_on_top_or_bottom = true;
}
