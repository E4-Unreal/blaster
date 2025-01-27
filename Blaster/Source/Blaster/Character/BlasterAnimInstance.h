// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BlasterAnimInstance.generated.h"

enum class ETurnInPlaceState : uint8;

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// Eliminated
	UFUNCTION(BlueprintCallable, Category = Eliminated, meta=(BlueprintThreadSafe))
	void OnEliminated(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

private:
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsWeaponEquipped;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsCrouched;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Lean;

	FRotator LastCharacterRotation;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	// Aim Offset
	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	float Yaw;

	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	float Pitch;

	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	bool bUseAimOffset;

	// Turn In Place
	UPROPERTY(BlueprintReadOnly, Category = TurnInPlace, meta = (AllowPrivateAccess = "true"))
	ETurnInPlaceState TurnInPlaceState;

	// Weapon Correction
	UPROPERTY(BlueprintReadOnly, Category = WeaponCorrection, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;

	UPROPERTY(BlueprintReadOnly, Category = WeaponCorrection, meta = (AllowPrivateAccess = "true"))
	bool bIsLocallyControlled;

	// Eliminated
	UPROPERTY(BlueprintReadOnly, Category = Eliminated, meta = (AllowPrivateAccess = "true"))
	bool bIsEliminated;

	// 오른손 총구 조정
	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	bool bUseTransformRightHand;

	// FABRRIK
	UPROPERTY(BlueprintReadOnly, Category = FABRIK, meta = (AllowPrivateAccess = "true"))
	bool bUseFABRIK;
};
