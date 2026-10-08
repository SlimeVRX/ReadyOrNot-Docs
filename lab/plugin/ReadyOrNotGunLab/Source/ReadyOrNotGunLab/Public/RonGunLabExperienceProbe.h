#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RonGunLabExperienceProbe.generated.h"
class APlayerCharacter;
class ABaseMagazineWeapon;
class AReadyOrNotCharacter;
class ARonGunLab;
class ATrainingTarget;
/** Opt-in test driver. Injects player input; observes native state without implementing gameplay. */
UCLASS()
class READYORNOTGUNLAB_API ARonGunLabExperienceProbe : public AActor
{
    GENERATED_BODY()
public:
    ARonGunLabExperienceProbe();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void MakeSteps();
    void Key(FKey Key, EInputEvent Event);
    void Observe(const FString& Name);
    void Save(bool bComplete);
    void AimAtNativeTarget();
    void AimAtFixture(bool bGlass);
    void SelectCatalogClass(const FString& ClassSuffix);
    void PressSelectorToward(uint8 DesiredMode);
    void InstallNativeAttachment(const FString& Type);
    UFUNCTION() void Fired(ABaseMagazineWeapon* Weapon, bool bServer);
    UFUNCTION() void TrainingHit(ATrainingTarget* HitTarget);
    UFUNCTION() void GlassHit(AActor* DamagedActor, float Damage, AController* DamageInstigator, FVector Location, UPrimitiveComponent* Component, FName Bone, FVector Direction, const UDamageType* DamageType, AActor* Causer);
    UPROPERTY() APlayerCharacter* Player = nullptr;
    UPROPERTY() ARonGunLab* Lab = nullptr;
    UPROPERTY() ABaseMagazineWeapon* BoundWeapon = nullptr;
    UPROPERTY() AReadyOrNotCharacter* Target = nullptr;
    UPROPERTY() AActor* GlassTarget = nullptr;
    UPROPERTY() ATrainingTarget* TrainingTarget = nullptr;
    int32 GlassHitEvents = 0, MaxGlassComponents = 0, TrainingHitEvents = 0;
    TArray<TPair<float, TFunction<void()>>> Steps;
    TArray<TSharedPtr<class FJsonValue>> Observations;
    FString RunId;
    float Alive = 0, Time = 0, MaxPendingRecoil = 0;
    int32 Step = 0, LocalFireEvents = 0, ServerFireEvents = 0;
    bool bStarted = false, bFinished = false, bScreenshotRequested = false;
    bool bLoadoutRequested = false, bLoadoutScreenshot = false, bLoadoutObserved = false;
    float LoadoutRequestTime = 0, LoadoutScreenshotTime = 0;
    float ScreenshotTime = 0;
    float MaxFrameSeconds = 0;
    double MaxCameraSequenceTranslation = 0, MaxCameraSequenceRotation = 0, MaxCameraSequencePlayback = 0;
    double MaxCameraOscillationRemaining = 0;
    int32 MaxActiveCameraSequences = 0, CameraRecoveryTracks = 0;
    FString CameraManagerClass, CachedShakeModifierClass, ActiveShakeClasses;
    int32 CameraModifierCount = 0, NativeCameraModifierCount = 0;
    int32 MaxActiveShakes = 0, MaxLegacyShakes = 0, MaxRecoveredShakes = 0, MaxSequencePatterns = 0, MaxPlayingSequences = 0;
};
