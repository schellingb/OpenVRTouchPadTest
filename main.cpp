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

static const char* VRButtonIdStr(vr::EVRButtonId Button)
{
	switch (Button)
	{
		case vr::k_EButton_System:           return "System";
		case vr::k_EButton_ApplicationMenu:  return "Menu";
		case vr::k_EButton_Grip:             return "Grip";
		case vr::k_EButton_DPad_Left:        return "DPad_Left";
		case vr::k_EButton_DPad_Up:          return "DPad_Up";
		case vr::k_EButton_DPad_Right:       return "DPad_Right";
		case vr::k_EButton_DPad_Down:        return "DPad_Down";
		case vr::k_EButton_A:                return "A";
		case vr::k_EButton_ProximitySensor:  return "ProximitySensor";
		case vr::k_EButton_SteamVR_Touchpad: return "Touchpad";
		case vr::k_EButton_SteamVR_Trigger:  return "Trigger";
		case vr::k_EButton_Axis2:            return "Axis2";
		case vr::k_EButton_Axis3:            return "Axis3";
		case vr::k_EButton_Axis4:            return "Axis4";
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

static void LogStartEnd(bool IsStart, bool IsTouch, int Button)
{
	static int TouchRefs[vr::k_EButton_Max], PressRefs[vr::k_EButton_Max], TouchCounts[vr::k_EButton_Max], PressCounts[vr::k_EButton_Max];
	int *Refs = (IsTouch ? TouchRefs : PressRefs), *Counts = (IsTouch ? TouchCounts : PressCounts);
	const char *TypeName = (IsTouch ? "TOUCH" : "PRESS"), *ButtonName = VRButtonIdStr((vr::EVRButtonId)Button);
	if ( IsStart && !Refs[Button]++) printf("\n--- %s [%s] %d START ---\n", TypeName, ButtonName, ++Counts[Button]);
	if (!IsStart && !--Refs[Button]) printf("--- %s [%s] %d END ---\n\n",   TypeName, ButtonName,   Counts[Button]);
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
	memset(PastStates, 0, sizeof(PastStates));

	//Loop forever
	for (;;)
	{
		//Poll events and print incoming button/touch events
		while (pVRSystem->PollNextEvent(&Event, sizeof(Event)))
		{
			if (Event.eventType < vr::VREvent_ButtonPress || Event.eventType > vr::VREvent_ButtonUntouch) continue;
			if (Event.eventType == vr::VREvent_ButtonPress || Event.eventType == vr::VREvent_ButtonTouch) LogStartEnd(true, Event.eventType == vr::VREvent_ButtonTouch, Event.data.controller.button);
			printf("[%.2f] Event %s (%d) - Age: %f - Button: %s\n", GetVRTime(pVRSystem), pVRSystem->GetEventTypeNameFromEnum((vr::EVREventType)Event.eventType), Event.eventType, Event.eventAgeSeconds, VRButtonIdStr((vr::EVRButtonId)Event.data.controller.button));
			if (Event.eventType == vr::VREvent_ButtonUnpress || Event.eventType == vr::VREvent_ButtonUntouch) LogStartEnd(false, Event.eventType == vr::VREvent_ButtonUntouch, Event.data.controller.button);
		}

		//Poll controller state and print status on state change
		for (int i = 0; i != 2; i++)
		{
			if (ControllerIndices[i] == vr::k_unTrackedDeviceIndexInvalid) continue;
			pVRSystem->GetControllerState(ControllerIndices[i], &State, sizeof(State));
			if (memcmp(&PastStates[i].ulButtonPressed, &State.ulButtonPressed, (char*)((&State)+1) - (char*)&State.ulButtonPressed))
			{
				uint64_t OldTouch = PastStates[i].ulButtonTouched, NewTouch = State.ulButtonTouched, OldPress = PastStates[i].ulButtonPressed, NewPress = State.ulButtonPressed;
				for (int j = 0; j != vr::k_EButton_Max; j++)
				{
					if (!(OldTouch & (1ull<<j)) &&  (NewTouch & (1ull<<j))) LogStartEnd(true,  true, j);
					if (!(OldPress & (1ull<<j)) &&  (NewPress & (1ull<<j))) LogStartEnd(true, false, j);
				}
				
				printf("[%.2f] Device: %d - PacketNum: %d - Touch: ", GetVRTime(pVRSystem), ControllerIndices[i], State.unPacketNum);
				for (int j = 0, n = 0; j != vr::k_EButton_Max; j++) if (NewTouch & (1ull<<j)) printf("%s%s", (n++ ? ", " : ""), VRButtonIdStr((vr::EVRButtonId)j));
				printf(" - Press: ");
				for (int j = 0, n = 0; j != vr::k_EButton_Max; j++) if (NewPress & (1ull<<j)) printf("%s%s", (n++ ? ", " : ""), VRButtonIdStr((vr::EVRButtonId)j));
				printf(" - X: %.2f - Y: %.2f - Trigger: %.2f\n", State.rAxis[0].x, State.rAxis[0].y, State.rAxis[1].x);

				for (int j = 0; j != vr::k_EButton_Max; j++)
				{
					if ( (OldTouch & (1ull<<j)) && !(NewTouch & (1ull<<j))) LogStartEnd(false,  true, j);
					if ( (OldPress & (1ull<<j)) && !(NewPress & (1ull<<j))) LogStartEnd(false, false, j);
				}
			}
			memcpy(&PastStates[i], &State, sizeof(State));
		}
	}
}
