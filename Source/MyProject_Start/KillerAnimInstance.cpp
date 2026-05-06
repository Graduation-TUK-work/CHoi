#include "KillerAnimInstance.h"
#include "GameFramework/Pawn.h"
#include "MyProject_Start/KillerCharacter.h"
#include "Math/UnrealMathUtility.h"

namespace
{
    float CalculateDirectionFromVelocity(const FVector& Velocity, const FRotator& ActorRotation)
    {
        FVector HorizontalVelocity = Velocity;
        HorizontalVelocity.Z = 0.0f;

        if (HorizontalVelocity.IsNearlyZero())
        {
            return 0.0f;
        }

        const FVector LocalVelocity = ActorRotation.UnrotateVector(HorizontalVelocity.GetSafeNormal());
        return FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
    }
}

void UKillerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    APawn* Pawn = TryGetPawnOwner();
    if (!IsValid(Pawn))
    {
        return;
    }

    AKillerCharacter* Character = Cast<AKillerCharacter>(Pawn);
    if (!Character)
    {
        return;
    }

    // 1. IsLocallyControlled()를 추가하여 로컬/리모트 구분을 더 명확히 합니다.
    if (Character->IsPlayerControlled() && Character->IsLocallyControlled())
    {
        Speed = Character->GetMovementSpeed(); // (참고: 함수가 아니라 변수면 괄호 제거)
        Direction = CalculateDirectionFromVelocity(Character->GetVelocity(), Character->GetActorRotation());
        bIsAttacking = Character->bIsAttacking;
    }
    else
    {
        // 1. 서버에서 받은 '목표 속도' (0 또는 600으로 뚝뚝 끊기는 값)
        float TargetSpeed = Character->GetRemoteMovementSpeed();

        // 2. 속도는 일반 숫자이므로 FInterpTo 사용 (정상 작동)
        Speed = FMath::FInterpTo(Speed, TargetSpeed, DeltaSeconds, 10.0f);

        bIsAttacking = Character->GetRemoteIsAttacking();

        // 3. 목표 속도가 있을 때만 방향(Direction) 계산 및 부드러운 회전 적용
        if (TargetSpeed > 0.1f)
        {
            float TargetDirection = FMath::RadiansToDegrees(FMath::Atan2(Character->GetRemoteRightValue(), Character->GetRemoteForwardValue()));

            // =============================================================
            // [수정된 부분] 회전 전용 보간 함수(RInterpTo)를 사용하여 빙글 도는 현상 방지
            // =============================================================
            FRotator CurrentRot(0.0f, Direction, 0.0f); // 현재 방향을 Rotator로 만듦
            FRotator TargetRot(0.0f, TargetDirection, 0.0f); // 목표 방향을 Rotator로 만듦

            // 가장 짧은 경로로 부드럽게 회전시킵니다.
            FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, 8.0f);

            // 계산된 Rotator에서 다시 Yaw(방향 값)만 쏙 빼서 Direction에 넣습니다.
            Direction = NewRot.Yaw;
        }
    }
}