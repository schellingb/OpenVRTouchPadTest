# OpenVRTouchPadTest

Due to a bug present up to the latest version of OpenVR/SteamVR the VIVE controller touch pad
release event might not be registered until another input action is performed. This causes
the touch pad to be in a touched state for longer than it actually was touched.

This tool logs events and states related to the touch pad to the screen to easily confirm
the existence of the bug on a system.

## Download
A Windows binary is provided on the [Releases](../../releases) page. 
Other platforms need to be compiled from source.

## Running

### Normal touch process
A regular touch event has a ButtonTouch event on start and ButtonUntouch event at the end
```
--- TOUCH START ---
[23.28] Event VREvent_ButtonTouch (202) - Age: 0.000013 - Button: 32
[23.28] Device: 1 - PacketNum: 11506 - Touch: 1 - Press: 0 - X: -0.503311 - Y: -0.748375 - Trigger: 0.000000
[23.29] Device: 1 - PacketNum: 11508 - Touch: 1 - Press: 0 - X: -0.314818 - Y: -0.771531 - Trigger: 0.000000
[23.31] Device: 1 - PacketNum: 11510 - Touch: 1 - Press: 0 - X: -0.045357 - Y: -0.770141 - Trigger: 0.000000
[23.32] Device: 1 - PacketNum: 11511 - Touch: 0 - Press: 0 - X: -0.045357 - Y: -0.770141 - Trigger: 0.000000
[23.32] Event VREvent_ButtonUntouch (203) - Age: 0.000315 - Button: 32
--- TOUCH END ---
```

### Bugged touch process
A touch event that is affected by the bug will only have a ButtonTouch event at first
```
--- TOUCH START ---
[36.32] Device: 1 - PacketNum: 19213 - Touch: 1 - Press: 0 - X: 0.000000 - Y: 0.000000 - Trigger: 0.000000
[36.32] Event VREvent_ButtonTouch (202) - Age: 0.000810 - Button: 32
[36.32] Device: 1 - PacketNum: 19215 - Touch: 1 - Press: 0 - X: -0.338450 - Y: -0.802515 - Trigger: 0.000000
[36.33] Device: 1 - PacketNum: 19216 - Touch: 1 - Press: 0 - X: -0.255793 - Y: -0.802515 - Trigger: 0.000000
[36.33] Device: 1 - PacketNum: 19217 - Touch: 1 - Press: 0 - X: -0.255793 - Y: -0.807405 - Trigger: 0.000000
[36.36] Device: 1 - PacketNum: 19219 - Touch: 1 - Press: 0 - X: -0.150097 - Y: -0.797556 - Trigger: 0.000000
[36.38] Device: 1 - PacketNum: 19220 - Touch: 1 - Press: 0 - X: 0.023835 - Y: -0.797556 - Trigger: 0.000000
[36.38] Device: 1 - PacketNum: 19221 - Touch: 1 - Press: 0 - X: 0.023835 - Y: -0.777610 - Trigger: 0.000000
[36.39] Device: 1 - PacketNum: 19223 - Touch: 1 - Press: 0 - X: 0.227989 - Y: -0.746477 - Trigger: 0.000000
```
After the 36.39 timestamp the finger was physically released from the touch pad but OpenVR did not process the untouch event.

10 seconds later the trigger on the VIVE controller was slightly pressed and thus another input action was performed
which in turn finally sends the ButtonUntouch event but way too late.
```
[45.93] Event VREvent_ButtonUntouch (203) - Age: -0.000009 - Button: 32
[45.93] Device: 1 - PacketNum: 19226 - Touch: 0 - Press: 0 - X: 0.000000 - Y: 0.000000 - Trigger: 0.000000
--- TOUCH END ---

[45.94] Device: 1 - PacketNum: 19229 - Touch: 0 - Press: 0 - X: 0.000000 - Y: 0.000000 - Trigger: 0.019608
[45.96] Device: 1 - PacketNum: 19232 - Touch: 0 - Press: 0 - X: 0.000000 - Y: 0.000000 - Trigger: 0.000000
```

## Workaround for developers
If you would like to fix this issue for a particular piece of software it is possible
to detect a stuck touch pad. If OpenVR returns the exact same floating point values
for the touch pad while touched over a few frames it is very likely that the bug is
in effect. Between the physical un-touch and the next input event OpenVR continues to
return the VIVE controller touch pad as touched at the exact same position. Because the
touch sensor is very sensitive it is unlikely that these values are the same over even
a very short time.

### Sample Code
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

### For engines with built-in VR support
This bug will affect games written in Unity or UE4 as well. It is possible to implement a custom OpenVR
input handling although not straight-forward. 
For UE4 one would need to copy the SteamVRController plugin and implement a custom plugin which
incorporates the fix, then disable the default SteamVRController plugin and enable the custom one. 
For Unity one would need to implement a custom OpenVR update loop using the SteamVR Plugin from
the Asset Store. Then use that instead of reading input events via the built-in UnityEngine.Input API.

### License
OpenVRTouchPadTest is available under the [Public Domain](https://www.unlicense.org).
