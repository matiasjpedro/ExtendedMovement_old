#pragma once

#include "Serialization/Archive.h"
#include "EXMContractTypes.generated.h"


USTRUCT()
struct FPredictionModifiers
{
	GENERATED_BODY()

	virtual ~FPredictionModifiers() {}
	virtual void SerializeData(FArchive& Ar, FPredictionModifiers* Data) const {}
};

USTRUCT()
struct FMovementStatsModifiers : public FPredictionModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float SpeedModifier;

	UPROPERTY(EditDefaultsOnly)
	float AccelerationModifier;

	FMovementStatsModifiers() {}

	inline void GetDelta(float CurrentSpeedModifier, float CurrentAccelerationModifier, FMovementStatsModifiers& OutDelta) const
	{
		OutDelta.SpeedModifier = SpeedModifier - CurrentSpeedModifier;
		OutDelta.AccelerationModifier = AccelerationModifier - CurrentAccelerationModifier;
	}

	virtual void SerializeData(FArchive& Ar, FPredictionModifiers* Data) const override
	{
		FMovementStatsModifiers::StaticStruct()->SerializeBin(Ar, Data);
	}

};

USTRUCT()
struct FImpulseModifier : public FPredictionModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	int32 ImpulseMagnitud;


	UPROPERTY()
	FVector ImpulseDirection;

	FImpulseModifier() {}

	virtual void SerializeData(FArchive& Ar, FPredictionModifiers* Data) const override
	{
		FImpulseModifier::StaticStruct()->SerializeBin(Ar, Data);
	}

};

USTRUCT()
struct FWeaponStatsModifiers : public FPredictionModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float FireRate;

	FWeaponStatsModifiers(){}

	inline void GetDelta(float CurrentFireRate, FWeaponStatsModifiers& OutDelta) const
	{
		OutDelta.FireRate = FireRate - CurrentFireRate;
	}

	virtual void SerializeData(FArchive& Ar, FPredictionModifiers* Data) const override
	{
		FWeaponStatsModifiers::StaticStruct()->SerializeBin(Ar, Data);
	}

};

USTRUCT()
struct FWeaponSwitchModifiers : public FPredictionModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	uint8 SocketIndex;

	FWeaponSwitchModifiers(){}

	FWeaponSwitchModifiers(uint8 InSocketIndex) : SocketIndex(InSocketIndex){}

	inline void GetDelta(uint8 CurrentSocket, FWeaponSwitchModifiers& OutDelta) const
	{
		OutDelta.SocketIndex = SocketIndex - CurrentSocket;
	}

	virtual void SerializeData(FArchive& Ar, FPredictionModifiers* Data) const override
	{
		FWeaponSwitchModifiers::StaticStruct()->SerializeBin(Ar, Data);
	}

};


UENUM()
enum class EModifiersSources : uint8
{
	ModifiersInInstigator,
	ModifiersInClass,
	ModifiersInRequest
};

const static uint8 InvalidModifierID = 255;
const static float InvalidDuration = -1;


USTRUCT()
struct FPredictionContractRequest
{
	GENERATED_BODY()

public:

	UPROPERTY()
	uint8 ModifierId;

	UPROPERTY()
	float Duration;

	UPROPERTY()
	class UObject* Instigator;

	UPROPERTY()
	class UClass* InstigatorClass;

	UPROPERTY()
	TArray<uint8> SerializedData;

	FPredictionContractRequest(){}

	FPredictionContractRequest(class UObject* InInstigator, uint8 InModifierId, float InDuration) :
		ModifierId(InModifierId),
		Duration(InDuration),
		Instigator(InInstigator),
		InstigatorClass(nullptr)
	{

	}

	FPredictionContractRequest(class UClass* InInstigatorClass, uint8 InModifierId, float InDuration) : 
		ModifierId(InModifierId),
		Duration(InDuration),
		Instigator(nullptr),
		InstigatorClass(InInstigatorClass)
	{

	}

	FPredictionContractRequest(class UClass* InInstigatorClass, uint8 InModifierId, float InDuration, const TArray<uint8>& InSerializedData) :
		ModifierId(InModifierId),
		Duration(InDuration),
		Instigator(nullptr),
		InstigatorClass(InInstigatorClass),
		SerializedData(InSerializedData)
	{

	}

	bool IsModifierSources(EModifiersSources ModifierSourcesRequest) const
	{
		switch (ModifierSourcesRequest)
		{
		case EModifiersSources::ModifiersInInstigator:
			return Instigator != nullptr;
			break;
		case EModifiersSources::ModifiersInClass:
			return InstigatorClass && SerializedData.Num() == 0;
			break;
		case EModifiersSources::ModifiersInRequest:
			return InstigatorClass && SerializedData.Num() != 0;
			break;
		default:
			return false;
			break;
		}

		return false;
	}

	/* Here we declare which fields of the contract will be sent over the network depending on the ModifierSources field */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPredictionContractRequest> : public TStructOpsTypeTraitsBase2<FPredictionContractRequest>
{
	enum
	{
		WithNetSerializer = true
	};
};

