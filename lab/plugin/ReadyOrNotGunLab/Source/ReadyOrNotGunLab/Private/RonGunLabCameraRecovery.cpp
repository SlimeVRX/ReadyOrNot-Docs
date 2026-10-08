#include "RonGunLabCameraRecovery.h"
#include "LegacyCameraShake.h"
#include "CameraAnimationSequence.h"
#include "SequenceCameraShake.h"
#include "CameraAnimationSequencePlayer.h"
#include "Camera/CameraModifier_CameraShake.h"
#include "Camera/PlayerCameraManager.h"
#include "Info/RONCameraModifier_CameraShake.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Policies/CondensedJsonPrintPolicy.h"

namespace
{
    FString CameraJson(const TSharedRef<FJsonObject>& Object)
    {
        FString Text;
        FJsonSerializer::Serialize(Object, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
        return Text;
    }

    FString CameraJsonError(const FString& Message)
    {
        TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("error"), Message);
        return CameraJson(Object);
    }
}

FString URonGunLabCameraValidation::AuthorAndReadCameraChannels(UMovieScene3DTransformSection* Section, const FString& RequestJson, bool bWriteNewChannels)
{
    if (!Section) return CameraJsonError(TEXT("Missing transform section"));
    TSharedPtr<FJsonObject> Request;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(RequestJson), Request) || !Request.IsValid())
        return CameraJsonError(TEXT("Invalid channel request JSON"));
    const TArray<TSharedPtr<FJsonValue>>* RequestedChannels = nullptr;
    if (!Request->TryGetArrayField(TEXT("channels"), RequestedChannels) || RequestedChannels->Num() != 6)
        return CameraJsonError(TEXT("Expected six movement channels"));
    const auto Channels = Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
    if (Channels.Num() != 9) return CameraJsonError(TEXT("Expected nine native double transform channels"));

    TArray<FFrameNumber> PreparedTimes[6];
    TArray<FMovieSceneDoubleValue> PreparedValues[6];
    for (int32 Axis = 0; Axis < 6; ++Axis)
    {
        const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
        if (!(*RequestedChannels)[Axis]->TryGetArray(Rows)) return CameraJsonError(TEXT("Invalid channel key array"));
        for (const TSharedPtr<FJsonValue>& RowValue : *Rows)
        {
            const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
            if (!RowValue->TryGetArray(Row) || Row->Num() != 5) return CameraJsonError(TEXT("Invalid key tuple"));
            double Tick = 0, Value = 0, Arrive = 0, Leave = 0;
            FString Mode;
            if (!(*Row)[0]->TryGetNumber(Tick) || !(*Row)[1]->TryGetNumber(Value)
                || !(*Row)[2]->TryGetNumber(Arrive) || !(*Row)[3]->TryGetNumber(Leave) || !(*Row)[4]->TryGetString(Mode)
                || !FMath::IsFinite(Tick) || !FMath::IsFinite(Value) || !FMath::IsFinite(Arrive) || !FMath::IsFinite(Leave)
                || Tick < MIN_int32 || Tick > MAX_int32 || Tick != static_cast<int32>(Tick))
                return CameraJsonError(TEXT("Invalid numeric channel key"));
            if (!PreparedTimes[Axis].IsEmpty() && Tick <= PreparedTimes[Axis].Last().Value)
                return CameraJsonError(TEXT("Channel keys must be strictly increasing"));
            FMovieSceneDoubleValue Key(Value);
            if (Mode == TEXT("linear")) Key.InterpMode = RCIM_Linear;
            else if (Mode == TEXT("constant")) Key.InterpMode = RCIM_Constant;
            else if (Mode == TEXT("cubic")) Key.InterpMode = RCIM_Cubic;
            else return CameraJsonError(TEXT("Unsupported channel interpolation"));
            Key.TangentMode = RCTM_Break;
            Key.Tangent.TangentWeightMode = RCTWM_WeightedNone;
            Key.Tangent.ArriveTangent = static_cast<float>(Arrive);
            Key.Tangent.LeaveTangent = static_cast<float>(Leave);
            PreparedTimes[Axis].Add(FFrameNumber(static_cast<int32>(Tick)));
            PreparedValues[Axis].Add(Key);
        }
    }
    if (bWriteNewChannels)
    {
        // Only accept empty channels. A rerun must read/validate the saved graph;
        // it must never destroy tracks or invalidate scripting channel proxies.
        for (FMovieSceneDoubleChannel* Channel : Channels)
            if (Channel->GetData().GetTimes().Num() != 0) return CameraJsonError(TEXT("Refusing to replace nonempty channels"));
        Section->Modify();
        for (int32 Axis = 0; Axis < 9; ++Axis)
        {
            Channels[Axis]->SetDefault(Axis < 6 ? 0.0 : 1.0);
            if (Axis < 6) Channels[Axis]->Set(MoveTemp(PreparedTimes[Axis]), MoveTemp(PreparedValues[Axis]));
        }
        Section->MarkAsChanged();
    }
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Defaults, Readback;
    for (int32 Axis = 0; Axis < 9; ++Axis)
    {
        const TOptional<double> Default = Channels[Axis]->GetDefault();
        if (!Default.IsSet()) return CameraJsonError(TEXT("Missing transform channel default"));
        Defaults.Add(MakeShared<FJsonValueNumber>(Default.GetValue()));
        const auto Data = Channels[Axis]->GetData();
        const auto Times = Data.GetTimes();
        const auto Values = Data.GetValues();
        if (Times.Num() != Values.Num()) return CameraJsonError(TEXT("Invalid native channel key count"));
        TArray<TSharedPtr<FJsonValue>> Keys;
        for (int32 Index = 0; Index < Times.Num(); ++Index)
        {
            const FMovieSceneDoubleValue& Key = Values[Index];
            const TCHAR* Mode = Key.InterpMode == RCIM_Linear ? TEXT("linear") : Key.InterpMode == RCIM_Constant ? TEXT("constant") : Key.InterpMode == RCIM_Cubic ? TEXT("cubic") : TEXT("unsupported");
            TArray<TSharedPtr<FJsonValue>> Row;
            Row.Add(MakeShared<FJsonValueNumber>(Times[Index].Value));
            Row.Add(MakeShared<FJsonValueNumber>(Key.Value));
            Row.Add(MakeShared<FJsonValueNumber>(Key.Tangent.ArriveTangent));
            Row.Add(MakeShared<FJsonValueNumber>(Key.Tangent.LeaveTangent));
            Row.Add(MakeShared<FJsonValueString>(Mode));
            Row.Add(MakeShared<FJsonValueNumber>(static_cast<int32>(Key.Tangent.TangentWeightMode)));
            Row.Add(MakeShared<FJsonValueNumber>(static_cast<int32>(Key.TangentMode)));
            Keys.Add(MakeShared<FJsonValueArray>(Row));
        }
        Readback.Add(MakeShared<FJsonValueArray>(Keys));
    }
    Result->SetArrayField(TEXT("defaults"), Defaults);
    Result->SetArrayField(TEXT("channels"), Readback);
    return CameraJson(Result);
}

FString URonGunLabCameraValidation::SampleNativeCameraSequenceJson(UWorld* World, UCameraAnimationSequence* Sequence, const FString& TimesJson)
{
    if (!World || !Sequence) return CameraJsonError(TEXT("Missing world or sequence"));
    TSharedPtr<FJsonObject> Request;
    const TArray<TSharedPtr<FJsonValue>>* TimesArray = nullptr;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TimesJson), Request) || !Request.IsValid()
        || !Request->TryGetArrayField(TEXT("times"), TimesArray)) return CameraJsonError(TEXT("Invalid sample time JSON"));
    TArray<float> Times;
    for (const TSharedPtr<FJsonValue>& Value : *TimesArray)
    {
        double Time = 0;
        if (!Value->TryGetNumber(Time) || !FMath::IsFinite(Time) || Time < 0 || Time > MAX_flt)
            return CameraJsonError(TEXT("Invalid sample time"));
        Times.Add(static_cast<float>(Time));
    }
    const TArray<FTransform> Poses = SampleNativeCameraSequence(World, Sequence, Times);
    TArray<TSharedPtr<FJsonValue>> Rows;
    Rows.Reserve(Poses.Num());
    for (const FTransform& Pose : Poses)
    {
        const FVector P = Pose.GetTranslation();
        const FQuat Q = Pose.GetRotation();
        TArray<TSharedPtr<FJsonValue>> Row;
        for (double Value : {P.X, P.Y, P.Z, Q.X, Q.Y, Q.Z, Q.W}) Row.Add(MakeShared<FJsonValueNumber>(Value));
        Rows.Add(MakeShared<FJsonValueArray>(Row));
    }
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetArrayField(TEXT("poses"), Rows);
    return CameraJson(Result);
}

TArray<FTransform> URonGunLabCameraValidation::SampleNativeCameraSequence(UWorld* World, UCameraAnimationSequence* Sequence, const TArray<float>& Times)
{
    TArray<FTransform> Result;
    if (!World || !Sequence || Times.Num() == 0) return Result;
    TStrongObjectPtr<USequenceCameraShakePattern> Pattern(NewObject<USequenceCameraShakePattern>(World));
    Pattern->Sequence = Sequence;
    Pattern->Scale = 1.f;
    Pattern->PlayRate = 1.f;
    Pattern->BlendInTime = 0.f;
    Pattern->BlendOutTime = 0.f;
    Pattern->StartShakePattern(FCameraShakeStartParams());
    for (const float Time : Times)
    {
        FCameraShakeScrubParams Params;
        Params.AbsoluteTime = Time;
        Params.POV.FOV = 90.f;
        FCameraShakeUpdateResult Sample;
        Pattern->ScrubShakePattern(Params, Sample);
        Result.Emplace(Sample.Rotation, Sample.Location);
    }
    Pattern->TeardownShakePattern();
    return Result;
}

namespace
{
    struct FCameraAssignment
    {
        TWeakObjectPtr<ULegacyCameraShake> Shake;
        TWeakObjectPtr<UCameraAnimationSequence> Original;
        TWeakObjectPtr<UCameraAnimationSequence> Recovered;
    };
    TWeakObjectPtr<UWorld> ActiveWorld;
    TArray<FCameraAssignment> Assignments;
    // Class-default edits are session-only. Keep them alive across GC while the
    // lab is open so a later weapon selection cannot reload unpatched defaults.
    TArray<TStrongObjectPtr<ULegacyCameraShake>> PinnedDefaults;

    UObject* ReadNativeObjectProperty(UObject* Object, const FName Name)
    {
        const FObjectPropertyBase* Property = Object ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
        return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
    }

    void RefreshNativeDefaultCopyList(UClass* Class)
    {
        // BP construction caches which inherited native properties differ from
        // the native CDO. A runtime CDO edit must refresh that list, otherwise a
        // newly created native weapon shake can still receive AnimSequence=null.
        if (UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class))
            BlueprintClass->UpdateCustomPropertyListForPostConstruction();
    }
}

FRonGunLabCameraSnapshot SnapshotRonGunLabCameraRecovery(APlayerCameraManager* CameraManager)
{
    FRonGunLabCameraSnapshot Result;
    if (!CameraManager || ActiveWorld.Get() != CameraManager->GetWorld()) return Result;
    Result.CameraManagerClass = CameraManager->GetClass()->GetPathName();
    UCameraModifier_CameraShake* CachedModifier = Cast<UCameraModifier_CameraShake>(ReadNativeObjectProperty(CameraManager, TEXT("CachedCameraShakeMod")));
    Result.CachedShakeModifierClass = CachedModifier ? CachedModifier->GetClass()->GetPathName() : TEXT("None");
    for (const FCameraAssignment& Assignment : Assignments)
        if (Assignment.Shake.IsValid() && Assignment.Shake->HasAnyFlags(RF_ClassDefaultObject) && Assignment.Shake->AnimSequence == Assignment.Recovered.Get())
            ++Result.AssignedTrackCount;
    // FindCameraModifierByClass tests exact class equality, so request RON's
    // modifier explicitly rather than silently missing its derived instance.
    UCameraModifier_CameraShake* Modifier = CachedModifier;
    if (!Modifier) Modifier = Cast<UCameraModifier_CameraShake>(CameraManager->FindCameraModifierByClass(URONCameraModifier_CameraShake::StaticClass()));
    if (!Modifier) Modifier = Cast<UCameraModifier_CameraShake>(CameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass()));
    const FArrayProperty* ModifiersProperty = FindFProperty<FArrayProperty>(CameraManager->GetClass(), TEXT("ModifierList"));
    if (ModifiersProperty)
    {
        const FObjectPropertyBase* Inner = CastField<FObjectPropertyBase>(ModifiersProperty->Inner);
        FScriptArrayHelper Modifiers(ModifiersProperty, ModifiersProperty->ContainerPtrToValuePtr<void>(CameraManager));
        Result.ModifierCount = Modifiers.Num();
        if (Inner) for (int32 Index = 0; Index < Modifiers.Num(); ++Index)
            if (Cast<URONCameraModifier_CameraShake>(Inner->GetObjectPropertyValue(Modifiers.GetRawPtr(Index)))) ++Result.NativeModifierCount;
    }
    if (!Modifier) return Result;
    TArray<FActiveCameraShakeInfo> ActiveShakes;
    Modifier->GetActiveCameraShakes(ActiveShakes);
    Result.ActiveShakeCount = ActiveShakes.Num();
    for (const FActiveCameraShakeInfo& Info : ActiveShakes)
    {
        if (Info.ShakeInstance)
        {
            if (!Result.ActiveShakeClasses.IsEmpty()) Result.ActiveShakeClasses += TEXT(";");
            Result.ActiveShakeClasses += Info.ShakeInstance->GetClass()->GetPathName();
        }
        ULegacyCameraShake* Shake = Cast<ULegacyCameraShake>(Info.ShakeInstance);
        if (!Shake) continue;
        ++Result.LegacyShakeCount;
        if (Shake->IsActive()) Result.MaxOscillationTimeRemaining = FMath::Max(Result.MaxOscillationTimeRemaining, static_cast<double>(Shake->OscillatorTimeRemaining));
        if (!Shake->IsActive() || !Shake->AnimSequence || !Shake->AnimSequence->GetPathName().StartsWith(TEXT("/Game/ReadyOrNot/Level/Study/CameraRecovery/"))) continue;
        ++Result.RecoveredShakeCount;
        USequenceCameraShakePattern* Pattern = Cast<USequenceCameraShakePattern>(ReadNativeObjectProperty(Shake, TEXT("SequenceShakePattern")));
        if (!Pattern) continue;
        ++Result.SequencePatternCount;
        if (Pattern->IsFinished()) continue;
        UCameraAnimationSequencePlayer* Player = Cast<UCameraAnimationSequencePlayer>(ReadNativeObjectProperty(Pattern, TEXT("Player")));
        UCameraAnimationSequenceCameraStandIn* StandIn = Cast<UCameraAnimationSequenceCameraStandIn>(ReadNativeObjectProperty(Pattern, TEXT("CameraStandIn")));
        if (!Player || !StandIn || Player->GetPlaybackStatus() != EMovieScenePlayerStatus::Playing) continue;
        ++Result.PlayingSequenceCount;
        ++Result.ActiveSequenceCount;
        const FTransform Pose = StandIn->GetTransform();
        Result.MaxTranslationCm = FMath::Max(Result.MaxTranslationCm, Pose.GetTranslation().Size());
        Result.MaxRotationDegrees = FMath::Max(Result.MaxRotationDegrees, FMath::RadiansToDegrees(Pose.GetRotation().AngularDistance(FQuat::Identity)));
        Result.MaxPlaybackSeconds = FMath::Max(Result.MaxPlaybackSeconds, Player->GetCurrentPosition().AsDecimal() / Player->GetInputRate().AsDecimal());
    }
    return Result;
}

void ApplyRonGunLabCameraRecovery(UWorld* World)
{
    if (!World || !World->IsGameWorld() || (ActiveWorld.IsValid() && ActiveWorld.Get() != World)) return;
    if (ActiveWorld.Get() == World) return;
    FString Text;
    const FString Filename = FPaths::ProjectSavedDir() / TEXT("GunLab/legacy-camera-recovery-map.json");
    if (!FFileHelper::LoadFileToString(Text, *Filename)) return;
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return;
    bool bValidatedTransformSemantics = false;
    if (!Root->TryGetBoolField(TEXT("validated_transform_semantics"), bValidatedTransformSemantics) || !bValidatedTransformSemantics)
    {
        UE_LOG(LogTemp, Warning, TEXT("GunLab camera recovery held: authored initial-transform conversion is not validated."));
        return;
    }
    const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
    if (!Root->TryGetArrayField(TEXT("mappings"), Rows)) return;
    int32 ClassCount = 0, InstanceCount = 0;
    for (const auto& Value : *Rows)
    {
        const TSharedPtr<FJsonObject>* Row = nullptr;
        if (!Value->TryGetObject(Row) || !Row || !Row->IsValid()) continue;
        FString ClassPath, SequencePath;
        if (!(*Row)->TryGetStringField(TEXT("shake_class"), ClassPath) || !(*Row)->TryGetStringField(TEXT("sequence"), SequencePath)) continue;
        // Only the audited original camera classes and generated Study assets are valid inputs.
        if (!ClassPath.StartsWith(TEXT("/Game/Blueprints/Camera/")) || !SequencePath.StartsWith(TEXT("/Game/ReadyOrNot/Level/Study/CameraRecovery/"))) continue;
        UClass* ShakeClass = LoadClass<ULegacyCameraShake>(nullptr, *ClassPath);
        UCameraAnimationSequence* Sequence = LoadObject<UCameraAnimationSequence>(nullptr, *SequencePath);
        if (!ShakeClass || !Sequence) continue;
        ULegacyCameraShake* Defaults = ShakeClass->GetDefaultObject<ULegacyCameraShake>();
        // Preserve any already-authored UE5 sequence rather than replacing it.
        if (Defaults->AnimSequence) continue;
        Assignments.Add({Defaults, Defaults->AnimSequence.Get(), Sequence});
        Defaults->AnimSequence = Sequence;
        RefreshNativeDefaultCopyList(ShakeClass);
        PinnedDefaults.Emplace(Defaults);
        ++ClassCount;
        for (TObjectIterator<ULegacyCameraShake> It; It; ++It)
        {
            if (It->GetClass() != ShakeClass || It->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || It->GetWorld() != World || It->AnimSequence) continue;
            Assignments.Add({*It, It->AnimSequence.Get(), Sequence});
            It->AnimSequence = Sequence;
            ++InstanceCount;
        }
    }
    ActiveWorld = World;
    UE_LOG(LogTemp, Display, TEXT("GunLab native camera recovery: %d shake CDOs, %d existing instances assigned authored CameraAnimationSequence tracks; oscillation/scales retained."), ClassCount, InstanceCount);
}

void RestoreRonGunLabCameraRecovery(UWorld* World)
{
    if (ActiveWorld.Get() != World) return;
    // Instances constructed after Apply inherited our session-only default and
    // were not present in the original assignment list. Restore them as well if
    // the lab is removed while its player/world remains alive.
    for (const FCameraAssignment& Assignment : Assignments)
    {
        ULegacyCameraShake* Defaults = Assignment.Shake.Get();
        if (!Defaults || !Defaults->HasAnyFlags(RF_ClassDefaultObject)) continue;
        for (TObjectIterator<ULegacyCameraShake> It; It; ++It)
        {
            if (It->GetClass() != Defaults->GetClass() || It->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
                || It->GetWorld() != World || It->AnimSequence != Assignment.Recovered.Get()) continue;
            USequenceCameraShakePattern* Pattern = Cast<USequenceCameraShakePattern>(ReadNativeObjectProperty(*It, TEXT("SequenceShakePattern")));
            if (Pattern && Pattern->Sequence == Assignment.Recovered.Get())
            {
                FCameraShakeStopParams StopParams;
                StopParams.bImmediately = true;
                Pattern->StopShakePattern(StopParams);
            }
            It->AnimSequence = Assignment.Original.Get();
        }
    }
    for (const FCameraAssignment& Assignment : Assignments)
    {
        if (Assignment.Shake.IsValid() && Assignment.Shake->AnimSequence == Assignment.Recovered.Get())
        {
            Assignment.Shake->AnimSequence = Assignment.Original.Get();
            if (Assignment.Shake->HasAnyFlags(RF_ClassDefaultObject))
                RefreshNativeDefaultCopyList(Assignment.Shake->GetClass());
        }
    }
    Assignments.Empty();
    PinnedDefaults.Empty();
    ActiveWorld.Reset();
}
