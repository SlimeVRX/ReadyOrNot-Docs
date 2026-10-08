#include "RonGunLab.h"
#include "RonGunLabCameraRecovery.h"
#include "ReadyOrNot.h"
#include "Characters/PlayerCharacter.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Components/InventoryComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#include "AssetCompilingManager.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogRonGunLab, Log, All);

ARonGunLab::ARonGunLab()
{
    PrimaryActorTick.bCanEverTick = true;
    // Edge-triggered input lives for one input frame; never poll it at a slower rate.
    PrimaryActorTick.TickInterval = 0.0f;
}

void ARonGunLab::BeginPlay()
{
    Super::BeginPlay();
    ApplyRonGunLabCameraRecovery(GetWorld());
    bSmoke = FParse::Param(FCommandLine::Get(), TEXT("GunLabSmoke"));
    bExitAfterSmoke = bSmoke && FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"));
    bProbeOnEquip = !bSmoke && FParse::Param(FCommandLine::Get(), TEXT("GunLabProbe"));
    bExitAfterProbe = bProbeOnEquip && FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"));
    FString BatchText;
    if (bProbeOnEquip && FParse::Value(FCommandLine::Get(), TEXT("GunLabProbeIndices="), BatchText))
    {
        TArray<FString> Entries;
        BatchText.ParseIntoArray(Entries, TEXT(","), true);
        for (const FString& Entry : Entries)
        {
            const int32 Index = FCString::Atoi(*Entry) - 1;
            if (WeaponClasses.IsValidIndex(Index)) ProbeIndices.AddUnique(Index);
            else UE_LOG(LogRonGunLab, Warning, TEXT("Ignoring invalid 1-based probe index: %s"), *Entry);
        }
    }
    ReceiptMode = bSmoke ? TEXT("equip_audit") : bProbeOnEquip ? TEXT("action_probe") : TEXT("session");
    RunId = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
    StatusText = TEXT("Waiting for native player pawn...");
    UE_LOG(LogRonGunLab, Display, TEXT("GUNLAB_BEGIN candidates=%d map=%s"), WeaponClasses.Num(), *GetWorld()->GetMapName());
    if (WeaponClasses.IsEmpty())
    {
        WriteReceipt(TEXT("empty_catalog"), TEXT("No weapon classes configured on the lab actor"));
        bSmoke = false;
        bProbeOnEquip = false;
        if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false);
    }
}

void ARonGunLab::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RestoreRonGunLabCameraRecovery(GetWorld());
    Super::EndPlay(EndPlayReason);
}

APlayerCharacter* ARonGunLab::GetNativePlayer() const
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    return PC ? Cast<APlayerCharacter>(PC->GetPawn()) : nullptr;
}

bool ARonGunLab::SelectWeapon(int32 Index)
{
    APlayerCharacter* Player = GetNativePlayer();
    if (!Player || !Player->HasAuthority() || !WeaponClasses.IsValidIndex(Index) || bPendingEquip || ProbeStage || Player->IsAnimationBlocking())
        return false;
    Player->EndPrimaryUse();
    UClass* Class = WeaponClasses[Index].LoadSynchronous();
    CurrentIndex = Index;
    if (!Class || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
    {
        WriteReceipt(TEXT("class_rejected"), !Class ? TEXT("Class could not be loaded")
            : Class->HasAnyClassFlags(CLASS_Abstract) ? TEXT("Abstract base class; use a concrete child Blueprint")
            : TEXT("Deprecated class"));
        return false;
    }
    const ABaseMagazineWeapon* Defaults = Class->GetDefaultObject<ABaseMagazineWeapon>();
    if (!Defaults || !Defaults->AnimationData || !Defaults->GetItemMesh() || !Defaults->GetItemMesh()->GetSkeletalMeshAsset())
    {
        WriteReceipt(TEXT("incomplete_asset"), TEXT("Native animation data or skeletal mesh missing; selection retained in catalog"));
        return false;
    }
    // Playable primary/secondary guns use the complete native workbench pipeline.
    // Hidden/suspect/test assets remain a clearly identified catalog inspection path.
    if (ShouldUseNativeLoadout(Class))
        return ApplyNativeLoadoutSelection(Class, Player);
    bLastNativeLoadout = false;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ABaseMagazineWeapon* Weapon = GetWorld()->SpawnActor<ABaseMagazineWeapon>(Class, FTransform::Identity, Params);
    if (!Weapon)
    {
        WriteReceipt(TEXT("spawn_failed"), TEXT("Native SpawnActor returned null"));
        return false;
    }
    // Exact ownership/equip pathway used by the project's development Equip command.
    UInventoryComponent* Inventory = Player->GetInventoryComponent();
    Inventory->AddInventoryItem(Weapon);
    // Empty per-mag overrides select the weapon's first authored ammunition type.
    Weapon->SetMagazineCount(FMath::Max(1, SuppliedMagazines), TArray<FName>());
    const ABaseItem* Outgoing = Player->GetEquippedItem();
    // Some suspect-only assets omit FP holster data. The native non-instant
    // path never completes that outgoing transition. Use its existing instant
    // branch, preserving all native ownership/draw/fire logic and report it.
    bLastInstantFallback = Outgoing && Outgoing->AnimationData && !Outgoing->AnimationData->Holster.Body_FP;
    if (!Inventory->PutItemInHands(Weapon, bLastInstantFallback))
    {
        Inventory->DestroyInventoryItem(Weapon);
        WriteReceipt(TEXT("equip_rejected"), TEXT("Native inventory declined the request"));
        return false;
    }
    // Never discard the player's real primary/secondary because a catalog-only
    // suspect/tactical asset was temporarily inspected.
    PreviousWeapon = IsValid(CurrentWeapon) && !IsNativeLoadoutWeapon(CurrentWeapon) ? CurrentWeapon : nullptr;
    CurrentWeapon = Weapon;
    bPendingEquip = true;
    PendingSeconds = 0;
    StatusText = FString::Printf(TEXT("Drawing %d/%d: %s"), Index + 1, WeaponClasses.Num(), *Weapon->ItemName.ToString());
    return true;
}

void ARonGunLab::CycleWeapon(int32 Direction)
{
    if (WeaponClasses.IsEmpty()) return;
    const int32 Index = (CurrentIndex + Direction + WeaponClasses.Num()) % WeaponClasses.Num();
    SelectWeapon(Index);
}

void ARonGunLab::Refill()
{
    if (APlayerCharacter* Player = GetNativePlayer())
    {
        if (ProbeStage || bSmoke || bPendingEquip) return;
        if (Player->IsAnimationBlocking()) { StatusText = TEXT("Wait for the native action to finish before supplying ammunition"); return; }
        ABaseMagazineWeapon* Held = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem());
        if (Held)
        {
            const FSavedLoadout Loadout = Player->GetInventoryComponent()->GetLastEquippedLoadout();
            const FSpawnedGear& Gear = Player->GetInventoryComponent()->GetSpawnedGear();
            const bool bPrimary = Held == Gear.Primary;
            const bool bSecondary = Held == Gear.Secondary;
            Held->SetMagazineCount(FMath::Max(1, bPrimary ? Loadout.PrimaryAmmoSlotsCount : bSecondary ? Loadout.SecondaryAmmoSlotsCount : SuppliedMagazines),
                bPrimary ? Loadout.PrimaryAmmoSlots : bSecondary ? Loadout.SecondaryAmmoSlots : TArray<FName>());
        }
        else
            Player->ReplenishAllMagazineAmmo();
        StatusText = TEXT("Native ammunition supply requested; selected loadout ammo type preserved");
    }
}

void ARonGunLab::ResetPosition()
{
    if (ProbeStage || bSmoke) return;
    if (APlayerCharacter* Player = GetNativePlayer())
    {
        Player->EndPrimaryUse();
        Player->SetActorLocation(FiringLine, false, nullptr, ETeleportType::TeleportPhysics);
        if (AController* PC = Player->GetController()) PC->SetControlRotation(FRotator::ZeroRotator);
    }
}

void ARonGunLab::StartSmokeTest()
{
    if (bPendingEquip || ProbeStage || bSmoke) return;
    bSmoke = true;
    SmokeNextIndex = 0;
    Receipts.Reset();
    ReceiptMode = TEXT("equip_audit");
    RunId = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
    NextSmokeTime = AliveSeconds;
}

void ARonGunLab::StartActionProbe()
{
    if (!GetNativePlayer() || !CurrentWeapon || bPendingEquip || bSmoke || ProbeStage) return;
    if (CurrentIndex != ActiveWeaponIndex) { StatusText = TEXT("Probe rejected: the requested lab selection did not complete"); return; }
    if (GetNativePlayer()->GetEquippedItem() != CurrentWeapon) { StatusText = TEXT("Probe rejected: native equipped item differs from selected lab gun"); return; }
    ResetPosition();
    GetNativePlayer()->EndSecondaryUse();
    ReceiptMode = TEXT("action_probe");
    if (ProbeIndices.IsEmpty())
    {
        RunId = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
        Receipts.Reset();
    }
    ProbeAmmoBefore = CurrentWeapon->GetAmmo();
    ProbeAmmoAfterFire = ProbeAmmoBefore;
    ProbeStage = 1;
    ProbeSeconds = 0;
    bProbeReloadRequested = false;
    bProbeCanReload = false;
}

void ARonGunLab::ContinueProbeBatch()
{
    while (++ProbeCursor < ProbeIndices.Num())
    {
        bProbeOnEquip = true;
        if (SelectWeapon(ProbeIndices[ProbeCursor])) return;
        bProbeOnEquip = false;
        // SelectWeapon wrote the concrete failure; continue to the next entry.
    }
    if (bExitAfterProbe && !FParse::Param(FCommandLine::Get(), TEXT("GunLabCapture")))
        FPlatformMisc::RequestExit(false);
}

void ARonGunLab::WriteReceipt(const FString& Status, const FString& Detail)
{
    if (Status != TEXT("equipped") && Status != TEXT("action_probe"))
        StatusText = FString::Printf(TEXT("%d/%d %s: %s"), CurrentIndex + 1, WeaponClasses.Num(), *Status, *Detail);
    TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
    Row->SetNumberField(TEXT("index"), CurrentIndex);
    Row->SetStringField(TEXT("class_path"), WeaponClasses.IsValidIndex(CurrentIndex) ? WeaponClasses[CurrentIndex].ToString() : TEXT(""));
    Row->SetStringField(TEXT("status"), Status);
    Row->SetStringField(TEXT("detail"), Detail);
    Row->SetNumberField(TEXT("world_time"), GetWorld()->GetTimeSeconds());
    Row->SetBoolField(TEXT("native_instant_transition"), bLastInstantFallback);
    if (Status == TEXT("action_probe"))
    {
        Row->SetNumberField(TEXT("ammo_before"), ProbeAmmoBefore);
        Row->SetNumberField(TEXT("ammo_after_native_fire"), ProbeAmmoAfterFire);
        Row->SetBoolField(TEXT("ammo_consumed"), ProbeAmmoAfterFire < ProbeAmmoBefore);
        Row->SetBoolField(TEXT("native_aiming_state_before_fire"), bProbeAiming);
        Row->SetBoolField(TEXT("native_reload_requested"), bProbeReloadRequested);
        Row->SetBoolField(TEXT("native_can_reload_before_request"), bProbeCanReload);
        Row->SetBoolField(TEXT("native_reload_replenished"), bProbeReloadRequested && CurrentWeapon && CurrentWeapon->GetAmmo() > ProbeAmmoAfterFire);
    }
    if (IsValid(CurrentWeapon) && CurrentWeapon->GetClass()->GetPathName() == Row->GetStringField(TEXT("class_path")))
    {
        Row->SetNumberField(TEXT("ammo"), CurrentWeapon->GetAmmo());
        Row->SetNumberField(TEXT("magazines"), CurrentWeapon->GetMagazineCount());
        Row->SetBoolField(TEXT("native_owner"), CurrentWeapon->GetOwner() == GetNativePlayer());
    }
    AppendNativeReadiness(Row);
    Receipts.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogRonGunLab, Display, TEXT("GUNLAB_RESULT index=%d status=%s class=%s detail=%s"), CurrentIndex, *Status, *Row->GetStringField(TEXT("class_path")), *Detail);
    SaveReceipts();
}

void ARonGunLab::SaveReceipts()
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("scope"), TEXT("Native gun lab instrumentation. Equip rows certify ownership/holding; action rows report aiming/ammo state after native calls. No audiovisual, impact or subjective-feel certification."));
    Root->SetStringField(TEXT("mode"), ReceiptMode);
    Root->SetStringField(TEXT("run_id_utc"), RunId);
    Root->SetStringField(TEXT("map"), GetWorld()->GetMapName());
    Root->SetNumberField(TEXT("candidate_count"), WeaponClasses.Num());
    Root->SetArrayField(TEXT("results"), Receipts);
    FString Json;
    FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
    const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GunLab"));
    IFileManager::Get().MakeDirectory(*Dir, true);
    FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Dir, TEXT("runtime_receipt.json")));
    FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Dir, ReceiptMode + TEXT("_receipt.json")));
    FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Dir, ReceiptMode + TEXT("_") + RunId + TEXT(".json")));
}

void ARonGunLab::FinishSmoke()
{
    bSmoke = false;
    SaveReceipts();
    UE_LOG(LogRonGunLab, Display, TEXT("GUNLAB_SMOKE_DONE results=%d candidates=%d"), Receipts.Num(), WeaponClasses.Num());
    if (bExitAfterSmoke) FPlatformMisc::RequestExit(false);
}

void ARonGunLab::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AliveSeconds += DeltaSeconds;
    APlayerCharacter* Player = GetNativePlayer();
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (Player && !bInitialized && AliveSeconds > 3 && !Player->IsAnimationBlocking())
    {
        bInitialized = true;
        EnsureNativeCrosshair();
        UE_LOG(LogRonGunLab, Display, TEXT("GUNLAB_NATIVE_PAWN %s"), *Player->GetClass()->GetPathName());
        int32 InitialIndex = 1;
        for (int32 Index = 0; Index < WeaponClasses.Num(); ++Index)
            if (WeaponClasses[Index].ToString().EndsWith(TEXT("Primary_SR16.Primary_SR16_C"))) { InitialIndex = Index + 1; break; }
        FParse::Value(FCommandLine::Get(), TEXT("GunLabIndex="), InitialIndex);
        if (!ProbeIndices.IsEmpty()) InitialIndex = ProbeIndices[0] + 1;
        if (!bSmoke && !WeaponClasses.IsEmpty())
        {
            const bool bSelected = SelectWeapon(FMath::Clamp(InitialIndex - 1, 0, WeaponClasses.Num() - 1));
            if (!bSelected && bExitAfterProbe) FPlatformMisc::RequestExit(false);
        }
    }
    if (Player && bPendingEquip)
    {
        PendingSeconds += DeltaSeconds;
        if (IsValid(CurrentWeapon) && CurrentWeapon->GetOwner() == Player
            && Player->GetEquippedItem() == CurrentWeapon && !Player->IsAnimationBlocking()
            && (!bLastNativeLoadout || IsNativeLoadoutWeapon(CurrentWeapon)))
        {
            bPendingEquip = false;
            ActiveWeaponIndex = CurrentIndex;
            if (IsValid(PreviousWeapon) && PreviousWeapon != CurrentWeapon && !IsNativeLoadoutWeapon(PreviousWeapon))
                Player->GetInventoryComponent()->DestroyInventoryItem(PreviousWeapon);
            PreviousWeapon = nullptr;
            StatusText = FString::Printf(TEXT("%d/%d  %s"), CurrentIndex + 1, WeaponClasses.Num(), *CurrentWeapon->ItemName.ToString());
            WriteReceipt(bLastInstantFallback ? TEXT("equipped_instant_fallback") : TEXT("equipped"), bLastInstantFallback
                ? TEXT("Native pawn owns/holds class; built-in instant transition used because outgoing asset lacks FP holster. Normal outgoing holster was not validated.")
                : TEXT("Native pawn owns and holds requested class; draw animation no longer blocks"));
            NextSmokeTime = AliveSeconds + 0.5f;
            if (bProbeOnEquip) { bProbeOnEquip = false; StartActionProbe(); }
        }
        else if (PendingSeconds > 20)
        {
            bPendingEquip = false;
            WriteReceipt(TEXT("equip_timeout"), TEXT("No completed native draw within 20 seconds"));
            // Preserve whichever tracked gun the native inventory still holds;
            // clean rejected pending guns without replacing native equip state.
            ABaseMagazineWeapon* Held = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem());
            const bool bHoldingNew = Held == CurrentWeapon;
            const bool bHoldingPrevious = Held == PreviousWeapon;
            if (IsValid(CurrentWeapon) && !bHoldingNew && !IsNativeLoadoutWeapon(CurrentWeapon)) Player->GetInventoryComponent()->DestroyInventoryItem(CurrentWeapon);
            if (IsValid(PreviousWeapon) && !bHoldingPrevious && !IsNativeLoadoutWeapon(PreviousWeapon)) Player->GetInventoryComponent()->DestroyInventoryItem(PreviousWeapon);
            CurrentWeapon = (bHoldingNew || bHoldingPrevious) ? Held : nullptr;
            PreviousWeapon = nullptr;
            ActiveWeaponIndex = bHoldingNew ? CurrentIndex : bHoldingPrevious ? ActiveWeaponIndex : INDEX_NONE;
            NextSmokeTime = AliveSeconds + 0.5f;
            if (bExitAfterProbe) FPlatformMisc::RequestExit(false);
        }
    }
    if (Player && IsValid(CurrentWeapon) && ProbeStage)
    {
        if (Player->GetEquippedItem() != CurrentWeapon)
        {
            Player->EndPrimaryUse();
            WriteReceipt(TEXT("probe_interrupted"), TEXT("Native equipped item changed during the probe; no action result claimed"));
            ProbeStage = 0;
            if (bExitAfterProbe) FPlatformMisc::RequestExit(false);
        }
        ProbeSeconds += DeltaSeconds;
        if (ProbeStage == 1 && ProbeSeconds > 0.5f) { Player->DoAimDownSights(); ProbeStage = 2; }
        if (ProbeStage == 2 && ProbeSeconds > 1.2f) { bProbeAiming = Player->bAiming; Player->PrimaryUse(); ProbeStage = 3; }
        if (ProbeStage == 3 && ProbeSeconds > 1.4f) { Player->EndPrimaryUse(); ProbeAmmoAfterFire = CurrentWeapon->GetAmmo(); ProbeStage = 4; }
        if (ProbeStage == 4 && ProbeSeconds > 3.0f)
        {
            // Let the native virtual CanReload/OnWeaponReload gates decide.
            // A requested reload is not counted as observed replenishment.
            bProbeCanReload = CurrentWeapon->CanReload();
            bProbeReloadRequested = true;
            Player->Reload();
            ProbeStage = 5;
        }
        if (ProbeStage == 5 && ProbeSeconds > 12.0f)
        {
            WriteReceipt(TEXT("action_probe"), TEXT("Requested native ADS and PrimaryUse/EndPrimaryUse; reload request and observed ammo/aiming states are explicit fields, not visual/audio feel certification"));
            ProbeStage = 0;
            ContinueProbeBatch();
        }
    }
    ReconcileNativeSelection();
    if (bSmoke && bInitialized && !bPendingEquip && AliveSeconds >= NextSmokeTime)
    {
        if (SmokeNextIndex >= WeaponClasses.Num()) FinishSmoke();
        else { SelectWeapon(SmokeNextIndex++); NextSmokeTime = AliveSeconds + 0.5f; }
    }
    if ((bSmoke || bProbeOnEquip || ProbeStage || FParse::Param(FCommandLine::Get(), TEXT("GunLabCapture"))) && !Player && AliveSeconds > 120)
    {
        WriteReceipt(TEXT("no_native_pawn"), TEXT("Game mode did not create a native PlayerCharacter within 120 seconds"));
        FinishSmoke();
        if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false);
    }
    if (PC && !bSmoke && !ProbeStage)
    {
        if (PC->WasInputKeyJustPressed(EKeys::F5)) CycleWeapon(-1);
        if (PC->WasInputKeyJustPressed(EKeys::F6)) CycleWeapon(1);
        if (PC->WasInputKeyJustPressed(EKeys::F7)) Refill();
        if (PC->WasInputKeyJustPressed(EKeys::F8)) ResetPosition();
        if (PC->WasInputKeyJustPressed(EKeys::F9)) StartSmokeTest();
        if (PC->WasInputKeyJustPressed(EKeys::F10)) bShowOverlay = !bShowOverlay;
    }
    if (Player && bInitialized && !bPendingEquip && !ProbeStage && !bProbeOnEquip && AliveSeconds > 15 && FParse::Param(FCommandLine::Get(), TEXT("GunLabCapture")))
    {
#if WITH_EDITOR
        const int32 RemainingShaders = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
        const int32 RemainingAssets = FAssetCompilingManager::Get().GetNumRemainingAssets();
        if (!bCaptureRequested && (RemainingShaders > 0 || RemainingAssets > 0))
        {
            StatusText = FString::Printf(TEXT("Waiting for renderer assets: %d shader jobs, %d assets"), RemainingShaders, RemainingAssets);
            if (AliveSeconds > 900)
            {
                WriteReceipt(TEXT("render_assets_timeout"), StatusText + TEXT("; no completed visual QA claimed"));
                if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false);
            }
            return;
        }
#endif
        const FString ScreenshotPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GunLab/Range.png")));
        if (!bCaptureRequested)
        {
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
            bCaptureRequested = true;
            CaptureTime = AliveSeconds;
            UE_LOG(LogRonGunLab, Display, TEXT("GUNLAB_SCREENSHOT_REQUEST %s"), *ScreenshotPath);
        }
        else if (AliveSeconds - CaptureTime > 3 && IFileManager::Get().FileExists(*ScreenshotPath))
        {
            UE_LOG(LogRonGunLab, Display, TEXT("GUNLAB_SCREENSHOT_SAVED %s"), *ScreenshotPath);
            CaptureTime = MAX_flt;
            if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false);
        }
        else if (AliveSeconds - CaptureTime > 45)
        {
            WriteReceipt(TEXT("screenshot_timeout"), TEXT("Renderer did not save a screenshot within 45 seconds of request"));
            CaptureTime = MAX_flt;
            if (FParse::Param(FCommandLine::Get(), TEXT("GunLabExit"))) FPlatformMisc::RequestExit(false);
        }
    }
    if (GEngine && bShowOverlay)
    {
        FString Text = TEXT("NATIVE GUN LAB | F5/F6 new loadout | 1/2 native slots | F7 refill | F8 firing line | F10 overlay\n") + StatusText;
        if (Player)
        {
            if (ABaseMagazineWeapon* Held = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem()))
            {
                if (Held != CurrentWeapon) Text += TEXT(" | Native inventory selected: ") + Held->ItemName.ToString();
                Text += FString::Printf(TEXT(" | Ammo %.0f | Magazines %d"), Held->GetAmmo(), Held->GetMagazineCount());
                const UEnum* Modes = StaticEnum<EFireMode>();
                if (Modes)
                {
                    Text += TEXT(" | X fire mode: ") + Modes->GetNameStringByValue(static_cast<int64>(Held->CurrentFireMode));
                    Text += TEXT(" [");
                    for (EFireMode Mode : Held->AvailableFireModes) Text += Modes->GetNameStringByValue(static_cast<int64>(Mode)) + TEXT(" ");
                    Text += TEXT("]");
                }
                Text += IsNativeLoadoutWeapon(Held) ? TEXT(" | Native loadout slot") : TEXT(" | Catalog-only asset");
            }
        }
        GEngine->AddOnScreenDebugMessage(uint64(GetUniqueID()), 0.1f, FColor::Cyan, Text);
    }
}

void ARonGunLab::Command(const TArray<FString>& Args)
{
    if (Args.IsEmpty()) return;
    if (bSmoke) { UE_LOG(LogRonGunLab, Warning, TEXT("Wait for the current equip audit to finish")); return; }
    if (ProbeStage) { UE_LOG(LogRonGunLab, Warning, TEXT("Wait for the current action probe to finish")); return; }
    if (HandleLoadoutCommand(Args)) return;
    if (Args[0] == TEXT("next")) CycleWeapon(1);
    else if (Args[0] == TEXT("prev")) CycleWeapon(-1);
    else if (Args[0] == TEXT("refill")) Refill();
    else if (Args[0] == TEXT("reset")) ResetPosition();
    else if (Args[0] == TEXT("audit")) StartSmokeTest();
    else if (Args[0] == TEXT("probe")) StartActionProbe();
    else if (Args[0] == TEXT("select") && Args.Num() > 1) SelectWeapon(FCString::Atoi(*Args[1]) - 1);
}
