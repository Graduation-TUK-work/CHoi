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
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject_Start/InteractionInterface.h"
#include "MyProject_Start/NetworkWorker.h"
#include "MyProject_Start/MyGameInstance.h"
#include "MyProject_Start/LobbyRoleSelectWidget.h" // ÔøΩﬂ∞ÔøΩ
#include "MyProject_Start/KillerCharacter.h"
#include "MyProject_Start/Generator.h"
#include "Networking.h"    // ÔøΩﬂ∞ÔøΩ
#include "Sockets.h"       // ÔøΩﬂ∞ÔøΩ

ATutorialCharacter::ATutorialCharacter()
{
    bCanVault = false;
    PrimaryActorTick.bCanEverTick = true;

    // ƒ´ÔøΩﬁ∂ÔøΩ ÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));

    SpringArm->SetupAttachment(GetCapsuleComponent());
    Camera->SetupAttachment(SpringArm);

    SpringArm->TargetArmLength = 300.f;
    SpringArm->SetRelativeLocationAndRotation(
        FVector(0.f, 0.f, 80.f),
        FRotator(0.f, 0.f, 0.f)
    );
    SpringArm->bUsePawnControlRotation = true;

    // ÔøΩﬁΩÔøΩ ÔøΩÔøΩƒ° ÔøΩÔøΩ »∏ÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ
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


    static ConstructorHelpers::FObjectFinder<UAnimMontage> HitMontageAsset(
        TEXT("/Script/Engine.AnimMontage'/Game/Animation/player/AM_Big_Kidney_Hit.AM_Big_Kidney_Hit'")
    );
    if (HitMontageAsset.Succeeded())
    {
        HitMontage = HitMontageAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> DownedMontageAsset(
        TEXT("/Script/Engine.AnimMontage'/Game/Animation/player/AM_Fallen_Idle.AM_Fallen_Idle'")
    );
    if (DownedMontageAsset.Succeeded())
    {
        DownedMontage = DownedMontageAsset.Object;
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
    // ÔøΩÃµÔøΩ ÔøΩÔøΩÔøΩÔøΩ
    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;

    GetCharacterMovement()->RotationRate = FRotator(0.f, 720.0f, 0.f);

    GetCharacterMovement()->MaxAcceleration = 2048.0f;

    GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
    GetCharacterMovement()->GroundFriction = 8.0f;
    GetCharacterMovement()->AirControl = 0.2f;

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
        RemotePlayers.Empty();
    }

    // Role selection fallback: if this gameplay pawn appears before choosing a role,
    // show the lobby UI immediately and avoid connecting to the server yet.
    if (IsPlayerControlled() && IsLocallyControlled())
    {
        UMyGameInstance* GI = GetGameInstance<UMyGameInstance>();
        if (!GI || GI->LocalSelectedRole == ROLE_NONE)
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                PC->bShowMouseCursor = true;
                FInputModeUIOnly InputMode;
                PC->SetInputMode(InputMode);

                ULobbyRoleSelectWidget* LobbyWidget = CreateWidget<ULobbyRoleSelectWidget>(PC, ULobbyRoleSelectWidget::StaticClass());
                if (LobbyWidget)
                {
                    LobbyWidget->AddToViewport(100);
                }
            }
            return;
        }
    }
    if (IsPlayerControlled() && IsLocallyControlled())
    {
        NetworkWorker = new FNetworkWorker(FNetworkWorker::GetDefaultServerIP(), FNetworkWorker::GetDefaultServerPort());
        NetworkWorker->SetOwnerCharacter(this);
        if (UMyGameInstance* GI = GetGameInstance<UMyGameInstance>())
        {
            NetworkWorker->SetSelectedRole(GI->LocalSelectedRole);
        }
        FRunnableThread::Create(NetworkWorker, TEXT("NetworkThread"));
    }
}

void ATutorialCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // ÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÂ∏¶ ÔøΩÔøΩÔøΩÔøΩÔøΩœ∞ÔøΩ ÔøΩÔøΩÔøΩÔøΩ
    if (NetworkWorker)
    {
        NetworkWorker->Stop();
        // ÔøΩﬁ∏ÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ ÔøΩ“∏ÔøΩÔøΩ⁄øÔøΩÔøΩÔøΩ √≥ÔøΩÔøΩÔøΩÔøΩ
    }

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
        SendLocationToServer();
    }

    if (IsInteracting)
    {
        UE_LOG(LogTemp, Log, TEXT("Current State: INTERACTING..."));
    }

    if (IsInteracting && CurrentInteractable)
    {
        AGenerator* Generator = Cast<AGenerator>(CurrentInteractable);
        if (Generator && Generator->bIsRepaired)
        {
            CancelInteraction();
            return;
        }

        const bool bWasGeneratorRepaired = Generator && Generator->bIsRepaired;

        // ?òÎ¶¨Í∞Ä ???ùÎÇ¨???åÎßå ?ÖÎç∞?¥Ìä∏ ?§Ìñâ
        IInteractionInterface::Execute_UpdateInteract(CurrentInteractable, DeltaTime);

        if (Generator && !bWasGeneratorRepaired && Generator->bIsRepaired)
        {
            SendGeneratorActionToServer(ACTION_GENERATOR_COMPLETE, Generator);
            SetRepairingGenerator(false);
            ShowGeneratorRepairCount();
            IsInteracting = false;
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

        // 1. ?ÑÏπò Î∞??åÏ†Ñ ?∏ÌåÖ
        FVector CurLocation = GetActorLocation();
        MovePkt.Data.X = CurLocation.X;
        MovePkt.Data.Y = CurLocation.Y;
        MovePkt.Data.Z = CurLocation.Z;
        MovePkt.Data.RotationYaw = GetActorRotation().Yaw;

        // 2. [?òÏ†ï??Î∂ÄÎ∂? Î≥µÏû°???çÎèÑ Í≥ÑÏÇ∞??ÏßÄ?∞Í≥†, ?ÖÎ†•Í∞íÏùÑ Í∑∏Î?Î°??£Ïäµ?àÎã§!
        MovePkt.Data.ForwardValue = MoveForwardValue;
        MovePkt.Data.RightValue = MoveRightValue;

        // 3. ?¨Î¶¨Í∏??ÅÌÉú ?∏ÌåÖ (Í∏∞Ï°¥ ÏΩîÎìú ?†Ï?)
        MovePkt.Data.bIsSprinting = (GetCharacterMovement()->MaxWalkSpeed > 350.0f);


        // 4. ?úÎ≤ÑÎ°??ÑÏÜ°
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
    // ÔøΩÔøΩÔøΩÔøΩ 1000ÔøΩÔøΩÔøΩÔøΩ 150 ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ 850 ÔøΩÔøΩÔøΩÔøΩ
    GetCharacterMovement()->MaxWalkSpeed = 350.0f;
}

void ATutorialCharacter::EndSprint()
{
    // ÔøΩÔøΩÔøΩÔøΩ 600ÔøΩÔøΩÔøΩÔøΩ 150 ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ 450 ÔøΩÔøΩÔøΩÔøΩ
    GetCharacterMovement()->MaxWalkSpeed = 350.0f;
}

void ATutorialCharacter::Turn(float Value) { AddControllerYawInput(Value); }
void ATutorialCharacter::LookUp(float Value) { AddControllerPitchInput(Value); }

void ATutorialCharacter::BeginCrouch() { Crouch(); }
void ATutorialCharacter::EndCrouch() { UnCrouch(); }


void ATutorialCharacter::TryVault()
{
    if (bIsVaulting) return;

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
            bIsVaulting = true;
            MotionWarping->AddOrUpdateWarpTargetFromLocation(FName("VaultTarget"), TargetLocation);
            if (VaultMontage)
            {
                PlayAnimMontage(VaultMontage);
            }
        }
    }
}

void ATutorialCharacter::TraceForInteractable()
{
    if (!Camera) return;

    FHitResult Hit;
    FVector StartLocation = Camera->GetComponentLocation();
    FVector ForwardVector = Camera->GetForwardVector();

    // ?§Ï†ú Ï°∞Ï???ÏµúÏ†Å?îÎêú ?¨Í±∞Î¶¨Ï? ?úÏûë ÏßÄ??
    FVector TraceStart = StartLocation;
    FVector TraceEnd = TraceStart + (ForwardVector * 1500.f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // ?êÏ†ï Î≤îÏúÑÎ•??âÎÑâ?òÍ≤å ?òÏó¨ ?ÑÏõå?àÎäî ?Ä?ÅÎèÑ ???°Ìûà?ÑÎ°ù ?†Ï? (Î∞òÏ?Î¶?40cm)
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(40.f);

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
    // ?ÅÌò∏?ëÏö© ?Ä?ÅÏù¥ Î∞îÎÄåÏóà???åÏùò Ï≤òÎ¶¨
    if (CurrentInteractable != NewInteractable)
    {
        if (IsInteracting && CurrentInteractable)
        {
            AGenerator* Generator = Cast<AGenerator>(CurrentInteractable);
            IInteractionInterface::Execute_CancelInteract(CurrentInteractable);
            IsInteracting = false;

            if (Generator)
            {
                SendGeneratorActionToServer(ACTION_GENERATOR_CANCEL, Generator);
            }
        }
        CurrentInteractable = NewInteractable;
    }
}

void ATutorialCharacter::StartInteraction()
{
    TraceForInteractable();

    if (!CurrentInteractable)
    {
        UE_LOG(LogTemp, Warning, TEXT("No interactable found."));
        return;
    }

    if (CurrentInteractable)
    {
        // [Ï∂îÍ???Î°úÏßÅ] ?¥Î? ?òÎ¶¨??Î∞úÏ†ÑÍ∏∞Ïù∏ÏßÄ ?ïÏù∏
        if (AGenerator* Generator = Cast<AGenerator>(CurrentInteractable))
        {
            if (Generator->bIsRepaired)
            {
                UE_LOG(LogTemp, Warning, TEXT("This generator is already done!"));
                return; // ?®Ïàò Ï¢ÖÎ£å
            }
        }

        IInteractionInterface::Execute_StartInteract(CurrentInteractable, this);
        IsInteracting = true;

        if (AGenerator* Generator = Cast<AGenerator>(CurrentInteractable))
        {
            SetRepairingGenerator(true);
            SendGeneratorActionToServer(ACTION_GENERATOR_START, Generator);
        }
    }
}

void ATutorialCharacter::CancelInteraction()
{
    if (IsInteracting && CurrentInteractable)
    {
        AGenerator* Generator = Cast<AGenerator>(CurrentInteractable);
        IInteractionInterface::Execute_CancelInteract(CurrentInteractable);
        IsInteracting = false;
        SetRepairingGenerator(false);

        if (Generator)
        {
            SendGeneratorActionToServer(ACTION_GENERATOR_CANCEL, Generator);
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
        if (RepairAnimation)
        {
            PlayLoopBodyAnimation(RepairAnimation);
        }
    }
    else
    {
        RestoreBodyAnimClass();
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

void ATutorialCharacter::ApplyGeneratorNetworkAction(uint8 ActionType, int32 GeneratorId, const FVector& Location, float RepairProgress)
{
    AGenerator* Generator = FindGeneratorForNetworkAction(GeneratorId, Location);
    if (!Generator) return;

    if (ActionType == ACTION_GENERATOR_START)
    {
        Generator->ApplyNetworkRepairState(true, false, RepairProgress);
    }
    else if (ActionType == ACTION_GENERATOR_CANCEL)
    {
        Generator->ApplyNetworkRepairState(false, false, RepairProgress);
    }
    else if (ActionType == ACTION_GENERATOR_COMPLETE)
    {
        const bool bWasRepaired = Generator->bIsRepaired;
        Generator->ApplyNetworkRepairState(false, true, 1.f);
        if (!bWasRepaired)
        {
            ShowGeneratorRepairCount();
        }
    }
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
    CurrentHealth = 0;
    IsDowned = true;
    bCanBeHit = false;

    if (DownedAnimation)
    {
        PlayLoopBodyAnimation(DownedAnimation);
    }
    else if (DownedMontage)
    {
        PlayAnimMontage(DownedMontage);
    }

    GetCapsuleComponent()->SetCapsuleHalfHeight(30.0f);
    GetCharacterMovement()->DisableMovement();
    bUseControllerRotationYaw = false;

    UE_LOG(LogTemp, Error, TEXT("Survivor is downed."));
}
void ATutorialCharacter::ApplyHitReaction(bool bRespectCooldown)
{
    if (IsDowned) return;
    if (bRespectCooldown && !bCanBeHit) return;

    bCanBeHit = false;
    CurrentHealth--;
    UE_LOG(LogTemp, Warning, TEXT("Survivor hit. CurrentHealth: %d"), CurrentHealth);

    if (CurrentHealth > 0)
    {
        if (HitReactionAnimation)
        {
            PlayTemporaryBodyAnimation(HitReactionAnimation);
        }
        else if (HitMontage)
        {
            PlayAnimMontage(HitMontage);
        }

        FTimerHandle HitCooldownTimer;
        GetWorldTimerManager().SetTimer(HitCooldownTimer, FTimerDelegate::CreateLambda([this]()
            {
                bCanBeHit = true;
                UE_LOG(LogTemp, Log, TEXT("ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ ÔøΩŸΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ"));
            }), 1.0f, false);
    }
    else
    {
        ForceDownedState();

    }
}

void ATutorialCharacter::UpdateRemotePlayer(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint)
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    if (!IsValid(this)) return;

    // 1. ÔøΩ øÔøΩ ÔøΩÿ¥ÔøΩ IDÔøΩÔøΩ ÔøΩ÷¥ÔøΩÔøΩÔøΩ »ÆÔøΩÔøΩ
    if (RemotePlayers.Contains(PlayerId))
    {
        // ÔøΩÔøΩÔøΩÔøΩÔøΩœ∞ÔøΩ ƒ≥ÔøΩÔøΩÔøΩÕ∑ÔøΩ ÔøΩÔøΩÔøΩÔøΩ»Ø
        ATutorialCharacter* Target = Cast<ATutorialCharacter>(RemotePlayers[PlayerId]);

        // [ÔøΩÔøΩÔøΩÔøΩ] TargetÔøΩÔøΩ ÔøΩﬁ∏∏ÆøÔøΩ ÔøΩÔøΩÔøΩÔøΩ÷±‚∏?ÔøΩœ∏ÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ∆Æ ÔøΩÔøΩÔøΩÔøΩ
        if (IsValid(Target))
        {
            Target->SetActorLocation(Location);
            Target->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));

            Target->RemoteForwardValue = Forward;
            Target->RemoteRightValue = Right;
            Target->RemoteIsSprinting = bSprint;

            // ÔøΩÔøΩÔøΩ‚º≠ ÔøΩ‘ºÔøΩ ÔøΩÔøΩÔøΩÔøΩ (ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ∆ÆÔøΩÔøΩ)
            return;
        }
        else
        {
            // ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ ÔøΩ√∑ÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩŸ∏Ôø?ÔøΩ øÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ
            RemotePlayers.Remove(PlayerId);
        }
    }

    // 2. ÔøΩ øÔøΩ ÔøΩÔøΩÔøΩ≈≥ÔøΩ TargetÔøΩÔøΩ ÔøΩÔøΩ»øÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÏø°ÔøΩÔø?ÔøΩÔøΩÔøΩÔøΩ (ÔøΩÔøΩÔøΩ‚∞° else ÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩÔøΩÔøΩ ÔøΩÔøΩ)
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATutorialCharacter* NewPlayer = World->SpawnActor<ATutorialCharacter>(GetClass(), Location, FRotator(0.0f, RotationYaw, 0.0f), SpawnParams);

    if (NewPlayer)
    {
        NewPlayer->MyPlayerId = PlayerId;
        NewPlayer->AutoPossessPlayer = EAutoReceiveInput::Disabled;

        // [?òÏ†ï] NoCollision(Ï∂©Îèå ?ÜÏùå)??QueryOnly(?àÏù¥?Ä Í∞êÏ?Îß?Í∞Ä??Î°?Î∞îÍøâ?àÎã§.
        // ?¥Î†áÍ≤??¥Ïïº ?àÏù¥?Ä(Trace)Í∞Ä Ï∫êÎ¶≠?∞Ïùò Î™∏Ïóê ÎßûÏäµ?àÎã§.
        NewPlayer->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // [Ï∂îÍ?] ?πÏãú Î™®Î•¥???àÏù¥?Ä Ï±ÑÎÑê(Visibility)???ïÏã§??'Block'?òÎèÑÎ°??§Ï†ï?©Îãà??
        NewPlayer->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

        NewPlayer->DisableInput(nullptr);
        RemotePlayers.Add(PlayerId, NewPlayer);
        UE_LOG(LogTemp, Warning, TEXT("New Remote Player Spawned! ID: %d"), PlayerId);
    }
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
            // [?µÏã¨ Ï∂îÍ?] ?úÎ≤Ñ?êÏÑú Î∞õÏ? ?†ÎãàÎ©îÏù¥???ÖÎ†•Í∞íÏùÑ ?¨Îü¨?êÍ≤å Ï£ºÏûÖ!
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
        // [?µÏã¨ Ï∂îÍ?] Ï≤òÏùå ?§Ìè∞???åÎèÑ Ï¥àÍ∏∞ ?¥Îèô Í∞íÏùÑ ?£Ïñ¥Ï§çÎãà??
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

// 1. ?ÅÌò∏?ëÏö© ?úÏûë
void ATutorialCharacter::StartInteract_Implementation(ACharacter* Interactor)
{
    // ?¥Í? ?ÑÏõå?àÏùÑ ?åÎßå ?Ä?∏Ïù¥ ?òÎ? ?¥Î¶¥ ???àÏùå
    if (!IsDowned) return;

    UE_LOG(LogTemp, Warning, TEXT("Someone is healing me!"));
}

// 2. ?ÅÌò∏?ëÏö© ?ÖÎç∞?¥Ìä∏ (ÏßÑÌñâ)
void ATutorialCharacter::UpdateInteract_Implementation(float DeltaTime)
{
    if (!IsDowned) return;

    RecoveryProgress += DeltaTime / MaxRecoveryTime;
    RecoveryProgress = FMath::Clamp(RecoveryProgress, 0.f, 1.f);

    // GEngine->AddOnScreenDebugMessage(...) ??†ú

    if (RecoveryProgress >= 1.0f)
    {
        IInteractionInterface::Execute_CompleteInteract(this);
    }
}

// 3. ?ÅÌò∏?ëÏö© Ï∑®ÏÜå
void ATutorialCharacter::CancelInteract_Implementation()
{
    // ÏπòÎ£åÎ∞õÎã§Í∞Ä Ï§ëÎã®??
    UE_LOG(LogTemp, Warning, TEXT("Healing interrupted."));
}

// 4. ?ÅÌò∏?ëÏö© ?ÑÎ£å (Î∂Ä??
void ATutorialCharacter::CompleteInteract_Implementation()
{
    if (!IsDowned) return;

    IsDowned = false;
    CurrentHealth = 1; // Î∂Ä???ÅÌÉúÎ°?Î≥µÍµ¨
    RecoveryProgress = 0.0f;

    // Ï∫°Ïäê ?íÏù¥?Ä ?¥Îèô ?•Î†• Î≥µÍµ¨
    GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
    GetCharacterMovement()->SetDefaultMovementMode();

    // ?†ÎãàÎ©îÏù¥??Î≥µÍµ¨ Î°úÏßÅ ?∏Ï∂ú
    RestoreBodyAnimClass();

    UE_LOG(LogTemp, Warning, TEXT("Survivor has been revived!"));
}




