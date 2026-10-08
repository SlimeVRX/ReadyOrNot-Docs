#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RonGunLabCameraRecovery.generated.h"

class UCameraAnimationSequence;
class UMovieScene3DTransformSection;
class APlayerCameraManager;

struct FRonGunLabCameraSnapshot
{
    int32 AssignedTrackCount = 0;
    int32 ActiveSequenceCount = 0;
    FString CameraManagerClass;
    FString CachedShakeModifierClass;
    FString ActiveShakeClasses;
    int32 ModifierCount = 0;
    int32 NativeModifierCount = 0;
    int32 ActiveShakeCount = 0;
    int32 LegacyShakeCount = 0;
    int32 RecoveredShakeCount = 0;
    int32 SequencePatternCount = 0;
    int32 PlayingSequenceCount = 0;
    double MaxOscillationTimeRemaining = 0.0;
    double MaxTranslationCm = 0.0;
    double MaxRotationDegrees = 0.0;
    double MaxPlaybackSeconds = 0.0;
};

// Read-only observation of sequence players inside active native legacy shakes.
FRonGunLabCameraSnapshot SnapshotRonGunLabCameraRecovery(APlayerCameraManager* CameraManager);

// Validation only: run the actual native sequence shake evaluator over migrated
// assets before the converter is allowed to enable their runtime mappings.
UCLASS()
class READYORNOTGUNLAB_API URonGunLabCameraValidation : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Gun Lab|Validation")
    static TArray<FTransform> SampleNativeCameraSequence(UWorld* World, UCameraAnimationSequence* Sequence, const TArray<float>& Times);

    // Bulk JSON boundaries keep validation independent of per-key/pose Python
    // wrappers. Authored data and full output are kept in the local Saved tree.
    UFUNCTION(BlueprintCallable, Category="Gun Lab|Validation")
    static FString AuthorAndReadCameraChannels(UMovieScene3DTransformSection* Section, const FString& RequestJson, bool bWriteNewChannels);

    UFUNCTION(BlueprintCallable, Category="Gun Lab|Validation")
    static FString SampleNativeCameraSequenceJson(UWorld* World, UCameraAnimationSequence* Sequence, const FString& TimesJson);
};

// Assign converted, authored camera tracks to the existing native shake objects.
// The mapping and sequence assets are generated locally; no authored data is embedded here.
void ApplyRonGunLabCameraRecovery(UWorld* World);
void RestoreRonGunLabCameraRecovery(UWorld* World);
