// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/BlasterTypes/TurnInPlaceState.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Character
	bUseControllerRotationYaw = false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Character Movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);

	// Net
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // CameraBoom에서 이미 설정됨

	// Overhead Widget
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	// Combat
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	// Dissolve
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	// Attached Grenade
	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("grenade_r"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetVisibility(false);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 데미지 이벤트
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::OnTakeAnyDamage_Event);
	}

	// TODO 임시
	if(bEnableStartingWeapon && HasAuthority() && !StartingWeapons.IsEmpty())
	{
		int32 Index = FMath::RandRange(0, StartingWeapons.Num() - 1);
		if(UWorld* World = GetWorld())
		{
			AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(
				StartingWeapons[Index],
				GetActorTransform()
			);

			if(Combat)
			{
				Combat->ServerEquipWeapon(StartingWeapon);
			}
		}
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
	HideCharacter();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &ThisClass::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ThisClass::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ThisClass::ThrowGrenadeButtonPressed);
	PlayerInputComponent->BindAction("Drop", IE_Pressed, this, &ThisClass::DropButtonPressed);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ThisClass, Health);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if(Combat)
	{
		Combat->Character = this;
	}

	/* Set Skeletal Mesh Socket */
	if(GetMesh())
	{
		RightHandSocket = GetMesh()->GetSocketByName(FName("weapon_r"));
		LeftHandSocket = GetMesh()->GetSocketByName(FName("weapon_l"));
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if(ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	BlasterPlayerController = Cast<ABlasterPlayerController>(NewController);
}

void ABlasterCharacter::Equipped()
{
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;

	if(BlasterPlayerController)
		BlasterPlayerController->ShowWeaponOverlay();
}

void ABlasterCharacter::UnEquipped()
{
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	
	// HUD Weapon UI 파괴 혹은 지우기
	if(BlasterPlayerController)
		BlasterPlayerController->HideWeaponOverlay();
}

USkeletalMeshSocket const* ABlasterCharacter::GetSkeletalMeshSocket(FName SocketName) const
{
	return GetMesh() == nullptr ? nullptr : GetMesh()->GetSocketByName(SocketName);
}

void ABlasterCharacter::PlayFireMontage(bool bIsAiming)
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = bIsAiming ? FName("RifleIronSights") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EW_AssaultRifle:
			SectionName = "Rifle";
			break;
		case EWeaponType::EW_RocketLauncher:
			SectionName = "Rocket";
			break;
		case EWeaponType::EW_Pistol:
			SectionName = "Pistol";
			break;
		case EWeaponType::EW_SMG:
			SectionName = "SMG";
			break;
		case EWeaponType::EW_ShotGun:
			SectionName = "ShotGun";
			break;
		case EWeaponType::EW_SniperRifle:
			SectionName = "SniperRifle";
			break;
		case EWeaponType::EW_GrenadeLauncher:
			SectionName = "GrenadeLauncher";
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayEliminatedMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && EliminatedMontage)
	{
		AnimInstance->Montage_Play(EliminatedMontage);
	}
}

void ABlasterCharacter::ServerEliminate()
{
	// Drop Weapon
	if(Combat)
	{
		Combat->ServerDropWeapon();
	}
	
	MulticastEliminate();
	GetWorldTimerManager().SetTimer(
		EliminatedTimer,
		this,
		&ThisClass::EliminatedTimerFinished,
		EliminatedDelay
	);
}

void ABlasterCharacter::MulticastEliminate_Implementation()
{
	OnEliminated.Broadcast();
	
	bIsEliminated = true;
	// PlayEliminatedMontage는 Blaster Anim Instance > 'EliminatedSlot' 슬롯 > 초기 업데이트 시 OnEliminated에서 호출됨

	// Dissolve Effect
	if(DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
		StartDissolve();
	}

	// Disable Character Movement
	GetCharacterMovement()->DisableMovement(); // Disable Character Move
	GetCharacterMovement()->StopMovementImmediately(); // Disable Character Rotate
	if(BlasterPlayerController)
	{
		DisableInput(BlasterPlayerController);
	}

	// Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn ElimBot
	if(ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			this,
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}

	if(ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	// Hide Scope
	if(IsLocallyControlled())
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::EliminatedTimerFinished()
{
	if(UWorld* World = GetWorld())
	{
		if(ABlasterGameMode* BlasterGameMode = World->GetAuthGameMode<ABlasterGameMode>())
		{
			BlasterGameMode->RequestRespawn(this, Controller);
		}
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName SectionName("FromFront"); // TODO 방향에 따라 다른 재생
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	// TODO AnimInstance 멤버 변수화?
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if(Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if(Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::Jump()
{
	// TODO UnCrouch > Jump 두 동작을 동시에?
	if(bIsCrouched)
		UnCrouch();
	else
		Super::Jump();
}

void ABlasterCharacter::EquipButtonPressed()
{
	ServerEquipButtonPressed();
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(Combat)
	{
		Combat->ServerEquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if(bIsCrouched)
		UnCrouch();
	else
		Crouch();
}

void ABlasterCharacter::AimButtonPressed()
{
	if(Combat)
	{
		Combat->SetIsAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if(Combat)
	{
		Combat->SetIsAiming(false);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if(Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if(Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if(Combat)
		Combat->Reload();
}

void ABlasterCharacter::ThrowGrenadeButtonPressed()
{
	if(Combat)
		Combat->ThrowGrenade();
}

void ABlasterCharacter::DropButtonPressed()
{
	ServerDropButtonPressed();
}

void ABlasterCharacter::ServerDropButtonPressed_Implementation()
{
	if(Combat)
	{
		Combat->ServerDropWeapon();
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// UnEquip 상태일 때는 무시
	if(Combat && Combat->EquippedWeapon == nullptr) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	// Yaw
	// Idle
	if(Speed == 0.f && !bIsInAir)
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		Yaw = DeltaAimRotation.Yaw;
		if(TurnInPlaceState == ETurnInPlaceState::ETIP_NotTurning)
		{
			InterpYaw = Yaw;
		}
		TurnInPlace(DeltaTime);
	}

	// Run or Jump
	if(Speed > 0.f || bIsInAir)
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		Yaw = 0.f;
		TurnInPlaceState = ETurnInPlaceState::ETIP_NotTurning;
	}

	Pitch = GetBaseAimRotation().Pitch;
	if(!IsLocallyControlled() && Pitch > 90.0f)
	{
		// [270, 360)를 [-90, 0)로 변환
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, Pitch);
	}
}

void ABlasterCharacter::OnTakeAnyDamage_Event(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	// 데미지 적용
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	OnHealthUpdated.Broadcast(Health);
	//UpdateHUD_Health();

	// 체력과 데미지에 따른 반응
	if(Health == 0.f) // Dead
	{
		if(UWorld* World = GetWorld())
		{
			if(ABlasterGameMode* BlasterGameMode = World->GetAuthGameMode<ABlasterGameMode>())
			{
				ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatedBy);
				BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
			}
		}
	}
	else if(Damage > 0)
	{
		if(Combat && Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			PlayHitReactMontage();
		}
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if(Yaw > 90.f)
	{
		// Turn Right
		TurnInPlaceState = ETurnInPlaceState::ETIP_Right;
	}
	else if(Yaw < -90.f)
	{
		// Turn Left
		TurnInPlaceState = ETurnInPlaceState::ETIP_Left;
	}

	if(TurnInPlaceState != ETurnInPlaceState::ETIP_NotTurning)
	{
		InterpYaw = FMath::FInterpTo(InterpYaw, 0.f, DeltaTime, 4.f);
		Yaw = InterpYaw;
		if(FMath::Abs(Yaw) < 15.f)
		{
			TurnInPlaceState = ETurnInPlaceState::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCharacter()
{
	if(!IsLocallyControlled()) return;
	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_MaxHealth()
{
	OnHealthUpdated.Broadcast(MaxHealth);
}

void ABlasterCharacter::OnRep_Health(float OldHealth)
{
	OnHealthUpdated.Broadcast(Health);
	
	//UpdateHUD_Health();

	if(OldHealth > Health)
		PlayHitReactMontage();
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// 플레이어가 서버인 경우 OnRep_OverlappingWeapon 대체
	if(IsLocallyControlled())
	{
		// Old Weapon
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
		
		// New Weapon
		if(Weapon)
		{
			Weapon->ShowPickupWidget(true);
		}
	}

	OverlappingWeapon = Weapon;
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return Combat && Combat->bIsAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	return Combat == nullptr ? nullptr : Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	return Combat == nullptr ? FVector() : Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	return Combat == nullptr ? ECombatState::ECS_MAX : Combat->CombatState;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* OldWeapon)
{
	// Last Weapon
	if(OldWeapon)
	{
		OldWeapon->ShowPickupWidget(false);
	}

	// New Weapon
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::ManualUpdateHUD()
{
	OnHealthUpdated.Broadcast(Health);
	OnMaxHealthUpdated.Broadcast(MaxHealth);
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if(DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	if(DissolveTimeline == nullptr || DissolveCurve == nullptr) return;
	
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
	DissolveTimeline->Play();
}

void ABlasterCharacter::ShowSniperScopeWidget(bool bShowScope)
{
	if(BlasterPlayerController)
	{
		BlasterPlayerController->ShowSniperScopeOverlay(bShowScope);
	}
}

void ABlasterCharacter::Montage_JumpToSection(FName SectionName) const
{
	if(GetMesh() == nullptr) return;
	
	if(UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}
