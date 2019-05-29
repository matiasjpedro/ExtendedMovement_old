// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/EXMExtendableMovement.h"
#include "GameFramework/Character.h"
#include "EXMCharacter.generated.h"

UCLASS()
class EXTENDEDMOVEMENT_API AEXMCharacter : public ACharacter, public IEXMExtendableMovement
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEXMCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	// IEXMExtendableMovement Begin

public:

	UFUNCTION(server, unreliable, WithValidation)
	virtual void Server_MoveOld(const TArray<uint8>& InData);

	UFUNCTION(server, unreliable, WithValidation)
	virtual void Server_MoveDual(const TArray<uint8>& InData);

	UFUNCTION(server, unreliable, WithValidation)
	virtual void Server_Move(const TArray<uint8>& InData);

	// IEXMExtendableMovement End
};
