// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject_Start/Player/TutorialCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MyProject_Start/InteractionInterface.h"
#include "MyProject_Start/NetworkWorker.h" // 추가
#include "MyProject_Start/KillerCharacter.h"
#include "Networking.h"    // 추가
#include "Sockets.h"       // 추가

TMap<int32, ATutorialCharacter*> ATutorialCharacter::RemotePlayers;
TMap<int32, AKillerCharacter*> ATutorialCharacter::RemoteKillers;

ATutorialCharacter::ATutorialCharacter()
{
    bCanVault = false;
    PrimaryActorTick.bCanEverTick = true;

    // 카메라 및 스프링암 설정
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

    // 메시 위치 및 회전 설정
    GetMesh()->SetRelativeLocationAndRotation(
        FVector(0.f, 0.f, -88.f),
        FRotator(0.f, -90.f, 0.f)
    );

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> SM(
        TEXT("/Script/Engine.SkeletalMesh'/Game/Animation/Walking.Walking'")
    );
    if (SM.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(SM.Object);
    }

    
    // 이동 설정
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

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        RemotePlayers.Empty();
    }

    if (IsPlayerControlled() && IsLocallyControlled())
    {
        NetworkWorker = new FNetworkWorker(TEXT("127.0.0.1"), 7777);
        NetworkWorker->SetOwnerCharacter(this);
        FRunnableThread::Create(NetworkWorker, TEXT("NetworkThread"));
    }
}

void ATutorialCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 게임 종료 시 스레드를 안전하게 종료
    if (NetworkWorker)
    {
        NetworkWorker->Stop();
        // 메모리 해제는 소멸자에서 처리됨
    }

    Super::EndPlay(EndPlayReason);
    ATutorialCharacter::RemotePlayers.Empty();
    ATutorialCharacter::RemoteKillers.Empty();
}

void ATutorialCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TraceForInteractable();

    // 내 캐릭터인 경우에만 서버로 위치 전송
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
        IInteractionInterface::Execute_UpdateInteract(CurrentInteractable, DeltaTime);
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

        // 위치 정보
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
        MovePkt.Data.bIsSprinting = (GetCharacterMovement()->MaxWalkSpeed > 600.0f);

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
    // 기존 1000에서 150 차감한 850 적용
    GetCharacterMovement()->MaxWalkSpeed = 850.0f;
}

void ATutorialCharacter::EndSprint()
{
    // 기존 600에서 150 차감한 450 적용
    GetCharacterMovement()->MaxWalkSpeed = 450.0f;
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
    if (!Camera) return; // 카메라가 없으면 실행 안 함

    FHitResult Hit;
    // 1. 카메라의 현재 월드 위치와 정면 방향을 가져옵니다.
    FVector StartLocation = Camera->GetComponentLocation();
    FVector ForwardVector = Camera->GetForwardVector();

    // 2. 시작점(Start)을 캐릭터 몸 밖으로 1미터 정도 밀어냅니다. (자기 자신 충돌 방지)
    FVector TraceStart = StartLocation + (ForwardVector * 100.f);

    // 3. 끝점(End)은 시작점에서 300 unit(3미터) 더 앞으로 설정합니다.
    FVector TraceEnd = TraceStart + (ForwardVector * 300.f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 본인(캐릭터 전체) 무시

    // 4. LineTrace 실행
    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

    AActor* NewInteractable = nullptr;
    if (bHit && Hit.GetActor() && Hit.GetActor()->Implements<UInteractionInterface>())
    {
        NewInteractable = Hit.GetActor();
    }

    // 대상이 바뀌거나 시야에서 벗어나면 Cancel 처리
    if (CurrentInteractable != NewInteractable)
    {
        if (IsInteracting && CurrentInteractable)
        {
            IInteractionInterface::Execute_CancelInteract(CurrentInteractable);
            IsInteracting = false;
        }
        CurrentInteractable = NewInteractable;
    }
}

void ATutorialCharacter::StartInteraction()
{
    if (CurrentInteractable)
    {
        IInteractionInterface::Execute_StartInteract(CurrentInteractable, this);
        IsInteracting = true;
    }
}

void ATutorialCharacter::CancelInteraction()
{
    if (IsInteracting && CurrentInteractable)
    {
        IInteractionInterface::Execute_CancelInteract(CurrentInteractable);
        IsInteracting = false;
    }
}

void ATutorialCharacter::PlayHitReaction()
{
    // 1. 이미 누워있거나, '피격 쿨타임' 중이라면 함수 실행 안 함
    if (IsDowned || !bCanBeHit) return;

    // 2. 일단 맞았으니 즉시 무적 상태로 전환
    bCanBeHit = false;

    // 3. 체력 감소 및 로그
    CurrentHealth--;
    UE_LOG(LogTemp, Warning, TEXT("도망자 피격됨! 남은 체력: %d"), CurrentHealth);

    // 4. 체력에 따른 분기 처리
    if (CurrentHealth > 0)
    {
        if (HitMontage)
        {
            PlayAnimMontage(HitMontage);
        }

        // --- 피격 무적 타이머 설정 ---
        // 살인마의 공격 판정이 완전히 끝날 때까지(약 0.5초~0.8초) 다시 안 맞게 설정
        FTimerHandle HitCooldownTimer;
        GetWorldTimerManager().SetTimer(HitCooldownTimer, FTimerDelegate::CreateLambda([this]()
            {
                bCanBeHit = true;
                UE_LOG(LogTemp, Log, TEXT("도망자 이제 다시 맞을 수 있음"));
            }), 1.0f, false);
        // --------------------------
    }
    else
    {
        // 두 번째 타격: 빈사(눕기)
        IsDowned = true;

        if (DownedMontage)
        {
            PlayAnimMontage(DownedMontage);
        }

        GetCapsuleComponent()->SetCapsuleHalfHeight(30.0f);
        GetCharacterMovement()->DisableMovement();
        bUseControllerRotationYaw = false;

        UE_LOG(LogTemp, Error, TEXT("도망자 빈사 상태(Downed)!"));
    }
}

void ATutorialCharacter::UpdateRemotePlayer(int32 PlayerId, FVector Location, float RotationYaw, float Forward, float Right, bool bSprint)
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    if (!IsValid(this)) return;

    // 1. 맵에 해당 ID가 있는지 확인
    if (RemotePlayers.Contains(PlayerId))
    {
        // 안전하게 캐릭터로 형변환
        ATutorialCharacter* Target = Cast<ATutorialCharacter>(RemotePlayers[PlayerId]);

        // [수정] Target이 메모리에 살아있기만 하면 업데이트 진행
        if (IsValid(Target))
        {
            Target->SetActorLocation(Location);
            Target->SetActorRotation(FRotator(0.0f, RotationYaw, 0.0f));

            Target->RemoteForwardValue = Forward;
            Target->RemoteRightValue = Right;
            Target->RemoteIsSprinting = bSprint;

            // 여기서 함수 종료 (성공적으로 업데이트함)
            return;
        }
        else
        {
            // 가비지 컬렉션 등으로 사라졌다면 맵에서 제거
            RemotePlayers.Remove(PlayerId);
        }
    }

    // 2. 맵에 없거나 Target이 유효하지 않은 경우에만 스폰 (여기가 else 없이 오는 곳)
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATutorialCharacter* NewPlayer = World->SpawnActor<ATutorialCharacter>(GetClass(), Location, FRotator(0.0f, RotationYaw, 0.0f), SpawnParams);

    if (NewPlayer)
    {
        NewPlayer->MyPlayerId = PlayerId;
        // 맵에 확실히 등록
        NewPlayer->AutoPossessPlayer = EAutoReceiveInput::Disabled;
        NewPlayer->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

            return;
        }

        RemoteKillers.Remove(PlayerId);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;

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