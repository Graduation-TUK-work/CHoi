#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeDoor.generated.h"

UCLASS()
class MYPROJECT_START_API AEscapeDoor : public AActor
{
    GENERATED_BODY()

public:
    AEscapeDoor();

    // 게임모드가 호출할 함수 (BlueprintNativeEvent로 만들어 연출은 블프에서 하기 편하게 함)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Escape")
    void OpenDoor();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UStaticMeshComponent* DoorMesh;

    // 문이 열렸는지 상태 저장
    UPROPERTY(BlueprintReadOnly, Category = "Escape")
    bool bIsOpen = false;
};