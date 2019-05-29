// Fill out your copyright notice in the Description page of Project Settings.

//TODO: Extend Reconciliation.
//TODO: SendClientAdjustment == CallServerMove
//TODO: Inheritence from FNetworkPredictionData_Server_Character and add it the struct payload and the flags to deserialize.
//TODO: ServerMoveHandleClientError I should re readthe buffer  sended by the client to compare.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EXMCharacterMovementComponent.generated.h"

UENUM()
enum class ESerializationMode : uint8
{
	Dynamic,
	Extended,
	Legacy
};

UENUM()
enum class EMoveType : uint8
{
	Single,
	Dual,
	Old
};

/**
 * 
 */
UCLASS()
class EXTENDEDMOVEMENT_API UEXMCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	
protected:

	UPROPERTY(EditDefaultsOnly)
	ESerializationMode SerializationMode;

	virtual void CallServerMove(const class FSavedMove_Character* NewMove, const class FSavedMove_Character* OldMove) override;

	/* Here we should add the replication flags that is going to be consider when the serialization runs. */
	virtual void FillDynamicFlagsForMove(uint8& OutDynamicFlags, EMoveType MoveType, const class FSavedMove_Character* Move);

	/* Here we should serialize taking in consideration the flags previously set. */
	virtual void SerializeExtraData(FArchive& Ar, uint8 OutDynamicFlags, EMoveType MoveType, const class FSavedMove_Character* Move = nullptr);

	inline bool ShouldUseLegacyMovement(uint8 DynamicFlags)
	{
		if (SerializationMode == ESerializationMode::Legacy)
			return true;

		if (SerializationMode == ESerializationMode::Dynamic && DynamicFlags == 0)
			return true;
		
		return false;
	}

public:

	void ResolveServerMove(const TArray<uint8>& InData, EMoveType MoveType);

	/** This should be on Check Jump input before execute the movement. */
	virtual void PerformActionsBeforeMove(float DeltaTime) {}
};
