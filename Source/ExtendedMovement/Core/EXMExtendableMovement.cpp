#include "EXMExtendableMovement.h"
#include "GameFramework/Character.h"
#include "EXMCharacterMovementComponent.h"

UEXMCharacterMovementComponent* IEXMExtendableMovement::GetEXMCharacterMovementComponent()
{
	return Cast<UEXMCharacterMovementComponent>(Cast<ACharacter>(this)->GetCharacterMovement());
}
