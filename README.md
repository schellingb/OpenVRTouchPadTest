# OpenVRTouchPadTest

Due to a bug still present in the latest version of OpenVR/SteamVR and firmwares, VIVE controller
input events might not be registered until another input action is performed. This can cause
the touch pad to be in a touched state for longer than it actually was touched or button
presses to be ignored.

This tool logs events and states related to the touch pad and buttons to the screen
to easily confirm the existence of the bug on a system.

## Download
A Windows binary is provided on the [Releases](../../releases) page. 
Other platforms need to be compiled from source.

## Running

### Normal press process
A regular button press and release process has a ButtonPress event on start and ButtonUnpress event at the end
```
--- PRESS [Menu] 13 START ---
[6.92] Event VREvent_ButtonPress (200) - Age: 0.000987 - Button: Menu
[6.92] Device: 4 - PacketNum: 28934 - Touch:  - Press: Menu - X: 0.00 - Y: 0.00 - Trigger: 0.00
[7.01] Device: 4 - PacketNum: 28935 - Touch:  - Press:  - X: 0.00 - Y: 0.00 - Trigger: 0.00
[7.01] Event VREvent_ButtonUnpress (201) - Age: 0.000708 - Button: Menu
--- PRESS [Menu] 13 END ---
```

### Bugged press process
A button pressing that is affected by the bug will only have a ButtonPress event at first
```
--- PRESS [Menu] 14 START ---
[7.34] Event VREvent_ButtonPress (200) - Age: -0.000007 - Button: Menu
[7.36] Device: 4 - PacketNum: 28936 - Touch:  - Press: Menu - X: 0.00 - Y: 0.00 - Trigger: 0.00
```
Soon after the finger was physically released from the button but OpenVR did not process the unpress event.

9 seconds later the trigger on the VIVE controller was slightly pressed and thus another input action was performed
which in turn finally sends the ButtonUnpress event but way too late.
```
[16.40] Device: 4 - PacketNum: 28937 - Touch:  - Press:  - X: 0.00 - Y: 0.00 - Trigger: 0.00
[16.41] Event VREvent_ButtonUnpress (201) - Age: 0.003613 - Button: Menu
--- PRESS [Menu] 14 END ---

[16.41] Device: 4 - PacketNum: 28938 - Touch:  - Press:  - X: 0.00 - Y: 0.00 - Trigger: 0.03
[16.42] Device: 4 - PacketNum: 28939 - Touch:  - Press:  - X: 0.00 - Y: 0.00 - Trigger: 0.00
```

### Normal touch process
A regular touch event has a ButtonTouch event on start and ButtonUntouch event at the end
```
--- TOUCH [Touchpad] 11 START ---
[14.33] Event VREvent_ButtonTouch (202) - Age: 0.000397 - Button: Touchpad
[14.33] Device: 4 - PacketNum: 29097 - Touch: Touchpad - Press:  - X: 0.03 - Y: -0.48 - Trigger: 0.00
[14.39] Device: 4 - PacketNum: 29104 - Touch: Touchpad - Press:  - X: 0.19 - Y: -0.41 - Trigger: 0.00
[14.43] Device: 4 - PacketNum: 29111 - Touch: Touchpad - Press:  - X: 0.47 - Y: -0.47 - Trigger: 0.00
[14.46] Device: 4 - PacketNum: 29112 - Touch:  - Press:  - X: 0.00 - Y: 0.00 - Trigger: 0.00
[14.46] Event VREvent_ButtonUntouch (203) - Age: 0.000290 - Button: Touchpad
--- TOUCH [Touchpad] 11 END ---
```

### Bugged touch process
A touch event that is affected by the bug will only have a ButtonTouch event at first
```
--- TOUCH [Touchpad] 28 START ---
[36.32] Event VREvent_ButtonTouch (202) - Age: 0.000810 - Button: Touchpad
[36.32] Device: 4 - PacketNum: 19213 - Touch: Touchpad - Press: 0 - X: 0.00 - Y: 0.00 - Trigger: 0.00
[36.32] Device: 4 - PacketNum: 19215 - Touch: Touchpad - Press: 0 - X: -0.34 - Y: -0.80 - Trigger: 0.00
[36.33] Device: 4 - PacketNum: 19216 - Touch: Touchpad - Press: 0 - X: -0.26 - Y: -0.80 - Trigger: 0.00
[36.33] Device: 4 - PacketNum: 19217 - Touch: Touchpad - Press: 0 - X: -0.26 - Y: -0.81 - Trigger: 0.00
[36.36] Device: 4 - PacketNum: 19219 - Touch: Touchpad - Press: 0 - X: -0.15 - Y: -0.80 - Trigger: 0.00
[36.38] Device: 4 - PacketNum: 19220 - Touch: Touchpad - Press: 0 - X: 0.02 - Y: -0.80 - Trigger: 0.00
[36.38] Device: 4 - PacketNum: 19221 - Touch: Touchpad - Press: 0 - X: 0.02 - Y: -0.78 - Trigger: 0.00
[36.39] Device: 4 - PacketNum: 19223 - Touch: Touchpad - Press: 0 - X: 0.23 - Y: -0.75 - Trigger: 0.00
```
After the 36.39 timestamp the finger was physically released from the touch pad but OpenVR did not process the untouch event.

10 seconds later the trigger on the VIVE controller was slightly pressed and thus another input action was performed
which in turn finally sends the ButtonUntouch event but way too late.
```
[45.93] Device: 4 - PacketNum: 19226 - Touch:  - Press: 0 - X: 0.00 - Y: 0.00 - Trigger: 0.00
[45.93] Event VREvent_ButtonUntouch (203) - Age: -0.000009 - Button: Touchpad
--- TOUCH [Touchpad] 28 END ---

[45.94] Device: 4 - PacketNum: 19229 - Touch:  - Press: 0 - X: 0.00 - Y: 0.00 - Trigger: 0.02
[45.96] Device: 4 - PacketNum: 19232 - Touch:  - Press: 0 - X: 0.00 - Y: 0.00 - Trigger: 0.00
```

## Workaround for developers

### Button events
For skipped button press/release events there is no known workaround.

### Skipped touch pad touch release events
If you would like to fix this issue for a particular piece of software it is possible
to detect a stuck touch pad. If OpenVR returns the exact same floating point values
for the touch pad while touched over a few frames it is very likely that the bug is
in effect. Between the physical un-touch and the next input event OpenVR continues to
return the VIVE controller touch pad as touched at the exact same position. Because the
touch sensor is very sensitive it is unlikely that these values are the same over even
a very short time.

#### Sample Code
```C++
//global
vr::TrackedDeviceIndex_t ControllerDeviceIndices[2];
vr::VRControllerState_t ControllerStates[2];
vr::VRControllerAxis_t ControllerTouchPadLastPos[2];
int ControllerTouchPadStuckCount[2];
...
//during init
ControllerDeviceIndices[0] = DetectLeftControllerDeviceIndex(); //Needs to be implemented
ControllerDeviceIndices[1] = DetectRightControllerDeviceIndex(); //Needs to be implemented
...
//during every frame when updating OpenVR state
const int MaxStuckFrames = 8; //could be different or calculated to be around 100ms based on framerate
for (int i = 0; i != 2; i++)
{
	pVRSystem->GetControllerState(ControllerDeviceIndices[i], &ControllerStates[i], sizeof(vr::VRControllerState_t));
	if ((ControllerStates[i].ulButtonTouched & (1ull << (int)vr::k_EButton_SteamVR_Touchpad)) != 0)
	{
		//Workaround for a bug in OpenVR for which a VIVE TouchPad untouch action is not registered until the next input event.
		//Between the physical untouch and the next input event OpenVR continues to return the VIVE TouchPad as touched at the exact same position.
		//Because the touch sensor is very sensitive it is unlikely that these values are the same over even a very short time.
		if (ControllerTouchPadLastPos[i].x == ControllerStates[i].rAxis[0].x && ControllerTouchPadLastPos[i].y == ControllerStates[i].rAxis[0].y)
		{
			if (++ControllerTouchPadStuckCount[i] >= MaxStuckFrames)
			{
				//TouchPad seems stuck in touched state, overwrite state as not touching
				ControllerStates[i].ulButtonTouched -= (1ull << (int)vr::k_EButton_SteamVR_Touchpad);
				ControllerStates[i].rAxis[0].x = 0.0f;
				ControllerStates[i].rAxis[0].y = 0.0f;
				ControllerTouchPadStuckCount[i] = 99999;
			}
		}
		else
		{
			//Touch pad values changed since last time, remember TouchPad position and reset stuck count
			ControllerTouchPadLastPos[i] = ControllerStates[i].rAxis[0];
			ControllerTouchPadStuckCount[i] = 0;
		}
	}
}
```

#### For engines with built-in VR support
This bug will affect games written in Unity or UE4 as well. It is possible to implement a custom OpenVR
input handling although not straight-forward. 
For UE4 one would need to copy the SteamVRController plugin and implement a custom plugin which
incorporates the fix, then disable the default SteamVRController plugin and enable the custom one. 
For Unity one would need to implement a custom OpenVR update loop using the SteamVR Plugin from
the Asset Store. Then use that instead of reading input events via the built-in UnityEngine.Input API.

## License
OpenVRTouchPadTest is available under the [Public Domain](https://www.unlicense.org).
