// Fill out your copyright notice in the Description page of Project Settings.

//TODO: Code Tool to debug the flow.
//TODO: ExtendedMovement Reconciliation need a method when the client is going to start the reconciliation.
//TODO: NetDelta in the contracts array.
//TODO: Modifier without instigator?

//TODO: ApplyContractAction should have an additional which should be a FPredictionModifierPtr* that it should be used to execute the contract operation.
//TODO: ContractInstigator should have a GetModifierTypeBy Modifier ID that return the proper type of the prediction modifier that is going to deserialize.

#pragma once

#include "CoreMinimal.h"
#include "EXMContract.h"
#include "TimerManager.h"
#include "Core/EXMCharacterMovementComponent.h"
#include "EXMContractMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class EXTENDEDMOVEMENT_API UEXMContractMovementComponent : public UEXMCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FEXMSavedMove_Movement;

public:

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPredictionContract(const FPredictionContractRequest& Request);

	//UFUNCTION(Server, Reliable, WithValidation)
	//void Server_RequestPredictionContractByInstigator(uint8 ModifierID, UObject* Instigator, float Duration);

	//UFUNCTION(Server, Reliable, WithValidation)
	//void Server_RequestPredictionContractByClass(uint8 ModifierID, UClass* InstigatorClass, float Duration);

	void SetupNewContract(FPredictionContract& OutContract, IEXMContractInstigator* Instigator, uint8 ModifierID, float Duration);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RemovePredictionContract(int32 ContractID);

	/** Get prediction data for a client game. Should not be used if not running as a client. Allocates the data on demand and can be overridden to allocate a custom override if desired. Result must be a FNetworkPredictionData_Client_Character. */
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PerformActionsBeforeMove(float DeltaTime) override;

protected:

	int ApprovedContractsNum = 0;

	/* Incremental counter to assign contracts a sequence number */
	int ApprovedContractSequenceNumber = 0;
	int RemovedContractSequenceNumber = 0;

	/* Last contract that has been predicted by client */
	uint16 LastAckApprovedContract = 0;
	uint16 LastAckRemovedContract = 0;

	UPROPERTY(Replicated)
	TArray<FPredictionContract> ApprovedContracts;
	
	TArray<FPredictionContract> ContractsAppliedThisMove;
	TArray<FPredictionContract> ContractsRemovedThisMove;

	/* When the contract time expires, this delegate executes an event that removes the contract. */
	FTimerDelegate RemoveContractDelegate;

	/* Server verifies if ContractToApply has been predicted by client.
	   Client verifies if ContractToApply has been removed by server. */
	bool IsValidContract(const FPredictionContract& ContractToApply, EContractActionType ContractAction) const;

	void TryToApplyPendingContracts();
	void TryToRemoveExpiredContracts();

	void ApplyContract(FPredictionContract& OutContractToApply);
	void RemoveContract(FPredictionContract& OutContractToRemove);

	void RollbackContracts(TArray<FSavedMovePtr>& MovesToRollback);
	void RollbackContractAction(const TArray<FPredictionContract>& OutContracts, EContractActionType ContractAction);

	void RemoveContractAfterDuration(int32 ContractID, float Duration);

	/** Event notification when client receives a correction from the server. Base implementation logs relevant data and draws debug info if "p.NetShowCorrections" is not equal to 0. */
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode);

	/* Here we should add the replication flags that is going to be consider when the serialization runs. */
	virtual void FillDynamicFlagsForMove(uint8& OutDynamicFlags, EMoveType MoveType, const class FSavedMove_Character* Move) override;

	/* Here we should serialize taking in consideration the flags previously set. */
	virtual void SerializeExtraData(FArchive& Ar, uint8 OutDynamicFlags, EMoveType MoveType, const class FSavedMove_Character* Move = nullptr) override;
};

class EXTENDEDMOVEMENT_API FEXMSavedMove_Movement : public FSavedMove_Character
{
public:

	TArray<FPredictionContract> ContractsAppliedThisMove;
	TArray<FPredictionContract> ContractsRemovedThisMove;

	bool bHasContracts;

	mutable uint16 LastAckApprovedContract = 0;
	mutable uint16 LastAckRemovedContract = 0;

	typedef FSavedMove_Character Super;

	/*Sets the default values for the saved move*/
	virtual void Clear() override;

	/*Checks if an old move can be combined with a new move for replication purposes (are they different or the same)*/
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

	/*This is used when the server plays back our saved move(s). This basically does the exact opposite of what
	*SetMoveFor does. Here we set the character movement controller variables from the data in FSavedMoves*/
	virtual void PrepMoveFor(class ACharacter* Character) override;

	/*Populates the FSavedMove fields from the corresponding character movement controller variables.
	*This is used when making a new SavedMove and the data will be used when playing back saved moves in the event that a correction needs to happen*/
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;
};


/*
*This subclass of FNetworkPredictionData_Client_Character is used to create new copies of our custom
*FSavedMove_Character class defined above.
*/
class EXTENDEDMOVEMENT_API FEXMContractNetworkPredictionData_Client_Character : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FEXMContractNetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {};

	/*Allocates a new copy of our custom saved move*/
	virtual FSavedMovePtr AllocateNewMove() override;
};

