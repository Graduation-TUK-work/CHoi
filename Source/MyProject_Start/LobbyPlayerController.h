#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "Types/SlateEnums.h"
#include "LobbyPlayerController.generated.h"

class UUserWidget;
class SEditableTextBox;
class STextBlock;
class SWidget;

UCLASS()
class MYPROJECT_START_API ALobbyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALobbyPlayerController();

protected:
    virtual void BeginPlay() override;

private:
    void ShowServerIpWidget();
    void ShowLobbyWidget();
    FReply HandleServerIpConfirmed();
    void HandleServerIpCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    void SetServerIpError(const FText& ErrorText);

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    TSubclassOf<UUserWidget> LobbyWidgetClass;

    UPROPERTY()
    UUserWidget* LobbyWidget;

    TSharedPtr<SWidget> ServerIpWidget;
    TSharedPtr<SEditableTextBox> ServerIpTextBox;
    TSharedPtr<STextBlock> ServerIpErrorText;
};
