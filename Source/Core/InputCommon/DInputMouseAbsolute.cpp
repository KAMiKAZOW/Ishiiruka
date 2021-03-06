#include "DInputMouseAbsolute.h"
#include "Core\Host.h"
#include "wx/window.h"
#include "DolphinWX/Frame.h"
#include "VideoCommon\OnScreenDisplay.h"

namespace prime
{

void InitMouse(IDirectInput8* const idi8)
{
  LPDIRECTINPUTDEVICE8 mo_device = nullptr;

  if (SUCCEEDED(idi8->CreateDevice(GUID_SysMouse, &mo_device, nullptr)) &&
    SUCCEEDED(mo_device->SetDataFormat(&c_dfDIMouse2)) &&
    SUCCEEDED(mo_device->SetCooperativeLevel(nullptr, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
  {
    {
      // Setting this property causes directinput to give the absolute coordinates so we can
      // retrieve more accurate representations of inputs
      DIPROPDWORD dipdw;
      dipdw.diph.dwSize = sizeof(DIPROPDWORD);
      dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
      dipdw.diph.dwObj = 0;
      dipdw.diph.dwHow = DIPH_DEVICE;
      dipdw.dwData = DIPROPAXISMODE_ABS;
      mo_device->SetProperty(DIPROP_AXISMODE, &dipdw.diph);
      auto mouse_input = new DInputMouse();
      mouse_input->Init(mo_device);
      g_mouse_input = mouse_input;
      return;
    }
  }

  if (mo_device)
  {
    mo_device->Release();
  }
}

DInputMouse::DInputMouse()
{
  dx = dy = 0;
  cursor_locked = false;
  m_mo_device = nullptr;
}

void DInputMouse::Init(LPDIRECTINPUTDEVICE8 mo_device)
{
  m_mo_device = mo_device;
  m_mo_device->Acquire();
  //we don't need to check device caps
}

void DInputMouse::UpdateInput()
{
  // safeguard
  if (m_mo_device == nullptr || !SConfig::GetInstance().bEnableCheats)
  {
    return;
  }

  DIMOUSESTATE2 input_temp;
  ULONGLONG cur_time = GetTickCount64();

  // Throw out inputs that are too out-of-date
  if (cur_time - last_update > 250)
  {
    m_mo_device->GetDeviceState(sizeof(DIMOUSESTATE2), &state_prev);
    last_update = cur_time;
  }

  // Retrieve input data from our device
  HRESULT hr = m_mo_device->GetDeviceState(sizeof(DIMOUSESTATE2), &input_temp);

  // ensure the device has been acquired
  if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
  {
    m_mo_device->Acquire();
  }

  // Only process inputs when the cursor is locked
  if (SUCCEEDED(hr) && cursor_locked)
  {
    dx += input_temp.lX - state_prev.lX;
    dy += input_temp.lY - state_prev.lY;

    state_prev = input_temp;
  }


  // Ensure the cursor is locked when it needs to be
  LockCursorToGameWindow();
}

void DInputMouse::LockCursorToGameWindow()
{
  if (Host_RendererHasFocus() && cursor_locked)
  {
    wxRect screen_area;
    Host_GetRendererClientRect(screen_area);
    int cX = (screen_area.GetRight() + screen_area.GetLeft()) / 2;
    int cY = (screen_area.GetBottom() + screen_area.GetTop()) / 2;

    SetCursorPos(cX, cY);
  }
  else
  {
    cursor_locked = false;
  }
}

}  // namespace prime
