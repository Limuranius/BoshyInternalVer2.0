#include "pch.h"
#include "Hook.h"


bool Detour32(BYTE* src, BYTE* dst, const int len) {
	if (len < 5) {
		return false;
	}

	DWORD curprotection;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curprotection);
	memset(src, '\x90', len);
	uintptr_t relativeAddress = dst - src - 5;
	*src = 0xE9;

	*(uintptr_t*)(src + 1) = relativeAddress;

	VirtualProtect(src, len, curprotection, &curprotection);
	return true;
}

BYTE* TrampHook32(BYTE* src, BYTE* dst, const int len) {
	if (len < 5) return 0;

	//create gateway
	BYTE* gateway = (BYTE*)VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	//write stolen bytes
	memcpy_s(gateway, len, src, len);

	//get the gateway to destination address
	uintptr_t gatewayRelativeAddress = src - gateway - 5;

	//add the jmp opcode to the end		of the gateway

	*(gateway + len) = 0xE9;

	//write the address of the gateway to the jmp
	*(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddress;

	//perform the detour

	Detour32(src, dst, len);

	return gateway;
}
Hook::Hook(BYTE* src, BYTE* dst, BYTE* PtrToGatewayPtr, const int len) {
	this->src = src;
	this->dst = dst;
	this->len = len;
	this->PtrToGatewayPtr = PtrToGatewayPtr;
}
void Hook::Enable() {
	memcpy(originalBytes, src, len);
	*(uintptr_t*)PtrToGatewayPtr = (uintptr_t)TrampHook32(src, dst, len);
	bStatus = true;
}
void Hook::Disable() {
	patch(src, originalBytes, len);
	bStatus = false; 
}
void Hook::Toggle() {
	if (!bStatus) Enable();
	else Disable();
}