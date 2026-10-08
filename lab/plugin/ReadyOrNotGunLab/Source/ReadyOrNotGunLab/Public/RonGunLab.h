#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RonGunLab.generated.h"

class ABaseMagazineWeapon;
class APlayerCharacter;
class UReadyOrNotLoadoutManager;

/** Selection/instrumentation only. Firing, recoil, ADS and reloading belong to the host game. */
UCLASS()
class READYORNOTGUNLAB_API ARonGunLab : public AActor
{
    GENERATED_BODY()
public:
    ARonGunLab();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
    void AppendNativeReadiness(const TSharedPtr<class FJsonObject>& Row) const;

private:
    APlayerCharacter* GetNativePlayer() const;
    bool ShouldUseNativeLoadout(UClass* WeaponClass);
    bool ApplyNativeLoadoutSelection(UClass* WeaponClass, APlayerCharacter* Player);
    bool IsNativeLoadoutWeapon(const ABaseMagazineWeapon* Weapon) const;
    bool HandleLoadoutCommand(const TArray<FString>& Args);
    void ReconcileNativeSelection();
    void EnsureNativeCrosshair();
    UReadyOrNotLoadoutManager* GetSessionLoadoutManager();
    UPROPERTY(Transient)
    UReadyOrNotLoadoutManager* SessionLoadoutManager = nullptr;
    void WriteReceipt(const FString& Status, const FString& Detail);
    void SaveReceipts();
    void FinishSmoke();
    void ContinueProbeBatch();
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
    bool bLastNativeLoadout = false;
    float PendingSeconds = 0;
    float NextSmokeTime = 0;
    float AliveSeconds = 0;
    int32 SmokeNextIndex = 0;
    int32 ProbeStage = 0;
    float ProbeSeconds = 0;
    float ProbeAmmoBefore = 0;
    float ProbeAmmoAfterFire = 0;
    bool bProbeAiming = false;
    bool bProbeReloadRequested = false;
    bool bProbeCanReload = false;
    TArray<int32> ProbeIndices;
    int32 ProbeCursor = 0;
    bool bProbeOnEquip = false;
    bool bExitAfterProbe = false;
    bool bCaptureRequested = false;
    float CaptureTime = 0;
    FString StatusText;
    FString SelectionRouteReason;
    FString ReceiptMode;
    FString RunId;
    TArray<TSharedPtr<class FJsonValue>> Receipts;
};
