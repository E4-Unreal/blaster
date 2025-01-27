﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UWaitingPostMatchOverlay;
class UMatchTimerOverlay;
class UWeaponOverlay;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
	
	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairsSpread;
	FLinearColor CrosshairsColor;
};

class UCharacterOverlay;
class UWaitingToStartOverlay;
class AWeapon;

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() override;
	virtual void DrawHUD() override;

	/* Match State */
	UFUNCTION()
	void OnMatchStateSet(FName NewState);

	void ShowCharacterOverlay();
	void ShowWaitingToStartOverlay();
	void ShowWaitingPostMatchOverlay();
	void ShowSniperScopeOverlay(bool bShowOverlay);
	
	void HideCharacterOverlay();
	void HideWaitingToStartOverlay();
	void HideWaitingPostMatchOverlay();

	// TODO 임시 for Lobby
	void HideAllExceptCharacterInfo() const;

	/* Set Overlay Value */
	// Character Overlay
	UFUNCTION()
	void SetHealth(float Health);
	
	UFUNCTION()
	void SetMaxHealth(float MaxHealth);

	UFUNCTION()
	void SetScore(int32 Score);

	UFUNCTION()
	void SetDefeats(int32 Defeats);

	UFUNCTION()
	void SetCarriedAmmo(int32 CarriedAmmo);

	// Match Timer Overlay
	UFUNCTION()
	void SetCountdownTime(float CountdownTime);

	// Waiting Post Match Overlay
	UFUNCTION()
	void SetTopScoringPlayers(const TArray<ABlasterPlayerState*>& InTopScoringPlayers);

	/* Weapon Overlay  */
	UFUNCTION()
	void SetEquippedWeapon(AWeapon* EquippedWeapon);

	UFUNCTION()
	void SetGrenadeCount(int32 GrenadeCount);

private:
	/* 사용자 위젯 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UCharacterOverlay* CharacterOverlay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UWaitingToStartOverlay* WaitingToStartOverlay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UWaitingPostMatchOverlay* WaitingPostMatchOverlay;

	// TODO 임시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UUserWidget* SniperScopeOverlay;

	UPROPERTY(EditAnywhere, Category = Overlay)
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = Overlay)
	TSubclassOf<UUserWidget> WaitingToStartOverlayClass;

	UPROPERTY(EditAnywhere, Category = Overlay)
	TSubclassOf<UUserWidget> WaitingPostMatchOverlayClass;

	UPROPERTY(EditAnywhere, Category = Overlay)
	TSubclassOf<UUserWidget> SniperScopeOverlayClass;
	
	// 크로스헤어
	FHUDPackage HUDPackage;

	void DrawCrosshairs(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor);

	UPROPERTY(EditAnywhere)
	float CrosshairsSpreadMax = 16.f;

	/* Match State */
	UMatchTimerOverlay* CurrentMatchTimerOverlay;
	FName MatchState;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& InPackage) { HUDPackage = InPackage; }

	// 사용자 위젯 Getter
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE UWaitingToStartOverlay* GetWaitingToStartOverlay() const { return WaitingToStartOverlay; }
	FORCEINLINE UWaitingPostMatchOverlay* GetWaitingPostMatchOverlay() const { return WaitingPostMatchOverlay; }
	
	UWeaponOverlay* GetWeaponOverlay() const;
	UMatchTimerOverlay* GetMatchTimerOverlay() const;

	/* Match State */
	FORCEINLINE UMatchTimerOverlay* GetCurrentMatchTimerOverlay() const { return CurrentMatchTimerOverlay; }
};
