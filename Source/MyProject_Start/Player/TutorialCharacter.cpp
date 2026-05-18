// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject_Start/Player/TutorialCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject_Start/InteractionInterface.h"
#include "MyProject_Start/MyGameInstance.h"
#include "MyProject_Start/NetworkWorker.h" // ?†Ïå©Í≥§Ïòô
#include "MyProject_Start/KillerCharacter.h"
#include "MyProject_Start/BandagePickup.h"
#include "MyProject_Start/Generator.h"
#include "MyProject_Start/ParkourInteractable.h"
#include "Networking.h"    // ?†Ïå©Í≥§Ïòô
#include "Sockets.h"       // ?†Ïå©Í≥§Ïòô
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

ATutorialCharacter::ATutorialCharacter()
{
    bCanVault = false;
    PrimaryActorTick.bCanEverTick = true;

    // Ïπ¥Âç†?®Îù∞???†Ïèô???†Ïèô?ôÂç†?ôÏòô?†Ïèô?ôÂç†?ôÏòô ?†Ïèô?ôÂç†?ôÏòô
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));
    MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

    SpringArm->SetupAttachment(GetCapsuleComponent());
    Camera->SetupAttachment(SpringArm);

    SpringArm->TargetArmLength = 300.f;
    SpringArm->SetRelativeLocationAndRotation(
        FVector(0.f, 0.f, 80.f),
        FRotator(0.f, 0.f, 0.f)
    );
    SpringArm->bUsePawnControlRotation = true;

    // ?†Ïå®?ôÏòô ?†Ïèô?ôÏπò ?†Ïèô???åÂç†?ôÏòô ?†Ïèô?ôÂç†?ôÏòô
    GetMesh()->SetRelativeLocationAndRotation(
        FVector(0.f, 0.f, -88.f),
        FRotator(0.f, -90.f, 0.f)
    );

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> SM(
        TEXT("/Script/Engine.SkeletalMesh'/Game/Player/Player.Player'")
    );
    if (SM.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(SM.Object);
    }


    static ConstructorHelpers::FObjectFinder<UAnimMontage> NewHitMontageAsset(
        TEXT("/Script/Engine.AnimMontage'/Game/Animation/player/AM_Hit_On_The_Back_Montag.AM_Hit_On_The_Back_Montag'")
    );
    if (NewHitMontageAsset.Succeeded())
    {
        HitMontage = NewHitMontageAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> NewDownedMontageAsset(
        TEXT("/Script/Engine.AnimMontage'/Game/Animation/player/AM_Player_Dying_Anim.AM_Player_Dying_Anim'")
    );
    if (NewDownedMontageAsset.Succeeded())
    {
        DownedMontage = NewDownedMontageAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> HitReactionAnimationAsset(
        TEXT("/Script/Engine.AnimSequence'/Game/Animation/player/Big_Kidney_Hit.Big_Kidney_Hit'")
    );
    if (HitReactionAnimationAsset.Succeeded())
    {
        HitReactionAnimation = HitReactionAnimationAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> DownedAnimationAsset(
        TEXT("/Script/Engine.AnimSequence'/Game/Animation/player/Fallen_Idle.Fallen_Idle'")
    );
    if (DownedAnimationAsset.Succeeded())
    {
        DownedAnimation = DownedAnimationAsset.Object;
    }

    
    static ConstructorHelpers::FObjectFinder<UAnimSequence> RepairAnimationAsset(
        TEXT("/Script/Engine.AnimSequence'/Game/Animation/player/Cow_Milking.Cow_Milking'")
    );
    if (RepairAnimationAsset.Succeeded())
    {
        RepairAnimation = RepairAnimationAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> RepairMontageAsset(
        TEXT("/Script/Engine.AnimMontage'/Game/Animation/player/AM_Repair.AM_Repair'")
    );
    if (RepairMontageAsset.Succeeded())
    {
        RepairMontage = RepairMontageAsset.Object;
    }
    // ?†Ïã±?∏Ïòô ?†Ïèô?ôÂç†?ôÏòô
    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;

    GetCharacterMovement()->RotationRate = FRotator(0.f, 720.0f, 0.f);

    GetCharacterMovement()->MaxAcceleration = 2048.0f;

    GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
    GetCharacterMovement()->GroundFriction = 8.0f;
    GetCharacterMovement()->AirControl = 0.2f;

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    // ------------------------------------



    NetworkWorker = nullptr;
}

void ATutorialCharacter::BeginPlay()
{
        Super::BeginPlay();

    if (UClass* SurvivorAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/BP/ABP_TutorialAnim.ABP_TutorialAnim_C")))
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        GetMesh()->SetAnimInstanceClass(SurvivorAnimClass);
    }

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        ApplyLocalPlayerInputMode();
        RemotePlayers.Empty();
        ShowGeneratorRepairWidget(nullptr);
    }

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        FString ServerIP;
        if (UMyGameInstance* GI = GetGameInstance<UMyGameInstance>())
        {
            if (GI->HasValidatedServerConnection())
            {
                ServerIP = GI->GetServerIP();
            }
        }

        if (ServerIP.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("Skipping survivor network connect because no validated server IP was provided."));
            return;
        }

        NetworkWorker = new FNetworkWorker(ServerIP, FNetworkWorker::GetDefaultServerPort());
        NetworkWorker->SetOwnerCharacter(this);
        FRunnableThread::Create(NetworkWorker, TEXT("NetworkThread"));
    }
}

void ATutorialCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    ApplyLocalPlayerInputMode();
}

void ATutorialCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // ?†Ïèô?ôÂç†?ôÏòô ?†Ïèô?ôÂç†?ôÏòô ?†Ïèô???†Ïèô?ôÂç†?ôÏòô?†Ïç≤Î•??†Ïèô?ôÂç†?ôÏòô?†ÏãπÍ≥§Ïòô ?†Ïèô?ôÂç†?ôÏòô
    if (NetworkWorker)
    {
        NetworkWorker->Stop();
        // ?†Ïå®Î™åÏòô ?†Ïèô?ôÂç†?ôÏòô?†Ïèô???†Ï?Î™åÏòô?†Ïåò?∏Ïòô?†Ïèô??Ï≤òÂç†?ôÏòô?†Ïèô??
    }

    HideGeneratorRepairWidget();

    HideReviveProgressWidget();
    Super::EndPlay(EndPlayReason);
    RemotePlayers.Empty();
    RemoteKillers.Empty();
}

void ATutorialCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!IsInteracting)
    {
        TraceForInteractable();
    }

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        if (!GeneratorRepairWidget.IsValid())
        {
            ShowGeneratorRepairWidget(nullptr);
        }

        SendLocationToServer();
    }

    if (IsInteracting)
    {
        UE_LOG(LogTemp, Log, TEXT("Current State: INTERACTING..."));
    }

    if (IsInteracting && CurrentInteractable)
    {
        AGenerator* Generator = Cast<AGenerator>(CurrentInteractable);
        ATutorialCharacter* DownedSurvivor = Cast<ATutorialCharacter>(CurrentInteractable);
        if (Generator && Generator->bIsRepaired)
        {
            CancelInteraction();
            return;
        }

        if (DownedSurvivor && (!DownedSurvivor->IsDowned || DownedSurvivor->IsBeingCarried))
        {
            StopInteractingWithCurrentTarget(true);
            return;
        }

        const bool bWasGeneratorRepaired = Generator && Generator->bIsRepaired;
        const bool bWasDowned = DownedSurvivor && DownedSurvivor->IsDowned;

        IInteractionInterface::Execute_UpdateInteract(CurrentInteractable, DeltaTime);

        if (Generator)
        {
            UpdateGeneratorRepairWidget(Generator);

            GeneratorRepairSyncAccumulator += DeltaTime;
            if (GeneratorRepairSyncAccumulator >= 0.15f)
            {
                GeneratorRepairSyncAccumulator = 0.0f;
                SendGeneratorActionToServer(ACTION_GENERATOR_START, Generator);
            }
        }

        if (DownedSurvivor && bWasDowned)
        {
            ReviveSyncAccumulator += DeltaTime;
            if (ReviveSyncAccumulator >= 0.15f)
            {
                ReviveSyncAccumulator = 0.0f;
                SendSurvivorActionToServer(DownedSurvivor == this ? ACTION_SELF_REVIVE_START : ACTION_SURVIVOR_REVIVE_START, DownedSurvivor);
            }
        }

        if (Generator && !bWasGeneratorRepaired && Generator->bIsRepaired)
        {
            SendGeneratorActionToServer(ACTION_GENERATOR_COMPLETE, Generator);
            SetRepairingGenerator(false);
            GeneratorRepairSyncAccumulator = 0.0f;
            UpdateGeneratorRepairWidget(Generator);
            ShowGeneratorRepairCount();
            IsInteracting = false;
        }
        else if (DownedSurvivor && bWasDowned && !DownedSurvivor->IsDowned)
        {
            if (DownedSurvivor == this)
            {
                bHasBandage = false;
                SendSurvivorActionToServer(ACTION_SELF_REVIVE_COMPLETE, DownedSurvivor);
            }
            else
            {
                SendSurvivorActionToServer(ACTION_SURVIVOR_REVIVE_COMPLETE, DownedSurvivor);
            }

            StopInteractingWithCurrentTarget(true);
        }
    }

    if (bIsVaultMoving)
    {
        VaultAlpha += DeltaTime * 2.0f;
        if (VaultAlpha >= 1.0f)
        {
            bIsVaultMoving = false;
            bIsVaulting = false;
        }
    }

    if (Controller)
    {
        FRotator ControlRot = Controller->GetControlRotation();
        FRotator ActorRot = GetActorRotation();
        FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
        AimYaw = Delta.Yaw;
        AimPitch = Delta.Pitch;
    }
} 

void ATutorialCharacter::SendLocationToServer()
{
    if (MyPlayerId == -1) return;

    if (NetworkWorker && NetworkWorker->GetSocket())
    {
        FPacketMove MovePkt;
        MovePkt.Type = PKT_MOVE;
        MovePkt.Data.PlayerId = MyPlayerId;
        MovePkt.Data.CharacterType = CHARACTER_SURVIVOR;

        // 1. ?Íæ©ÌäÇ Ë´????üæ ?Î™ÖÎòø
        FVector CurLocation = GetActorLocation();
        MovePkt.Data.X = CurLocation.X;
        MovePkt.Data.Y = CurLocation.Y;
        MovePkt.Data.Z = CurLocation.Z;
        MovePkt.Data.RotationYaw = GetActorRotation().Yaw;

        // 2. [??èÏ†ô???∫¬Ä?? ËπÇÎì≠?????æÎ£Ñ ?®Íæ©Í∂??Ôßû¬Ä?Í≥åÌÄ? ??ÖÏ†∞Â™õÎ???Ê¥πÎ™É?Êø??ÔΩåÎíø??àÎñé!
        MovePkt.Data.ForwardValue = MoveForwardValue;
        MovePkt.Data.RightValue = MoveRightValue;

        // 3. ???ÅÊπ≤??Í≥πÍπ≠ ?Î™ÖÎòø (Êπ≤Í≥ó???ÑÎ∂æÎ±??Ï¢?)
        MovePkt.Data.bIsSprinting = (GetCharacterMovement()->MaxWalkSpeed > 350.0f);
        MovePkt.Data.CurrentHealth = CurrentHealth;
        MovePkt.Data.bIsDowned = IsDowned;
        MovePkt.Data.bIsBeingCarried = IsBeingCarried;


        // 4. ??ïÏæ≠Êø??Íæ©ÎÑö
        int32 BytesSent = 0;
        NetworkWorker->GetSocket()->Send((uint8*)&MovePkt, sizeof(FPacketMove), BytesSent);
    }
}

void ATutorialCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATutorialCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATutorialCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("TurnCamera"), this, &ATutorialCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ATutorialCharacter::LookUp);

    PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ATutorialCharacter::BeginCrouch);
    PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ATutorialCharacter::EndCrouch);
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ATutorialCharacter::BeginSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ATutorialCharacter::EndSprint);
    PlayerInputComponent->BindAction("Vault", IE_Pressed, this, &ATutorialCharacter::TryVault);
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ATutorialCharacter::StartInteraction);
    PlayerInputComponent->BindAction("Interact", IE_Released, this, &ATutorialCharacter::CancelInteraction);
}

void ATutorialCharacter::MoveForward(float Value)
{
    MoveForwardValue = Value;
    if (Controller && Value != 0.0f)
    {
        FRotator Rotation = Controller->GetControlRotation();
        FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
        FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void ATutorialCharacter::MoveRight(float Value)
{
    MoveRightValue = Value;
    if (Controller && Value != 0.0f)
    {
        FRotator Rotation = Controller->GetControlRotation();
        FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
        FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
    }
}

void ATutorialCharacter::BeginSprint()
{
    // ?†Ïèô?ôÂç†?ôÏòô 1000?†Ïèô?ôÂç†?ôÏòô 150 ?†Ïèô?ôÂç†?ôÏòô?†Ïèô??850 ?†Ïèô?ôÂç†?ôÏòô
    GetCharacterMovement()->MaxWalkSpeed = 350.0f;
}

void ATutorialCharacter::EndSprint()
{
    // ?†Ïèô?ôÂç†?ôÏòô 600?†Ïèô?ôÂç†?ôÏòô 150 ?†Ïèô?ôÂç†?ôÏòô?†Ïèô??450 ?†Ïèô?ôÂç†?ôÏòô
    GetCharacterMovement()->MaxWalkSpeed = 350.0f;
}

void ATutorialCharacter::Turn(float Value) { AddControllerYawInput(Value); }
void ATutorialCharacter::LookUp(float Value) { AddControllerPitchInput(Value); }

void ATutorialCharacter::BeginCrouch() { Crouch(); }
void ATutorialCharacter::EndCrouch() { UnCrouch(); }


void ATutorialCharacter::TryVault()
{
    if (!CanStartVault()) return;

    FVector Start = GetActorLocation() + FVector(0, 0, 10);
    FVector End = Start + GetActorForwardVector() * 150;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 2.0f);

    if (bHit)
    {
        FVector TopStart = Hit.ImpactPoint + FVector(0, 0, 100);
        FVector TopEnd = TopStart + FVector(0, 0, 50);
        FHitResult TopHit;

        FVector Forward = GetActorForwardVector();
        FVector TargetLocation = Hit.ImpactPoint + Forward * 100;
        TargetLocation.Z += 50;

        bool bTopBlocked = GetWorld()->LineTraceSingleByChannel(
            TopHit, TopStart, TopEnd, ECC_Visibility, Params
        );

        if (!bTopBlocked)
        {
            BeginVaultToLocation(TargetLocation);
        }
    }
}

bool ATutorialCharacter::CanStartVault() const
{
    return !bIsVaulting && !IsDowned && !IsBeingCarried;
}

void ATutorialCharacter::StartVaultFromInteractable(AParkourInteractable* ParkourInteractable)
{
    if (!CanStartVault() || !ParkourInteractable)
    {
        return;
    }

    if (!MotionWarping)
    {
        UE_LOG(LogTemp, Warning, TEXT("Parkour failed: MotionWarping component is missing."));
        return;
    }

    if (!VaultMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Parkour failed: VaultMontage is not assigned."));
        return;
    }

    BeginVaultToLocation(ParkourInteractable->GetVaultTargetLocation());
}

void ATutorialCharacter::BeginVaultToLocation(const FVector& TargetLocation)
{
    if (!MotionWarping || !VaultMontage)
    {
        return;
    }

    VaultTargetLocation = TargetLocation;
    bIsVaulting = true;
    VaultAlpha = 0.0f;
    bIsVaultMoving = true;

    GetCharacterMovement()->StopMovementImmediately();
    MotionWarping->AddOrUpdateWarpTargetFromLocation(FName("VaultTarget"), VaultTargetLocation);

    const float PlayedLength = PlayAnimMontage(VaultMontage);
    const float VaultDuration = PlayedLength > 0.0f ? PlayedLength : 0.8f;

    GetWorldTimerManager().ClearTimer(VaultFinishTimerHandle);
    GetWorldTimerManager().SetTimer(
        VaultFinishTimerHandle,
        this,
        &ATutorialCharacter::FinishVault,
        VaultDuration,
        false
    );
}

void ATutorialCharacter::FinishVault()
{
    SetActorLocation(VaultTargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
    bIsVaultMoving = false;
    bIsVaulting = false;
    VaultAlpha = 0.0f;
}

void ATutorialCharacter::TraceForInteractable()
{
    if (!Camera) return;

    FHitResult Hit;
    FVector StartLocation = Camera->GetComponentLocation();
    FVector ForwardVector = Camera->GetForwardVector();

    // ??ºÏ†£ Ë≠∞Í≥ó???Ôß§Ïíñ??Î∂æÎßÇ ??ÍµÖÁî±?? ??ñÏòâ Ôßû¬Ä??
    FVector TraceStart = StartLocation;
    FVector TraceEnd = TraceStart + (ForwardVector * 1500.f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // ?Î®?†ô Ë∏∞Î∂ø?ûÁëú???∞ÍºÆ??çÏæ∂ ??èÎø¨ ?Íæ©Ïçô??àÎíó ???Í≥∑Î£Ñ ????´Ïó≥?Íæ®Ï§â ?Ï¢? (Ë´õÏÑè???40cm)
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(90.f);

    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByChannel(
        Hits, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, SphereShape, Params
    );

    AActor* NewInteractable = nullptr;

    for (const FHitResult& CandidateHit : Hits)
    {
        AActor* HitActor = CandidateHit.GetActor();
        if (!HitActor || HitActor == this)
        {
            continue;
        }

        if (!Cast<IInteractionInterface>(HitActor))
        {
            continue;
        }

        if (ATutorialCharacter* TargetChar = Cast<ATutorialCharacter>(HitActor))
        {
            if (TargetChar->IsDowned)
            {
                NewInteractable = HitActor;
                break;
            }
        }
        else
        {
            NewInteractable = HitActor;
            break;
        }
    }

    if (!NewInteractable)
    {
        TArray<AActor*> FoundSurvivors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATutorialCharacter::StaticClass(), FoundSurvivors);

        const FVector SearchOrigin = Camera->GetComponentLocation();
        const FVector SearchForward = Camera->GetForwardVector().GetSafeNormal();
        constexpr float MaxReviveInteractDistance = 500.f;
        constexpr float MinReviveFacingDot = -0.15f;

        ATutorialCharacter* BestDownedSurvivor = nullptr;
        float BestScore = TNumericLimits<float>::Max();

        for (AActor* Actor : FoundSurvivors)
        {
            ATutorialCharacter* Survivor = Cast<ATutorialCharacter>(Actor);
            if (!IsValid(Survivor) || Survivor == this || !Survivor->IsDowned || Survivor->IsBeingCarried)
            {
                continue;
            }

            FVector SurvivorTargetLocation = Survivor->GetActorLocation();
            SurvivorTargetLocation.Z += 50.0f;

            const FVector ToSurvivor = SurvivorTargetLocation - SearchOrigin;
            const float Distance = ToSurvivor.Size();
            if (Distance > MaxReviveInteractDistance)
            {
                continue;
            }

            const float FacingDot = FVector::DotProduct(SearchForward, ToSurvivor.GetSafeNormal());
            if (FacingDot < MinReviveFacingDot)
            {
                continue;
            }

            const float Score = Distance - (FacingDot * 250.f);
            if (Score < BestScore)
            {
                BestScore = Score;
                BestDownedSurvivor = Survivor;
            }
        }

        NewInteractable = BestDownedSurvivor;
    }

    if (!NewInteractable)
    {
        TArray<AActor*> FoundGenerators;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGenerator::StaticClass(), FoundGenerators);

        const FVector SearchOrigin = Camera->GetComponentLocation();
        const FVector SearchForward = Camera->GetForwardVector().GetSafeNormal();
        constexpr float MaxGeneratorInteractDistance = 650.f;
        constexpr float MinGeneratorFacingDot = 0.25f;

        AGenerator* BestGenerator = nullptr;
        float BestScore = TNumericLimits<float>::Max();

        for (AActor* Actor : FoundGenerators)
        {
            AGenerator* Generator = Cast<AGenerator>(Actor);
            if (!IsValid(Generator) || Generator->bIsRepaired)
            {
                continue;
            }

            const FVector ToGenerator = Generator->GetActorLocation() - SearchOrigin;
            const float Distance = ToGenerator.Size();
            if (Distance > MaxGeneratorInteractDistance)
            {
                continue;
            }

            const float FacingDot = FVector::DotProduct(SearchForward, ToGenerator.GetSafeNormal());
            if (FacingDot < MinGeneratorFacingDot)
            {
                continue;
            }

            const float Score = Distance - (FacingDot * 200.f);
            if (Score < BestScore)
            {
                BestScore = Score;
                BestGenerator = Generator;
            }
        }

        NewInteractable = BestGenerator;
    }
    // ?Í≥πÏÉá?Î¨íÏäú ???Í≥∏Ïî† Ë´õÎ∂æ?????????ìΩ Ôß£ÏÑé??
    if (CurrentInteractable != NewInteractable)
    {
        if (IsInteracting && CurrentInteractable)
        {
            AGenerator* Generator = Cast<AGenerator>(CurrentInteractable);
            ATutorialCharacter* DownedSurvivor = Cast<ATutorialCharacter>(CurrentInteractable);
            IInteractionInterface::Execute_CancelInteract(CurrentInteractable);
            IsInteracting = false;
            SetRepairingGenerator(false);
            SetRevivingSurvivor(false);
            GeneratorRepairSyncAccumulator = 0.0f;
            UpdateGeneratorRepairWidget(Generator);

            if (Generator)
            {
                SendGeneratorActionToServer(ACTION_GENERATOR_CANCEL, Generator);
            }
            else if (DownedSurvivor)
            {
                SendSurvivorActionToServer(ACTION_SURVIVOR_REVIVE_CANCEL, DownedSurvivor);
            }
        }
        CurrentInteractable = NewInteractable;
    }
}

void ATutorialCharacter::StartInteraction()
{
    if (IsBeingCarried)
    {
        return;
    }

    if (IsDowned)
    {
        if (!bHasBandage)
        {
            UE_LOG(LogTemp, Warning, TEXT("Self revive requires a bandage."));
            return;
        }

        CurrentInteractable = this;
        IInteractionInterface::Execute_StartInteract(this, this);
        IsInteracting = true;
        ReviveSyncAccumulator = 0.0f;
        SendSurvivorActionToServer(ACTION_SELF_REVIVE_START, this);
        return;
    }

    TraceForInteractable();

    if (!CurrentInteractable)
    {
        UE_LOG(LogTemp, Warning, TEXT("No interactable found."));
        return;
    }

    if (CurrentInteractable)
    {
        if (ABandagePickup* BandagePickup = Cast<ABandagePickup>(CurrentInteractable))
        {
            if (bHasBandage)
            {
                UE_LOG(LogTemp, Warning, TEXT("You already have a bandage."));
                return;
            }

            IInteractionInterface::Execute_StartInteract(CurrentInteractable, this);
            SendBandageActionToServer(ACTION_BANDAGE_PICKUP, BandagePickup);
            CurrentInteractable = nullptr;
            return;
        }

        if (AParkourInteractable* ParkourInteractable = Cast<AParkourInteractable>(CurrentInteractable))
        {
            StartVaultFromInteractable(ParkourInteractable);
            return;
        }
        // [?∞Î∂Ω???Êø°ÏíñÏ≠? ??Ä? ??é‚îÅ??Ë´õÏíñ?æÊπ≤Í≥óÏî§Ôßû¬Ä ?Î∫§Ïî§
        if (AGenerator* Generator = Cast<AGenerator>(CurrentInteractable))
        {
            if (Generator->bIsRepaired)
            {
                UE_LOG(LogTemp, Warning, TEXT("This generator is already done!"));
                return; // ??•Îãî ?´ÎÇÖÏ¶?
            }
        }

        IInteractionInterface::Execute_StartInteract(CurrentInteractable, this);
        IsInteracting = true;

        if (AGenerator* Generator = Cast<AGenerator>(CurrentInteractable))
        {
            SetRepairingGenerator(true);
            UpdateGeneratorRepairWidget(Generator);
            GeneratorRepairSyncAccumulator = 0.0f;
            SendGeneratorActionToServer(ACTION_GENERATOR_START, Generator);
        }
        else if (ATutorialCharacter* DownedSurvivor = Cast<ATutorialCharacter>(CurrentInteractable))
        {
            SetRevivingSurvivor(true);
            ReviveSyncAccumulator = 0.0f;
            SendSurvivorActionToServer(ACTION_SURVIVOR_REVIVE_START, DownedSurvivor);
        }
    }
}

void ATutorialCharacter::CancelInteraction()
{
    if (IsInteracting && CurrentInteractable)
    {
        AGenerator* Generator = Cast<AGenerator>(CurrentInteractable);
        ATutorialCharacter* DownedSurvivor = Cast<ATutorialCharacter>(CurrentInteractable);
        IInteractionInterface::Execute_CancelInteract(CurrentInteractable);
        IsInteracting = false;
        SetRepairingGenerator(false);
        SetRevivingSurvivor(false);
        GeneratorRepairSyncAccumulator = 0.0f;
        UpdateGeneratorRepairWidget(Generator);

        if (Generator)
        {
            SendGeneratorActionToServer(ACTION_GENERATOR_CANCEL, Generator);
        }
        else if (DownedSurvivor)
        {
            SendSurvivorActionToServer(DownedSurvivor == this ? ACTION_SELF_REVIVE_CANCEL : ACTION_SURVIVOR_REVIVE_CANCEL, DownedSurvivor);
        }
    }
}


void ATutorialCharacter::SetRepairingGenerator(bool bRepairing)
{
    if (bIsRepairingGenerator == bRepairing)
    {
        return;
    }

    bIsRepairingGenerator = bRepairing;

    if (bIsRepairingGenerator)
    {
        PlayRepairMontageStart();
    }
    else
    {
        PlayRepairMontageEnd();
    }
}
void ATutorialCharacter::SendGeneratorActionToServer(uint8 ActionType, AGenerator* Generator)
{
    if (!Generator || !NetworkWorker || !NetworkWorker->GetSocket() || MyPlayerId == -1) return;

    FPacketAction ActionPkt;
    ActionPkt.Type = PKT_ACTION;
    ActionPkt.ActionType = ActionType;
    ActionPkt.InstigatorId = MyPlayerId;
    ActionPkt.TargetId = Generator->GetGeneratorId();

    const FVector Loc = Generator->GetActorLocation();
    ActionPkt.X = Loc.X;
    ActionPkt.Y = Loc.Y;
    ActionPkt.Z = Loc.Z;
    ActionPkt.RotationYaw = Generator->GetRepairProgress();

    int32 BytesSent = 0;
    NetworkWorker->GetSocket()->Send((uint8*)&ActionPkt, sizeof(FPacketAction), BytesSent);
}

void ATutorialCharacter::SendBandageActionToServer(uint8 ActionType, ABandagePickup* BandagePickup)
{
    if (!BandagePickup || !NetworkWorker || !NetworkWorker->GetSocket() || MyPlayerId == -1) return;

    FPacketAction ActionPkt;
    ActionPkt.Type = PKT_ACTION;
    ActionPkt.ActionType = ActionType;
    ActionPkt.InstigatorId = MyPlayerId;
    ActionPkt.TargetId = BandagePickup->GetBandageId();

    const FVector Loc = BandagePickup->GetActorLocation();
    ActionPkt.X = Loc.X;
    ActionPkt.Y = Loc.Y;
    ActionPkt.Z = Loc.Z;
    ActionPkt.RotationYaw = 0.0f;

    int32 BytesSent = 0;
    NetworkWorker->GetSocket()->Send((uint8*)&ActionPkt, sizeof(FPacketAction), BytesSent);
}

void ATutorialCharacter::SendSurvivorActionToServer(uint8 ActionType, ATutorialCharacter* TargetCharacter)
{
    if (!TargetCharacter || !NetworkWorker || !NetworkWorker->GetSocket() || MyPlayerId == -1) return;

    FPacketAction ActionPkt;
    ActionPkt.Type = PKT_ACTION;
    ActionPkt.ActionType = ActionType;
    ActionPkt.InstigatorId = MyPlayerId;
    ActionPkt.TargetId = TargetCharacter->MyPlayerId;

    const FVector Loc = TargetCharacter->GetActorLocation();
    ActionPkt.X = Loc.X;
    ActionPkt.Y = Loc.Y;
    ActionPkt.Z = Loc.Z;
    if (ActionType == ACTION_SURVIVOR_REVIVE_START || ActionType == ACTION_SURVIVOR_REVIVE_CANCEL || ActionType == ACTION_SURVIVOR_REVIVE_COMPLETE ||
        ActionType == ACTION_SELF_REVIVE_START || ActionType == ACTION_SELF_REVIVE_CANCEL || ActionType == ACTION_SELF_REVIVE_COMPLETE)
    {
        ActionPkt.RotationYaw = FMath::Clamp(TargetCharacter->RecoveryProgress, 0.f, 1.f);
    }
    else
    {
        ActionPkt.RotationYaw = GetActorRotation().Yaw;
    }

    int32 BytesSent = 0;
    NetworkWorker->GetSocket()->Send((uint8*)&ActionPkt, sizeof(FPacketAction), BytesSent);
}

void ATutorialCharacter::ShowGeneratorRepairCount() const
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> FoundGenerators;
    UGameplayStatics::GetAllActorsOfClass(World, AGenerator::StaticClass(), FoundGenerators);

    int32 TotalGenerators = 0;
    int32 RepairedGenerators = 0;

    for (AActor* Actor : FoundGenerators)
    {
        AGenerator* Generator = Cast<AGenerator>(Actor);
        if (!Generator) continue;

        ++TotalGenerators;
        if (Generator->bIsRepaired)
        {
            ++RepairedGenerators;
        }
    }

    const FString Message = FString::Printf(TEXT("Generator Repaired: %d/%d"), TotalGenerators, RepairedGenerators);
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(3001, 4.0f, FColor::Yellow, Message);
    }
}

void ATutorialCharacter::ShowGeneratorRepairWidget(AGenerator* Generator)
{
    if (!IsPlayerControlled() || !IsLocallyControlled() || !GEngine || !GEngine->GameViewport)
    {
        return;
    }

    if (!GeneratorRepairBackgroundTexture)
    {
        GeneratorRepairBackgroundTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/1_Map/UI1.UI1"));
    }

    if (!GeneratorRepairFillMaterial)
    {
        GeneratorRepairFillMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/1_Map/C_Progressbar.C_Progressbar"));
    }

    GeneratorRepairBackgroundBrush = FSlateBrush();
    GeneratorRepairBackgroundBrush.SetResourceObject(GeneratorRepairBackgroundTexture);
    GeneratorRepairBackgroundBrush.ImageSize = FVector2D(372.f, 425.f);
    GeneratorRepairBackgroundBrush.DrawAs = ESlateBrushDrawType::Image;

    if (GeneratorRepairFillMaterial && !GeneratorRepairFillMID1)
    {
        GeneratorRepairFillMID1 = UMaterialInstanceDynamic::Create(GeneratorRepairFillMaterial, this);
        GeneratorRepairFillMID2 = UMaterialInstanceDynamic::Create(GeneratorRepairFillMaterial, this);
        GeneratorRepairFillMID3 = UMaterialInstanceDynamic::Create(GeneratorRepairFillMaterial, this);
    }

    GeneratorRepairFillBrush1 = FSlateBrush();
    GeneratorRepairFillBrush1.SetResourceObject(GeneratorRepairFillMID1);
    GeneratorRepairFillBrush1.ImageSize = FVector2D(48.f, 48.f);
    GeneratorRepairFillBrush1.DrawAs = ESlateBrushDrawType::Image;

    GeneratorRepairFillBrush2 = FSlateBrush();
    GeneratorRepairFillBrush2.SetResourceObject(GeneratorRepairFillMID2);
    GeneratorRepairFillBrush2.ImageSize = FVector2D(48.f, 48.f);
    GeneratorRepairFillBrush2.DrawAs = ESlateBrushDrawType::Image;

    GeneratorRepairFillBrush3 = FSlateBrush();
    GeneratorRepairFillBrush3.SetResourceObject(GeneratorRepairFillMID3);
    GeneratorRepairFillBrush3.ImageSize = FVector2D(48.f, 48.f);
    GeneratorRepairFillBrush3.DrawAs = ESlateBrushDrawType::Image;

    if (!GeneratorRepairWidget.IsValid())
    {
        SAssignNew(GeneratorRepairWidget, SConstraintCanvas)
            + SConstraintCanvas::Slot()
            .Anchors(FAnchors(0.f, 0.5f))
            .Alignment(FVector2D(0.f, 0.5f))
            .Offset(FMargin(24.f, 0.f, 372.f, 425.f))
            .AutoSize(true)
            [
                SNew(SBox)
                .WidthOverride(372.f)
                .HeightOverride(425.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    .ZOrder(0)
                    [
                        SNew(SImage)
                        .Image(&GeneratorRepairBackgroundBrush)
                    ]
                    + SOverlay::Slot()
                    .ZOrder(10)
                    [
                        SNew(SConstraintCanvas)
                        + SConstraintCanvas::Slot()
                        .Offset(FMargin(300.f, 139.f, 48.f, 48.f))
                        [
                            SAssignNew(GeneratorRepairProgressImage1, SImage)
                            .Image(&GeneratorRepairFillBrush1)
                        ]
                        + SConstraintCanvas::Slot()
                        .Offset(FMargin(300.f, 264.f, 48.f, 48.f))
                        [
                            SAssignNew(GeneratorRepairProgressImage2, SImage)
                            .Image(&GeneratorRepairFillBrush2)
                        ]
                        + SConstraintCanvas::Slot()
                        .Offset(FMargin(300.f, 373.f, 48.f, 48.f))
                        [
                            SAssignNew(GeneratorRepairProgressImage3, SImage)
                            .Image(&GeneratorRepairFillBrush3)
                        ]
                    ]
                ]
            ];

        GEngine->GameViewport->AddViewportWidgetContent(GeneratorRepairWidget.ToSharedRef(), 60);
    }

    UpdateGeneratorRepairWidget(Generator);
}

void ATutorialCharacter::UpdateGeneratorRepairWidget(AGenerator* Generator)
{
    if (!GeneratorRepairWidget.IsValid())
    {
        return;
    }

    if (Generator)
    {
        const int32 ActiveGeneratorIndex = FMath::Clamp(Generator->GetGeneratorId(), 0, 2);
        GeneratorRepairProgressValues[ActiveGeneratorIndex] = FMath::Clamp(Generator->GetRepairProgress(), 0.f, 1.f);
    }

    if (GeneratorRepairFillMID1)
    {
        GeneratorRepairFillMID1->SetScalarParameterValue(TEXT("Progress"), GeneratorRepairProgressValues[0]);
    }

    if (GeneratorRepairFillMID2)
    {
        GeneratorRepairFillMID2->SetScalarParameterValue(TEXT("Progress"), GeneratorRepairProgressValues[1]);
    }

    if (GeneratorRepairFillMID3)
    {
        GeneratorRepairFillMID3->SetScalarParameterValue(TEXT("Progress"), GeneratorRepairProgressValues[2]);
    }
}

void ATutorialCharacter::HideGeneratorRepairWidget()
{
    if (GeneratorRepairWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(GeneratorRepairWidget.ToSharedRef());
    }

    GeneratorRepairWidget.Reset();
    GeneratorRepairProgressImage1.Reset();
    GeneratorRepairProgressImage2.Reset();
    GeneratorRepairProgressImage3.Reset();
    GeneratorRepairText.Reset();
}

void ATutorialCharacter::ShowReviveProgressWidget()
{
    if (!IsPlayerControlled() || !IsLocallyControlled() || !GEngine || !GEngine->GameViewport)
    {
        return;
    }

    if (ReviveProgressWidget.IsValid())
    {
        return;
    }

    SAssignNew(ReviveProgressWidget, SConstraintCanvas)
        + SConstraintCanvas::Slot()
        .Anchors(FAnchors(0.5f, 0.78f))
        .Alignment(FVector2D(0.5f, 0.5f))
        .Offset(FMargin(0.f, 0.f, 380.f, 56.f))
        .AutoSize(true)
        [
            SNew(SBox)
            .WidthOverride(380.f)
            .HeightOverride(56.f)
            [
                SNew(SBorder)
                .Padding(FMargin(12.f, 8.f))
                .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.72f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SAssignNew(ReviveProgressText, STextBlock)
                        .Text(FText::FromString(TEXT("REVIVE 0%")))
                        .ColorAndOpacity(FLinearColor::White)
                        .Justification(ETextJustify::Center)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0.f, 8.f, 0.f, 0.f))
                    [
                        SNew(SBox)
                        .HeightOverride(18.f)
                        [
                            SAssignNew(ReviveProgressBar, SProgressBar)
                            .Percent(0.f)
                            .FillColorAndOpacity(FLinearColor(0.1f, 0.78f, 1.f, 1.f))
                        ]
                    ]
                ]
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(ReviveProgressWidget.ToSharedRef(), 100);
}

void ATutorialCharacter::UpdateReviveProgressWidget(float Progress)
{
    if (!ReviveProgressWidget.IsValid())
    {
        ShowReviveProgressWidget();
    }

    const float ClampedProgress = FMath::Clamp(Progress, 0.f, 1.f);

    if (ReviveProgressBar.IsValid())
    {
        ReviveProgressBar->SetPercent(ClampedProgress);
    }

    if (ReviveProgressText.IsValid())
    {
        const int32 Percent = FMath::RoundToInt(ClampedProgress * 100.f);
        ReviveProgressText->SetText(FText::FromString(FString::Printf(TEXT("REVIVE %d%%"), Percent)));
    }
}

void ATutorialCharacter::HideReviveProgressWidget()
{
    if (ReviveProgressWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(ReviveProgressWidget.ToSharedRef());
    }

    ReviveProgressWidget.Reset();
    ReviveProgressBar.Reset();
    ReviveProgressText.Reset();
}
AGenerator* ATutorialCharacter::FindGeneratorForNetworkAction(int32 GeneratorId, const FVector& Location) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    TArray<AActor*> FoundGenerators;
    UGameplayStatics::GetAllActorsOfClass(World, AGenerator::StaticClass(), FoundGenerators);

    AGenerator* NearestGenerator = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();

    for (AActor* Actor : FoundGenerators)
    {
        AGenerator* Generator = Cast<AGenerator>(Actor);
        if (!Generator) continue;

        if (Generator->GetGeneratorId() == GeneratorId)
        {
            return Generator;
        }

        const float DistanceSq = FVector::DistSquared(Generator->GetActorLocation(), Location);
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            NearestGenerator = Generator;
        }
    }

    return NearestGenerator;
}

ABandagePickup* ATutorialCharacter::FindBandageForNetworkAction(int32 BandageId, const FVector& Location) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    TArray<AActor*> FoundBandages;
    UGameplayStatics::GetAllActorsOfClass(World, ABandagePickup::StaticClass(), FoundBandages);

    ABandagePickup* NearestBandage = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();

    for (AActor* Actor : FoundBandages)
    {
        ABandagePickup* BandagePickup = Cast<ABandagePickup>(Actor);
        if (!BandagePickup) continue;

        if (BandagePickup->GetBandageId() == BandageId)
        {
            return BandagePickup;
        }

        const float DistanceSq = FVector::DistSquared(BandagePickup->GetActorLocation(), Location);
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            NearestBandage = BandagePickup;
        }
    }

    return NearestBandage;
}

void ATutorialCharacter::ApplyGeneratorNetworkAction(uint8 ActionType, int32 GeneratorId, const FVector& Location, float RepairProgress)
{
    AGenerator* Generator = FindGeneratorForNetworkAction(GeneratorId, Location);
    if (!Generator) return;

    if (ActionType == ACTION_GENERATOR_START)
    {
        Generator->ApplyNetworkRepairState(true, false, RepairProgress);
        UpdateGeneratorRepairWidget(Generator);
    }
    else if (ActionType == ACTION_GENERATOR_CANCEL)
    {
        Generator->ApplyNetworkRepairState(false, false, RepairProgress);
        UpdateGeneratorRepairWidget(Generator);
    }
    else if (ActionType == ACTION_GENERATOR_COMPLETE)
    {
        const bool bWasRepaired = Generator->bIsRepaired;
        Generator->ApplyNetworkRepairState(false, true, 1.f);
        UpdateGeneratorRepairWidget(Generator);
        if (!bWasRepaired)
        {
            ShowGeneratorRepairCount();
        }
    }
}

void ATutorialCharacter::ApplyBandageNetworkAction(int32 BandageId, const FVector& Location)
{
    ABandagePickup* BandagePickup = FindBandageForNetworkAction(BandageId, Location);
    if (!BandagePickup) return;

    BandagePickup->ApplyCollectedState();
}
void ATutorialCharacter::PlayHitReaction()
{
    ApplyHitReaction(true);
}

void ATutorialCharacter::PlayNetworkHitReaction()
{
    ApplyHitReaction(false);
}

void ATutorialCharacter::ForceDownedState()
{
    if (IsInteracting)
    {
        CancelInteraction();
    }

    CurrentHealth = 0;
    IsDowned = true;
    bIsBeingRevived = false;
    bIsSelfReviving = false;
    RecoveryProgress = 0.0f;
    CurrentReviver = nullptr;
    bCanBeHit = false;

    float DownedMontageLength = 0.0f;
    if (DownedMontage)
    {
        DownedMontageLength = PlayAnimMontage(DownedMontage);
    }

    if (DownedAnimation)
    {
        if (DownedMontageLength > 0.0f)
        {
            FTimerHandle DownedLoopTimer;
            GetWorldTimerManager().SetTimer(
                DownedLoopTimer,
                FTimerDelegate::CreateLambda([this]()
                    {
                        if (IsValid(this) && IsDowned && !IsBeingCarried)
                        {
                            PlayLoopBodyAnimation(DownedAnimation);
                        }
                    }),
                DownedMontageLength,
                false
            );
        }
        else
        {
            PlayLoopBodyAnimation(DownedAnimation);
        }
    }

    GetCapsuleComponent()->SetCapsuleHalfHeight(30.0f);
    GetCharacterMovement()->DisableMovement();
    bUseControllerRotationYaw = false;

    UE_LOG(LogTemp, Error, TEXT("Survivor entered the downed state."));
}

void ATutorialCharacter::ForceInjuredState()
{
    if (IsInteracting)
    {
        CancelInteraction();
    }

    CurrentHealth = 1;
    IsDowned = false;
    IsBeingCarried = false;
    if (bIsSelfReviving)
    {
        UpdateReviveProgressWidget(1.0f);
        HideReviveProgressWidget();
    }
    else if (CurrentReviver.IsValid())
    {
        CurrentReviver->UpdateReviveProgressWidget(1.0f);
        CurrentReviver->HideReviveProgressWidget();
    }

    bIsBeingRevived = false;
    bIsSelfReviving = false;
    RecoveryProgress = 0.0f;
    CurrentReviver = nullptr;
    bCanBeHit = true;

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    RestoreBodyAnimClass();

    if (DownedMontage)
    {
        StopAnimMontage(DownedMontage);
    }

    GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    UE_LOG(LogTemp, Warning, TEXT("Survivor entered the injured state."));
}
void ATutorialCharacter::ApplyHitReaction(bool bRespectCooldown)
{
    if (IsDowned) return;
    if (bRespectCooldown && !bCanBeHit) return;

    bCanBeHit = false;
    CurrentHealth--;
    UE_LOG(LogTemp, Warning, TEXT("Survivor took damage. Current health: %d"), CurrentHealth);

    if (CurrentHealth > 0)
    {
        if (HitMontage)
        {
            PlayAnimMontage(HitMontage);
        }
        else if (HitReactionAnimation)
        {
            PlayTemporaryBodyAnimation(HitReactionAnimation);
        }

        FTimerHandle HitCooldownTimer;
        GetWorldTimerManager().SetTimer(HitCooldownTimer, FTimerDelegate::CreateLambda([this]()
            {
                bCanBeHit = true;
                UE_LOG(LogTemp, Log, TEXT("?†Ïèô?ôÂç†?ôÏòô?†Ïèô???†Ïèô?ôÂç†?ôÏòô ?†Ïåï?ôÏòô ?†Ïèô?ôÂç†?ôÏòô ?†Ïèô???†Ïèô?ôÂç†?ôÏòô"));
            }), 1.0f, false);
    }
    else
    {
        ForceDownedState();

    }
}

void ATutorialCharacter::UpdateRemotePlayer(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint, int32 InHealth, bool bInDowned, bool bInBeingCarried)
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    if (!IsValid(this)) return;

    // 1. ?†Ïã≠?∏Ïòô ?†Ïåî?êÏòô ID?†Ïèô???†Ïåç?êÏòô?†Ïèô???ïÂç†?ôÏòô
    if (RemotePlayers.Contains(PlayerId))
    {
        // ?†Ïèô?ôÂç†?ôÏòô?†ÏãπÍ≥§Ïòô Ï∫êÂç†?ôÏòô?†Ïã∂Î§ÑÏòô ?†Ïèô?ôÂç†?ôÏòô??
        ATutorialCharacter* Target = Cast<ATutorialCharacter>(RemotePlayers[PlayerId]);

        // [?†Ïèô?ôÂç†?ôÏòô] Target?†Ïèô???†Ïå®Î™®Î¶¨?∏Ïòô ?†Ïèô?ôÂç†?ôÏòôÊÆ?∂¨??†ÏãπÎ™åÏòô ?†Ïèô?ôÂç†?ôÏòô?†Ïèô?ôÌä∏ ?†Ïèô?ôÂç†?ôÏòô
        if (IsValid(Target))
        {
            Target->SetActorLocation(Location);
            Target->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));

            Target->RemoteForwardValue = Forward;
            Target->RemoteRightValue = Right;
            Target->RemoteIsSprinting = bSprint;
            Target->SyncRemoteState(InHealth, bInDowned, bInBeingCarried);

            // ?†Ïèô?ôÂç†?©ÏÑú ?†Ïåâ?ΩÏòô ?†Ïèô?ôÂç†?ôÏòô (?†Ïèô?ôÂç†?ôÏòô?†Ïèô?ôÂç†?ôÏòô?†Ïèô???†Ïèô?ôÂç†?ôÏòô?†Ïèô?ôÌä∏?†Ïèô??
            return;
        }
        else
        {
            // ?†Ïèô?ôÂç†?ôÏòô?†Ïèô???†ÏãúÎ§ÑÏòô?†Ïèô???†Ïèô?ôÂç†?ôÏòô?†Ïèô???†Ïèô?ôÂç†?ôÏòô?†Ïèô?ôÊë∏???†Ïã≠?∏Ïòô?†Ïèô???†Ïèô?ôÂç†?ôÏòô
            RemotePlayers.Remove(PlayerId);
        }
    }

    // 2. ?†Ïã≠?∏Ïòô ?†Ïèô?ôÂç†?†ÎÜÇ??Target?†Ïèô???†Ïèô?ôÌö®?†Ïèô?ôÂç†?ôÏòô ?†Ïèô?ôÂç†?ôÏòô ?†Ïèô?ôËçë?ΩÏòô???†Ïèô?ôÂç†?ôÏòô (?†Ïèô?ôÂç†?©Í? else ?†Ïèô?ôÂç†?ôÏòô ?†Ïèô?ôÂç†?ôÏòô ?†Ïèô??
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATutorialCharacter* NewPlayer = World->SpawnActor<ATutorialCharacter>(GetClass(), Location, FRotator(0.0f, RotationYaw, 0.0f), SpawnParams);

    if (NewPlayer)
    {
        NewPlayer->MyPlayerId = PlayerId;
        NewPlayer->AutoPossessPlayer = EAutoReceiveInput::Disabled;

        // [??èÏ†ô] NoCollision(?∞‚ë∏Î£???ÅÏì¨)??QueryOnly(??âÏî†?? Â™õÎ®Ø?Ôß?Â™õ¬Ä??Êø?Ë´õÎ∂Ω???àÎñé.
        // ??Ä?ÉÂØÉ???ÅÎπû ??âÏî†??(Trace)Â™õ¬Ä Ôß?®Æ??Í≥óÏìΩ ÔßèÎ™ÑÎø?ÔßçÏöé???àÎñé.
        NewPlayer->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // [?∞Î∂Ω?] ?Î±Ä??Ôßè‚ë§?????âÏî†?? Ôß?æ®Íº?Visibility)???Î∫§Îñé??'Block'??éÎ£ÑÊø???ºÏ†ô??∏Îï≤??
        NewPlayer->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        NewPlayer->SyncRemoteState(InHealth, bInDowned, bInBeingCarried);

        NewPlayer->DisableInput(nullptr);
        RemotePlayers.Add(PlayerId, NewPlayer);
        UE_LOG(LogTemp, Warning, TEXT("New Remote Player Spawned! ID: %d"), PlayerId);
    }
}

void ATutorialCharacter::SyncRemoteState(int32 InHealth, bool bInDowned, bool bInBeingCarried)
{
    CurrentHealth = InHealth;
    IsBeingCarried = bInBeingCarried;

    if (bInDowned)
    {
        if (!IsDowned)
        {
            ForceDownedState();
        }

        CurrentHealth = 0;
        IsBeingCarried = bInBeingCarried;
        return;
    }

    if (IsDowned)
    {
        if (InHealth <= 1)
        {
            ForceInjuredState();
        }
        else
        {
            IsDowned = false;
            IsBeingCarried = false;
            if (bIsSelfReviving)
    {
        UpdateReviveProgressWidget(1.0f);
        HideReviveProgressWidget();
    }
    else if (CurrentReviver.IsValid())
    {
        CurrentReviver->UpdateReviveProgressWidget(1.0f);
        CurrentReviver->HideReviveProgressWidget();
    }

    bIsBeingRevived = false;
            bIsSelfReviving = false;
            RecoveryProgress = 0.0f;
            CurrentReviver = nullptr;
            bCanBeHit = true;
            GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
            GetCharacterMovement()->SetDefaultMovementMode();
            GetCharacterMovement()->bOrientRotationToMovement = true;
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            bUseControllerRotationYaw = false;
            RestoreBodyAnimClass();
        }
    }

    CurrentHealth = InHealth;
    IsBeingCarried = bInBeingCarried;
}

void ATutorialCharacter::UpdateRemoteKiller(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint)
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    if (!IsValid(this)) return;

    if (RemoteKillers.Contains(PlayerId))
    {
        AKillerCharacter* Target = Cast<AKillerCharacter>(RemoteKillers[PlayerId]);
        if (IsValid(Target))
        {
            Target->SetActorLocation(Location);
            Target->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));

            // =========================================================
            // [???ññ ?∞Î∂Ω?] ??ïÏæ≠?Î®?Ωå Ë´õÏèÜ? ?Ï¢äÎï≤ÔßéÎ∂ø?????ÖÏ†∞Â™õÎ???????Î®?æ∂ ‰∫åÏá±??
            // =========================================================
            Target->RemoteForwardValue = Forward;
            Target->RemoteRightValue = Right;
            Target->RemoteMovementSpeed = FVector2D(Forward, Right).Size() * 400.0f;

            return;
        }

        RemoteKillers.Remove(PlayerId);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AKillerCharacter* NewKiller = World->SpawnActor<AKillerCharacter>(AKillerCharacter::StaticClass(), Location, FRotator(0.0f, RotationYaw, 0.0f), SpawnParams);
    if (NewKiller)
    {
        NewKiller->MyPlayerId = PlayerId;

        // =========================================================
        // [???ññ ?∞Î∂Ω?] Ôß£ÏÑè????ΩÎ£ø?????£Ñ ?•ÎçáÎ¶???ÄÎ£?Â™õÎ????ÔΩåÎº±‰ª•Îùæ???
        // =========================================================
        NewKiller->RemoteForwardValue = Forward;
        NewKiller->RemoteRightValue = Right;
        NewKiller->RemoteMovementSpeed = FVector2D(Forward, Right).Size() * 400.0f;

        NewKiller->AutoPossessPlayer = EAutoReceiveInput::Disabled;
        NewKiller->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        NewKiller->DisableInput(nullptr);
        RemoteKillers.Add(PlayerId, NewKiller);
        UE_LOG(LogTemp, Warning, TEXT("New Remote Killer Spawned! ID: %d"), PlayerId);
    }
}

void ATutorialCharacter::HandleNetworkAction(uint8 ActionType, int32 InstigatorId, int32 TargetId, FVector Location, float RotationYaw)
{
    if (ActionType == ACTION_GENERATOR_START || ActionType == ACTION_GENERATOR_CANCEL || ActionType == ACTION_GENERATOR_COMPLETE)
    {
        if (RemotePlayers.Contains(InstigatorId) && IsValid(RemotePlayers[InstigatorId]))
        {
            RemotePlayers[InstigatorId]->SetRepairingGenerator(ActionType == ACTION_GENERATOR_START);
            RemotePlayers[InstigatorId]->IsInteracting = (ActionType == ACTION_GENERATOR_START);
        }

        ApplyGeneratorNetworkAction(ActionType, TargetId, Location, RotationYaw);
        return;
    }

    if (ActionType == ACTION_SURVIVOR_REVIVE_START || ActionType == ACTION_SURVIVOR_REVIVE_CANCEL || ActionType == ACTION_SURVIVOR_REVIVE_COMPLETE)
    {
        ATutorialCharacter* Reviver = nullptr;
        if (InstigatorId == MyPlayerId)
        {
            Reviver = this;
        }
        else if (RemotePlayers.Contains(InstigatorId) && IsValid(RemotePlayers[InstigatorId]))
        {
            Reviver = RemotePlayers[InstigatorId];
        }

        ATutorialCharacter* Survivor = nullptr;
        if (TargetId == MyPlayerId)
        {
            Survivor = this;
        }
        else if (RemotePlayers.Contains(TargetId) && IsValid(RemotePlayers[TargetId]))
        {
            Survivor = RemotePlayers[TargetId];
        }

        if (ActionType == ACTION_SURVIVOR_REVIVE_START)
        {
            const float SyncedRecoveryProgress = FMath::Clamp(RotationYaw, 0.f, 1.f);

            if (Reviver)
            {
                Reviver->IsInteracting = true;
                Reviver->SetRevivingSurvivor(true);
            }

            if (Survivor)
            {
                Survivor->bIsBeingRevived = true;
                Survivor->CurrentReviver = Reviver;
                Survivor->RecoveryProgress = SyncedRecoveryProgress;
            }

            if (IsPlayerControlled() && IsLocallyControlled())
            {
                ShowReviveProgressWidget();
                UpdateReviveProgressWidget(SyncedRecoveryProgress);
            }
        }
        else if (ActionType == ACTION_SURVIVOR_REVIVE_CANCEL)
        {
            if (Reviver)
            {
                Reviver->StopInteractingWithCurrentTarget(true);
            }

            if (Survivor)
            {
                Survivor->bIsBeingRevived = false;
                Survivor->RecoveryProgress = 0.0f;
                Survivor->CurrentReviver = nullptr;
            }

            if (IsPlayerControlled() && IsLocallyControlled())
            {
                HideReviveProgressWidget();
            }
        }
        else if (ActionType == ACTION_SURVIVOR_REVIVE_COMPLETE)
        {
            if (IsPlayerControlled() && IsLocallyControlled())
            {
                UpdateReviveProgressWidget(1.0f);
                HideReviveProgressWidget();
            }

            if (Survivor && Survivor->IsDowned)
            {
                Survivor->CompleteInteract_Implementation();
            }

            if (Reviver)
            {
                Reviver->StopInteractingWithCurrentTarget(true);
            }
        }

        return;
    }

    if (ActionType == ACTION_SELF_REVIVE_START || ActionType == ACTION_SELF_REVIVE_CANCEL || ActionType == ACTION_SELF_REVIVE_COMPLETE)
    {
        ATutorialCharacter* Survivor = nullptr;
        if (TargetId == MyPlayerId)
        {
            Survivor = this;
        }
        else if (RemotePlayers.Contains(TargetId) && IsValid(RemotePlayers[TargetId]))
        {
            Survivor = RemotePlayers[TargetId];
        }

        if (!Survivor)
        {
            return;
        }

        if (ActionType == ACTION_SELF_REVIVE_START)
        {
            const float SyncedRecoveryProgress = FMath::Clamp(RotationYaw, 0.f, 1.f);

            Survivor->bIsBeingRevived = true;
            Survivor->bIsSelfReviving = true;
            Survivor->IsInteracting = true;
            Survivor->RecoveryProgress = SyncedRecoveryProgress;
            Survivor->SetRevivingSurvivor(true);

            if (IsPlayerControlled() && IsLocallyControlled())
            {
                ShowReviveProgressWidget();
                UpdateReviveProgressWidget(SyncedRecoveryProgress);
            }
        }
        else if (ActionType == ACTION_SELF_REVIVE_CANCEL)
        {
            Survivor->bIsBeingRevived = false;
            Survivor->bIsSelfReviving = false;
            Survivor->RecoveryProgress = 0.0f;
            Survivor->IsInteracting = false;
            Survivor->SetRevivingSurvivor(false);

            if (IsPlayerControlled() && IsLocallyControlled())
            {
                HideReviveProgressWidget();
            }
        }
        else if (ActionType == ACTION_SELF_REVIVE_COMPLETE)
        {
            if (IsPlayerControlled() && IsLocallyControlled())
            {
                UpdateReviveProgressWidget(1.0f);
                HideReviveProgressWidget();
            }

            Survivor->bHasBandage = false;
            if (Survivor->IsDowned)
            {
                Survivor->CompleteInteract_Implementation();
            }
        }

        return;
    }

    if (ActionType == ACTION_BANDAGE_PICKUP)
    {
        ApplyBandageNetworkAction(TargetId, Location);

        if (InstigatorId == MyPlayerId)
        {
            bHasBandage = true;
        }
        else if (RemotePlayers.Contains(InstigatorId) && IsValid(RemotePlayers[InstigatorId]))
        {
            RemotePlayers[InstigatorId]->bHasBandage = true;
        }

        return;
    }

    if (ActionType == ACTION_KILLER_ATTACK)
    {
        if (RemoteKillers.Contains(InstigatorId) && IsValid(RemoteKillers[InstigatorId]))
        {
            RemoteKillers[InstigatorId]->SetActorLocation(Location);
            RemoteKillers[InstigatorId]->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));
            RemoteKillers[InstigatorId]->StartAttack();
        }
        return;
    }

    if (ActionType == ACTION_SURVIVOR_HIT)
    {
        const bool bForceDown = TargetId < 0;
        const int32 RealTargetId = bForceDown ? -TargetId : TargetId;

        if (RealTargetId == MyPlayerId || (RealTargetId > 0 && !RemotePlayers.Contains(RealTargetId)))
        {
            if (bForceDown)
            {
                ForceDownedState();
            }
            else
            {
                PlayNetworkHitReaction();
            }
        }
        else if (RemotePlayers.Contains(RealTargetId) && IsValid(RemotePlayers[RealTargetId]))
        {
            if (bForceDown)
            {
                RemotePlayers[RealTargetId]->ForceDownedState();
            }
            else
            {
                RemotePlayers[RealTargetId]->PlayNetworkHitReaction();
            }
        }
        return;
    }

    if (ActionType == ACTION_SURVIVOR_PICKUP)
    {
        AKillerCharacter* Killer = nullptr;
        if (RemoteKillers.Contains(InstigatorId) && IsValid(RemoteKillers[InstigatorId]))
        {
            Killer = RemoteKillers[InstigatorId];
        }

        ATutorialCharacter* Survivor = nullptr;
        if (TargetId == MyPlayerId)
        {
            Survivor = this;
        }
        else if (RemotePlayers.Contains(TargetId) && IsValid(RemotePlayers[TargetId]))
        {
            Survivor = RemotePlayers[TargetId];
        }

        if (Killer && Survivor)
        {
            Survivor->IsBeingCarried = true;
            Survivor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Survivor->GetCharacterMovement()->DisableMovement();
            Survivor->AttachToComponent(Killer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("CarrySocket"));
            Killer->PlayCarryAnimation();
        }
    }
}

void ATutorialCharacter::PlayTemporaryBodyAnimation(UAnimSequence* Animation)
{
    if (!Animation || !GetMesh())
    {
        return;
    }

    GetMesh()->PlayAnimation(Animation, false);

    FTimerHandle RestoreAnimTimer;
    GetWorldTimerManager().SetTimer(
        RestoreAnimTimer,
        this,
        &ATutorialCharacter::RestoreBodyAnimClass,
        Animation->GetPlayLength(),
        false
    );
}

void ATutorialCharacter::PlayLoopBodyAnimation(UAnimSequence* Animation)
{
    if (!Animation || !GetMesh())
    {
        return;
    }

    GetMesh()->PlayAnimation(Animation, true);
}

void ATutorialCharacter::PlayRepairMontageStart()
{
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (RepairMontage && AnimInstance)
    {
        const bool bIsAlreadyPlaying = AnimInstance->Montage_IsPlaying(RepairMontage);
        if (!bIsAlreadyPlaying)
        {
            AnimInstance->Montage_Play(RepairMontage);
        }

        AnimInstance->Montage_JumpToSection(FName(TEXT("Start")), RepairMontage);
        return;
    }

    if (RepairAnimation)
    {
        PlayLoopBodyAnimation(RepairAnimation);
    }
}

void ATutorialCharacter::PlayRepairMontageEnd()
{
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (RepairMontage && AnimInstance && AnimInstance->Montage_IsPlaying(RepairMontage))
    {
        AnimInstance->Montage_JumpToSection(FName(TEXT("End")), RepairMontage);
        return;
    }

    RestoreBodyAnimClass();
}

void ATutorialCharacter::RestoreBodyAnimClass()
{
    if (GetMesh() && !IsDowned && !IsBeingCarried)
    {
        if (UClass* SurvivorAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/BP/ABP_TutorialAnim.ABP_TutorialAnim_C")))
        {
            GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            GetMesh()->SetAnimInstanceClass(SurvivorAnimClass);
        }
    }
}

void ATutorialCharacter::ApplyLocalPlayerInputMode()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    FInputModeGameOnly InputMode;
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = false;
}
void ATutorialCharacter::SetRevivingSurvivor(bool bReviving)
{
    if (bReviving)
    {
        if (RepairAnimation)
        {
            PlayLoopBodyAnimation(RepairAnimation);
        }
    }
    else if (!bIsRepairingGenerator)
    {
        RestoreBodyAnimClass();
    }
}

void ATutorialCharacter::StopInteractingWithCurrentTarget(bool bRestoreAnimation)
{
    IsInteracting = false;
    SetRepairingGenerator(false);
    HideReviveProgressWidget();
    ReviveSyncAccumulator = 0.0f;

    if (bRestoreAnimation)
    {
        SetRevivingSurvivor(false);
    }
}

// 1. ?Í≥πÏÉá?Î¨íÏäú ??ñÏòâ
void ATutorialCharacter::StartInteract_Implementation(ACharacter* Interactor)
{
    if (!IsDowned || IsBeingCarried || !Interactor) return;

    ATutorialCharacter* Reviver = Cast<ATutorialCharacter>(Interactor);
    if (!Reviver) return;

    if (Reviver == this)
    {
        if (!bHasBandage) return;

        bIsSelfReviving = true;
        bIsBeingRevived = true;
        RecoveryProgress = 0.0f;
        CurrentReviver = nullptr;
        ShowReviveProgressWidget();
        UpdateReviveProgressWidget(0.0f);
        UE_LOG(LogTemp, Warning, TEXT("Started self revive with bandage."));
        return;
    }

    CurrentReviver = Reviver;
    bIsSelfReviving = false;
    bIsBeingRevived = true;
    RecoveryProgress = 0.0f;
    Reviver->ShowReviveProgressWidget();
    Reviver->UpdateReviveProgressWidget(0.0f);
    UE_LOG(LogTemp, Warning, TEXT("Someone is healing me!"));
}

// 2. ?Í≥πÏÉá?Î¨íÏäú ??ÖÎú≤??ÑÎìÉ (ÔßûÍæ™Îª?
void ATutorialCharacter::UpdateInteract_Implementation(float DeltaTime)
{
    if (!IsDowned) return;
    if (!bIsSelfReviving && !CurrentReviver.IsValid()) return;

    const float RecoveryTime = bIsSelfReviving ? SelfRecoveryTime : MaxRecoveryTime;
    RecoveryProgress += DeltaTime / RecoveryTime;
    RecoveryProgress = FMath::Clamp(RecoveryProgress, 0.f, 1.f);
    if (bIsSelfReviving)
    {
        UpdateReviveProgressWidget(RecoveryProgress);
    }
    else if (CurrentReviver.IsValid())
    {
        CurrentReviver->UpdateReviveProgressWidget(RecoveryProgress);
    }

    if (RecoveryProgress >= 1.0f)
    {
        IInteractionInterface::Execute_CompleteInteract(this);
    }
}

// 3. ?Í≥πÏÉá?Î¨íÏäú ?ç‚ë•??
void ATutorialCharacter::CancelInteract_Implementation()
{
    if (bIsSelfReviving)
    {
        HideReviveProgressWidget();
    }
    else if (CurrentReviver.IsValid())
    {
        CurrentReviver->HideReviveProgressWidget();
    }
    bIsBeingRevived = false;
    bIsSelfReviving = false;
    RecoveryProgress = 0.0f;
    CurrentReviver = nullptr;
    IsInteracting = false;
    CurrentInteractable = nullptr;
    UE_LOG(LogTemp, Warning, TEXT("Healing interrupted."));
}

// 4. ?Í≥πÏÉá?Î¨íÏäú ?Íæ®Ï¶∫ (?∫¬Ä??
void ATutorialCharacter::CompleteInteract_Implementation()
{
    if (!IsDowned) return;

    IsDowned = false;
    IsBeingCarried = false;
    if (bIsSelfReviving)
    {
        UpdateReviveProgressWidget(1.0f);
        HideReviveProgressWidget();
    }
    else if (CurrentReviver.IsValid())
    {
        CurrentReviver->UpdateReviveProgressWidget(1.0f);
        CurrentReviver->HideReviveProgressWidget();
    }

    bIsBeingRevived = false;
    bIsSelfReviving = false;
    CurrentHealth = 1; // ?∫¬Ä???Í≥πÍπ≠Êø?ËπÇÎì¶??
    RecoveryProgress = 0.0f;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Ôß¶‚â™???ÎØ™Ïî†?? ??ÄÎ£??Œª??ËπÇÎì¶??
    GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
    GetCharacterMovement()->SetDefaultMovementMode();
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    bUseControllerRotationYaw = false;
    bCanBeHit = true;
    CurrentReviver = nullptr;

    // ?Ï¢äÎï≤ÔßéÎ∂ø???ËπÇÎì¶??Êø°ÏíñÏ≠??Î™ÑÌÖß
    RestoreBodyAnimClass();

    UE_LOG(LogTemp, Warning, TEXT("Survivor has been revived!"));
}

float ATutorialCharacter::GetInteractDuration_Implementation() const
{
    return bIsSelfReviving ? SelfRecoveryTime : MaxRecoveryTime;
}


