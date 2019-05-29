#pragma once

#include "EXMContractTypes.h"
#include "EXMContractInstigator.h"
#include "EXMContract.generated.h"

USTRUCT()
struct FPredictionContract
{
	GENERATED_BODY()

	bool bWaitingToServer = false;
	bool bAlreadyApplied = false;

	UPROPERTY()
	int ContractID = 0;

	UPROPERTY()
	EModifiersSources ModifierSources;

	/* The ApprovalSequenceNum defines the order in which this contract was approved.
	   An ApprovalSequenceNum of n means that this was the nth contract that was approved. */
	UPROPERTY()
	int ApprovalSequenceNum = INDEX_NONE;

	/* RemovalSequenceNum defines the order in which this contract was removed (same as ApprovalSequenceNum). */
	UPROPERTY()
	int RemovalSequenceNum = INDEX_NONE;

	UPROPERTY()
	uint8 ModifierID = 0;

	//ModifiersInInstigator
	UPROPERTY()
	class UObject* Instigator = nullptr;

	//ModifiersInClass
	UPROPERTY()
	class UClass* InstigatorClass = nullptr;

	TArray<uint8> ModifiersApplied;

	/* Here we declare which fields of the contract will be sent over the network depending on the ModifierSources field */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << ModifierSources;

		if (ModifierSources == EModifiersSources::ModifiersInInstigator)
		{
			Ar << Instigator;
			Ar << ModifierID;
		}
		else if (ModifierSources == EModifiersSources::ModifiersInClass)
		{
			Ar << InstigatorClass;
			Ar << ModifierID;
		}

		if (Ar.IsLoading())
		{
			IEXMContractInstigator* ContractInstigator = nullptr;

			if (ModifierSources == EModifiersSources::ModifiersInClass)
			{
				Instigator = InstigatorClass->GetDefaultObject<UObject>();
			}

			ContractInstigator = Cast<IEXMContractInstigator>(Instigator);
		}

		Ar << ApprovalSequenceNum;
		Ar << RemovalSequenceNum;
		Ar << ContractID;

		bOutSuccess = true;

		return true;
	}

	bool operator==(const FPredictionContract& ValueA) const
	{
		return ContractID == ValueA.ContractID;
	}
};

template<>
struct TStructOpsTypeTraits<FPredictionContract> : public TStructOpsTypeTraitsBase2<FPredictionContract>
{
	enum
	{
		WithNetSerializer = true
	};
};
