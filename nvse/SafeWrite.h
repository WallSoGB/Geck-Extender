#pragma once

void SafeWrite8(UInt32 addr, UInt32 data);
void SafeWrite16(UInt32 addr, UInt32 data);
void SafeWrite32(UInt32 addr, UInt32 data);
void SafeWrite64(UInt32 addr, UInt32 data);
void SafeWriteBuf(UInt32 addr, void * data, UInt32 len);
void SafeWriteBuf(UInt32 addr, const char* data, UInt32 len);
void SafeWriteFloat(UInt32 addr, float data);

// 5 bytes
void WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt);
void WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt);

// 6 bytes
void WriteRelJnz(UInt32 jumpSrc, UInt32 jumpTgt);
void WriteRelJz(UInt32 jumpSrc, UInt32 jumpTgt);
void WriteRelJle(UInt32 jumpSrc, UInt32 jumpTgt);
void WriteRelJa(UInt32 jumpSrc, UInt32 jumpTgt);

[[nodiscard]] __declspec(noinline) UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst);
[[nodiscard]] __declspec(noinline) UInt32 __stdcall DetourRelCall(UInt32 jumpSrc, UInt32 jumpTgt);
UInt32 __stdcall GetRelJumpAddr(UInt32 jumpSrc);

void ReplaceCall(UInt32 jumpSrc, UInt32 jumpTgt);

template <typename C, typename Ret, typename... Args>
void ReplaceCallEx(UInt32 source, Ret(C::* const target)(Args...)) {
	union
	{
		Ret(C::* tgt)(Args...);
		UInt32 funcPtr;
	} conversion;
	conversion.tgt = target;

	ReplaceCall(source, conversion.funcPtr);
}

static inline SIZE_T GetWriteAddr(SIZE_T writeAddr) {
	return *(SIZE_T*)(writeAddr);
}

class CallDetour {
	SIZE_T overwritten_addr = 0;
public:
	__declspec(noinline) void WriteRelCall(SIZE_T jumpSrc, void* jumpTgt)
	{
		__assume(jumpSrc != 0);
		__assume(jumpTgt != nullptr);
		if (*reinterpret_cast<uint8_t*>(jumpSrc) != 0xE8) {
			char cTextBuffer[72];
			sprintf_s(cTextBuffer, "Cannot write detour; jumpSrc is not a function call. (0x%08X)", jumpSrc);
			MessageBoxA(nullptr, cTextBuffer, "WriteRelCall", MB_OK | MB_ICONERROR);
			return;
		}
		overwritten_addr = GetRelJumpAddr(jumpSrc);
		::WriteRelCall(jumpSrc, uint32_t(jumpTgt));
	}

	template <typename T>
	void ReplaceCall(SIZE_T jumpSrc, T jumpTgt) {
		__assume(jumpSrc != 0);
		if (*reinterpret_cast<uint8_t*>(jumpSrc) != 0xE8) {
			char cTextBuffer[72];
			sprintf_s(cTextBuffer, "Cannot write detour; jumpSrc is not a function call. (0x%08X)", jumpSrc);
			MessageBoxA(nullptr, cTextBuffer, "WriteRelCall", MB_OK | MB_ICONERROR);
			return;
		}
		overwritten_addr = GetRelJumpAddr(jumpSrc);
		::ReplaceCall(jumpSrc, (SIZE_T)jumpTgt);
	}

	template <typename C, typename Ret, typename... Args>
	void ReplaceCallEx(SIZE_T source, Ret(C::* const target)(Args...) const) {
		union
		{
			Ret(C::* tgt)(Args...) const;
			SIZE_T funcPtr;
		} conversion;
		conversion.tgt = target;

		ReplaceCall(source, conversion.funcPtr);
	}

	template <typename C, typename Ret, typename... Args>
	void ReplaceCallEx(SIZE_T source, Ret(C::* const target)(Args...)) {
		union
		{
			Ret(C::* tgt)(Args...);
			SIZE_T funcPtr;
		} conversion;
		conversion.tgt = target;

		ReplaceCall(source, conversion.funcPtr);
	}

	template <typename T>
	void SafeWrite32(SIZE_T jumpSrc, T jumpTgt) {
		__assume(jumpSrc != 0);
		overwritten_addr = GetWriteAddr(jumpSrc);
		::SafeWrite32(jumpSrc, (SIZE_T)jumpTgt);
	}

	[[nodiscard]] SIZE_T GetOverwrittenAddr() const { return overwritten_addr; }
};