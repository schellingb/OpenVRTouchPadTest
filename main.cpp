#include <string.h>
#include <stdio.h>
#include "openvr/openvr.h"

static const char* TrackedControllerRoleStr(vr::ETrackedControllerRole Role)
{
	switch (Role)
	{
		case vr::TrackedControllerRole_Invalid  : return "Unknown Hand";
		case vr::TrackedControllerRole_LeftHand : return "Left Hand";
		case vr::TrackedControllerRole_RightHand: return "Right Hand";
		case vr::TrackedControllerRole_OptOut   : return "Opt Out";
		case vr::TrackedControllerRole_Max      : return "Max";
	}
	return "UNKNOWN";
}

static float GetVRTime(vr::IVRSystem *pVRSystem)
{
	uint64_t FrameCounter;
	float SecondsSinceLastVsync;
	pVRSystem->GetTimeSinceLastVsync(&SecondsSinceLastVsync, &FrameCounter);
	static uint64_t FirstFrameCounter = 0;
	if (!FirstFrameCounter) FirstFrameCounter = FrameCounter;
	return (FrameCounter - FirstFrameCounter) / 90.0f;
}

int main(int argc, const char* argv[])
{
	if (!vr::VR_Init(0, vr::VRApplication_Other) || !vr::VRSystem() || !vr::VRCompositor()) { fprintf(stderr, "Error: Could not connect to SteamVR\n"); return 1; }
	vr::IVRSystem *pVRSystem = vr::VRSystem();

	//Find tracked controllers (VIVE Controller)
	vr::TrackedDeviceIndex_t ControllerIndices[2] = { vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
	for (vr::TrackedDeviceIndex_t i = 0; i != vr::k_unMaxTrackedDeviceCount; i++)
	{
		if (pVRSystem->GetTrackedDeviceClass(i) != vr::TrackedDeviceClass_Controller) continue;
		if (pVRSystem->GetBoolTrackedDeviceProperty(i, vr::Prop_NeverTracked_Bool)) continue;
		printf("Found tracked controller with device index %d (Role: %s)\n", i, TrackedControllerRoleStr(pVRSystem->GetControllerRoleForTrackedDeviceIndex(i)));
		if (ControllerIndices[0] == vr::k_unTrackedDeviceIndexInvalid) ControllerIndices[0] = i;
		else { ControllerIndices[1] = i; break; }
	}
	printf("Starting Test, abort with CTRL-C\n\n");

	vr::VREvent_t Event;
	vr::VRControllerState_t PastStates[2], State;
	int TouchpadCount = 0;
	memset(PastStates, 0, sizeof(PastStates));

	//Loop forever
	for (;;)
	{
		//Poll events and print incoming button/touch events
		while (pVRSystem->PollNextEvent(&Event, sizeof(Event)))
		{
			if (Event.eventType < vr::VREvent_ButtonPress || Event.eventType > vr::VREvent_ButtonUntouch) continue;
			bool IsTouchPad = (Event.data.controller.button == vr::k_EButton_SteamVR_Touchpad);
			if (IsTouchPad && Event.eventType == vr::VREvent_ButtonTouch) { if (!TouchpadCount) printf("\n--- TOUCH START ---\n"); TouchpadCount++; }
			printf("[%.2f] Event %s (%d) - Age: %f - Button: %d\n", GetVRTime(pVRSystem), pVRSystem->GetEventTypeNameFromEnum((vr::EVREventType)Event.eventType), Event.eventType, Event.eventAgeSeconds, Event.data.controller.button);
			if (IsTouchPad && Event.eventType == vr::VREvent_ButtonUntouch) { TouchpadCount--; if (!TouchpadCount) printf("--- TOUCH END ---\n\n"); }
		}

		//Poll controller state and print touch pad status on state change
		for (int i = 0; i != 2; i++)
		{
			if (ControllerIndices[i] == vr::k_unTrackedDeviceIndexInvalid) continue;
			pVRSystem->GetControllerState(ControllerIndices[i], &State, sizeof(State));
			if (memcmp(&PastStates[i].ulButtonPressed, &State.ulButtonPressed, (char*)((&State)+1) - (char*)&State.ulButtonPressed))
			{
				bool OldTouch = !!(PastStates[i].ulButtonTouched & (1ull << vr::k_EButton_SteamVR_Touchpad)), NewTouch = !!(State.ulButtonTouched & (1ull << vr::k_EButton_SteamVR_Touchpad));
				if (!OldTouch && NewTouch) { if (!TouchpadCount) printf("\n--- TOUCH START ---\n"); TouchpadCount++; }
				printf("[%.2f] Device: %d - PacketNum: %d - Touch: %d - Press: %d - X: %f - Y: %f - Trigger: %f\n", GetVRTime(pVRSystem), ControllerIndices[i], State.unPacketNum, NewTouch, !!(State.ulButtonPressed & (1ull << vr::k_EButton_SteamVR_Touchpad)), State.rAxis[0].x, State.rAxis[0].y, State.rAxis[1].x);
				if (OldTouch && !NewTouch) { TouchpadCount--; if (!TouchpadCount) printf("--- TOUCH END ---\n\n"); }
			}
			memcpy(&PastStates[i], &State, sizeof(State));
		}
	}
}
