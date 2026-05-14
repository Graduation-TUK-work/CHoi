#include "MyGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "NetworkWorker.h"
#include "Shared.h"

void UMyGameInstance::SetServerIP(const FString& InServerIP)
{
    ServerIP = InServerIP;
    ServerIP.TrimStartAndEndInline();
}

FString UMyGameInstance::GetServerIP() const
{
    FString Result = ServerIP;
    Result.TrimStartAndEndInline();
    return Result.IsEmpty() ? FNetworkWorker::GetDefaultServerIP() : Result;
}

void UMyGameInstance::SelectKiller()
{
    LocalSelectedRole = ROLE_KILLER;
}

void UMyGameInstance::SelectRunner()
{
    LocalSelectedRole = ROLE_SURVIVOR;
}

void UMyGameInstance::SelectSurvivor()
{
    SelectRunner();
}

void UMyGameInstance::SelectKillerAndOpenGame()
{
    SelectKiller();
    OpenSelectedGameLevel();
}

void UMyGameInstance::SelectRunnerAndOpenGame()
{
    SelectRunner();
    OpenSelectedGameLevel();
}

void UMyGameInstance::SelectSurvivorAndOpenGame()
{
    SelectSurvivor();
    OpenSelectedGameLevel();
}

FString UMyGameInstance::GetSelectedGameModeOption() const
{
    if (LocalSelectedRole == ROLE_KILLER)
    {
        return TEXT("game=/Script/MyProject_Start.KillerGameModeBase");
    }

    return TEXT("game=/Script/MyProject_Start.MyGameModeBase");
}

void UMyGameInstance::OpenSelectedGameLevel()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (LocalSelectedRole == ROLE_NONE)
    {
        SelectRunner();
    }

    UGameplayStatics::OpenLevel(World, GameLevelName, true, GetSelectedGameModeOption());
}

void UMyGameInstance::SendReady()
{
    OpenSelectedGameLevel();
}
