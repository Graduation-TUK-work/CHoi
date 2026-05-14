#include "LobbyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "MyGameInstance.h"
#include "Networking.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

ALobbyPlayerController::ALobbyPlayerController()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> LobbyWidgetFinder(TEXT("/Game/1_Map/WBP_Lobby"));
    if (LobbyWidgetFinder.Succeeded())
    {
        LobbyWidgetClass = LobbyWidgetFinder.Class;
    }
}

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    SetInputMode(InputMode);

    ShowServerIpWidget();
}

void ALobbyPlayerController::ShowServerIpWidget()
{
    if (!GEngine || !GEngine->GameViewport)
    {
        ShowLobbyWidget();
        return;
    }

    const FString DefaultServerIP = GetGameInstance<UMyGameInstance>()
        ? GetGameInstance<UMyGameInstance>()->GetServerIP()
        : TEXT("127.0.0.1");

    SAssignNew(ServerIpWidget, SScaleBox)
        .Stretch(EStretch::ScaleToFit)
        [
            SNew(SBox)
            .WidthOverride(520.0f)
            .HeightOverride(320.0f)
            [
                SNew(SBorder)
                .Padding(32.0f)
                .BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.035f, 0.92f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("SERVER IP")))
                        .Justification(ETextJustify::Center)
                        .ColorAndOpacity(FSlateColor(FLinearColor::White))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        SAssignNew(ServerIpTextBox, SEditableTextBox)
                        .Text(FText::FromString(DefaultServerIP))
                        .HintText(FText::FromString(TEXT("127.0.0.1")))
                        .SelectAllTextWhenFocused(true)
                        .OnTextCommitted_UObject(this, &ALobbyPlayerController::HandleServerIpCommitted)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                    [
                        SAssignNew(ServerIpErrorText, STextBlock)
                        .Text(FText::GetEmpty())
                        .ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.25f, 0.2f, 1.0f)))
                        .Justification(ETextJustify::Center)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("CONNECT")))
                        .OnClicked_UObject(this, &ALobbyPlayerController::HandleServerIpConfirmed)
                    ]
                ]
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(ServerIpWidget.ToSharedRef(), 100);

    if (ServerIpTextBox.IsValid())
    {
        FSlateApplication::Get().SetKeyboardFocus(ServerIpTextBox.ToSharedRef());
    }
}

void ALobbyPlayerController::ShowLobbyWidget()
{
    if (LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
        if (LobbyWidget)
        {
            LobbyWidget->AddToViewport();
        }
    }
}

FReply ALobbyPlayerController::HandleServerIpConfirmed()
{
    if (!ServerIpTextBox.IsValid())
    {
        return FReply::Handled();
    }

    FString EnteredIP = ServerIpTextBox->GetText().ToString();
    EnteredIP.TrimStartAndEndInline();

    FIPv4Address ParsedAddress;
    if (!FIPv4Address::Parse(EnteredIP, ParsedAddress))
    {
        SetServerIpError(FText::FromString(TEXT("Enter a valid IPv4 address. Example: 127.0.0.1")));
        return FReply::Handled();
    }

    if (UMyGameInstance* GI = GetGameInstance<UMyGameInstance>())
    {
        GI->SetServerIP(EnteredIP);
    }

    if (GEngine && GEngine->GameViewport && ServerIpWidget.IsValid())
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(ServerIpWidget.ToSharedRef());
    }

    ServerIpWidget.Reset();
    ServerIpTextBox.Reset();
    ServerIpErrorText.Reset();

    ShowLobbyWidget();
    return FReply::Handled();
}

void ALobbyPlayerController::HandleServerIpCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        HandleServerIpConfirmed();
    }
}

void ALobbyPlayerController::SetServerIpError(const FText& ErrorText)
{
    if (ServerIpErrorText.IsValid())
    {
        ServerIpErrorText->SetText(ErrorText);
    }
}
