// Fill out your copyright notice in the Description page of Project Settings.

#include "EXMContractMovementComponent.h"
#include "EXMContractInstigator.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "Core/EXMTypes.h"

void UEXMContractMovementComponent::Server_RemovePredictionContract_Implementation(int32 ContractID)
{
	FPredictionContract* FoundContract = ApprovedContracts.FindByPredicate([ContractID](const FPredictionContract& CurrentContract)
	{
		return CurrentContract.ContractID == ContractID;
	});

	if (FoundContract == nullptr)
		return;

	RemovedContractSequenceNumber++;
	FoundContract->RemovalSequenceNum = RemovedContractSequenceNumber;

	if (CharacterOwner->IsLocallyControlled())
		LastAckRemovedContract = RemovedContractSequenceNumber;
}

bool UEXMContractMovementComponent::Server_RemovePredictionContract_Validate(int32 ContractID)
{
	return true;
}

void UEXMContractMovementComponent::Server_RequestPredictionContract_Implementation(const FPredictionContractRequest& Request)
{
	UObject* Instigator = nullptr;

	if (Request.IsModifierSources(EModifiersSources::ModifiersInClass) || Request.IsModifierSources(EModifiersSources::ModifiersInRequest))
		Instigator = Request.InstigatorClass->GetDefaultObject<UObject>();
	else
		Instigator = Request.Instigator;

	IEXMContractInstigator* ContractInstigator = Cast<IEXMContractInstigator>(Instigator);

	if (!ContractInstigator->CanApplyModifier(Request.ModifierId))
		return;

	FPredictionContract NewPredictionContract;

	NewPredictionContract.Instigator = Instigator;
	NewPredictionContract.ModifierID = Request.ModifierId;

	if (Request.IsModifierSources(EModifiersSources::ModifiersInClass))
	{
		NewPredictionContract.ModifierSources = EModifiersSources::ModifiersInClass;
		NewPredictionContract.InstigatorClass = Request.InstigatorClass;
	}
	else if (Request.IsModifierSources(EModifiersSources::ModifiersInInstigator))
	{
		NewPredictionContract.ModifierSources = EModifiersSources::ModifiersInInstigator;
	}
	else if(Request.IsModifierSources(EModifiersSources::ModifiersInRequest))
	{
		NewPredictionContract.ModifierSources = EModifiersSources::ModifiersInRequest;
		NewPredictionContract.InstigatorClass = Request.InstigatorClass;
		NewPredictionContract.ModifiersApplied = Request.SerializedData;
	}

	ApprovedContractSequenceNumber++;
	const int32 HashContractID = GetTypeHash(&NewPredictionContract);

	NewPredictionContract.ContractID = HashContractID;
	NewPredictionContract.ApprovalSequenceNum = ApprovedContractSequenceNumber;

	ApprovedContracts.Add(NewPredictionContract);

	if (CharacterOwner->IsLocallyControlled())
		LastAckApprovedContract = ApprovedContractSequenceNumber;

	if (Request.Duration != InvalidDuration)
		RemoveContractAfterDuration(NewPredictionContract.ContractID, Request.Duration);
}

bool UEXMContractMovementComponent::Server_RequestPredictionContract_Validate(const FPredictionContractRequest& Request)
{
	return true;
}

void UEXMContractMovementComponent::SetupNewContract(FPredictionContract& OutContract, IEXMContractInstigator* Instigator, uint8 ModifierID, float Duration)
{
	ApprovedContractSequenceNumber++;
	const int32 HashContractID = GetTypeHash(&OutContract);

	OutContract.ContractID = HashContractID;
	OutContract.ApprovalSequenceNum = ApprovedContractSequenceNumber;

	OutContract.ModifierID = ModifierID;

	ApprovedContracts.Add(OutContract);

	if (CharacterOwner->IsLocallyControlled())
		LastAckApprovedContract = ApprovedContractSequenceNumber;

	if (Duration != InvalidDuration)
		RemoveContractAfterDuration(OutContract.ContractID, Duration);
}

void UEXMContractMovementComponent::RemoveContractAfterDuration(int32 ContractID, float Duration)
{
	if (Duration != 0)
	{
		APlayerState* PS = CharacterOwner->GetPlayerState();

		if (PS != nullptr)
			Duration += PS->ExactPing / 1000.f;

		RemoveContractDelegate.BindUFunction(this, FName("Server_RemovePredictionContract"), ContractID);

		FTimerHandle TempHandle;
		GetWorld()->GetTimerManager().SetTimer(TempHandle, RemoveContractDelegate, Duration, false);
	}
}

void UEXMContractMovementComponent::OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode)
{
	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode);

	RollbackContracts(ClientData.SavedMoves);
}

FNetworkPredictionData_Client* UEXMContractMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UEXMContractMovementComponent* MutableThis = const_cast<UEXMContractMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FEXMContractNetworkPredictionData_Client_Character(*this);
	}

	return ClientPredictionData;
}

void UEXMContractMovementComponent::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UEXMContractMovementComponent, ApprovedContracts, COND_AutonomousOnly);
}

void UEXMContractMovementComponent::PerformActionsBeforeMove(float DeltaTime)
{
	TryToApplyPendingContracts();
	TryToRemoveExpiredContracts();
}

bool UEXMContractMovementComponent::IsValidContract(const FPredictionContract& ContractToApply, EContractActionType ContractAction) const
{
	if (CharacterOwner->HasAuthority())
	{
		if(ContractAction == EContractActionType::Apply)
			return !ContractToApply.bAlreadyApplied && ContractToApply.ApprovalSequenceNum != INDEX_NONE && ContractToApply.ApprovalSequenceNum <= LastAckApprovedContract;
		else if(ContractAction == EContractActionType::Remove)
			return ContractToApply.bAlreadyApplied && ContractToApply.RemovalSequenceNum != INDEX_NONE && ContractToApply.RemovalSequenceNum <= LastAckRemovedContract;
	}
	else
	{
		if (ContractToApply.bWaitingToServer)
			return false;

		if (ContractAction == EContractActionType::Apply)
			return !ContractToApply.bAlreadyApplied && ContractToApply.ApprovalSequenceNum != INDEX_NONE;
		else if (ContractAction == EContractActionType::Remove)
			return ContractToApply.bAlreadyApplied && ContractToApply.RemovalSequenceNum != INDEX_NONE;
	}

	return false;
}

void UEXMContractMovementComponent::TryToApplyPendingContracts()
{
	TArray<FPredictionContract>& PredictionContracts = CharacterOwner->bClientUpdating ? ContractsAppliedThisMove : ApprovedContracts;

	for (int i = 0; i < PredictionContracts.Num(); i++)
	{
		FPredictionContract& Contract = PredictionContracts[i];

		if (CharacterOwner->bClientUpdating || IsValidContract(Contract, EContractActionType::Apply))
		{
			ApplyContract(Contract);
		}
	}
}

void UEXMContractMovementComponent::TryToRemoveExpiredContracts()
{
	TArray<FPredictionContract>& PredictionContracts = CharacterOwner->bClientUpdating ? ContractsRemovedThisMove : ApprovedContracts;

	for (int i = 0; i < PredictionContracts.Num(); i++)
	{
		FPredictionContract& Contract = PredictionContracts[i];

		if (CharacterOwner->bClientUpdating || IsValidContract(Contract, EContractActionType::Remove))
		{
			RemoveContract(Contract);

			if (CharacterOwner->HasAuthority())
			{
				PredictionContracts.RemoveAt(i);
				i--;
			}
		}
	}
}

void UEXMContractMovementComponent::ApplyContract(FPredictionContract& OutContractToApply)
{
	OutContractToApply.bAlreadyApplied = true;

	IEXMContractInstigator* ContractInstigator = Cast<IEXMContractInstigator>(OutContractToApply.Instigator);
	int SizeOf = ContractInstigator->GetSizeOfModifier(OutContractToApply.ModifierID);

	if (IsNetMode(NM_Standalone) || IsNetMode(NM_ListenServer))
	{
		FBitWriter SimpleWriter = FBitWriter(SizeOf);
		ContractInstigator->ApplyContractAction(OutContractToApply.ModifierID, SimpleWriter, EContractActionType::Apply, CharacterOwner);
		OutContractToApply.ModifiersApplied = *SimpleWriter.GetBuffer();
	}
	else
	{
		FNetBitWriter NetWriter = FNetBitWriter(CharacterOwner->GetNetConnection()->PackageMap, SizeOf);
		ContractInstigator->ApplyContractAction(OutContractToApply.ModifierID, NetWriter, EContractActionType::Apply, CharacterOwner);
		OutContractToApply.ModifiersApplied = *NetWriter.GetBuffer();
	}

	if (!CharacterOwner->bClientUpdating && !CharacterOwner->HasAuthority())
	{
		ContractsAppliedThisMove.Add(OutContractToApply);

		if (LastAckApprovedContract < OutContractToApply.ApprovalSequenceNum)
			LastAckApprovedContract = OutContractToApply.ApprovalSequenceNum;
	}
}

void UEXMContractMovementComponent::RemoveContract(FPredictionContract& OutContractToRemove)
{
	IEXMContractInstigator* ContractInstigator = Cast<IEXMContractInstigator>(OutContractToRemove.Instigator);
	int SizeOf = ContractInstigator->GetSizeOfModifier(OutContractToRemove.ModifierID);

	if (IsNetMode(NM_Standalone) || IsNetMode(NM_ListenServer))
	{
		FBitReader SimpleReader = FBitReader(OutContractToRemove.ModifiersApplied.GetData(), SizeOf);
		ContractInstigator->ApplyContractAction(OutContractToRemove.ModifierID, SimpleReader, EContractActionType::Remove, CharacterOwner);
	}
	else
	{
		FNetBitReader NetReader = FNetBitReader(CharacterOwner->GetNetConnection()->PackageMap,OutContractToRemove.ModifiersApplied.GetData(), SizeOf);
		ContractInstigator->ApplyContractAction(OutContractToRemove.ModifierID, NetReader, EContractActionType::Remove, CharacterOwner);
	}

	if (!CharacterOwner->bClientUpdating && !CharacterOwner->HasAuthority())
	{
		ContractsRemovedThisMove.Add(OutContractToRemove);

		if (LastAckRemovedContract < OutContractToRemove.RemovalSequenceNum)
			LastAckRemovedContract = OutContractToRemove.RemovalSequenceNum;

		OutContractToRemove.bWaitingToServer = true;
	}
}

void UEXMContractMovementComponent::RollbackContracts(TArray<FSavedMovePtr>& MovesToRollback)
{
	for (int i = MovesToRollback.Num() -1 ; i > 1; i--)
	{
		const FEXMSavedMove_Movement& MoveToRollback = (const FEXMSavedMove_Movement&)*MovesToRollback[i];

		RollbackContractAction(MoveToRollback.ContractsAppliedThisMove, EContractActionType::ApplyRollback);
		RollbackContractAction(MoveToRollback.ContractsRemovedThisMove, EContractActionType::RemoveRollback);
	}
}

void UEXMContractMovementComponent::RollbackContractAction(const TArray<FPredictionContract>& OutContracts, EContractActionType ContractAction)
{
	for (int i = OutContracts.Num() - 1; i > 1; i--)
	{
		const FPredictionContract& ContractToRollback = OutContracts[i];

		IEXMContractInstigator* ContractInstigator = Cast<IEXMContractInstigator>(ContractToRollback.Instigator);
		int SizeOf = ContractInstigator->GetSizeOfModifier(ContractToRollback.ModifierID);

		FNetBitReader Reader = FNetBitReader(CharacterOwner->GetNetConnection()->PackageMap,const_cast<uint8*>(ContractToRollback.ModifiersApplied.GetData()), SizeOf);

		ContractInstigator->ApplyContractAction(ContractToRollback.ModifierID, Reader, ContractAction, CharacterOwner);
	}
}

void UEXMContractMovementComponent::FillDynamicFlagsForMove(uint8& OutDynamicFlags, EMoveType MoveType, const class FSavedMove_Character* Move)
{
	const FEXMSavedMove_Movement* EXMMove = (const FEXMSavedMove_Movement*)Move;

	if (EXMMove->bHasContracts)
		SET_BIT(OutDynamicFlags, EDynamicFlags::Custom0);
}

void UEXMContractMovementComponent::SerializeExtraData(FArchive& Ar, uint8 OutDynamicFlags, EMoveType MoveType, const class FSavedMove_Character* Move /*= nullptr*/)
{
	//Example:
	//If we are loading we deserialize to the local variables taking in consideration which flags are set.
	if (Ar.IsLoading())
	{
		if (TEST_BIT(OutDynamicFlags, EDynamicFlags::Custom0))
		{
			Ar << LastAckApprovedContract;
			Ar << LastAckRemovedContract;
		}
	}
	//If we are saving we should take the data from the savedMove taking in consideration which flags are previously set.
	else
	{
		const FEXMSavedMove_Movement* EXMMove = (const FEXMSavedMove_Movement*)Move;

		if (TEST_BIT(OutDynamicFlags, EDynamicFlags::Custom0))
		{
			Ar << EXMMove->LastAckApprovedContract;
			Ar << EXMMove->LastAckRemovedContract;
		}
	}
}


void FEXMSavedMove_Movement::Clear()
{
	Super::Clear();

	LastAckApprovedContract = 0;
	LastAckRemovedContract = 0;
	bHasContracts = false;
	ContractsAppliedThisMove.Empty();
	ContractsRemovedThisMove.Empty();
}

bool FEXMSavedMove_Movement::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	const FEXMSavedMove_Movement* EXMMove = (const FEXMSavedMove_Movement*)NewMove.Get();

	if (bHasContracts != EXMMove->bHasContracts)
	{
		return false;
	}
	else if (bHasContracts)
	{
		if (LastAckApprovedContract != EXMMove->LastAckApprovedContract)
			return false;

		if (LastAckRemovedContract != EXMMove->LastAckRemovedContract)
			return false;
	}
	
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FEXMSavedMove_Movement::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	/*Saved Move ---> Character Movement Data*/
	UEXMContractMovementComponent* CharMove = Cast<UEXMContractMovementComponent>(Character->GetCharacterMovement());
	if (CharMove)
	{
		CharMove->ContractsAppliedThisMove = ContractsAppliedThisMove;
		CharMove->ContractsRemovedThisMove = ContractsRemovedThisMove;
	}
}

void FEXMSavedMove_Movement::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character & ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UEXMContractMovementComponent* CharMove = Cast<UEXMContractMovementComponent>(C->GetCharacterMovement());
	if (CharMove)
	{
		bHasContracts = CharMove->ApprovedContracts.Num() > 0;

		ContractsAppliedThisMove = CharMove->ContractsAppliedThisMove;
		CharMove->ContractsAppliedThisMove.Empty();
		ContractsRemovedThisMove = CharMove->ContractsRemovedThisMove;
		CharMove->ContractsRemovedThisMove.Empty();

		LastAckApprovedContract = CharMove->LastAckApprovedContract;
		LastAckRemovedContract = CharMove->LastAckRemovedContract;
	}
}

FSavedMovePtr FEXMContractNetworkPredictionData_Client_Character::AllocateNewMove()
{
	return FSavedMovePtr(new FEXMSavedMove_Movement());
}
