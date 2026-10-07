#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RonGunLab.generated.h"

class ABaseMagazineWeapon;
class APlayerCharacter;

/** Selection/instrumentation only. Firing, recoil, ADS and reloading belong to the host game. */
UCLASS()
class READYORNOTGUNLAB_API ARonGunLab : public AActor
{
    GENERATED_BODY()
public:
    ARonGunLab();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category="Gun Lab")
    TArray<TSoftClassPtr<ABaseMagazineWeapon>> WeaponClasses;
    UPROPERTY(EditAnywhere, Category="Gun Lab")
    int32 SuppliedMagazines = 4;
    UPROPERTY(EditAnywhere, Category="Gun Lab")
    FVector FiringLine = FVector(0, 0, 120);
    UPROPERTY(EditAnywhere, Category="Gun Lab")
    bool bShowOverlay = true;

    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    bool SelectWeapon(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    void CycleWeapon(int32 Direction);
    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    void Refill();
    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    void ResetPosition();
    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    void StartSmokeTest();
    UFUNCTION(BlueprintCallable, Category="Gun Lab")
    void StartActionProbe();
    void Command(const TArray<FString>& Args);

private:
    APlayerCharacter* GetNativePlayer() const;
    void WriteReceipt(const FString& Status, const FString& Detail);
    void SaveReceipts();
    void FinishSmoke();
    UPROPERTY(Transient)
    ABaseMagazineWeapon* CurrentWeapon = nullptr;
    UPROPERTY(Transient)
    ABaseMagazineWeapon* PreviousWeapon = nullptr;
    int32 CurrentIndex = INDEX_NONE;
    int32 ActiveWeaponIndex = INDEX_NONE;
    bool bSmoke = false;
    bool bExitAfterSmoke = false;
    bool bInitialized = false;
    bool bPendingEquip = false;
    bool bLastInstantFallback = false;
    float PendingSeconds = 0;
    float NextSmokeTime = 0;
    float AliveSeconds = 0;
    int32 SmokeNextIndex = 0;
    int32 ProbeStage = 0;
    float ProbeSeconds = 0;
    float ProbeAmmoBefore = 0;
    float ProbeAmmoAfterFire = 0;
    bool bProbeAiming = false;
    bool bProbeOnEquip = false;
    bool bExitAfterProbe = false;
    bool bCaptureRequested = false;
    float CaptureTime = 0;
    FString StatusText;
    FString ReceiptMode;
    FString RunId;
    TArray<TSharedPtr<class FJsonValue>> Receipts;
};
