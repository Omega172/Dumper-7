#pragma once

#include <array>
#include <string>
#include <iostream>
#include <Windows.h>

#include "Unreal/Enums.h"
#include "OffsetFinder/Offsets.h"

#include "Encoding/UnicodeNames.h"

#include "Utils.h"
#include "UnrealContainers.h"

using namespace UC;

extern std::string MakeNameValid(std::wstring&& Name);

template<typename Type>
struct TImplementedInterface
{
public:
	void* InterfaceClass;
	int32 PointerOffset;
	bool bImplementedByK2;

public:
	std::string GetInterfaceVTableCppName() const;
};

using FImplementedInterface = TImplementedInterface<class UEClass>;

class FFreableString : public FString
{
public:
	using FString::FString;

	FFreableString(uint32_t NumElementsToReserve)
	{
		if (NumElementsToReserve > 0x100000)
			return;

		this->Data = static_cast<wchar_t*>(malloc(sizeof(wchar_t) * NumElementsToReserve));
		this->NumElements = 0;
		this->MaxElements = NumElementsToReserve;
	}

	~FFreableString()
	{
		/* If we're using FName::ToString the allocation comes from the engine and we can not free it. Just leak those 2048 bytes. */
		if (Off::InSDK::Name::bIsUsingAppendStringOverToString)
			FreeArray();
	}

public:
	inline void ResetNum()
	{
		this->NumElements = 0;
	}

private:
	inline void FreeArray()
	{
		if (this->Data)
			free(this->Data);

		this->Data = nullptr;
		this->NumElements = 0;
		this->MaxElements = 0;
	}
};

class FName
{
public:
	enum class EOffsetOverrideType
	{
		AppendString,
		ToString,
		GNames
	};

private:
	inline static void(*AppendString)(const void*, FString&) = nullptr;

	inline static std::wstring(*ToStr)(const void* Name) = nullptr;

private:
	const uint8* Address;

public:
	FName() = default;

	FName(const void* Ptr);

public:
	static void Init(bool bForceGNames = false);
	static void InitFallback();

	static void Init(int32 OverrideOffset, EOffsetOverrideType OverrideType = EOffsetOverrideType::AppendString, bool bIsNamePool = false, const char* const ModuleName = nullptr);

public:
	inline const void* GetAddress() const { return Address; }

	std::wstring ToWString() const;
	std::wstring ToRawWString() const;

	std::string ToString() const;
	std::string ToRawString() const;
	std::string ToValidString() const;

	int32 GetCompIdx() const;
	uint32 GetNumber() const;

	bool operator==(FName Other) const;

	bool operator!=(FName Other) const;

	static std::string CompIdxToString(int CmpIdx);

	static void* DEBUGGetAppendString();
};
