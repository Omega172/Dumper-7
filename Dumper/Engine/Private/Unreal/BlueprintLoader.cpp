#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "Unreal/BlueprintLoader.h"
#include "Unreal/UnrealObjects.h"
#include "Unreal/ObjectArray.h"
#include "OffsetFinder/Offsets.h"
#include "Settings.h"

namespace BlueprintLoader
{
	using ProcessEventFn = void(*)(void*, void*, void*);

	enum class EState : int
	{
		Idle,
		Waiting,
		Loading,
		Loaded,
		TimedOut,
	};

	struct FRawArray
	{
		uint8* Data;
		int32 Num;
		int32 Max;
	};

	struct FAssetLayout
	{
		int32 Size = 0;
		int32 ObjectPathOffset = -1;
		int32 PackageNameOffset = -1;
		int32 AssetNameOffset = -1;
		int32 AssetClassOffset = -1;
	};

	struct FAssetEntry
	{
		std::vector<uint8> Data;
		std::string Label;
	};

	static std::atomic<EState> State = EState::Idle;
	static std::atomic<int32> InFlight = 0;
	static thread_local bool bInsideLoader = false;

	static ProcessEventFn OriginalProcessEvent = nullptr;
	static std::vector<void**> PatchedTables;
	static DWORD GameThreadId = 0;
	static int32 ReceiveTickNameIndex = -1;
	static std::chrono::steady_clock::time_point HookTime;

	static HANDLE LoadedEvent = nullptr;
	static HANDLE ReleaseEvent = nullptr;

	static UEFunction GetAssetFunction;
	static UEFunction StringToNameFunction;
	static UEObject AssetRegistryHelpersDefault;
	static UEObject StringLibraryDefault;
	static int32 NameSize = 0;

	static DWORD FindMainThreadId()
	{
		const DWORD ProcessId = GetCurrentProcessId();
		HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

		if (Snapshot == INVALID_HANDLE_VALUE)
			return 0;

		DWORD BestId = 0;
		ULONGLONG BestTime = ~0ull;

		THREADENTRY32 Entry = { sizeof(THREADENTRY32) };
		for (BOOL bOk = Thread32First(Snapshot, &Entry); bOk; bOk = Thread32Next(Snapshot, &Entry))
		{
			if (Entry.th32OwnerProcessID != ProcessId)
				continue;

			HANDLE Thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, Entry.th32ThreadID);
			if (!Thread)
				continue;

			FILETIME Creation, Exit, Kernel, User;
			if (GetThreadTimes(Thread, &Creation, &Exit, &Kernel, &User))
			{
				const ULONGLONG Time = (static_cast<ULONGLONG>(Creation.dwHighDateTime) << 32) | Creation.dwLowDateTime;
				if (Time < BestTime)
				{
					BestTime = Time;
					BestId = Entry.th32ThreadID;
				}
			}

			CloseHandle(Thread);
		}

		CloseHandle(Snapshot);
		return BestId;
	}

	static bool CallGuarded(UEObject Object, UEFunction Function, void* Params)
	{
		__try
		{
			Object.ProcessEvent(Function, Params);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	static bool MakeName(const std::wstring& Value, uint8* OutName)
	{
		std::vector<uint8> Params(StringToNameFunction.GetStructSize(), 0);

		const UEProperty InString = StringToNameFunction.FindMember("InString");
		const UEProperty ReturnValue = StringToNameFunction.GetReturnProperty();

		if (!InString || !ReturnValue)
			return false;

		std::wstring Copy = Value;
		new (Params.data() + InString.GetOffset()) FString(Copy.data());

		if (!CallGuarded(StringLibraryDefault, StringToNameFunction, Params.data()))
			return false;

		memcpy(OutName, Params.data() + ReturnValue.GetOffset(), NameSize);
		return true;
	}

	static std::string NameAt(const uint8* Base, int32 Offset)
	{
		if (Offset < 0)
			return {};

		return FName(Base + Offset).ToRawString();
	}

	static bool EndsWith(const std::string& Value, const std::string& Suffix)
	{
		return Value.size() >= Suffix.size() && Value.compare(Value.size() - Suffix.size(), Suffix.size(), Suffix) == 0;
	}

	static bool IsWanted(const std::string& ClassName)
	{
		return EndsWith(ClassName, "Blueprint") || EndsWith(ClassName, "BlueprintGeneratedClass") || ClassName == "UserDefinedStruct" || ClassName == "UserDefinedEnum";
	}

	static bool IsExcluded(const std::string& PackageName)
	{
		for (const std::string& Prefix : Settings::Config::LoadBlueprintsExcludePaths)
		{
			if (PackageName.rfind(Prefix, 0) == 0)
				return true;
		}

		return false;
	}

	static bool ResolveLayout(FAssetLayout& OutLayout)
	{
		const UEStruct AssetData = ObjectArray::FindStructFast("AssetData");
		if (!AssetData)
			return false;

		OutLayout.Size = AssetData.GetStructSize();

		auto OffsetOf = [&](const char* Member) -> int32
		{
			const UEProperty Property = AssetData.FindMember(Member);
			return Property ? Property.GetOffset() : -1;
		};

		OutLayout.ObjectPathOffset = OffsetOf("ObjectPath");
		OutLayout.PackageNameOffset = OffsetOf("PackageName");
		OutLayout.AssetNameOffset = OffsetOf("AssetName");
		const int32 ClassPathOffset = OffsetOf("AssetClassPath");
		const UEStruct TopLevelAssetPath = ObjectArray::FindStructFast("TopLevelAssetPath");
		const UEProperty ClassAssetName = TopLevelAssetPath ? TopLevelAssetPath.FindMember("AssetName") : UEProperty();

		if (ClassPathOffset >= 0 && ClassAssetName)
			OutLayout.AssetClassOffset = ClassPathOffset + ClassAssetName.GetOffset();
		else
			OutLayout.AssetClassOffset = OffsetOf("AssetClass");

		return OutLayout.PackageNameOffset >= 0 && OutLayout.AssetNameOffset >= 0 && OutLayout.AssetClassOffset >= 0;
	}

	static UEObject GetAssetRegistry()
	{
		const UEFunction GetRegistry = AssetRegistryHelpersDefault.GetClass().GetFunction("AssetRegistryHelpers", "GetAssetRegistry");
		if (!GetRegistry)
			return UEObject();

		const UEProperty ReturnValue = GetRegistry.GetReturnProperty();
		if (!ReturnValue)
			return UEObject();

		std::vector<uint8> Params(GetRegistry.GetStructSize(), 0);
		if (!CallGuarded(AssetRegistryHelpersDefault, GetRegistry, Params.data()))
			return UEObject();

		return UEObject(*reinterpret_cast<void**>(Params.data() + ReturnValue.GetOffset()));
	}

	static void CollectAssets(UEObject Registry, const FAssetLayout& Layout, std::vector<FAssetEntry>& OutAssets)
	{
		const UEClass RegistryInterface = ObjectArray::FindClassFast("AssetRegistry");
		const UEFunction GetAssetsByPath = RegistryInterface ? RegistryInterface.GetFunction("AssetRegistry", "GetAssetsByPath") : UEFunction();

		if (!GetAssetsByPath)
		{
			std::cerr << "LoadAllBlueprints: AssetRegistry::GetAssetsByPath not found\n";
			return;
		}

		const UEProperty PackagePath = GetAssetsByPath.FindMember("PackagePath");
		const UEProperty OutAssetData = GetAssetsByPath.FindMember("OutAssetData");
		const UEProperty Recursive = GetAssetsByPath.FindMember("bRecursive");
		const UEProperty OnDiskOnly = GetAssetsByPath.FindMember("bIncludeOnlyOnDiskAssets");

		if (!PackagePath || !OutAssetData)
		{
			std::cerr << "LoadAllBlueprints: unexpected GetAssetsByPath parameters\n";
			return;
		}

		std::unordered_set<std::string> Seen;

		for (const std::string& Path : Settings::Config::LoadBlueprintsPaths)
		{
			std::vector<uint8> Params(GetAssetsByPath.GetStructSize(), 0);

			if (!MakeName(std::wstring(Path.begin(), Path.end()), Params.data() + PackagePath.GetOffset()))
			{
				std::cerr << "LoadAllBlueprints: could not make a name for " << Path << "\n";
				continue;
			}

			if (Recursive)
				Params[Recursive.GetOffset()] = 1;

			if (OnDiskOnly)
				Params[OnDiskOnly.GetOffset()] = 0;

			if (!CallGuarded(Registry, GetAssetsByPath, Params.data()))
			{
				std::cerr << "LoadAllBlueprints: GetAssetsByPath faulted for " << Path << "\n";
				continue;
			}

			const FRawArray& Assets = *reinterpret_cast<const FRawArray*>(Params.data() + OutAssetData.GetOffset());
			std::cerr << "LoadAllBlueprints: " << std::dec << Assets.Num << " assets under " << Path << "\n";

			int32 Wanted = 0;
			std::vector<std::string> SampleClasses;

			for (int32 i = 0; i < Assets.Num; i++)
			{
				const uint8* Asset = Assets.Data + static_cast<size_t>(i) * Layout.Size;

				const std::string ClassName = NameAt(Asset, Layout.AssetClassOffset);
				if (!IsWanted(ClassName))
				{
					if (SampleClasses.size() < 8 && std::find(SampleClasses.begin(), SampleClasses.end(), ClassName) == SampleClasses.end())
						SampleClasses.push_back(ClassName);

					continue;
				}

				Wanted++;

				const std::string PackageName = NameAt(Asset, Layout.PackageNameOffset);
				if (IsExcluded(PackageName))
					continue;

				std::string AssetName = NameAt(Asset, Layout.AssetNameOffset);
				FAssetEntry Entry;
				Entry.Data.assign(Asset, Asset + Layout.Size);

				if (EndsWith(ClassName, "Blueprint") && !EndsWith(AssetName, "_C"))
				{
					const std::string ClassAssetName = AssetName + "_C";

					if (!MakeName(std::wstring(ClassAssetName.begin(), ClassAssetName.end()), Entry.Data.data() + Layout.AssetNameOffset))
						continue;

					if (Layout.ObjectPathOffset >= 0)
					{
						const std::string ObjectPath = PackageName + "." + ClassAssetName;
						if (!MakeName(std::wstring(ObjectPath.begin(), ObjectPath.end()), Entry.Data.data() + Layout.ObjectPathOffset))
							continue;
					}

					AssetName = ClassAssetName;
				}

				Entry.Label = PackageName + "." + AssetName;

				if (Seen.insert(Entry.Label).second)
					OutAssets.push_back(std::move(Entry));
			}

			if (Wanted == 0 && Assets.Num > 0)
			{
				std::cerr << "LoadAllBlueprints: no Blueprint assets matched under " << Path << ", sample classes:";
				for (const std::string& ClassName : SampleClasses)
					std::cerr << " '" << ClassName << "'";
				std::cerr << "\n";
			}
		}
	}

	static void LoadAssets()
	{
		AssetRegistryHelpersDefault = ObjectArray::FindClassFast("AssetRegistryHelpers").GetDefaultObject();
		StringLibraryDefault = ObjectArray::FindClassFast("KismetStringLibrary").GetDefaultObject();

		if (!AssetRegistryHelpersDefault || !StringLibraryDefault)
		{
			std::cerr << "LoadAllBlueprints: AssetRegistryHelpers or KismetStringLibrary not found, nothing loaded\n";
			return;
		}

		GetAssetFunction = AssetRegistryHelpersDefault.GetClass().GetFunction("AssetRegistryHelpers", "GetAsset");
		StringToNameFunction = StringLibraryDefault.GetClass().GetFunction("KismetStringLibrary", "Conv_StringToName");

		if (!GetAssetFunction || !StringToNameFunction || !StringToNameFunction.GetReturnProperty())
		{
			std::cerr << "LoadAllBlueprints: GetAsset or Conv_StringToName not found, nothing loaded\n";
			return;
		}

		NameSize = StringToNameFunction.GetReturnProperty().GetSize();

		FAssetLayout Layout;
		if (!ResolveLayout(Layout))
		{
			std::cerr << "LoadAllBlueprints: could not resolve the FAssetData layout, nothing loaded\n";
			return;
		}

		const UEObject Registry = GetAssetRegistry();
		if (!Registry)
		{
			std::cerr << "LoadAllBlueprints: no AssetRegistry, nothing loaded\n";
			return;
		}

		std::vector<FAssetEntry> Assets;
		CollectAssets(Registry, Layout, Assets);

		const UEProperty InAssetData = GetAssetFunction.FindMember("InAssetData");
		const UEProperty ReturnValue = GetAssetFunction.GetReturnProperty();

		if (!InAssetData || !ReturnValue)
		{
			std::cerr << "LoadAllBlueprints: unexpected GetAsset parameters, nothing loaded\n";
			return;
		}

		const int32 Total = static_cast<int32>(Assets.size());
		std::cerr << "LoadAllBlueprints: loading " << std::dec << Total << " assets\n";

		int32 Loaded = 0;
		int32 Failed = 0;

		for (int32 i = 0; i < Total; i++)
		{
			const FAssetEntry& Entry = Assets[i];
			std::cerr << "[" << std::dec << (i + 1) << "/" << Total << "] " << Entry.Label << std::endl;

			std::vector<uint8> Params(GetAssetFunction.GetStructSize(), 0);
			memcpy(Params.data() + InAssetData.GetOffset(), Entry.Data.data(), Entry.Data.size());

			if (!CallGuarded(AssetRegistryHelpersDefault, GetAssetFunction, Params.data()))
			{
				std::cerr << "LoadAllBlueprints: faulted loading " << Entry.Label << "\n";
				Failed++;
				continue;
			}

			if (*reinterpret_cast<void**>(Params.data() + ReturnValue.GetOffset()))
				Loaded++;
			else
				Failed++;
		}

		std::cerr << "LoadAllBlueprints: " << std::dec << Loaded << " loaded, " << Failed << " failed\n";
	}

	static void HookedProcessEvent(void* Object, void* Function, void* Params)
	{
		InFlight++;

		if (!bInsideLoader && State.load() == EState::Waiting && GetCurrentThreadId() == GameThreadId)
		{
			const bool bTimedFallback = std::chrono::steady_clock::now() - HookTime > std::chrono::seconds(10);
			const bool bTick = Function && UEObject(Function).GetFName().GetCompIdx() == ReceiveTickNameIndex;

			EState Expected = EState::Waiting;
			if ((bTick || bTimedFallback) && State.compare_exchange_strong(Expected, EState::Loading))
			{
				bInsideLoader = true;
				std::cerr << "LoadAllBlueprints: running on the game thread (" << (bTick ? "ReceiveTick" : "ProcessEvent") << ")\n";

				LoadAssets();

				State = EState::Loaded;
				SetEvent(LoadedEvent);
				WaitForSingleObject(ReleaseEvent, INFINITE);
				bInsideLoader = false;
			}
		}

		OriginalProcessEvent(Object, Function, Params);
		InFlight--;
	}

	static void SetTableEntry(void** Table, void* Value)
	{
		void** Slot = Table + Off::InSDK::ProcessEvent::PEIndex;
		DWORD OldProtect = 0;

		if (VirtualProtect(Slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &OldProtect))
		{
			InterlockedExchangePointer(reinterpret_cast<PVOID*>(Slot), Value);
			VirtualProtect(Slot, sizeof(void*), OldProtect, &OldProtect);
		}
	}

	static void InstallHook()
	{
		std::unordered_set<void**> Tables;

		for (UEObject Object : ObjectArray())
		{
			void** Table = static_cast<void**>(Object.GetVft());
			if (!Table)
				continue;

			if (!OriginalProcessEvent)
				OriginalProcessEvent = reinterpret_cast<ProcessEventFn>(Table[Off::InSDK::ProcessEvent::PEIndex]);

			if (reinterpret_cast<ProcessEventFn>(Table[Off::InSDK::ProcessEvent::PEIndex]) == OriginalProcessEvent)
				Tables.insert(Table);
		}

		for (void** Table : Tables)
		{
			SetTableEntry(Table, reinterpret_cast<void*>(&HookedProcessEvent));
			PatchedTables.push_back(Table);
		}

		std::cerr << "LoadAllBlueprints: hooked ProcessEvent in " << std::dec << PatchedTables.size() << " vtables\n";
	}

	static void RemoveHook()
	{
		for (void** Table : PatchedTables)
			SetTableEntry(Table, reinterpret_cast<void*>(OriginalProcessEvent));

		PatchedTables.clear();
	}

	void LoadAll()
	{
		if (!Settings::Config::bLoadAllBlueprints)
			return;

		GameThreadId = FindMainThreadId();
		if (!GameThreadId)
		{
			std::cerr << "LoadAllBlueprints: could not find the game thread, skipping\n";
			return;
		}

		const UEFunction ReceiveTick = ObjectArray::FindClassFast("Actor").GetFunction("Actor", "ReceiveTick");
		ReceiveTickNameIndex = ReceiveTick ? ReceiveTick.GetFName().GetCompIdx() : -1;

		LoadedEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
		ReleaseEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);

		HookTime = std::chrono::steady_clock::now();
		State = EState::Waiting;
		InstallHook();

		std::cerr << "LoadAllBlueprints: waiting for the game thread\n";

		if (WaitForSingleObject(LoadedEvent, 60000) == WAIT_OBJECT_0)
			return;

		EState Expected = EState::Waiting;
		if (State.compare_exchange_strong(Expected, EState::TimedOut))
		{
			std::cerr << "LoadAllBlueprints: the game thread never called ProcessEvent within 60 seconds, dumping without loading\n";
			RemoveHook();
			return;
		}

		WaitForSingleObject(LoadedEvent, INFINITE);
	}

	void Release()
	{
		if (State.load() == EState::Idle)
			return;

		RemoveHook();

		if (ReleaseEvent)
			SetEvent(ReleaseEvent);
	}

	void WaitForIdle()
	{
		if (State.load() == EState::Idle)
			return;

		const auto Start = std::chrono::steady_clock::now();
		while (InFlight.load() > 0 && std::chrono::steady_clock::now() - Start < std::chrono::seconds(30))
			Sleep(10);

		Sleep(200);
	}
}
