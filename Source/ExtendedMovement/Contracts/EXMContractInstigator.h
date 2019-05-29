#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EXMContractInstigator.generated.h"

UENUM()
enum class EContractActionType : uint8
{
	Apply,
	Remove,
	ApplyRollback,
	RemoveRollback
};

UINTERFACE(MinimalAPI)
class UEXMContractInstigator : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class EXTENDEDMOVEMENT_API IEXMContractInstigator
{
	GENERATED_BODY()

public:

	virtual bool CanApplyModifier(uint8 ModifierID) const = 0;
	virtual int GetSizeOfModifier(uint8 ModifierID) const = 0;

	virtual void ApplyContractAction(uint8 ModifierID, FArchive& Ar, EContractActionType Action, class ACharacter* Character) = 0;
};