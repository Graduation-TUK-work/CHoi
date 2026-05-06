#include "KillerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "MyProject_Start/Player/TutorialCharacter.h"
#include "MyProject_Start/NetworkWorker.h"
#include "MyProject_Start/MyGameInstance.h"
#include "MyProject_Start/Generator.h"
#include "Kismet/GameplayStatics.h"
#include "Networking.h"
#include "Sockets.h"

// Sets default values
AKillerCharacter::AKillerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCharacterMovement()->MaxWalkSpeed = 400.0f;

    FPSCamerComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FPSCamerComponent->SetupAttachment(GetCapsuleComponent());
    FPSCamerComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f + BaseEyeHeight));
    FPSCamerComponent->bUsePawnControlRotation = true;

    FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
    FPSMesh->SetOnlyOwnerSee(true);
    FPSMesh->SetupAttachment(FPSCamerComponent);
    FPSMesh->bCastDynamicShadow = false;
    FPSMesh->CastShadow = false;

    FPSMesh->BoundsScale = 5.0f;
    FPSMesh->SetHiddenInGame(false);
    FPSMesh->SetRelativeLocation(FVector(45.0f, 0.0f, -45.0f));
    FPSMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> FirstPersonArmMesh(
        TEXT("/Script/Engine.SkeletalMesh'/Game/Animation/zombi/zombi_Arm.zombi_Arm'")
    );
    if (FirstPersonArmMesh.Succeeded())
    {
        FPSMesh->SetSkeletalMesh(FirstPersonArmMesh.Object);
        FPSMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    }
    GetMesh()->SetRelativeLocationAndRotation(
        FVector(0.0f, 0.0f, -88.0f),
        FRotator(0.0f, -90.0f, 0.0f)
    );

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> KillerMesh(
        TEXT("/Script/Engine.SkeletalMesh'/Game/zombi/zombi.zombi'")
    );
    if (KillerMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(KillerMesh.Object);
    }


    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageAsset(
        TEXT("/Script/Engine.AnimMontage'/Game/Animation/zombi/AM_arm_Attack.AM_arm_Attack'")
    );
    if (AttackMontageAsset.Succeeded())
    {
        AttackMontage = AttackMontageAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> FirstPersonAttackAnimationAsset(
        TEXT("/Script/Engine.AnimSequence'/Game/Animation/zombi/arm_Attack.arm_Attack'")
    );
    if (FirstPersonAttackAnimationAsset.Succeeded())
    {
        FirstPersonAttackAnimation = FirstPersonAttackAnimationAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> BodyAttackAnimationAsset(
        TEXT("/Script/Engine.AnimSequence'/Game/Animation/zombi/atack.atack'")
    );
    if (BodyAttackAnimationAsset.Succeeded())
    {
        BodyAttackAnimation = BodyAttackAnimationAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> CarryAnimationAsset(
        TEXT("/Script/Engine.AnimSequence'/Game/Animation/zombi/Walk_With.Walk_With'")
    );
    if (CarryAnimationAsset.Succeeded())
    {
        CarryAnimation = CarryAnimationAsset.Object;
    }

    // 3��Ī �ٵ��?�ڽſ��� ������ �ʰ� ����
    GetMesh()->SetOwnerNoSee(true);

    // ���� ���� ���� �ʱ�ȭ
    bIsAttacking = false;


    NetworkWorker = nullptr;
}

void AKillerCharacter::BeginPlay()
{
        Super::BeginPlay();

    if (UClass* KillerAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/BP/ABP_Zombi.ABP_Zombi_C")))
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        GetMesh()->SetAnimInstanceClass(KillerAnimClass);
    }

    if (FPSMesh)
    {
        if (UClass* KillerArmAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/BP/ABP_zombi_Arm.ABP_zombi_Arm_C")))
        {
            FPSMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            FPSMesh->SetAnimInstanceClass(KillerArmAnimClass);
        }
    }

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        RemoteKillers.Empty();
        RemoteSurvivors.Empty();

        if (NetworkWorker == nullptr)
        {
            NetworkWorker = new FNetworkWorker(FNetworkWorker::GetDefaultServerIP(), FNetworkWorker::GetDefaultServerPort());
            NetworkWorker->SetOwnerKiller(this);
            if (UMyGameInstance* GI = GetGameInstance<UMyGameInstance>())
            {
                NetworkWorker->SetSelectedRole(GI->LocalSelectedRole);
            }
            FRunnableThread::Create(NetworkWorker, TEXT("KillerNetworkThread"));
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Killer Character Spawned"));
    }
}

void AKillerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (NetworkWorker)
    {
        NetworkWorker->Stop();
    }

    Super::EndPlay(EndPlayReason);
    RemoteKillers.Empty();
    RemoteSurvivors.Empty();
}

void AKillerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector Velocity = GetVelocity();
    FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
    MovementSpeed = LateralVelocity.Size();

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        SendLocationToServer();
    }
}

void AKillerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AKillerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AKillerCharacter::MoveRight);
    PlayerInputComponent->BindAxis("TurnCamera", this, &AKillerCharacter::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &AKillerCharacter::AddControllerPitchInput);
    PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AKillerCharacter::StartAttack);
    PlayerInputComponent->BindAction("Pickup", IE_Pressed, this, &AKillerCharacter::PickupSurvivor);
}

void AKillerCharacter::MoveForward(float AxisValue)
{
    MoveForwardValue = AxisValue;
    if (Controller && AxisValue != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, AxisValue);
    }
}

void AKillerCharacter::MoveRight(float AxisValue)
{
    MoveRightValue = AxisValue;
    if (Controller && AxisValue != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, AxisValue);
    }
}

void AKillerCharacter::StartAttack()
{
    if (bIsAttacking) return;
    if (!AttackMontage && !BodyAttackAnimation && !FirstPersonAttackAnimation) return;

    bIsAttacking = true;
    bHasDealtDamage = false;
    GetCharacterMovement()->MaxWalkSpeed = 100.0f;

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        if (FPSMesh && FirstPersonAttackAnimation)
        {
            FPSMesh->SetHiddenInGame(false);
            FPSMesh->PlayAnimation(FirstPersonAttackAnimation, false);
        }
        else if (FPSMesh && AttackMontage)
        {
            if (UAnimInstance* FPSAnimInstance = FPSMesh->GetAnimInstance())
            {
                FPSAnimInstance->Montage_Play(AttackMontage);
            }
        }

        SendActionToServer(ACTION_KILLER_ATTACK);

        FTimerHandle HitCheckTimer;
        GetWorldTimerManager().SetTimer(HitCheckTimer, this, &AKillerCharacter::CheckHit, 0.3f, false);
    }
    else
    {
        if (BodyAttackAnimation)
        {
            PlayTemporaryBodyAnimation(BodyAttackAnimation);
        }
    }

    const float AttackDuration = (IsPlayerControlled() && IsLocallyControlled() && FirstPersonAttackAnimation)
        ? FirstPersonAttackAnimation->GetPlayLength()
        : (BodyAttackAnimation ? BodyAttackAnimation->GetPlayLength() : 0.8f);

    FTimerHandle AttackEndTimer;
    GetWorldTimerManager().SetTimer(
        AttackEndTimer,
        this,
        &AKillerCharacter::EndAttack,
        FMath::Max(AttackDuration, 0.2f),
        false
    );
}

// �� �Լ��� ���߿� �ִϸ��̼� ��Ƽ����(Notify)���� ȣ���� �����Դϴ�.
void AKillerCharacter::EndAttack()
{
    bIsAttacking = false;
    // �ٽ� ���� �ӵ��� ����
    GetCharacterMovement()->MaxWalkSpeed = 400.0f;
    UE_LOG(LogTemp, Warning, TEXT("EndAttack called"));
}

void AKillerCharacter::CheckHit()
{
    if (!IsPlayerControlled() || !IsLocallyControlled()) return;
    if (bHasDealtDamage) return;

    FVector Start = FPSCamerComponent->GetComponentLocation();
    FVector ForwardVector = FPSCamerComponent->GetForwardVector().GetSafeNormal();
    FVector End = Start + (ForwardVector * AttackRange);

    ATutorialCharacter* Victim = nullptr;
    float BestDistance = AttackRange;

    for (TPair<int32, ATutorialCharacter*>& Pair : RemoteSurvivors)
    {
        ATutorialCharacter* Candidate = Pair.Value;
        if (!IsValid(Candidate) || Candidate->IsBeingCarried)
        {
            continue;
        }

        FVector ToCandidate = Candidate->GetActorLocation() - Start;
        const float ForwardDistance = FVector::DotProduct(ToCandidate, ForwardVector);
        if (ForwardDistance < 0.0f || ForwardDistance > AttackRange)
        {
            continue;
        }

        const float SideDistance = (ToCandidate - ForwardVector * ForwardDistance).Size();
        if (SideDistance > 100.0f)
        {
            continue;
        }

        if (ForwardDistance < BestDistance)
        {
            BestDistance = ForwardDistance;
            Victim = Candidate;
        }
    }

    DrawDebugLine(GetWorld(), Start, End, Victim ? FColor::Green : FColor::Red, false, 1.0f, 0, 2.0f);

    if (Victim)
    {
        bHasDealtDamage = true;
        Victim->PlayNetworkHitReaction();
        const int32 HitTargetId = Victim->IsDowned ? -Victim->MyPlayerId : Victim->MyPlayerId;
        SendActionToServer(ACTION_SURVIVOR_HIT, HitTargetId);

        UE_LOG(LogTemp, Warning, TEXT("Hit Target: %s / ID: %d"), *Victim->GetName(), Victim->MyPlayerId);
        DrawDebugSphere(GetWorld(), Victim->GetActorLocation(), 20.0f, 12, FColor::Yellow, false, 1.0f);
    }

    UE_LOG(LogTemp, Warning, TEXT("CheckHit called | bHasDealtDamage = %s"), bHasDealtDamage ? TEXT("true") : TEXT("false"));
}

// ������ ���� �Լ�
void AKillerCharacter::PickupSurvivor()
{
    if (CarriedSurvivor) return;

    FVector Start = FPSCamerComponent->GetComponentLocation();
    FVector ForwardVector = FPSCamerComponent->GetForwardVector().GetSafeNormal();

    ATutorialCharacter* Target = nullptr;
    float BestDistance = AttackRange;

    for (TPair<int32, ATutorialCharacter*>& Pair : RemoteSurvivors)
    {
        ATutorialCharacter* Candidate = Pair.Value;
        if (!IsValid(Candidate) || !Candidate->IsDowned || Candidate->IsBeingCarried)
        {
            continue;
        }

        FVector ToCandidate = Candidate->GetActorLocation() - Start;
        const float ForwardDistance = FVector::DotProduct(ToCandidate, ForwardVector);
        if (ForwardDistance < 0.0f || ForwardDistance > AttackRange)
        {
            continue;
        }

        const float SideDistance = (ToCandidate - ForwardVector * ForwardDistance).Size();
        if (SideDistance > 120.0f)
        {
            continue;
        }

        if (ForwardDistance < BestDistance)
        {
            BestDistance = ForwardDistance;
            Target = Candidate;
        }
    }

    if (Target && Target->IsDowned)
    {
        CarriedSurvivor = Target;
        CarriedSurvivor->IsBeingCarried = true;
        CarriedSurvivor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CarriedSurvivor->GetCharacterMovement()->DisableMovement();
        CarriedSurvivor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("CarrySocket"));

        UE_LOG(LogTemp, Warning, TEXT("Survivor picked up."));

        PlayCarryAnimation();
        if (PickupMontage) PlayAnimMontage(PickupMontage);
        SendActionToServer(ACTION_SURVIVOR_PICKUP, CarriedSurvivor->MyPlayerId);
    }
}
void AKillerCharacter::SendLocationToServer()
{
    if (MyPlayerId == -1) return;

    if (NetworkWorker && NetworkWorker->GetSocket())
    {
        FPacketMove MovePkt;
        MovePkt.Type = PKT_MOVE;
        MovePkt.Data.PlayerId = MyPlayerId;
        MovePkt.Data.CharacterType = CHARACTER_KILLER; // ?�인�??�??

        MovePkt.Data.X = GetActorLocation().X;
        MovePkt.Data.Y = GetActorLocation().Y;
        MovePkt.Data.Z = GetActorLocation().Z;
        MovePkt.Data.RotationYaw = GetActorRotation().Yaw;

        // [중요] ?�도 계산 ?�???�력값을 직접 ?�습?�다.
        MovePkt.Data.ForwardValue = MoveForwardValue;
        MovePkt.Data.RightValue = MoveRightValue;
        MovePkt.Data.bIsSprinting = false;

        int32 BytesSent = 0;
        NetworkWorker->GetSocket()->Send((uint8*)&MovePkt, sizeof(FPacketMove), BytesSent);
    }
}

void AKillerCharacter::UpdateRemoteKiller(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint)
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

void AKillerCharacter::UpdateRemoteSurvivor(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint)
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    if (!IsValid(this)) return;

    if (RemoteSurvivors.Contains(PlayerId))
    {
        ATutorialCharacter* Target = Cast<ATutorialCharacter>(RemoteSurvivors[PlayerId]);
        if (IsValid(Target))
        {
            Target->SetActorLocation(Location);
            Target->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));
            Target->RemoteForwardValue = Forward;
            Target->RemoteRightValue = Right;
            Target->RemoteIsSprinting = bSprint;
            return;
        }

        RemoteSurvivors.Remove(PlayerId);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATutorialCharacter* NewSurvivor = World->SpawnActor<ATutorialCharacter>(ATutorialCharacter::StaticClass(), Location, FRotator(0.0f, RotationYaw, 0.0f), SpawnParams);
    if (NewSurvivor)
    {
        NewSurvivor->MyPlayerId = PlayerId;
        NewSurvivor->AutoPossessPlayer = EAutoReceiveInput::Disabled;
        NewSurvivor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        NewSurvivor->DisableInput(nullptr);
        RemoteSurvivors.Add(PlayerId, NewSurvivor);
        UE_LOG(LogTemp, Warning, TEXT("New Remote Survivor Spawned! ID: %d"), PlayerId);
    }
}
void AKillerCharacter::SendActionToServer(uint8 ActionType, int32 TargetId)
{
    if (!NetworkWorker || !NetworkWorker->GetSocket()) return;

    FPacketAction ActionPkt;
    ActionPkt.Type = PKT_ACTION; // Shared.h???�의???�??
    ActionPkt.ActionType = ActionType;
    ActionPkt.InstigatorId = MyPlayerId;
    ActionPkt.TargetId = TargetId;

    // ?�재 ?�치?� ?�전값도 같이 ?�어 보냅?�다 (?�른 ?�라?�서 ?�확???�치??보이?�록)
    FVector Loc = GetActorLocation();
    ActionPkt.X = Loc.X;
    ActionPkt.Y = Loc.Y;
    ActionPkt.Z = Loc.Z;
    ActionPkt.RotationYaw = GetActorRotation().Yaw;
    ActionPkt.RotationYaw = GetActorRotation().Yaw;

    int32 BytesSent = 0;
    NetworkWorker->GetSocket()->Send((uint8*)&ActionPkt, sizeof(FPacketAction), BytesSent);
}



AGenerator* AKillerCharacter::FindGeneratorForNetworkAction(int32 GeneratorId, const FVector& Location) const
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

void AKillerCharacter::ApplyGeneratorNetworkAction(uint8 ActionType, int32 GeneratorId, const FVector& Location, float RepairProgress)
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
        Generator->ApplyNetworkRepairState(false, true, 1.f);
    }
}
void AKillerCharacter::HandleNetworkAction(uint8 ActionType, int32 InstigatorId, int32 TargetId, FVector Location, float RotationYaw)
{
    if (ActionType == ACTION_GENERATOR_START || ActionType == ACTION_GENERATOR_CANCEL || ActionType == ACTION_GENERATOR_COMPLETE)
    {
        if (RemoteSurvivors.Contains(InstigatorId) && IsValid(RemoteSurvivors[InstigatorId]))
        {
            RemoteSurvivors[InstigatorId]->SetRepairingGenerator(ActionType == ACTION_GENERATOR_START);
            RemoteSurvivors[InstigatorId]->IsInteracting = (ActionType == ACTION_GENERATOR_START);
        }

        ApplyGeneratorNetworkAction(ActionType, TargetId, Location, RotationYaw);
        return;
    }

    if (ActionType == ACTION_KILLER_ATTACK)
    {
        if (RemoteKillers.Contains(InstigatorId) && IsValid(RemoteKillers[InstigatorId]))
        {
            AKillerCharacter* RemoteKiller = RemoteKillers[InstigatorId];
            RemoteKiller->SetActorLocation(Location);
            RemoteKiller->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));

            // StartAttack() ?�???�신 ?�니메이?�만 바로 강제 ?�생?�킴 (무한 루프 방�?)
            if (RemoteKiller->BodyAttackAnimation)
            {
                RemoteKiller->PlayTemporaryBodyAnimation(RemoteKiller->BodyAttackAnimation);
            }
        }
        return;
    }
    if (ActionType == ACTION_SURVIVOR_HIT)
    {
        const bool bForceDown = TargetId < 0;
        const int32 RealTargetId = bForceDown ? -TargetId : TargetId;

        if (RemoteSurvivors.Contains(RealTargetId) && IsValid(RemoteSurvivors[RealTargetId]))
        {
            if (bForceDown)
            {
                RemoteSurvivors[RealTargetId]->ForceDownedState();
            }
            else
            {
                RemoteSurvivors[RealTargetId]->PlayNetworkHitReaction();
            }
        }
        return;
    }

    if (ActionType == ACTION_SURVIVOR_PICKUP)
    {
        AKillerCharacter* Killer = nullptr;
        if (InstigatorId == MyPlayerId)
        {
            Killer = this;
        }
        else if (RemoteKillers.Contains(InstigatorId) && IsValid(RemoteKillers[InstigatorId]))
        {
            Killer = RemoteKillers[InstigatorId];
        }

        ATutorialCharacter* Survivor = nullptr;
        if (RemoteSurvivors.Contains(TargetId) && IsValid(RemoteSurvivors[TargetId]))
        {
            Survivor = RemoteSurvivors[TargetId];
        }

        if (Killer && Survivor)
        {
            Killer->CarriedSurvivor = Survivor;
            Survivor->IsBeingCarried = true;
            Survivor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Survivor->GetCharacterMovement()->DisableMovement();
            Survivor->AttachToComponent(Killer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("CarrySocket"));
            Killer->PlayCarryAnimation();
        }
    }
}

void AKillerCharacter::PlayTemporaryBodyAnimation(UAnimSequence* Animation)
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
        &AKillerCharacter::RestoreBodyAnimClass,
        Animation->GetPlayLength(),
        false
    );
}

void AKillerCharacter::PlayCarryAnimation()
{
    if (!CarryAnimation || !GetMesh())
    {
        return;
    }

    GetMesh()->PlayAnimation(CarryAnimation, true);
}

void AKillerCharacter::RestoreBodyAnimClass()
{
    bIsAttacking = false;
    bHasDealtDamage = false;

    if (GetMesh())
    {
        if (UClass* KillerAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/BP/ABP_Zombi.ABP_Zombi_C")))
        {
            GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            GetMesh()->SetAnimInstanceClass(KillerAnimClass);
        }
    }
}



