#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RonGunLabFixtures.generated.h"

class IConsoleObject;
class AVolume;
class UBlueprint;

/** Runs native AI spawners placed by the lab. Does not implement damage or AI. */
UCLASS()
class READYORNOTGUNLAB_API ARonGunLabFixtures : public AActor
{
    GENERATED_BODY()
public:
    ARonGunLabFixtures();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    void ResetTargets();
    /** Editor-only authoring helper for native volume brush geometry. */
    UFUNCTION(BlueprintCallable, Category="Gun Lab|Editor")
    static bool BuildEditorBoxVolume(AVolume* Volume, FVector FullSize);
    /** Restore three disconnected original event entry wires in a lab-only copy. */
    UFUNCTION(BlueprintCallable, Category="Gun Lab|Editor")
    static FString RestoreOriginalGlassEvents(UBlueprint* Blueprint);

private:
    void EnsureTargets(bool bReplaceExisting);
    void Command(const TArray<FString>& Args);
    UFUNCTION()
    void OnNativeGlassPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
        FVector HitLocation, UPrimitiveComponent* HitComponent, FName BoneName,
        FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser);
    IConsoleObject* RegisteredCommand = nullptr;
    float StartupSeconds = 0.0f;
    bool bInitialized = false;
};
