#pragma once

#include <cstdint>
#include <type_traits>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>

#include <cassert>

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;


template<typename T>
constexpr T Align(T Size, T Alignment)
{
	static_assert(std::is_integral_v<T>, "Align can only hanlde integral types!");
	assert(Alignment != 0 && "Alignment was 0, division by zero exception.");

	const T RequiredAlign = Alignment - (Size % Alignment);

	return Size + (RequiredAlign != Alignment ? RequiredAlign : 0x0);
}


#define ENUM_OPERATORS(EEnumClass)																																		\
																																										\
inline constexpr EEnumClass operator|(EEnumClass Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass)((std::underlying_type<EEnumClass>::type)(Left) | (std::underlying_type<EEnumClass>::type)(Right));												\
}																																										\
																																										\
inline constexpr EEnumClass& operator|=(EEnumClass& Left, EEnumClass Right)																								\
{																																										\
	return (EEnumClass&)((std::underlying_type<EEnumClass>::type&)(Left) |= (std::underlying_type<EEnumClass>::type)(Right));											\
}																																										\
																																										\
inline bool operator&(EEnumClass Left, EEnumClass Right)																												\
{																																										\
	return (((std::underlying_type<EEnumClass>::type)(Left) & (std::underlying_type<EEnumClass>::type)(Right)) == (std::underlying_type<EEnumClass>::type)(Right));		\
}																																										


enum class EPropertyFlags : uint64
{
	None							= 0x0000000000000000,

	Edit							= 0x0000000000000001,
	ConstParm						= 0x0000000000000002,
	BlueprintVisible				= 0x0000000000000004,
	ExportObject					= 0x0000000000000008,
	BlueprintReadOnly				= 0x0000000000000010,
	Net								= 0x0000000000000020,
	EditFixedSize					= 0x0000000000000040,
	Parm							= 0x0000000000000080,
	OutParm							= 0x0000000000000100,
	ZeroConstructor					= 0x0000000000000200,
	ReturnParm						= 0x0000000000000400,
	DisableEditOnTemplate 			= 0x0000000000000800,

	Transient						= 0x0000000000002000,
	Config							= 0x0000000000004000,

	DisableEditOnInstance			= 0x0000000000010000,
	EditConst						= 0x0000000000020000,
	GlobalConfig					= 0x0000000000040000,
	InstancedReference				= 0x0000000000080000,	

	DuplicateTransient				= 0x0000000000200000,	
	SubobjectReference				= 0x0000000000400000,	

	SaveGame						= 0x0000000001000000,
	NoClear							= 0x0000000002000000,

	ReferenceParm					= 0x0000000008000000,
	BlueprintAssignable				= 0x0000000010000000,
	Deprecated						= 0x0000000020000000,
	IsPlainOldData					= 0x0000000040000000,
	RepSkip							= 0x0000000080000000,
	RepNotify						= 0x0000000100000000, 
	Interp							= 0x0000000200000000,
	NonTransactional				= 0x0000000400000000,
	EditorOnly						= 0x0000000800000000,
	NoDestructor					= 0x0000001000000000,

	AutoWeak						= 0x0000004000000000,
	ContainsInstancedReference		= 0x0000008000000000,	
	AssetRegistrySearchable			= 0x0000010000000000,
	SimpleDisplay					= 0x0000020000000000,
	AdvancedDisplay					= 0x0000040000000000,
	Protected						= 0x0000080000000000,
	BlueprintCallable				= 0x0000100000000000,
	BlueprintAuthorityOnly			= 0x0000200000000000,
	TextExportTransient				= 0x0000400000000000,
	NonPIEDuplicateTransient		= 0x0000800000000000,
	ExposeOnSpawn					= 0x0001000000000000,
	PersistentInstance				= 0x0002000000000000,
	UObjectWrapper					= 0x0004000000000000, 
	HasGetValueTypeHash				= 0x0008000000000000, 
	NativeAccessSpecifierPublic		= 0x0010000000000000,	
	NativeAccessSpecifierProtected	= 0x0020000000000000,
	NativeAccessSpecifierPrivate	= 0x0040000000000000,	
	SkipSerialization				= 0x0080000000000000, 
};

enum class EFunctionFlags : uint32
{
	None					= 0x00000000,

	Final					= 0x00000001,
	RequiredAPI				= 0x00000002,
	BlueprintAuthorityOnly	= 0x00000004, 
	BlueprintCosmetic		= 0x00000008, 
	Net						= 0x00000040,  
	NetReliable				= 0x00000080, 
	NetRequest				= 0x00000100,	
	Exec					= 0x00000200,	
	Native					= 0x00000400,	
	Event					= 0x00000800,   
	NetResponse				= 0x00001000,  
	Static					= 0x00002000,   
	NetMulticast			= 0x00004000,	
	UbergraphFunction		= 0x00008000,  
	MulticastDelegate		= 0x00010000,
	Public					= 0x00020000,	
	Private					= 0x00040000,	
	Protected				= 0x00080000,
	Delegate				= 0x00100000,	
	NetServer				= 0x00200000,	
	HasOutParms				= 0x00400000,	
	HasDefaults				= 0x00800000,
	NetClient				= 0x01000000,
	DLLImport				= 0x02000000,
	BlueprintCallable		= 0x04000000,
	BlueprintEvent			= 0x08000000,
	BlueprintPure			= 0x10000000,	
	EditorOnly				= 0x20000000,	
	Const					= 0x40000000,
	NetValidate				= 0x80000000,

	AllFlags = 0xFFFFFFFF,
};

//
// Evaluatable expression item types.
//
enum class EExprToken : uint8
{
	// Variable references.
	LocalVariable					= 0x00,	// A local variable.
	InstanceVariable				= 0x01,	// An object variable.
	DefaultVariable					= 0x02, // Default variable for a class context.

	Return							= 0x04,	// Return from function.

	Jump							= 0x06,	// Goto a local address in code.
	JumpIfNot						= 0x07,	// Goto if not expression.

	Assert							= 0x09,	// Assertion.

	Nothing							= 0x0B,	// No operation.
	NothingInt32					= 0x0C, // No operation with an int32 argument (useful for debugging script disassembly)

	Let								= 0x0F,	// Assign an arbitrary size value to a variable.

	BitFieldConst					= 0x11, // assign to a single bit, defined by an FProperty
	ClassContext					= 0x12,	// Class default object context.
	MetaCast						= 0x13, // Metaclass cast.
	LetBool							= 0x14, // Let boolean variable.
	EndParmValue					= 0x15,	// end of default value for optional function parameter
	EndFunctionParms				= 0x16,	// End of function call parameters.
	Self							= 0x17,	// Self object.
	Skip							= 0x18,	// Skippable expression.
	Context							= 0x19,	// Call a function through an object context.
	Context_FailSilent				= 0x1A, // Call a function through an object context (can fail silently if the context is NULL; only generated for functions that don't have output or return values).
	VirtualFunction					= 0x1B,	// A function call with parameters.
	FinalFunction					= 0x1C,	// A prebound function call with parameters.
	IntConst						= 0x1D,	// Int constant.
	FloatConst						= 0x1E,	// Floating point constant.
	StringConst						= 0x1F,	// String constant.
	ObjectConst						= 0x20,	// An object constant.
	NameConst						= 0x21,	// A name constant.
	RotationConst					= 0x22,	// A rotation constant.
	VectorConst						= 0x23,	// A vector constant.
	ByteConst						= 0x24,	// A byte constant.
	IntZero							= 0x25,	// Zero.
	IntOne							= 0x26,	// One.
	True							= 0x27,	// Bool True.
	False							= 0x28,	// Bool False.
	TextConst						= 0x29, // FText constant
	NoObject						= 0x2A,	// NoObject.
	TransformConst					= 0x2B, // A transform constant
	IntConstByte					= 0x2C,	// Int constant that requires 1 byte.
	NoInterface						= 0x2D, // A null interface (similar to EX_NoObject, but for interfaces)
	DynamicCast						= 0x2E,	// Safe dynamic class casting.
	StructConst						= 0x2F, // An arbitrary UStruct constant
	EndStructConst					= 0x30, // End of UStruct constant
	SetArray						= 0x31, // Set the value of arbitrary array
	EndArray						= 0x32,
	PropertyConst					= 0x33, // FProperty constant.
	UnicodeStringConst				= 0x34, // Unicode string constant.
	Int64Const						= 0x35,	// 64-bit integer constant.
	UInt64Const						= 0x36,	// 64-bit unsigned integer constant.
	DoubleConst						= 0x37, // Double constant.
	Cast							= 0x38,	// A casting operator which reads the type as the subsequent byte
	SetSet							= 0x39,
	EndSet							= 0x3A,
	SetMap							= 0x3B,
	EndMap							= 0x3C,
	SetConst						= 0x3D,
	EndSetConst						= 0x3E,
	MapConst						= 0x3F,
	EndMapConst						= 0x40,
	Vector3fConst					= 0x41,	// A float vector constant.
	StructMemberContext				= 0x42, // Context expression to address a property within a struct
	LetMulticastDelegate			= 0x43, // Assignment to a multi-cast delegate
	LetDelegate						= 0x44, // Assignment to a delegate
	LocalVirtualFunction			= 0x45, // Special instructions to quickly call a virtual function that we know is going to run only locally
	LocalFinalFunction				= 0x46, // Special instructions to quickly call a final function that we know is going to run only locally
	//								= 0x47, // CST_ObjectToBool
	LocalOutVariable				= 0x48, // local out (pass by reference) function parameter
	//								= 0x49, // CST_InterfaceToBool
	DeprecatedOp4A					= 0x4A,
	InstanceDelegate				= 0x4B,	// const reference to a delegate or normal function object
	PushExecutionFlow				= 0x4C, // push an address on to the execution flow stack for future execution when a EX_PopExecutionFlow is executed.   Execution continues on normally and doesn't change to the pushed address.
	PopExecutionFlow				= 0x4D, // continue execution at the last address previously pushed onto the execution flow stack.
	ComputedJump					= 0x4E,	// Goto a local address in code, specified by an integer value.
	PopExecutionFlowIfNot			= 0x4F, // continue execution at the last address previously pushed onto the execution flow stack, if the condition is not true.
	Breakpoint						= 0x50, // Breakpoint.  Only observed in the editor, otherwise it behaves like EX_Nothing.
	InterfaceContext				= 0x51,	// Call a function through a native interface variable
	ObjToInterfaceCast				= 0x52,	// Converting an object reference to native interface variable
	EndOfScript						= 0x53, // Last byte in script code
	CrossInterfaceCast				= 0x54, // Converting an interface variable reference to native interface variable
	InterfaceToObjCast				= 0x55, // Converting an interface variable reference to an object

	WireTracepoint					= 0x5A, // Trace point.  Only observed in the editor, otherwise it behaves like EX_Nothing.
	SkipOffsetConst					= 0x5B, // A CodeSizeSkipOffset constant
	AddMulticastDelegate			= 0x5C, // Adds a delegate to a multicast delegate's targets
	ClearMulticastDelegate			= 0x5D, // Clears all delegates in a multicast target
	Tracepoint						= 0x5E, // Trace point.  Only observed in the editor, otherwise it behaves like EX_Nothing.
	LetObj							= 0x5F,	// assign to any object ref pointer
	LetWeakObjPtr					= 0x60, // assign to a weak object pointer
	BindDelegate					= 0x61, // bind object and name to delegate
	RemoveMulticastDelegate			= 0x62, // Remove a delegate from a multicast delegate's targets
	CallMulticastDelegate			= 0x63, // Call multicast delegate
	LetValueOnPersistentFrame		= 0x64,
	ArrayConst						= 0x65,
	EndArrayConst					= 0x66,
	SoftObjectConst					= 0x67,
	CallMath						= 0x68, // static pure function from on local call space
	SwitchValue						= 0x69,
	InstrumentationEvent			= 0x6A, // Instrumentation event
	ArrayGetByRef					= 0x6B,
	ClassSparseDataVariable			= 0x6C, // Sparse data variable
	FieldPathConst					= 0x6D,

	AutoRtfmTransact				= 0x70, // AutoRTFM: run following code in a transaction
	AutoRtfmStopTransact			= 0x71, // AutoRTFM: if in a transaction, abort or break, otherwise no operation
	AutoRtfmAbortIfNot				= 0x72, // AutoRTFM: evaluate bool condition, abort transaction on false
	Max								= 0xFF,
};

enum class EObjectFlags
{
	NoFlags							= 0x00000000,

	Public							= 0x00000001,
	Standalone						= 0x00000002,
	MarkAsNative					= 0x00000004,
	Transactional					= 0x00000008,
	ClassDefaultObject				= 0x00000010,
	ArchetypeObject					= 0x00000020,
	Transient						= 0x00000040,

	MarkAsRootSet					= 0x00000080,
	TagGarbageTemp					= 0x00000100,

	NeedInitialization				= 0x00000200,
	NeedLoad						= 0x00000400,
	KeepForCooker					= 0x00000800,
	NeedPostLoad					= 0x00001000,
	NeedPostLoadSubobjects			= 0x00002000,
	NewerVersionExists				= 0x00004000,
	BeginDestroyed					= 0x00008000,
	FinishDestroyed					= 0x00010000,

	BeingRegenerated				= 0x00020000,
	DefaultSubObject				= 0x00040000,
	WasLoaded						= 0x00080000,
	TextExportTransient				= 0x00100000,
	LoadCompleted					= 0x00200000,
	InheritableComponentTemplate	= 0x00400000,
	DuplicateTransient				= 0x00800000,
	StrongRefOnFrame				= 0x01000000,
	NonPIEDuplicateTransient		= 0x02000000,
	Dynamic							= 0x04000000,
	WillBeLoaded					= 0x08000000, 
};

enum class EFieldClassID : uint64
{
	Int8					= 1llu << 1,
	Byte					= 1llu << 6,
	Int						= 1llu << 7,
	Float					= 1llu << 8,
	UInt64					= 1llu << 9,
	Class					= 1llu << 10,
	UInt32					= 1llu << 11,
	Interface				= 1llu << 12,
	Name					= 1llu << 13,
	String					= 1llu << 14,
	Object					= 1llu << 16,
	Bool					= 1llu << 17,
	UInt16					= 1llu << 18,
	Struct					= 1llu << 20,
	Array					= 1llu << 21,
	Int64					= 1llu << 22,
	Delegate				= 1llu << 23,
	SoftObject				= 1llu << 27,
	LazyObject				= 1llu << 28,
	WeakObject				= 1llu << 29,
	Text					= 1llu << 30,
	Int16					= 1llu << 31,
	Double					= 1llu << 32,
	SoftClass				= 1llu << 33,
	Map						= 1llu << 46,
	Set						= 1llu << 47,
	Enum					= 1llu << 48,
	MulticastInlineDelegate = 1llu << 50,
	MulticastSparseDelegate = 1llu << 51,
	ObjectPointer			= 1llu << 53
};

enum class EClassCastFlags : uint64
{
	None								= 0x0000000000000000,

	Field								= 0x0000000000000001,
	Int8Property						= 0x0000000000000002,
	Enum								= 0x0000000000000004,
	Struct								= 0x0000000000000008,
	ScriptStruct						= 0x0000000000000010,
	Class								= 0x0000000000000020,
	ByteProperty						= 0x0000000000000040,
	IntProperty							= 0x0000000000000080,
	FloatProperty						= 0x0000000000000100,
	UInt64Property						= 0x0000000000000200,
	ClassProperty						= 0x0000000000000400,
	UInt32Property						= 0x0000000000000800,
	InterfaceProperty					= 0x0000000000001000,
	NameProperty						= 0x0000000000002000,
	StrProperty							= 0x0000000000004000,
	Property							= 0x0000000000008000,
	ObjectProperty						= 0x0000000000010000,
	BoolProperty						= 0x0000000000020000,
	UInt16Property						= 0x0000000000040000,
	Function							= 0x0000000000080000,
	StructProperty						= 0x0000000000100000,
	ArrayProperty						= 0x0000000000200000,
	Int64Property						= 0x0000000000400000,
	DelegateProperty					= 0x0000000000800000,
	NumericProperty						= 0x0000000001000000,
	MulticastDelegateProperty			= 0x0000000002000000,
	ObjectPropertyBase					= 0x0000000004000000,
	WeakObjectProperty					= 0x0000000008000000,
	LazyObjectProperty					= 0x0000000010000000,
	SoftObjectProperty					= 0x0000000020000000,
	TextProperty						= 0x0000000040000000,
	Int16Property						= 0x0000000080000000,
	DoubleProperty						= 0x0000000100000000,
	SoftClassProperty					= 0x0000000200000000,
	Package								= 0x0000000400000000,
	Level								= 0x0000000800000000,
	Actor								= 0x0000001000000000,
	PlayerController					= 0x0000002000000000,
	Pawn								= 0x0000004000000000,
	SceneComponent						= 0x0000008000000000,
	PrimitiveComponent					= 0x0000010000000000,
	SkinnedMeshComponent				= 0x0000020000000000,
	SkeletalMeshComponent				= 0x0000040000000000,
	Blueprint							= 0x0000080000000000,
	DelegateFunction					= 0x0000100000000000,
	StaticMeshComponent					= 0x0000200000000000,
	MapProperty							= 0x0000400000000000,
	SetProperty							= 0x0000800000000000,
	EnumProperty						= 0x0001000000000000,
	SparseDelegateFunction				= 0x0002000000000000,
	MulticastInlineDelegateProperty		= 0x0004000000000000,
	MulticastSparseDelegateProperty		= 0x0008000000000000,
	FieldPathProperty					= 0x0010000000000000,
	LargeWorldCoordinatesRealProperty	= 0x0080000000000000,
	OptionalProperty					= 0x0100000000000000,
	VValueProperty						= 0x0200000000000000,
	VerseVMClass						= 0x0400000000000000,
	VRestValueProperty					= 0x0800000000000000,
};

enum class EClassFlags
{
	None						= 0x00000000u,
	Abstract					= 0x00000001u,
	DefaultConfig				= 0x00000002u,
	Config						= 0x00000004u,
	Transient					= 0x00000008u,
	Parsed						= 0x00000010u,
	MatchedSerializers			= 0x00000020u,
	ProjectUserConfig			= 0x00000040u,
	Native						= 0x00000080u,
	NoExport					= 0x00000100u,
	NotPlaceable				= 0x00000200u,
	PerObjectConfig				= 0x00000400u,
	ReplicationDataIsSetUp		= 0x00000800u,
	EditInlineNew				= 0x00001000u,
	CollapseCategories			= 0x00002000u,
	Interface					= 0x00004000u,
	CustomConstructor			= 0x00008000u,
	Const						= 0x00010000u,
	LayoutChanging				= 0x00020000u,
	CompiledFromBlueprint		= 0x00040000u,
	MinimalAPI					= 0x00080000u,
	RequiredAPI					= 0x00100000u,
	DefaultToInstanced			= 0x00200000u,
	TokenStreamAssembled		= 0x00400000u,
	HasInstancedReference		= 0x00800000u,
	Hidden						= 0x01000000u,
	Deprecated					= 0x02000000u,
	HideDropDown				= 0x04000000u,
	GlobalUserConfig			= 0x08000000u,
	Intrinsic					= 0x10000000u,
	Constructed					= 0x20000000u,
	ConfigDoNotCheckDefaults	= 0x40000000u,
	NewerVersionExists			= 0x80000000u,
};

// Script instrumentation event types
enum class EScriptInstrumentation : uint8
{
	Class = 0,
	ClassScope,
	Instance,
	Event,
	InlineEvent,
	ResumeEvent,
	PureNodeEntry,
	NodeDebugSite,
	NodeEntry,
	NodeExit,
	PushState,
	RestoreState,
	ResetState,
	SuspendState,
	PopState,
	TunnelEndOfThread,
	Stop
};

enum class EBlueprintTextLiteralType : uint8
{
	/* Text is an empty string. The bytecode contains no strings, and you should use FText::GetEmpty() to initialize the FText instance. */
	Empty,
	/** Text is localized. The bytecode will contain three strings - source, key, and namespace - and should be loaded via FInternationalization */
	LocalizedText,
	/** Text is culture invariant. The bytecode will contain one string, and you should use FText::AsCultureInvariant to initialize the FText instance. */
	InvariantText,
	/** Text is a literal FString. The bytecode will contain one string, and you should use FText::FromString to initialize the FText instance. */
	LiteralString,
	/** Text is from a string table. The bytecode will contain an object pointer (not used) and two strings - the table ID, and key - and should be found via FText::FromStringTable */
	StringTableEntry,
};

enum class EMappingsTypeFlags : uint8
{
	ByteProperty,
	BoolProperty,
	IntProperty,
	FloatProperty,
	ObjectProperty,
	NameProperty,
	DelegateProperty,
	DoubleProperty,
	ArrayProperty,
	StructProperty,
	StrProperty,
	TextProperty,
	InterfaceProperty,
	MulticastDelegateProperty,
	WeakObjectProperty, //
	LazyObjectProperty, // When deserialized, these 3 properties will be SoftObjects
	AssetObjectProperty, //
	SoftObjectProperty,
	UInt64Property,
	UInt32Property,
	UInt16Property,
	Int64Property,
	Int16Property,
	Int8Property,
	MapProperty,
	SetProperty,
	EnumProperty,
	FieldPathProperty,
	OptionalProperty,

	Unknown = 0xFF
};


ENUM_OPERATORS(EObjectFlags);
ENUM_OPERATORS(EFunctionFlags);
ENUM_OPERATORS(EPropertyFlags);
ENUM_OPERATORS(EClassCastFlags);
ENUM_OPERATORS(EClassFlags);
ENUM_OPERATORS(EMappingsTypeFlags);
ENUM_OPERATORS(EFieldClassID);

static std::string StringifyFunctionFlags(EFunctionFlags FunctionFlags, const char* Seperator = ", ")
{
	/* Make sure the size of this vector is always greater, or equal, to the number of flags existing */
	std::array<const char*, 0x30> StringFlags;
	int32 CurrentIdx = 0x0;

	if (FunctionFlags & EFunctionFlags::Final) { StringFlags[CurrentIdx++] = "Final"; }
	if (FunctionFlags & EFunctionFlags::RequiredAPI) { StringFlags[CurrentIdx++] = "RequiredAPI"; }
	if (FunctionFlags & EFunctionFlags::BlueprintAuthorityOnly) { StringFlags[CurrentIdx++] = "BlueprintAuthorityOnly"; }
	if (FunctionFlags & EFunctionFlags::BlueprintCosmetic) { StringFlags[CurrentIdx++] = "BlueprintCosmetic"; }
	if (FunctionFlags & EFunctionFlags::Net) { StringFlags[CurrentIdx++] = "Net"; }
	if (FunctionFlags & EFunctionFlags::NetReliable) { StringFlags[CurrentIdx++] = "NetReliable"; }
	if (FunctionFlags & EFunctionFlags::NetRequest) { StringFlags[CurrentIdx++] = "NetRequest"; }
	if (FunctionFlags & EFunctionFlags::Exec) { StringFlags[CurrentIdx++] = "Exec"; }
	if (FunctionFlags & EFunctionFlags::Native) { StringFlags[CurrentIdx++] =  "Native"; }
	if (FunctionFlags & EFunctionFlags::Event) { StringFlags[CurrentIdx++] = "Event"; }
	if (FunctionFlags & EFunctionFlags::NetResponse) { StringFlags[CurrentIdx++] = "NetResponse"; }
	if (FunctionFlags & EFunctionFlags::Static) { StringFlags[CurrentIdx++] = "Static"; }
	if (FunctionFlags & EFunctionFlags::NetMulticast) { StringFlags[CurrentIdx++] = "NetMulticast"; }
	if (FunctionFlags & EFunctionFlags::UbergraphFunction) { StringFlags[CurrentIdx++] =  "UbergraphFunction"; }
	if (FunctionFlags & EFunctionFlags::MulticastDelegate) { StringFlags[CurrentIdx++] =  "MulticastDelegate"; }
	if (FunctionFlags & EFunctionFlags::Public) { StringFlags[CurrentIdx++] = "Public"; }
	if (FunctionFlags & EFunctionFlags::Private) { StringFlags[CurrentIdx++] = "Private"; }
	if (FunctionFlags & EFunctionFlags::Protected) { StringFlags[CurrentIdx++] = "Protected"; }
	if (FunctionFlags & EFunctionFlags::Delegate) { StringFlags[CurrentIdx++] = "Delegate"; }
	if (FunctionFlags & EFunctionFlags::NetServer) { StringFlags[CurrentIdx++] = "NetServer"; }
	if (FunctionFlags & EFunctionFlags::HasOutParms) { StringFlags[CurrentIdx++] =  "HasOutParams"; }
	if (FunctionFlags & EFunctionFlags::HasDefaults) { StringFlags[CurrentIdx++] =  "HasDefaults"; }
	if (FunctionFlags & EFunctionFlags::NetClient) { StringFlags[CurrentIdx++] = "NetClient"; }
	if (FunctionFlags & EFunctionFlags::DLLImport) { StringFlags[CurrentIdx++] = "DLLImport"; }
	if (FunctionFlags & EFunctionFlags::BlueprintCallable) { StringFlags[CurrentIdx++] = "BlueprintCallable"; }
	if (FunctionFlags & EFunctionFlags::BlueprintEvent) { StringFlags[CurrentIdx++] = "BlueprintEvent"; }
	if (FunctionFlags & EFunctionFlags::BlueprintPure) { StringFlags[CurrentIdx++] = "BlueprintPure"; }
	if (FunctionFlags & EFunctionFlags::EditorOnly) { StringFlags[CurrentIdx++] = "EditorOnly"; }
	if (FunctionFlags & EFunctionFlags::Const) { StringFlags[CurrentIdx++] = "Const"; }
	if (FunctionFlags & EFunctionFlags::NetValidate) { StringFlags[CurrentIdx++] = "NetValidate"; }

	std::string RetFlags;
	RetFlags.reserve(CurrentIdx * 0xF);

	for (int i = 0; i < CurrentIdx; i++)
	{
		RetFlags += StringFlags[i];

		if (i != (CurrentIdx - 1))
			RetFlags += Seperator;
	}

	return RetFlags;
}

static std::string StringifyPropertyFlags(EPropertyFlags PropertyFlags)
{
	std::string RetFlags;

	if (PropertyFlags & EPropertyFlags::Edit) { RetFlags += "Edit, "; }
	if (PropertyFlags & EPropertyFlags::ConstParm) { RetFlags += "ConstParm, "; }
	if (PropertyFlags & EPropertyFlags::BlueprintVisible) { RetFlags += "BlueprintVisible, "; }
	if (PropertyFlags & EPropertyFlags::ExportObject) { RetFlags += "ExportObject, "; }
	if (PropertyFlags & EPropertyFlags::BlueprintReadOnly) { RetFlags += "BlueprintReadOnly, "; }
	if (PropertyFlags & EPropertyFlags::Net) { RetFlags += "Net, "; }
	if (PropertyFlags & EPropertyFlags::EditFixedSize) { RetFlags += "EditFixedSize, "; }
	if (PropertyFlags & EPropertyFlags::Parm) { RetFlags += "Parm, "; }
	if (PropertyFlags & EPropertyFlags::OutParm) { RetFlags += "OutParm, "; }
	if (PropertyFlags & EPropertyFlags::ZeroConstructor) { RetFlags += "ZeroConstructor, "; }
	if (PropertyFlags & EPropertyFlags::ReturnParm) { RetFlags += "ReturnParm, "; }
	if (PropertyFlags & EPropertyFlags::DisableEditOnTemplate) { RetFlags += "DisableEditOnTemplate, "; }
	if (PropertyFlags & EPropertyFlags::Transient) { RetFlags += "Transient, "; }
	if (PropertyFlags & EPropertyFlags::Config) { RetFlags += "Config, "; }
	if (PropertyFlags & EPropertyFlags::DisableEditOnInstance) { RetFlags += "DisableEditOnInstance, "; }
	if (PropertyFlags & EPropertyFlags::EditConst) { RetFlags += "EditConst, "; }
	if (PropertyFlags & EPropertyFlags::GlobalConfig) { RetFlags += "GlobalConfig, "; }
	if (PropertyFlags & EPropertyFlags::InstancedReference) { RetFlags += "InstancedReference, "; }
	if (PropertyFlags & EPropertyFlags::DuplicateTransient) { RetFlags += "DuplicateTransient, "; }
	if (PropertyFlags & EPropertyFlags::SubobjectReference) { RetFlags += "SubobjectReference, "; }
	if (PropertyFlags & EPropertyFlags::SaveGame) { RetFlags += "SaveGame, "; }
	if (PropertyFlags & EPropertyFlags::NoClear) { RetFlags += "NoClear, "; }
	if (PropertyFlags & EPropertyFlags::ReferenceParm) { RetFlags += "ReferenceParm, "; }
	if (PropertyFlags & EPropertyFlags::BlueprintAssignable) { RetFlags += "BlueprintAssignable, "; }
	if (PropertyFlags & EPropertyFlags::Deprecated) { RetFlags += "Deprecated, "; }
	if (PropertyFlags & EPropertyFlags::IsPlainOldData) { RetFlags += "IsPlainOldData, "; }
	if (PropertyFlags & EPropertyFlags::RepSkip) { RetFlags += "RepSkip, "; }
	if (PropertyFlags & EPropertyFlags::RepNotify) { RetFlags += "RepNotify, "; }
	if (PropertyFlags & EPropertyFlags::Interp) { RetFlags += "Interp, "; }
	if (PropertyFlags & EPropertyFlags::NonTransactional) { RetFlags += "NonTransactional, "; }
	if (PropertyFlags & EPropertyFlags::EditorOnly) { RetFlags += "EditorOnly, "; }
	if (PropertyFlags & EPropertyFlags::NoDestructor) { RetFlags += "NoDestructor, "; }
	if (PropertyFlags & EPropertyFlags::AutoWeak) { RetFlags += "AutoWeak, "; }
	if (PropertyFlags & EPropertyFlags::ContainsInstancedReference) { RetFlags += "ContainsInstancedReference, "; }
	if (PropertyFlags & EPropertyFlags::AssetRegistrySearchable) { RetFlags += "AssetRegistrySearchable, "; }
	if (PropertyFlags & EPropertyFlags::SimpleDisplay) { RetFlags += "SimpleDisplay, "; }
	if (PropertyFlags & EPropertyFlags::AdvancedDisplay) { RetFlags += "AdvancedDisplay, "; }
	if (PropertyFlags & EPropertyFlags::Protected) { RetFlags += "Protected, "; }
	if (PropertyFlags & EPropertyFlags::BlueprintCallable) { RetFlags += "BlueprintCallable, "; }
	if (PropertyFlags & EPropertyFlags::BlueprintAuthorityOnly) { RetFlags += "BlueprintAuthorityOnly, "; }
	if (PropertyFlags & EPropertyFlags::TextExportTransient) { RetFlags += "TextExportTransient, "; }
	if (PropertyFlags & EPropertyFlags::NonPIEDuplicateTransient) { RetFlags += "NonPIEDuplicateTransient, "; }
	if (PropertyFlags & EPropertyFlags::ExposeOnSpawn) { RetFlags += "ExposeOnSpawn, "; }
	if (PropertyFlags & EPropertyFlags::PersistentInstance) { RetFlags += "PersistentInstance, "; }
	if (PropertyFlags & EPropertyFlags::UObjectWrapper) { RetFlags += "UObjectWrapper, "; }
	if (PropertyFlags & EPropertyFlags::HasGetValueTypeHash) { RetFlags += "HasGetValueTypeHash, "; }
	if (PropertyFlags & EPropertyFlags::NativeAccessSpecifierPublic) { RetFlags += "NativeAccessSpecifierPublic, "; }
	if (PropertyFlags & EPropertyFlags::NativeAccessSpecifierProtected) { RetFlags += "NativeAccessSpecifierProtected, "; }
	if (PropertyFlags & EPropertyFlags::NativeAccessSpecifierPrivate) { RetFlags += "NativeAccessSpecifierPrivate, "; }

	return RetFlags.size() > 2 ? RetFlags.erase(RetFlags.size() - 2) : RetFlags;
}

static std::string StringifyObjectFlags(EObjectFlags ObjFlags)
{
	std::string RetFlags;

	if (ObjFlags & EObjectFlags::Public) { RetFlags += "Public, "; }
	if (ObjFlags & EObjectFlags::Standalone) { RetFlags += "Standalone, "; }
	if (ObjFlags & EObjectFlags::MarkAsNative) { RetFlags += "MarkAsNative, "; }
	if (ObjFlags & EObjectFlags::Transactional) { RetFlags += "Transactional, "; }
	if (ObjFlags & EObjectFlags::ClassDefaultObject) { RetFlags += "ClassDefaultObject, "; }
	if (ObjFlags & EObjectFlags::ArchetypeObject) { RetFlags += "ArchetypeObject, "; }
	if (ObjFlags & EObjectFlags::Transient) { RetFlags += "Transient, "; }
	if (ObjFlags & EObjectFlags::MarkAsRootSet) { RetFlags += "MarkAsRootSet, "; }
	if (ObjFlags & EObjectFlags::TagGarbageTemp) { RetFlags += "TagGarbageTemp, "; }
	if (ObjFlags & EObjectFlags::NeedInitialization) { RetFlags += "NeedInitialization, "; }
	if (ObjFlags & EObjectFlags::NeedLoad) { RetFlags += "NeedLoad, "; }
	if (ObjFlags & EObjectFlags::KeepForCooker) { RetFlags += "KeepForCooker, "; }
	if (ObjFlags & EObjectFlags::NeedPostLoad) { RetFlags += "NeedPostLoad, "; }
	if (ObjFlags & EObjectFlags::NeedPostLoadSubobjects) { RetFlags += "NeedPostLoadSubobjects, "; }
	if (ObjFlags & EObjectFlags::NewerVersionExists) { RetFlags += "NewerVersionExists, "; }
	if (ObjFlags & EObjectFlags::BeginDestroyed) { RetFlags += "BeginDestroyed, "; }
	if (ObjFlags & EObjectFlags::FinishDestroyed) { RetFlags += "FinishDestroyed, "; }
	if (ObjFlags & EObjectFlags::BeingRegenerated) { RetFlags += "BeingRegenerated, "; }
	if (ObjFlags & EObjectFlags::DefaultSubObject) { RetFlags += "DefaultSubObject, "; }
	if (ObjFlags & EObjectFlags::WasLoaded) { RetFlags += "WasLoaded, "; }
	if (ObjFlags & EObjectFlags::TextExportTransient) { RetFlags += "TextExportTransient, "; }
	if (ObjFlags & EObjectFlags::LoadCompleted) { RetFlags += "LoadCompleted, "; }
	if (ObjFlags & EObjectFlags::InheritableComponentTemplate) { RetFlags += "InheritableComponentTemplate, "; }
	if (ObjFlags & EObjectFlags::DuplicateTransient) { RetFlags += "DuplicateTransient, "; }
	if (ObjFlags & EObjectFlags::StrongRefOnFrame) { RetFlags += "StrongRefOnFrame, "; }
	if (ObjFlags & EObjectFlags::NonPIEDuplicateTransient) { RetFlags += "NonPIEDuplicateTransient, "; }
	if (ObjFlags & EObjectFlags::Dynamic) { RetFlags += "Dynamic, "; }
	if (ObjFlags & EObjectFlags::WillBeLoaded) { RetFlags += "WillBeLoaded, "; }

	return RetFlags.size() > 2 ? RetFlags.erase(RetFlags.size() - 2) : RetFlags;
}

static std::string StringifyClassCastFlags(EClassCastFlags CastFlags)
{
	std::string RetFlags;

	if (CastFlags & EClassCastFlags::Field) { RetFlags += "Field, "; }
	if (CastFlags & EClassCastFlags::Int8Property) { RetFlags += "Int8Property, "; }
	if (CastFlags & EClassCastFlags::Enum) { RetFlags += "Enum, "; }
	if (CastFlags & EClassCastFlags::Struct) { RetFlags += "Struct, "; }
	if (CastFlags & EClassCastFlags::ScriptStruct) { RetFlags += "ScriptStruct, "; }
	if (CastFlags & EClassCastFlags::Class) { RetFlags += "Class, "; }
	if (CastFlags & EClassCastFlags::ByteProperty) { RetFlags += "ByteProperty, "; }
	if (CastFlags & EClassCastFlags::IntProperty) { RetFlags += "IntProperty, "; }
	if (CastFlags & EClassCastFlags::FloatProperty) { RetFlags += "FloatProperty, "; }
	if (CastFlags & EClassCastFlags::UInt64Property) { RetFlags += "UInt64Property, "; }
	if (CastFlags & EClassCastFlags::ClassProperty) { RetFlags += "ClassProperty, "; }
	if (CastFlags & EClassCastFlags::UInt32Property) { RetFlags += "UInt32Property, "; }
	if (CastFlags & EClassCastFlags::InterfaceProperty) { RetFlags += "InterfaceProperty, "; }
	if (CastFlags & EClassCastFlags::NameProperty) { RetFlags += "NameProperty, "; }
	if (CastFlags & EClassCastFlags::StrProperty) { RetFlags += "StrProperty, "; }
	if (CastFlags & EClassCastFlags::Property) { RetFlags += "Property, "; }
	if (CastFlags & EClassCastFlags::ObjectProperty) { RetFlags += "ObjectProperty, "; }
	if (CastFlags & EClassCastFlags::BoolProperty) { RetFlags += "BoolProperty, "; }
	if (CastFlags & EClassCastFlags::UInt16Property) { RetFlags += "UInt16Property, "; }
	if (CastFlags & EClassCastFlags::Function) { RetFlags += "Function, "; }
	if (CastFlags & EClassCastFlags::StructProperty) { RetFlags += "StructProperty, "; }
	if (CastFlags & EClassCastFlags::ArrayProperty) { RetFlags += "ArrayProperty, "; }
	if (CastFlags & EClassCastFlags::Int64Property) { RetFlags += "Int64Property, "; }
	if (CastFlags & EClassCastFlags::DelegateProperty) { RetFlags += "DelegateProperty, "; }
	if (CastFlags & EClassCastFlags::NumericProperty) { RetFlags += "NumericProperty, "; }
	if (CastFlags & EClassCastFlags::MulticastDelegateProperty) { RetFlags += "MulticastDelegateProperty, "; }
	if (CastFlags & EClassCastFlags::ObjectPropertyBase) { RetFlags += "ObjectPropertyBase, "; }
	if (CastFlags & EClassCastFlags::WeakObjectProperty) { RetFlags += "WeakObjectProperty, "; }
	if (CastFlags & EClassCastFlags::LazyObjectProperty) { RetFlags += "LazyObjectProperty, "; }
	if (CastFlags & EClassCastFlags::SoftObjectProperty) { RetFlags += "SoftObjectProperty, "; }
	if (CastFlags & EClassCastFlags::TextProperty) { RetFlags += "TextProperty, "; }
	if (CastFlags & EClassCastFlags::Int16Property) { RetFlags += "Int16Property, "; }
	if (CastFlags & EClassCastFlags::DoubleProperty) { RetFlags += "DoubleProperty, "; }
	if (CastFlags & EClassCastFlags::SoftClassProperty) { RetFlags += "SoftClassProperty, "; }
	if (CastFlags & EClassCastFlags::Package) { RetFlags += "Package, "; }
	if (CastFlags & EClassCastFlags::Level) { RetFlags += "Level, "; }
	if (CastFlags & EClassCastFlags::Actor) { RetFlags += "Actor, "; }
	if (CastFlags & EClassCastFlags::PlayerController) { RetFlags += "PlayerController, "; }
	if (CastFlags & EClassCastFlags::Pawn) { RetFlags += "Pawn, "; }
	if (CastFlags & EClassCastFlags::SceneComponent) { RetFlags += "SceneComponent, "; }
	if (CastFlags & EClassCastFlags::PrimitiveComponent) { RetFlags += "PrimitiveComponent, "; }
	if (CastFlags & EClassCastFlags::SkinnedMeshComponent) { RetFlags += "SkinnedMeshComponent, "; }
	if (CastFlags & EClassCastFlags::SkeletalMeshComponent) { RetFlags += "SkeletalMeshComponent, "; }
	if (CastFlags & EClassCastFlags::Blueprint) { RetFlags += "Blueprint, "; }
	if (CastFlags & EClassCastFlags::DelegateFunction) { RetFlags += "DelegateFunction, "; }
	if (CastFlags & EClassCastFlags::StaticMeshComponent) { RetFlags += "StaticMeshComponent, "; }
	if (CastFlags & EClassCastFlags::MapProperty) { RetFlags += "MapProperty, "; }
	if (CastFlags & EClassCastFlags::SetProperty) { RetFlags += "SetProperty, "; }
	if (CastFlags & EClassCastFlags::EnumProperty) { RetFlags += "EnumProperty, "; }
	if (CastFlags & EClassCastFlags::SparseDelegateFunction) { RetFlags += "SparseDelegateFunction, "; }
	if (CastFlags & EClassCastFlags::MulticastInlineDelegateProperty) { RetFlags += "MulticastInlineDelegateProperty, "; }
	if (CastFlags & EClassCastFlags::MulticastSparseDelegateProperty) { RetFlags += "MulticastSparseDelegateProperty, "; }
	if (CastFlags & EClassCastFlags::FieldPathProperty) { RetFlags += "MarkAsFieldPathPropertyRootSet, "; }
	if (CastFlags & EClassCastFlags::LargeWorldCoordinatesRealProperty) { RetFlags += "LargeWorldCoordinatesRealProperty, "; }
	if (CastFlags & EClassCastFlags::OptionalProperty) { RetFlags += "OptionalProperty, "; }
	if (CastFlags & EClassCastFlags::VValueProperty) { RetFlags += "VValueProperty, "; }
	if (CastFlags & EClassCastFlags::VerseVMClass) { RetFlags += "VerseVMClass, "; }
	if (CastFlags & EClassCastFlags::VRestValueProperty) { RetFlags += "VRestValueProperty, "; }

	return RetFlags.size() > 2 ? RetFlags.erase(RetFlags.size() - 2) : RetFlags;
}
