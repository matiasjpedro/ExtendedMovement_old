#pragma once

#include "Classes/Engine/NetSerialization.h"
#include "Classes/Components/PrimitiveComponent.h"
#include "EXMTypes.generated.h"

#define TEST_BIT(Bitmask, Bit) (((Bitmask) & (1 << static_cast<uint32>(Bit))) > 0)
#define SET_BIT(Bitmask, Bit) (Bitmask |= 1 << static_cast<uint32>(Bit))
#define CLEAR_BIT(Bitmask, Bit) (Bitmask &= ~(1 << static_cast<uint32>(Bit)))

UENUM(BlueprintType, meta = (Bitflags))
enum class EDynamicFlags : uint8
{
	WithBase,
	WithRoll,
	Custom0,
	Custom1,
	Custom2,
	Custom3,
	Custom4,
	Custom5
};

USTRUCT()
struct FLegacyBaseServerMove
{
	GENERATED_BODY()

	FLegacyBaseServerMove(){}
	virtual ~FLegacyBaseServerMove() {}

	virtual void CustomSerialize(FArchive& Ar, uint8 DynamicFlags) {}
};

USTRUCT()
struct FLegacyServerMove : public FLegacyBaseServerMove
{
	GENERATED_BODY()

	float TimeStamp;
	FVector_NetQuantize10 Accel;
	FVector_NetQuantize100 ClientLoc;
	uint8 CompressedMoveFlags;
	uint8 ClientRoll = 0;
	uint32 View;
	UPrimitiveComponent* ClientMovementBase = nullptr;
	FName ClientBaseBoneName = NAME_None;
	uint8 ClientMovementMode;


	FLegacyServerMove(){}

	FLegacyServerMove(float InTimeStamp, FVector_NetQuantize10 InAccel, FVector_NetQuantize100 InClientLoc, uint8 InCompressedMoveFlags, uint8 InClientRoll, uint32 InView, UPrimitiveComponent* InClientMovementBase, FName InClientBaseBoneName,uint8 InClientMovementMode) :
		TimeStamp(InTimeStamp),
		Accel(InAccel),
		ClientLoc(InClientLoc),
		CompressedMoveFlags(InCompressedMoveFlags),
		ClientRoll(InClientRoll),
		View(InView),
		ClientMovementBase(InClientMovementBase),
		ClientBaseBoneName(InClientBaseBoneName),
		ClientMovementMode(InClientMovementMode)
	{

	}

	void CustomSerialize(FArchive& Ar, uint8 DynamicFlags) override
	{
		bool Temp;

		Ar << TimeStamp;
		Accel.NetSerialize(Ar, nullptr, Temp);
		ClientLoc.NetSerialize(Ar, nullptr, Temp);
		Ar << CompressedMoveFlags;
		Ar << ClientRoll;
		Ar << View;

		if (TEST_BIT(DynamicFlags, EDynamicFlags::WithBase))
		{
			Ar << ClientMovementBase;
			Ar << ClientBaseBoneName;
		}

		Ar << ClientMovementMode;
	}
};

USTRUCT()
struct FLegacyServerMoveDual : public FLegacyBaseServerMove
{
	GENERATED_BODY()

	float TimeStamp0;
	FVector_NetQuantize10 Accel0;
	uint8 PendingFlags;
	uint32 View0;
	float TimeStamp;
	FVector_NetQuantize10 Accel;
	FVector_NetQuantize100 ClientLoc;
	uint8 NewFlags;
	uint8 ClientRoll = 0;
	uint32 View;
	UPrimitiveComponent* ClientMovementBase = nullptr;
	FName ClientBaseBoneName = NAME_None;
	uint8 ClientMovementMode;

	FLegacyServerMoveDual(){}

	FLegacyServerMoveDual(float InTimeStamp0, FVector_NetQuantize10 InAccel0, uint8 InPendingFlags, uint32 InView0, float InTimeStamp, FVector_NetQuantize10 InAccel, FVector_NetQuantize100 InClientLoc, uint8 InNewFlags, uint8 InClientRoll, uint32 InView, UPrimitiveComponent* InClientMovementBase, FName InClientBaseBoneName, uint8 InClientMovementMode) :
		TimeStamp0(InTimeStamp0),
		Accel0(InAccel0),
		PendingFlags(InPendingFlags),
		View0(InView0),
		TimeStamp(InTimeStamp),
		Accel(InAccel),
		ClientLoc(InClientLoc),
		NewFlags(InNewFlags),
		ClientRoll(InClientRoll),
		View(InView),
		ClientMovementBase(InClientMovementBase),
		ClientBaseBoneName(InClientBaseBoneName),
		ClientMovementMode(InClientMovementMode)
	{

	}

	void CustomSerialize(FArchive& Ar, uint8 DynamicFlags) override
	{
		bool Temp;

		//TODO: DeltaCompress.

		Ar << TimeStamp0;
		Accel0.NetSerialize(Ar, nullptr, Temp);
		Ar << PendingFlags;
		Ar << View0;
		Ar << TimeStamp;
		Accel.NetSerialize(Ar, nullptr, Temp);
		ClientLoc.NetSerialize(Ar, nullptr, Temp);
		Ar << NewFlags;
		Ar << ClientRoll;
		Ar << View;

		if (TEST_BIT(DynamicFlags, EDynamicFlags::WithBase))
		{
			Ar << ClientMovementBase;
			Ar << ClientBaseBoneName;
		}

		Ar << ClientMovementMode;
	}
};

USTRUCT()
struct FLegacyServerMoveOld : public FLegacyBaseServerMove
{
	GENERATED_BODY()

	float TimeStamp;
	FVector_NetQuantize10 Accel;
	uint8 MoveFlags;

	FLegacyServerMoveOld(){}

	FLegacyServerMoveOld(float InTimeStamp, FVector_NetQuantize10 InAccel, uint8 InMoveFlags) :
		TimeStamp(InTimeStamp),
		Accel(InAccel),
		MoveFlags(InMoveFlags)
	{

	}

	void CustomSerialize(FArchive& Ar, uint8 DynamicFlags) override
	{
		bool Temp = false;

		Ar << TimeStamp;
		Accel.NetSerialize(Ar, nullptr, Temp);
		Ar << MoveFlags;
	}
};

/**
 * FNetBitWriter
 *	A bit writer that serializes FNames and UObject* through
 *	a network packagemap.
 */
class FEXMNetBitWriter : public FNetBitWriter
{
public:

	FEXMNetBitWriter(){}

	FEXMNetBitWriter(UPackageMap * InPackageMap, int64 InMaxBits) :
		FNetBitWriter(InPackageMap, InMaxBits)
	{

	}

	const TArray<uint8>& GetPackedBuffer()
	{
		 (*(const_cast<TArray<uint8>*>(GetBuffer()))).SetNum(GetNumBytes());

		 return *GetBuffer();
	}
};

