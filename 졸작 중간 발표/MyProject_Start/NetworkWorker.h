#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Shared.h"

class ATutorialCharacter;
class AKillerCharacter;

class FNetworkWorker : public FRunnable
{
public:
    FNetworkWorker(FString IP, int32 Port);
    virtual ~FNetworkWorker();

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    FSocket* GetSocket() { return Socket; }
    void SetOwnerCharacter(ATutorialCharacter* InCharacter) { OwnerTutorialCharacter = InCharacter; OwnerKillerCharacter = nullptr; }
    void SetOwnerKiller(AKillerCharacter* InCharacter) { OwnerKillerCharacter = InCharacter; OwnerTutorialCharacter = nullptr; }

private:
    FSocket* Socket;
    FString ServerIP;
    int32 ServerPort;
    bool bRunning;
    ATutorialCharacter* OwnerTutorialCharacter;
    AKillerCharacter* OwnerKillerCharacter;
    int32 CachedMyPlayerId = -1;
};
