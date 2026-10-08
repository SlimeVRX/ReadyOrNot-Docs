#include "RonGunLabExperienceProbe.h"
#include "RonGunLab.h"
#include "RonGunLabCameraRecovery.h"
#include "LegacyCameraShake.h"
#include "CameraAnimationSequence.h"
#include "ReadyOrNotGameState.h"
#include "HUD/Widgets/Loadout/V2/Loadout_V2.h"
#include "Characters/PlayerCharacter.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/TrainingTarget.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Actors/Attachments/WeaponAttachment.h"
#include "Actors/Attachments/ScopedWeaponAttachment.h"
#include "Actors/Attachments/LightAttachment.h"
#include "Actors/Attachments/LaserAttachment.h"
#include "Components/InventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#endif

ARonGunLabExperienceProbe::ARonGunLabExperienceProbe() { PrimaryActorTick.bCanEverTick = true; }
void ARonGunLabExperienceProbe::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(FParse::Param(FCommandLine::Get(), TEXT("GunLabExperienceQA")));
    RunId = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
}
void ARonGunLabExperienceProbe::Key(FKey InKey, EInputEvent Event)
{
    if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr)
        PC->InputKey(FInputKeyParams(InKey, Event, Event == IE_Released ? 0.0 : 1.0));
}
void ARonGunLabExperienceProbe::Fired(ABaseMagazineWeapon* Weapon, bool bServer)
{
    if (bServer) ++ServerFireEvents; else ++LocalFireEvents;
}
void ARonGunLabExperienceProbe::TrainingHit(ATrainingTarget* HitTarget) { ++TrainingHitEvents; }
void ARonGunLabExperienceProbe::GlassHit(AActor* DamagedActor, float Damage, AController* DamageInstigator, FVector Location, UPrimitiveComponent* Component, FName Bone, FVector Direction, const UDamageType* DamageType, AActor* Causer)
{ if (Damage > 0 && Cast<ABaseMagazineWeapon>(Causer)) ++GlassHitEvents; }
void ARonGunLabExperienceProbe::Observe(const FString& Name)
{
    TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("step"), Name); Row->SetNumberField(TEXT("time"), Time);
    if (Lab) Lab->AppendNativeReadiness(Row);
    Row->SetNumberField(TEXT("training_success_events"), TrainingHitEvents);
    Row->SetNumberField(TEXT("camera_recovery_track_count"), CameraRecoveryTracks);
    Row->SetNumberField(TEXT("camera_recovery_max_active_sequences"), MaxActiveCameraSequences);
    Row->SetNumberField(TEXT("camera_recovery_max_translation_cm"), MaxCameraSequenceTranslation);
    Row->SetNumberField(TEXT("camera_recovery_max_rotation_deg"), MaxCameraSequenceRotation);
    Row->SetNumberField(TEXT("camera_recovery_max_playback_seconds"), MaxCameraSequencePlayback);
    Row->SetStringField(TEXT("camera_manager_class"), CameraManagerClass);
    Row->SetStringField(TEXT("cached_camera_shake_modifier_class"), CachedShakeModifierClass);
    Row->SetStringField(TEXT("observed_active_shake_classes"), ActiveShakeClasses);
    Row->SetNumberField(TEXT("camera_modifier_count"), CameraModifierCount);
    Row->SetNumberField(TEXT("native_camera_modifier_count"), NativeCameraModifierCount);
    Row->SetNumberField(TEXT("max_active_shakes"), MaxActiveShakes);
    Row->SetNumberField(TEXT("max_legacy_shakes"), MaxLegacyShakes);
    Row->SetNumberField(TEXT("max_recovered_shakes"), MaxRecoveredShakes);
    Row->SetNumberField(TEXT("max_sequence_patterns"), MaxSequencePatterns);
    Row->SetNumberField(TEXT("max_playing_sequences"), MaxPlayingSequences);
    Row->SetNumberField(TEXT("max_camera_oscillation_time_remaining"), MaxCameraOscillationRemaining);
    if (IsValid(GlassTarget))
    {
        Row->SetStringField(TEXT("glass_actor_id"), GlassTarget->GetName());
        Row->SetNumberField(TEXT("glass_procedural_components"), MaxGlassComponents);
        Row->SetNumberField(TEXT("glass_native_hit_events"), GlassHitEvents);
    }
    if (Player)
    {
        Row->SetBoolField(TEXT("ads"), Player->bAiming);
        Row->SetBoolField(TEXT("canted"), Player->bCantedSightEnabled);
        Row->SetBoolField(TEXT("crouched"), Player->bIsCrouched);
        Row->SetBoolField(TEXT("low_ready"), Player->IsLowReady());
        Row->SetBoolField(TEXT("freelook"), Player->IsFreelooking());
        Row->SetBoolField(TEXT("freelean_active"), Player->bFreeLeaning);
        Row->SetNumberField(TEXT("freelean_x"), Player->FreeLeanX);
        Row->SetNumberField(TEXT("freelean_z"), Player->FreeLeanZ);
        Row->SetBoolField(TEXT("holding_fast_walk"), Player->IsHoldingFastWalk());
        Row->SetBoolField(TEXT("sprinting"), Player->IsSprinting());
        Row->SetNumberField(TEXT("lean"), Player->QuickLeanAmount);
        Row->SetBoolField(TEXT("magcheck_playing"), Player->IsMagCheckPlaying());
        Row->SetBoolField(TEXT("animation_blocking"), Player->IsAnimationBlocking());
        Row->SetStringField(TEXT("location"), Player->GetActorLocation().ToString());
        Row->SetNumberField(TEXT("speed"), Player->GetVelocity().Size());
        Row->SetNumberField(TEXT("max_frame_seconds_observed"), MaxFrameSeconds);
        Row->SetNumberField(TEXT("max_pending_recoil_observed"), MaxPendingRecoil);
        Row->SetNumberField(TEXT("local_fire_events"), LocalFireEvents);
        Row->SetNumberField(TEXT("server_fire_events"), ServerFireEvents);
        if (UCameraComponent* Camera = Player->GetFirstPersonCameraComponent())
        { Row->SetNumberField(TEXT("camera_fov"), Camera->FieldOfView); Row->SetStringField(TEXT("camera_rotation"), Camera->GetComponentRotation().ToString()); }
        if (UAnimInstance* Anim = Player->GetMesh1P()->GetAnimInstance())
        {
            Row->SetStringField(TEXT("fp_anim_instance"), Anim->GetClass()->GetPathName());
            UAnimMontage* Montage = Anim->GetCurrentActiveMontage();
            Row->SetStringField(TEXT("active_montage"), Montage ? Montage->GetPathName() : TEXT(""));
        }
        if (ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem()))
        {
            Row->SetStringField(TEXT("weapon"), Weapon->GetClass()->GetPathName());
            Row->SetStringField(TEXT("fire_camera_shake_class"), Weapon->FireCameraShake ? Weapon->FireCameraShake->GetPathName() : TEXT(""));
            Row->SetStringField(TEXT("fire_camera_shake_instance_class"), Weapon->FireCameraShakeInst ? Weapon->FireCameraShakeInst->GetClass()->GetPathName() : TEXT(""));
            if (ULegacyCameraShake* Shake = Cast<ULegacyCameraShake>(Weapon->FireCameraShakeInst))
            {
                Row->SetStringField(TEXT("fire_camera_sequence"), GetPathNameSafe(Shake->AnimSequence.Get()));
                Row->SetNumberField(TEXT("fire_camera_anim_scale"), Shake->AnimScale);
                Row->SetBoolField(TEXT("fire_camera_instance_active"), Shake->IsActive());
            }
            Row->SetNumberField(TEXT("ammo"), Weapon->GetAmmo());
            Row->SetNumberField(TEXT("magazines"), Weapon->GetMagazineCount());
            Row->SetBoolField(TEXT("weapon_reloading"), Weapon->IsCurrentlyReloading());
            Row->SetBoolField(TEXT("weapon_tactical_reload"), Weapon->bTacticalReload);
            Row->SetBoolField(TEXT("weapon_can_reload"), Weapon->CanReload());
            Row->SetNumberField(TEXT("authored_burst_bullet_count"), Weapon->BurstBulletCount);
            Row->SetStringField(TEXT("ammo_type"), Weapon->GetCurrentAmmoTypeRowName().ToString());
            Row->SetStringField(TEXT("fire_mode"), StaticEnum<EFireMode>()->GetNameStringByValue(static_cast<int64>(Weapon->CurrentFireMode)));
            Row->SetBoolField(TEXT("native_primary_slot"), Weapon == Player->GetInventoryComponent()->GetSpawnedGear().Primary);
            Row->SetBoolField(TEXT("native_secondary_slot"), Weapon == Player->GetInventoryComponent()->GetSpawnedGear().Secondary);
            TArray<UWeaponAttachment*> Components; Weapon->GetComponents(Components);
            TArray<TSharedPtr<FJsonValue>> Attachments;
            for (UWeaponAttachment* Attachment : Components) Attachments.Add(MakeShared<FJsonValueString>(Attachment->GetClass()->GetPathName()));
            Row->SetArrayField(TEXT("attachments"), Attachments);
            if (ULightAttachment* Light = Weapon->GetLightAttachment()) Row->SetBoolField(TEXT("light_on"), Light->IsLightOn());
            if (ULaserAttachment* Laser = Weapon->GetLaserAttachment()) Row->SetBoolField(TEXT("laser_on"), Laser->IsLaserOn());
        }
    }
    if (IsValid(Target)) { Row->SetStringField(TEXT("target"), Target->GetClass()->GetPathName()); Row->SetNumberField(TEXT("target_health"), Target->GetCurrentHealth()); }
    Observations.Add(MakeShared<FJsonValueObject>(Row)); Save(false);
    UE_LOG(LogTemp, Display, TEXT("GUNLAB_EXPERIENCE %s time=%.2f"), *Name, Time);
}
void ARonGunLabExperienceProbe::AimAtNativeTarget()
{
    for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
        if (*It != Player && It->ActorHasTag(TEXT("GunLabDamageTarget"))) { Target = *It; break; }
    if (!Target) { Observe(TEXT("native_target_missing")); return; }
    Player->EndPrimaryUse(); Player->EndSecondaryUse();
    const FVector TargetPoint = Target->GetActorLocation() + FVector(0, 0, 35);
    Player->SetActorLocation(Target->GetActorLocation() + FVector(700, 0, 15), false, nullptr, ETeleportType::TeleportPhysics);
    FVector Eye; FRotator Rotation; Player->GetActorEyesViewPoint(Eye, Rotation);
    Player->GetController()->SetControlRotation((TargetPoint - Eye).Rotation());
    Observe(TEXT("native_target_before"));
}
void ARonGunLabExperienceProbe::SelectCatalogClass(const FString& ClassSuffix)
{
    if (!Lab) return;
    for (int32 Index = 0; Index < Lab->WeaponClasses.Num(); ++Index)
    {
        if (Lab->WeaponClasses[Index].ToString().EndsWith(ClassSuffix))
        {
            if (!Lab->SelectWeapon(Index)) Observe(TEXT("catalog_selection_rejected_") + ClassSuffix);
            return;
        }
    }
    Observe(TEXT("catalog_class_missing_") + ClassSuffix);
}
void ARonGunLabExperienceProbe::AimAtFixture(bool bGlass)
{
    AActor* Fixture = nullptr;
    FVector AimPoint = FVector::ZeroVector;
    FVector Offset = FVector::ZeroVector;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (!It->ActorHasTag(bGlass ? TEXT("GunLabProceduralGlass") : TEXT("GunLabTrainingTarget"))) continue;
        if (!bGlass && (!Cast<ATrainingTarget>(*It) || It->GetClass()->GetName().Contains(TEXT("Hostage")))) continue;
        Fixture = *It; break;
    }
    if (!Fixture) { Observe(bGlass ? TEXT("native_glass_missing") : TEXT("training_target_missing")); return; }
    if (bGlass)
    {
        GlassTarget = Fixture;
        GlassTarget->OnTakePointDamage.AddUniqueDynamic(this, &ARonGunLabExperienceProbe::GlassHit);
        FVector Extent; Fixture->GetActorBounds(true, AimPoint, Extent);
        Offset = Extent.X < Extent.Y ? FVector(500, 0, 0) : FVector(0, 500, 0);
    }
    else
    {
        TrainingTarget = Cast<ATrainingTarget>(Fixture);
        TrainingTarget->OnSuccessfulShot.AddUniqueDynamic(this, &ARonGunLabExperienceProbe::TrainingHit);
        TArray<UBoxComponent*> Boxes; Fixture->GetComponents(Boxes);
        bool bFoundSuccessBox = false;
        for (UBoxComponent* Box : Boxes) if (Box->GetName() == TEXT("SuccessBox")) { AimPoint = Box->GetComponentLocation(); bFoundSuccessBox = true; }
        if (!bFoundSuccessBox) { Observe(TEXT("training_success_box_missing")); return; }
        Offset = FVector(-400, 0, 0);
    }
    Player->EndPrimaryUse(); Player->EndSecondaryUse();
    FVector Position = AimPoint + Offset; Position.Z = 105;
    Player->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
    FVector Eye; FRotator Rotation; Player->GetActorEyesViewPoint(Eye, Rotation);
    Player->GetController()->SetControlRotation((AimPoint - Eye).Rotation());
}
void ARonGunLabExperienceProbe::PressSelectorToward(uint8 DesiredMode)
{
    ABaseMagazineWeapon* Weapon = Player ? Cast<ABaseMagazineWeapon>(Player->GetEquippedItem()) : nullptr;
    if (!Weapon || Player->IsAnimationBlocking()) return;
    const EFireMode Desired = static_cast<EFireMode>(DesiredMode);
    if (Weapon->AvailableFireModes.Contains(Desired) && Weapon->CurrentFireMode != Desired)
        Key(EKeys::X, IE_Pressed);
}
void ARonGunLabExperienceProbe::InstallNativeAttachment(const FString& Type)
{
    ABaseMagazineWeapon* Weapon = Player ? Cast<ABaseMagazineWeapon>(Player->GetEquippedItem()) : nullptr;
    if (!Weapon || !Lab) return;
    TArray<TPair<FString, TArray<TSubclassOf<UWeaponAttachment>>>> Lists;
    if (Type == TEXT("optics")) Lists.Emplace(Type, TArray<TSubclassOf<UWeaponAttachment>>(Weapon->AvailableScopeAttachments));
    else
    {
        Lists.Emplace(TEXT("underbarrel"), Weapon->AvailableUnderbarrelAttachments);
        Lists.Emplace(TEXT("overbarrel"), Weapon->AvailableOverbarrelAttachments);
        Lists.Emplace(TEXT("illuminator"), Weapon->AvailableIlluminatorAttachments);
    }
    for (const auto& List : Lists)
    for (int32 Index = 0; Index < List.Value.Num(); ++Index)
    {
        const auto Choice = List.Value[Index];
        if (Choice && !Choice.GetDefaultObject()->bNullAttachmentOnly && Weapon->CanAddAttachment(Choice)
            && (Type == TEXT("optics") || Choice->IsChildOf(ULightAttachment::StaticClass())))
        {
            Lab->Command({TEXT("attachment"), List.Key, FString::FromInt(Index + 1)});
            return;
        }
    }
    Observe(TEXT("no_compatible_attachment_") + Type);
}
void ARonGunLabExperienceProbe::MakeSteps()
{
    auto At = [&](float Seconds, TFunction<void()> Action) { Steps.Emplace(Seconds, MoveTemp(Action)); };
    auto Press = [&](float Seconds, FKey InKey) { At(Seconds, [this, InKey] { Key(InKey, IE_Pressed); }); };
    auto Release = [&](float Seconds, FKey InKey) { At(Seconds, [this, InKey] { Key(InKey, IE_Released); }); };
    auto Tap = [&](float Seconds, FKey InKey) { Press(Seconds, InKey); Release(Seconds + 0.08f, InKey); };
    auto Sample = [&](float Seconds, FString Label) { At(Seconds, [this, Label] { Observe(Label); }); };
    auto SeekMode = [&](float Seconds, EFireMode Mode)
    {
        At(Seconds, [this, Mode] { PressSelectorToward(static_cast<uint8>(Mode)); });
        Release(Seconds + 0.12f, EKeys::X);
    };
    Sample(0.1f, TEXT("baseline"));
    Tap(1, EKeys::X); Sample(2.5f, TEXT("fire_selector_after_short_press"));
    Press(3, EKeys::RightMouseButton); Sample(4.5f, TEXT("ads"));
    Tap(5, EKeys::O); Sample(6, TEXT("canted"));
    At(6.4f, [this] { MaxActiveCameraSequences = 0; MaxCameraSequenceTranslation = 0; MaxCameraSequenceRotation = 0; MaxCameraSequencePlayback = 0;
        MaxActiveShakes = 0; MaxLegacyShakes = 0; MaxRecoveredShakes = 0; MaxSequencePatterns = 0; MaxPlayingSequences = 0; MaxCameraOscillationRemaining = 0; ActiveShakeClasses.Empty(); });
    Press(6.5f, EKeys::LeftMouseButton); Release(7.0f, EKeys::LeftMouseButton); Sample(7.5f, TEXT("fire_and_recoil"));
    Tap(8, EKeys::R); Sample(12.5f, TEXT("tactical_reload"));
    Press(13, EKeys::R); Sample(13.9f, TEXT("held_magcheck")); Release(14.1f, EKeys::R);
    Release(16, EKeys::RightMouseButton); Tap(16.2f, EKeys::O);
    Tap(17, EKeys::SpaceBar); Sample(18, TEXT("low_ready")); Tap(18.2f, EKeys::SpaceBar);
    Press(19, EKeys::E); Sample(20, TEXT("lean_right")); Release(20.1f, EKeys::E);
    Press(21, EKeys::LeftControl); Sample(22, TEXT("crouch")); Release(22.1f, EKeys::LeftControl);
    Press(23, EKeys::CapsLock); Sample(24, TEXT("free_look")); Release(24.1f, EKeys::CapsLock);
    Tap(25, EKeys::Two); Sample(28, TEXT("secondary_slot"));
    Tap(28.2f, EKeys::One); Sample(31, TEXT("primary_slot_restored"));
    At(32, [this] { InstallNativeAttachment(TEXT("optics")); });
    Sample(33, TEXT("optic_attached"));
    At(34, [this] { InstallNativeAttachment(TEXT("illuminator")); });
    Sample(34.8f, TEXT("light_before_toggle"));
    Press(35, EKeys::RightMouseButton); Sample(36, TEXT("optic_ads"));
    Tap(36.2f, EKeys::ThumbMouseButton2); Sample(37, TEXT("attachment_toggle")); Release(37.2f, EKeys::RightMouseButton);
    At(39, [this] { AimAtNativeTarget(); });
    Press(40.5f, EKeys::RightMouseButton);
    Press(42, EKeys::LeftMouseButton); Release(42.5f, EKeys::LeftMouseButton); Sample(45, TEXT("native_target_after_live_fire"));
    Release(45.1f, EKeys::RightMouseButton);
    At(46, [this] { Lab->Command({TEXT("reset")}); });
    Sample(47, TEXT("movement_before"));
    Press(48, EKeys::W); Sample(49.8f, TEXT("standing_movement")); Release(50, EKeys::W);
    Press(51, EKeys::LeftShift); Press(51.2f, EKeys::W); Sample(53, TEXT("fast_walk_movement"));
    Release(53.2f, EKeys::W); Release(53.3f, EKeys::LeftShift);
    Press(54, EKeys::LeftControl); Press(54.2f, EKeys::W); Sample(55.5f, TEXT("crouched_movement"));
    Release(55.7f, EKeys::W); Release(55.8f, EKeys::LeftControl);
    Press(56.2f, EKeys::LeftAlt); Press(56.4f, EKeys::D); Sample(57.5f, TEXT("free_lean_axis"));
    Release(57.7f, EKeys::D); Release(57.8f, EKeys::LeftAlt);
    At(58.5f, [this] { Lab->Command({TEXT("reset")}); });
    At(60, [this] { SelectCatalogClass(TEXT("Primary_MP5A2.Primary_MP5A2_C")); });
    Sample(64, TEXT("mp5a2_equipped"));
    SeekMode(65, EFireMode::FM_Single); SeekMode(66, EFireMode::FM_Single);
    Sample(67, TEXT("single_before_hold")); Press(67.2f, EKeys::LeftMouseButton);
    Release(68.2f, EKeys::LeftMouseButton); Sample(68.5f, TEXT("single_after_hold"));
    SeekMode(69, EFireMode::FM_Burst); SeekMode(70, EFireMode::FM_Burst);
    Sample(71, TEXT("burst_before_hold")); Press(71.2f, EKeys::LeftMouseButton);
    Release(72.2f, EKeys::LeftMouseButton); Sample(72.5f, TEXT("burst_after_hold"));
    SeekMode(73, EFireMode::FM_Auto); SeekMode(74, EFireMode::FM_Auto);
    Sample(75, TEXT("auto_before_hold")); Press(75.2f, EKeys::LeftMouseButton);
    Release(76.2f, EKeys::LeftMouseButton); Sample(76.5f, TEXT("auto_after_hold"));
    Press(77, EKeys::LeftMouseButton); Release(82, EKeys::LeftMouseButton);
    Sample(82.4f, TEXT("before_empty_reload")); Tap(83, EKeys::R);
    Sample(87, TEXT("after_empty_reload"));
    Press(88, EKeys::LeftMouseButton); Release(88.4f, EKeys::LeftMouseButton);
    Sample(89, TEXT("before_speed_reload"));
    At(90, [this] { Key(EKeys::R, IE_DoubleClick); }); Release(90.12f, EKeys::R);
    Sample(94, TEXT("after_speed_reload"));
    // SR16's authored shake uses oscillation, not CameraAnim. Test a weapon that
    // actually references an authored camera sequence before asserting migration.
    At(95, [this] { SelectCatalogClass(TEXT("Primary_M16A4.Primary_M16A4_C")); });
    Sample(99, TEXT("m16a4_equipped"));
    At(99.5f, [this] { MaxActiveCameraSequences = 0; MaxCameraSequenceTranslation = 0; MaxCameraSequenceRotation = 0; MaxCameraSequencePlayback = 0; });
    Tap(100, EKeys::LeftMouseButton); Sample(102, TEXT("recovered_camera_fire"));
    At(103, [this] { SelectCatalogClass(TEXT("Primary_SR16.Primary_SR16_C")); });
    Sample(108, TEXT("sr16_restored"));
    At(108.2f, [this] { AimAtFixture(false); });
    Sample(109, TEXT("training_target_before"));
    Press(109.2f, EKeys::RightMouseButton); Press(110.5f, EKeys::LeftMouseButton); Release(111, EKeys::LeftMouseButton);
    Sample(113, TEXT("training_target_after_live_fire")); Release(113.2f, EKeys::RightMouseButton);
    At(114, [this] { AimAtFixture(true); }); Sample(115, TEXT("native_glass_before"));
    Press(115.2f, EKeys::RightMouseButton); Press(116.5f, EKeys::LeftMouseButton); Release(117, EKeys::LeftMouseButton);
    Sample(120, TEXT("native_glass_after_live_fire")); Release(120.2f, EKeys::RightMouseButton);
    At(121, [this] { Lab->Command({TEXT("reset")}); });
    Tap(122, EKeys::F1); Sample(123, TEXT("help_open"));
    At(124, [this] { Lab->Command({TEXT("status")}); bFinished = true; Save(true); });
    Steps.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });
}
void ARonGunLabExperienceProbe::Save(bool bComplete)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("run_id_utc"), RunId); Root->SetStringField(TEXT("map"), GetWorld()->GetMapName());
    Root->SetStringField(TEXT("mode"), TEXT("native_experience_input_probe"));
    Root->SetBoolField(TEXT("complete"), bComplete);
    Root->SetBoolField(TEXT("audio_disabled_by_command_line"), FParse::Param(FCommandLine::Get(), TEXT("nosound")));
    Root->SetBoolField(TEXT("native_loadout_ui_opened_by_probe"), bLoadoutObserved);
    Root->SetStringField(TEXT("native_loadout_ui_test_scope"), bLoadoutObserved ? TEXT("Original LoadoutPortal streamed its scene and presented the native widget. Opening/sanitization and native widget teardown can save the profile. The optional UI runner backs up/restores MetaGameProfile.sav; verify profile_preservation.json separately. This probe does not navigate or change menu selections.") : TEXT("Native loadout UI not observed. Use -InspectLoadoutUI for a separate open/capture after input checks."));
    Root->SetStringField(TEXT("speed_reload_input"), TEXT("Native R IE_DoubleClick event, followed by release; no direct weapon reload-state mutation."));
    Root->SetStringField(TEXT("scope"), TEXT("Native player input injection and observed state. No replacement gameplay, injected damage or subjective audiovisual parity claim. Inspect observations individually."));
    Root->SetArrayField(TEXT("observations"), Observations);
    FString Text; FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
    FFileHelper::SaveStringToFile(Text, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GunLab/experience_receipt.json")));
}
void ARonGunLabExperienceProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Alive += DeltaSeconds;
    if (!bStarted)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        Player = PC ? Cast<APlayerCharacter>(PC->GetPawn()) : nullptr;
        if (Alive > 12 && Player && Player->GetEquippedItem() && !Player->IsAnimationBlocking())
        {
            for (TActorIterator<ARonGunLab> It(GetWorld()); It; ++It) { Lab = *It; break; }
            if (Lab) { bStarted = true; MakeSteps(); }
        }
        if (Alive > 180) { Observe(TEXT("startup_timeout")); SetActorTickEnabled(false); if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false); }
        return;
    }
    Time += DeltaSeconds;
    if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
    {
        const FRonGunLabCameraSnapshot Camera = SnapshotRonGunLabCameraRecovery(PC->PlayerCameraManager);
        CameraManagerClass = Camera.CameraManagerClass;
        CachedShakeModifierClass = Camera.CachedShakeModifierClass;
        if (!Camera.ActiveShakeClasses.IsEmpty()) ActiveShakeClasses = Camera.ActiveShakeClasses;
        CameraModifierCount = Camera.ModifierCount; NativeCameraModifierCount = Camera.NativeModifierCount;
        MaxActiveShakes = FMath::Max(MaxActiveShakes, Camera.ActiveShakeCount);
        MaxLegacyShakes = FMath::Max(MaxLegacyShakes, Camera.LegacyShakeCount);
        MaxRecoveredShakes = FMath::Max(MaxRecoveredShakes, Camera.RecoveredShakeCount);
        MaxSequencePatterns = FMath::Max(MaxSequencePatterns, Camera.SequencePatternCount);
        MaxPlayingSequences = FMath::Max(MaxPlayingSequences, Camera.PlayingSequenceCount);
        MaxCameraOscillationRemaining = FMath::Max(MaxCameraOscillationRemaining, Camera.MaxOscillationTimeRemaining);
        CameraRecoveryTracks = Camera.AssignedTrackCount;
        MaxActiveCameraSequences = FMath::Max(MaxActiveCameraSequences, Camera.ActiveSequenceCount);
        MaxCameraSequenceTranslation = FMath::Max(MaxCameraSequenceTranslation, Camera.MaxTranslationCm);
        MaxCameraSequenceRotation = FMath::Max(MaxCameraSequenceRotation, Camera.MaxRotationDegrees);
        MaxCameraSequencePlayback = FMath::Max(MaxCameraSequencePlayback, Camera.MaxPlaybackSeconds);
    }
    if (IsValid(GlassTarget))
    {
        TArray<UPrimitiveComponent*> Parts; GlassTarget->GetComponents(Parts);
        int32 Count = 0;
        for (UPrimitiveComponent* Part : Parts) if (Part->GetClass()->GetName() == TEXT("ProceduralMeshComponent")) ++Count;
        MaxGlassComponents = FMath::Max(MaxGlassComponents, Count);
    }
    MaxFrameSeconds = FMath::Max(MaxFrameSeconds, DeltaSeconds);
    MaxPendingRecoil = FMath::Max(MaxPendingRecoil, static_cast<float>(Player->PendingRecoil.GetManhattanDistance(FRotator::ZeroRotator)));
    ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem());
    if (Weapon && Weapon != BoundWeapon)
    {
        if (IsValid(BoundWeapon)) BoundWeapon->OnWeaponFire.RemoveDynamic(this, &ARonGunLabExperienceProbe::Fired);
        BoundWeapon = Weapon; Weapon->OnWeaponFire.AddDynamic(this, &ARonGunLabExperienceProbe::Fired);
    }
    while (Step < Steps.Num() && Time >= Steps[Step].Key) { Steps[Step].Value(); ++Step; }
    if (bFinished)
    {
#if WITH_EDITOR
        if (!bScreenshotRequested && (GShaderCompilingManager->GetNumRemainingJobs() || FAssetCompilingManager::Get().GetNumRemainingAssets()))
        {
            if (Alive > 900) { Observe(TEXT("render_timeout")); if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false); }
            return;
        }
#endif
        const FString Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GunLab/Experience.png")));
        if (!bScreenshotRequested) { FScreenshotRequest::RequestScreenshot(Path, true, false); bScreenshotRequested = true; ScreenshotTime = Time; }
        if (bScreenshotRequested && Time > ScreenshotTime + 4 && IFileManager::Get().FileExists(*Path))
        {
            if (FParse::Param(FCommandLine::Get(), TEXT("GunLabInspectLoadoutUI")))
            {
                if (!bLoadoutRequested)
                {
                    Lab->Command({TEXT("loadout")}); bLoadoutRequested = true; LoadoutRequestTime = Time; return;
                }
                AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>();
                const FString UIPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GunLab/NativeLoadout.png")));
                if (!bLoadoutScreenshot && GS && GS->Loadout_V2 && GS->Loadout_V2->IsInViewport() && Time > LoadoutRequestTime + 12)
                {
#if WITH_EDITOR
                    if ((GShaderCompilingManager && GShaderCompilingManager->GetNumRemainingJobs()) || FAssetCompilingManager::Get().GetNumRemainingAssets()) return;
#endif
                    bLoadoutObserved = true; Observe(TEXT("native_loadout_ui_present")); Save(true);
                    FScreenshotRequest::RequestScreenshot(UIPath, true, false); bLoadoutScreenshot = true; LoadoutScreenshotTime = Time; return;
                }
                if (!(bLoadoutScreenshot && Time > LoadoutScreenshotTime + 4 && IFileManager::Get().FileExists(*UIPath)))
                {
                    if (Time < LoadoutRequestTime + 180) return;
                    Observe(TEXT("native_loadout_ui_timeout")); Save(true);
                }
            }
            SetActorTickEnabled(false); if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false);
        }
        else if (Time > ScreenshotTime + 45) { Observe(TEXT("screenshot_timeout")); SetActorTickEnabled(false); if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false); }
    }
}
