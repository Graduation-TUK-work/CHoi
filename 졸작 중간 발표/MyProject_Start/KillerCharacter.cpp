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
#include "Networking.h"
#include "Sockets.h"

// Sets default values
AKillerCharacter::AKillerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 카메라 설정
    FPSCamerComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FPSCamerComponent->SetupAttachment(GetCapsuleComponent());
    FPSCamerComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f + BaseEyeHeight));
    FPSCamerComponent->bUsePawnControlRotation = true;

    // 1인칭 팔 메시 설정
    FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
    FPSMesh->SetOnlyOwnerSee(true);
    FPSMesh->SetupAttachment(FPSCamerComponent);
    FPSMesh->bCastDynamicShadow = false;
    FPSMesh->CastShadow = false;

    // 아까 해결한 바운드 스케일 코드 적용 (깜빡임 방지)
    FPSMesh->BoundsScale = 5.0f;
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

    // 3인칭 바디는 자신에게 보이지 않게 설정
    GetMesh()->SetOwnerNoSee(true);

    // 공격 관련 변수 초기화
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

        NetworkWorker = new FNetworkWorker(TEXT("127.0.0.1"), 7777);
        NetworkWorker->SetOwnerKiller(this);
        FRunnableThread::Create(NetworkWorker, TEXT("KillerNetworkThread"));
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
    // 1. 현재 이동 속도 계산 (XY 평면상의 속도만 추출)
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

    // 이동 및 시선 처리
    PlayerInputComponent->BindAxis("MoveForward", this, &AKillerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AKillerCharacter::MoveRight);
    PlayerInputComponent->BindAxis("TurnCamera", this, &AKillerCharacter::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &AKillerCharacter::AddControllerPitchInput);

    // 공격 입력 바인딩 (프로젝트 세팅 - 입력에 "Attack"이 등록되어 있어야 합니다)
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

// --- 공격 로직 ---
void AKillerCharacter::StartAttack()
{
    if (bIsAttacking && (!IsPlayerControlled() || !IsLocallyControlled()))
    {
        bIsAttacking = false;
    }

    if (bIsAttacking || (!AttackMontage && !BodyAttackAnimation))
    {
        return;
    }

    bool bPlayedMontage = false;
    if (IsPlayerControlled() && IsLocallyControlled() && FPSMesh && AttackMontage)
    {
        if (UAnimInstance* FPSAnimInstance = FPSMesh->GetAnimInstance())
        {
            FPSAnimInstance->Montage_Play(AttackMontage);
            bPlayedMontage = true;
        }
    }

    if ((!IsPlayerControlled() || !IsLocallyControlled()) && BodyAttackAnimation)
    {
        PlayTemporaryBodyAnimation(BodyAttackAnimation);
        bPlayedMontage = true;
    }
    else if (!bPlayedMontage && AttackMontage)
    {
        bPlayedMontage = PlayAnimMontage(AttackMontage) > 0.0f;
    }

    if (bPlayedMontage)
    {
        bIsAttacking = true;
        bHasDealtDamage = false;
        GetCharacterMovement()->MaxWalkSpeed = 600.0f;

        const bool bLocalAttacker = IsPlayerControlled() && IsLocallyControlled();
        const float AttackDuration = bLocalAttacker && AttackMontage ? AttackMontage->GetPlayLength() : (BodyAttackAnimation ? BodyAttackAnimation->GetPlayLength() : 0.8f);
        FTimerHandle AttackEndTimer;
        GetWorldTimerManager().SetTimer(
            AttackEndTimer,
            this,
            &AKillerCharacter::EndAttack,
            FMath::Max(AttackDuration, 0.2f),
            false
        );
    }

    UE_LOG(LogTemp, Warning, TEXT("StartAttack called"));
    if (IsPlayerControlled() && IsLocallyControlled())
    {
        SendActionToServer(ACTION_KILLER_ATTACK);
    }
}

// 이 함수는 나중에 애니메이션 노티파이(Notify)에서 호출할 예정입니다.
void AKillerCharacter::EndAttack()
{
    bIsAttacking = false;
    // 다시 원래 속도로 복구
    GetCharacterMovement()->MaxWalkSpeed = 600.0f;
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

// 생존자 업기 함수
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

        UE_LOG(LogTemp, Warning, TEXT("생존자를 들었습니다!"));

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
        MovePkt.Data.CharacterType = CHARACTER_KILLER;

        FVector CurLocation = GetActorLocation();
        MovePkt.Data.X = CurLocation.X;
        MovePkt.Data.Y = CurLocation.Y;
        MovePkt.Data.Z = CurLocation.Z;
        MovePkt.Data.RotationYaw = GetActorRotation().Yaw;

        FVector Velocity2D = GetVelocity();
        Velocity2D.Z = 0.0f;
        if (Velocity2D.SizeSquared() > 1.0f)
        {
            const FVector MoveDir = Velocity2D.GetSafeNormal();
            FVector ActorForward = GetActorForwardVector();
            FVector ActorRight = GetActorRightVector();
            ActorForward.Z = 0.0f;
            ActorRight.Z = 0.0f;
            MovePkt.Data.ForwardValue = FVector::DotProduct(MoveDir, ActorForward.GetSafeNormal());
            MovePkt.Data.RightValue = FVector::DotProduct(MoveDir, ActorRight.GetSafeNormal());
        }
        else
        {
            MovePkt.Data.ForwardValue = 0.0f;
            MovePkt.Data.RightValue = 0.0f;
        }
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
    if (MyPlayerId == -1) return;

    if (NetworkWorker && NetworkWorker->GetSocket())
    {
        FPacketAction ActionPkt;
        ActionPkt.Type = PKT_ACTION;
        ActionPkt.ActionType = ActionType;
        ActionPkt.InstigatorId = MyPlayerId;
        ActionPkt.TargetId = TargetId;

        FVector CurLocation = GetActorLocation();
        ActionPkt.X = CurLocation.X;
        ActionPkt.Y = CurLocation.Y;
        ActionPkt.Z = CurLocation.Z;
        ActionPkt.RotationYaw = GetActorRotation().Yaw;

        int32 BytesSent = 0;
        NetworkWorker->GetSocket()->Send((uint8*)&ActionPkt, sizeof(FPacketAction), BytesSent);
    }
}

void AKillerCharacter::HandleNetworkAction(uint8 ActionType, int32 InstigatorId, int32 TargetId, FVector Location, float RotationYaw)
{
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