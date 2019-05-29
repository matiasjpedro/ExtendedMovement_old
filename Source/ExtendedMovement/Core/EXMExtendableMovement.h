#pragma once

#include "UObject/Interface.h"
#include "EXMExtendableMovement.generated.h"

UINTERFACE(MinimalAPI)
class UEXMExtendableMovement : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class EXTENDEDMOVEMENT_API IEXMExtendableMovement
{
	GENERATED_BODY()

public:

	//UFUNCTION(unreliable, server, WithValidation)
	virtual void Server_MoveOld(const TArray<uint8>& InData) = 0;
	//UFUNCTION(unreliable, server, WithValidation)
	virtual void Server_MoveDual(const TArray<uint8>& InData) = 0;
	//UFUNCTION(unreliable, server, WithValidation)
	virtual void Server_Move(const TArray<uint8>& InData) = 0;

protected:

	class UEXMCharacterMovementComponent* GetEXMCharacterMovementComponent();
};
