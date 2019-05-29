// Fill out your copyright notice in the Description page of Project Settings.

#include "EXMCharacterMovementComponent.h"
#include "EXMExtendableMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Classes/Engine/NetConnection.h"
#include "EXMTypes.h"

void UEXMCharacterMovementComponent::CallServerMove(const class FSavedMove_Character* NewMove, const class FSavedMove_Character* OldMove)
{
	check(NewMove != nullptr);

	//EXM Override
	IEXMExtendableMovement* ExtendableMovement = Cast<IEXMExtendableMovement>(CharacterOwner);
	//EXM Override end

	// Compress rotation down to 5 bytes
	const uint32 ClientYawPitchINT = PackYawAndPitchTo32(NewMove->SavedControlRotation.Yaw, NewMove->SavedControlRotation.Pitch);
	const uint8 ClientRollBYTE = FRotator::CompressAxisToByte(NewMove->SavedControlRotation.Roll);

	// Determine if we send absolute or relative location
	UPrimitiveComponent* ClientMovementBase = NewMove->EndBase.Get();
	const FName ClientBaseBone = NewMove->EndBoneName;
	const FVector SendLocation = MovementBaseUtility::UseRelativeLocation(ClientMovementBase) ? NewMove->SavedRelativeLocation : NewMove->SavedLocation;

	// send old move if it exists
	if (OldMove)
	{
		//EXM Override
		uint8 DynamicFlags = 0;
		FillDynamicFlagsForMove(DynamicFlags, EMoveType::Old, OldMove);

		if(ShouldUseLegacyMovement(DynamicFlags))
		{
			ServerMoveOld(OldMove->TimeStamp, OldMove->Acceleration, OldMove->GetCompressedFlags());
		}
		else
		{
			FLegacyServerMoveOld Old = FLegacyServerMoveOld(OldMove->TimeStamp, OldMove->Acceleration, OldMove->GetCompressedFlags());

			FEXMNetBitWriter TempNetWriter = FEXMNetBitWriter(CharacterOwner->GetNetConnection()->PackageMap, sizeof(FLegacyServerMoveOld) * 8);
			TempNetWriter << DynamicFlags;

			Old.CustomSerialize(TempNetWriter, DynamicFlags);
			SerializeExtraData(TempNetWriter, DynamicFlags, EMoveType::Old, OldMove);

			ExtendableMovement->Server_MoveOld(TempNetWriter.GetPackedBuffer());
		}
		//EXM Override End
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (const FSavedMove_Character* const PendingMove = ClientData->PendingMove.Get())
	{
		const uint32 OldClientYawPitchINT = PackYawAndPitchTo32(ClientData->PendingMove->SavedControlRotation.Yaw, ClientData->PendingMove->SavedControlRotation.Pitch);

		// If we delayed a move without root motion, and our new move has root motion, send these through a special function, so the server knows how to process them.
		if ((PendingMove->RootMotionMontage == NULL) && (NewMove->RootMotionMontage != NULL))
		{
			// send two moves simultaneously
			ServerMoveDualHybridRootMotion(
				PendingMove->TimeStamp,
				PendingMove->Acceleration,
				PendingMove->GetCompressedFlags(),
				OldClientYawPitchINT,
				NewMove->TimeStamp,
				NewMove->Acceleration,
				SendLocation,
				NewMove->GetCompressedFlags(),
				ClientRollBYTE,
				ClientYawPitchINT,
				ClientMovementBase,
				ClientBaseBone,
				NewMove->EndPackedMovementMode
			);
		}
		else
		{
			//EXM Override
			uint8 PendingMoveDynamicFlags = 0;
			uint8 NewMoveDynamicFlags = 0;

			FillDynamicFlagsForMove(PendingMoveDynamicFlags, EMoveType::Dual, PendingMove);
			FillDynamicFlagsForMove(NewMoveDynamicFlags, EMoveType::Dual, NewMove);

			if(ShouldUseLegacyMovement(PendingMoveDynamicFlags) && ShouldUseLegacyMovement(NewMoveDynamicFlags))
			{
				ServerMoveDual(
					PendingMove->TimeStamp,
					PendingMove->Acceleration,
					PendingMove->GetCompressedFlags(),
					OldClientYawPitchINT,
					NewMove->TimeStamp,
					NewMove->Acceleration,
					SendLocation,
					NewMove->GetCompressedFlags(),
					ClientRollBYTE,
					ClientYawPitchINT,
					ClientMovementBase,
					ClientBaseBone,
					NewMove->EndPackedMovementMode
				);
			}
			else
			{
				if (MovementBaseUtility::IsDynamicBase(ClientMovementBase))
				{
					SET_BIT(PendingMoveDynamicFlags, EDynamicFlags::WithBase);
					SET_BIT(NewMoveDynamicFlags, EDynamicFlags::WithBase);
				}

				FLegacyServerMoveDual Dual = FLegacyServerMoveDual(
					PendingMove->TimeStamp,
					PendingMove->Acceleration,
					PendingMove->GetCompressedFlags(),
					OldClientYawPitchINT,
					NewMove->TimeStamp,
					NewMove->Acceleration,
					SendLocation,
					NewMove->GetCompressedFlags(),
					ClientRollBYTE,
					ClientYawPitchINT,
					ClientMovementBase,
					ClientBaseBone,
					NewMove->EndPackedMovementMode
				);

				FEXMNetBitWriter TempNetWriter = FEXMNetBitWriter(CharacterOwner->GetNetConnection()->PackageMap, sizeof(FLegacyServerMoveDual) * 8);
				TempNetWriter << PendingMoveDynamicFlags;
				TempNetWriter << NewMoveDynamicFlags;

				Dual.CustomSerialize(TempNetWriter, NewMoveDynamicFlags);
				SerializeExtraData(TempNetWriter, PendingMoveDynamicFlags, EMoveType::Dual, PendingMove);
				SerializeExtraData(TempNetWriter, NewMoveDynamicFlags, EMoveType::Dual, NewMove);

				ExtendableMovement->Server_MoveDual(TempNetWriter.GetPackedBuffer());
			}
			//EXM Override End			
		}
	}
	else
	{
		//EXM Override
		uint8 DynamicFlags = 0;
		FillDynamicFlagsForMove(DynamicFlags, EMoveType::Single, NewMove);

		if (ShouldUseLegacyMovement(DynamicFlags))
		{
			ServerMove(
				NewMove->TimeStamp,
				NewMove->Acceleration,
				SendLocation,
				NewMove->GetCompressedFlags(),
				ClientRollBYTE,
				ClientYawPitchINT,
				ClientMovementBase,
				ClientBaseBone,
				NewMove->EndPackedMovementMode
			);
		}
		else
		{
			if (MovementBaseUtility::IsDynamicBase(ClientMovementBase))
				SET_BIT(DynamicFlags, EDynamicFlags::WithBase);

			FLegacyServerMove Single = FLegacyServerMove(
				NewMove->TimeStamp,
				NewMove->Acceleration,
				SendLocation,
				NewMove->GetCompressedFlags(),
				ClientRollBYTE,
				ClientYawPitchINT,
				ClientMovementBase,
				ClientBaseBone,
				NewMove->EndPackedMovementMode
			);

			FEXMNetBitWriter TempNetWriter = FEXMNetBitWriter(CharacterOwner->GetNetConnection()->PackageMap, sizeof(FLegacyServerMove) * 8);
			TempNetWriter << DynamicFlags;

			Single.CustomSerialize(TempNetWriter, DynamicFlags);
			SerializeExtraData(TempNetWriter, DynamicFlags, EMoveType::Single, NewMove);

			ExtendableMovement->Server_Move(TempNetWriter.GetPackedBuffer());
		}
		//EXM Override End
	}

	MarkForClientCameraUpdate();
}

void UEXMCharacterMovementComponent::ResolveServerMove(const TArray<uint8>& InData, EMoveType MoveType)
{
	FNetBitReader TempNetReader = FNetBitReader(CharacterOwner->GetNetConnection()->PackageMap, const_cast<uint8*>(InData.GetData()), (sizeof(uint8) * InData.Num()) * 8);

	if (MoveType == EMoveType::Old)
	{
		uint8 DynamicFlags = 0;
		TempNetReader << DynamicFlags;

		FLegacyServerMoveOld Old;
		Old.CustomSerialize(TempNetReader, DynamicFlags);

		SerializeExtraData(TempNetReader, DynamicFlags, MoveType);
		ServerMoveOld_Implementation(Old.TimeStamp, Old.Accel, Old.MoveFlags);
	}
	else if (MoveType == EMoveType::Dual)
	{
		uint8 PendingMoveDynamicFlags = 0;
		TempNetReader << PendingMoveDynamicFlags;

		uint8 NewMoveDynamicFlags = 0;
		TempNetReader << NewMoveDynamicFlags;

		FLegacyServerMoveDual Dual;
		Dual.CustomSerialize(TempNetReader, NewMoveDynamicFlags);

		SerializeExtraData(TempNetReader, PendingMoveDynamicFlags, MoveType);
		ServerMove_Implementation(Dual.TimeStamp0, Dual.Accel0, FVector(1.f, 2.f, 3.f), Dual.PendingFlags, Dual.ClientRoll, Dual.View0, Dual.ClientMovementBase, Dual.ClientBaseBoneName, Dual.ClientMovementMode);

		SerializeExtraData(TempNetReader, NewMoveDynamicFlags, MoveType);
		ServerMove_Implementation(Dual.TimeStamp, Dual.Accel, Dual.ClientLoc, Dual.NewFlags, Dual.ClientRoll, Dual.View, Dual.ClientMovementBase, Dual.ClientBaseBoneName, Dual.ClientMovementMode);
	}
	else if(MoveType == EMoveType::Single)
	{
		uint8 DynamicFlags = 0;
		TempNetReader << DynamicFlags;

		FLegacyServerMove Single;
		Single.CustomSerialize(TempNetReader, DynamicFlags);

		SerializeExtraData(TempNetReader, DynamicFlags, MoveType);
		ServerMove_Implementation(Single.TimeStamp, Single.Accel, Single.ClientLoc, Single.CompressedMoveFlags, Single.ClientRoll, Single.View, Single.ClientMovementBase, Single.ClientBaseBoneName, Single.ClientMovementMode);
	}
}

void UEXMCharacterMovementComponent::FillDynamicFlagsForMove(uint8& OutDynamicFlags, EMoveType MoveType, const FSavedMove_Character* Move)
{
	//Example:
	//We should see which data is stored in the saved move in order to add replication flags so the serialization could add it.

	/*FEXMSavedMove_Character* EXMMove = (FEXMSavedMove_Character*)Move;

	if (EXMMove.ShootPosition != FVector::ZeroVector)
		SET_BIT(OutDynamicFlags, EDynamicFlags::Custom0);*/
}

void UEXMCharacterMovementComponent::SerializeExtraData(FArchive& Ar, uint8 OutDynamicFlags, EMoveType MoveType, const FSavedMove_Character* Move /*= nullptr*/)
{
	//Example:
	//If we are loading we deserialize to the local variables taking in consideration which flags are set.
	/*if (Ar.IsLoading())
	{
		if (TEST_BIT(OutDynamicFlags, EDynamicFlags::Custom0))
		{
			Ar << LastShootPosition;
		}
	}
	//If we are saving we should take the data from the savedMove taking in consideration which flags are previously set.
	else
	{
		FEXMSavedMove_Character* EXMMove = (FEXMSavedMove_Character*)Move;

		if (TEST_BIT(OutDynamicFlags, EDynamicFlags::Custom0))
		{
			Ar << EXMMove->ShootPosition;
		}
	}*/
}
