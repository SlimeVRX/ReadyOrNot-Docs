#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "RonGunLab.h"

class FReadyOrNotGunLabModule : public IModuleInterface
{
    IConsoleObject* Console = nullptr;
public:
    virtual void StartupModule() override
    {
        Console = IConsoleManager::Get().RegisterConsoleCommand(TEXT("ronlab"), TEXT("ronlab next|prev|select <1-based index>|refill|reset|audit|probe"),
            FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
            {
                if (!World) return;
                for (TActorIterator<ARonGunLab> It(World); It; ++It) { It->Command(Args); break; }
            }), ECVF_Default);
    }
    virtual void ShutdownModule() override
    {
        if (Console) IConsoleManager::Get().UnregisterConsoleObject(Console);
    }
};
IMPLEMENT_MODULE(FReadyOrNotGunLabModule, ReadyOrNotGunLab)
