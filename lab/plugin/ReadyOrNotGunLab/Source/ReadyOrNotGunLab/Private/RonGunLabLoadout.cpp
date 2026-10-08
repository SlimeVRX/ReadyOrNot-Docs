#include "RonGunLab.h"
#include "ReadyOrNot.h"
#include "Characters/PlayerCharacter.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/SWATArmour.h"
#include "Actors/Attachments/WeaponAttachment.h"
#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Actors/Triggers/LoadoutPortal.h"
#include "Components/InventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "ReadyOrNotGameState.h"
#include "Data/ItemData.h"
#include "lib/BpGameplayHelperLib.h"
#include "lib/ReadyOrNotLoadoutManager.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogRonGunLabLoadout, Log, All);

namespace
{
    bool ParseAttachmentType(const FString& Name, EWeaponAttachmentType& Type)
    {
        const TMap<FString, EWeaponAttachmentType> Types = {
            {TEXT("optics"), EWeaponAttachmentType::Optics}, {TEXT("scope"), EWeaponAttachmentType::Optics},
            {TEXT("muzzle"), EWeaponAttachmentType::Muzzle}, {TEXT("underbarrel"), EWeaponAttachmentType::Underbarrel},
            {TEXT("overbarrel"), EWeaponAttachmentType::Overbarrel}, {TEXT("stock"), EWeaponAttachmentType::Stock},
            {TEXT("grip"), EWeaponAttachmentType::Grip}, {TEXT("illuminator"), EWeaponAttachmentType::Illuminators},
            {TEXT("ammunition"), EWeaponAttachmentType::Ammunition}
        };
        if (const EWeaponAttachmentType* Found = Types.Find(Name.ToLower())) { Type = *Found; return true; }
        return false;
    }

    void RemoveNativeAttachment(ABaseWeapon* Weapon, EWeaponAttachmentType Type)
    {
        Weapon->RemoveAttachment(Type == EWeaponAttachmentType::Optics, Type == EWeaponAttachmentType::Muzzle,
            Type == EWeaponAttachmentType::Underbarrel, Type == EWeaponAttachmentType::Overbarrel,
            Type == EWeaponAttachmentType::Stock, Type == EWeaponAttachmentType::Grip,
            Type == EWeaponAttachmentType::Illuminators, Type == EWeaponAttachmentType::Ammunition);
        Weapon->OnRep_AttachmentRep();
    }

    FString ObjectPath(const UObject* Object) { return IsValid(Object) ? Object->GetClass()->GetPathName() : TEXT(""); }

    bool HasNativeWidget(UWorld* World, UClass* WidgetClass)
    {
        if (!WidgetClass) return false;
        for (TObjectIterator<UUserWidget> It; It; ++It)
            if (It->GetWorld() == World && It->GetClass() == WidgetClass && It->IsInViewport()) return true;
        return false;
    }

    void ConfigureNativeReticle(UWorld* World, UClass* WidgetClass)
    {
        // The original debug overlay uses full-screen CrosshairDebug artwork.
        // Keep its authored widget tree, but display the game's existing HUD dot
        // at a practical size. This affects presentation only; not aim or traces.
        UTexture2D* Reticle = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReadyOrNot/UI/HUD_Revised/HUD_Reticle.HUD_Reticle"));
        for (TObjectIterator<UUserWidget> It; It; ++It)
        {
            if (It->GetWorld() != World || It->GetClass() != WidgetClass || !It->IsInViewport()) continue;
            UImage* Image = Cast<UImage>(It->GetWidgetFromName(TEXT("Image_0")));
            if (!Reticle || !Image)
            {
                // An unavailable HUD asset must not leave giant debug artwork on screen.
                It->RemoveFromParent();
                UE_LOG(LogRonGunLabLoadout, Warning, TEXT("Native reticle texture or Image_0 missing; no usable crosshair claimed"));
                continue;
            }
            Image->SetBrushFromTexture(Reticle, false);
            Image->SetDesiredSizeOverride(FVector2D(8, 8));
            Image->SetColorAndOpacity(FLinearColor::White);
            It->SetPositionInViewport(FVector2D::ZeroVector, false);
            It->SetDesiredSizeInViewport(FVector2D(8, 8));
            // Position and size helpers reset anchors to top-left in UE5.3.
            It->SetAnchorsInViewport(FAnchors(0.5f, 0.5f));
            It->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
            It->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

void ARonGunLab::EnsureNativeCrosshair()
{
    APlayerCharacter* Player = GetNativePlayer();
    UClass* WidgetClass = UBpGameplayHelperLib::GetWidgetDataFromLookupData(TEXT("CrossHairOverlay"), false).WidgetClass;
    if (Player && WidgetClass && !HasNativeWidget(GetWorld(), WidgetClass)) Player->ToggleCrosshairOverlay();
    if (Player && WidgetClass) ConfigureNativeReticle(GetWorld(), WidgetClass);
}

UReadyOrNotLoadoutManager* ARonGunLab::GetSessionLoadoutManager()
{
    if (!SessionLoadoutManager)
    {
        SessionLoadoutManager = NewObject<UReadyOrNotLoadoutManager>(this);
        SessionLoadoutManager->Initialize(GetWorld());
    }
    return SessionLoadoutManager;
}

bool ARonGunLab::IsNativeLoadoutWeapon(const ABaseMagazineWeapon* Weapon) const
{
    const APlayerCharacter* Player = GetNativePlayer();
    if (!IsValid(Weapon) || !Player) return false;
    const FSpawnedGear& Gear = Player->GetInventoryComponent()->GetSpawnedGear();
    return Weapon == Gear.Primary || Weapon == Gear.Secondary || Weapon == Gear.LongTactical || Gear.TacticalDevices.Contains(Weapon);
}

bool ARonGunLab::ShouldUseNativeLoadout(UClass* WeaponClass)
{
    const ABaseMagazineWeapon* Defaults = WeaponClass ? WeaponClass->GetDefaultObject<ABaseMagazineWeapon>() : nullptr;
    if (!Defaults) { SelectionRouteReason = TEXT("not_a_native_magazine_weapon"); return false; }
    if (Defaults->GetAmmunitionTypes().IsEmpty())
    { SelectionRouteReason = TEXT("missing_authored_ammunition_types; catalog inspection only"); return false; }
    UReadyOrNotLoadoutManager* Manager = GetSessionLoadoutManager();
    if (Manager->GetUsableAmmoTypes(Defaults).IsEmpty())
    { SelectionRouteReason = TEXT("no_native_player_usable_ammunition; catalog inspection only"); return false; }
    // ItemCategories is a legacy grouping. The actual loadout UI is populated
    // by LoadoutManager's CategoryFlags lists, after visibility/DLC filtering.
    const bool bPrimary = Manager->GetItemsByLoadoutCategory(ELoadoutCategory::Primary).Contains(Defaults);
    const bool bSecondary = Manager->GetItemsByLoadoutCategory(ELoadoutCategory::Secondary).Contains(Defaults);
    if (!Defaults->bShowInLoadout || (!bPrimary && !bSecondary))
    { SelectionRouteReason = TEXT("not_in_native_primary_or_secondary_menu; catalog inspection only"); return false; }
    SelectionRouteReason = bSecondary ? TEXT("native_secondary_menu_with_authored_player_ammo") : TEXT("native_primary_menu_with_authored_player_ammo");
    return true;
}

bool ARonGunLab::ApplyNativeLoadoutSelection(UClass* WeaponClass, APlayerCharacter* Player)
{
    ABaseMagazineWeapon* Defaults = WeaponClass->GetDefaultObject<ABaseMagazineWeapon>();
    UInventoryComponent* Inventory = Player->GetInventoryComponent();
    UReadyOrNotLoadoutManager* Manager = GetSessionLoadoutManager();
    const bool bSecondary = Manager->GetItemsByLoadoutCategory(ELoadoutCategory::Secondary).Contains(Defaults);
    Manager->SetActiveLoadout(Inventory->GetLastEquippedLoadout());
    // These native setters keep authored ammunition choices valid for the new weapon.
    if (bSecondary) Manager->SetActiveSecondary(WeaponClass);
    else Manager->SetActivePrimary(WeaponClass);
    for (uint8 Value = static_cast<uint8>(EWeaponAttachmentType::Optics); Value <= static_cast<uint8>(EWeaponAttachmentType::Ammunition); ++Value)
    {
        const EWeaponAttachmentType Type = static_cast<EWeaponAttachmentType>(Value);
        const TSubclassOf<UWeaponAttachment> Choice = bSecondary ? Manager->GetActiveSecondaryAttachmentByType(Type) : Manager->GetActivePrimaryAttachmentByType(Type);
        if (Choice && !Defaults->CanAddAttachment(Choice))
        {
            if (bSecondary) Manager->SetSecondaryAttachment(nullptr, Type);
            else Manager->SetPrimaryAttachment(nullptr, Type);
        }
    }
    FSavedLoadout Loadout = Manager->GetActiveLoadout();
    // A catalog change is the native workbench's new-loadout operation. Normal
    // 1/2 switching uses the retained primary/secondary and never resets ammunition.
    // The helper recreates tactical arrays, so follow RequestNewLoadout's cleanup.
    FLoadoutEquipOptions Options;
    Options.EquipItemCategory = bSecondary ? EItemCategory::IC_Secondary : EItemCategory::IC_Primary;
    Options.bSanitizeLoadout = true;
    UBpGameplayHelperLib::SanitizeLoadout(Loadout);
    if ((bSecondary ? Loadout.Secondary.Get() : Loadout.Primary.Get()) != WeaponClass)
    {
        WriteReceipt(TEXT("loadout_rejected"), TEXT("Native loadout sanitizer replaced the requested class; select a playable class or use the explicit catalog asset path"));
        return false;
    }
    // Native setters only replace existing ammo rows. A previous invalid asset
    // can leave zero slots which later valid weapons would otherwise inherit.
    // Supply finite, authored loadout ammunition at this workbench operation;
    // ordinary 1/2 swaps never execute this path.
    const ASWATArmour* Armour = Cast<ASWATArmour>(Loadout.Armor.GetDefaultObject());
    const UItemData* ItemData = UBpGameplayHelperLib::GetItemData(GetWorld());
    bool bRepairedEmptyAmmoSlots = false;
    auto NormalizeAmmoSlots = [&](bool bSecondarySlot, bool bRestoreEmptyCount)
    {
        const ABaseWeapon* SlotWeapon = Cast<ABaseWeapon>(bSecondarySlot ? Loadout.Secondary.GetDefaultObject() : Loadout.Primary.GetDefaultObject());
        const TArray<FName> UsableAmmo = Manager->GetUsableAmmoTypes(SlotWeapon);
        if (UsableAmmo.IsEmpty()) return;
        int32& Count = bSecondarySlot ? Loadout.SecondaryAmmoSlotsCount : Loadout.PrimaryAmmoSlotsCount;
        TArray<FName>& Slots = bSecondarySlot ? Loadout.SecondaryAmmoSlots : Loadout.PrimaryAmmoSlots;
        if (bRestoreEmptyCount && Count <= 0)
        {
            int32 NativeDefault = Armour ? (bSecondarySlot ? Armour->DefaultSecondaryAmmoSlots : Armour->DefaultPrimaryAmmoSlots) : -1;
            if (NativeDefault <= 0 && ItemData && ItemData->DefaultLoadouts.IsValidIndex(0))
                NativeDefault = bSecondarySlot ? ItemData->DefaultLoadouts[0].SecondaryAmmoSlotsCount : ItemData->DefaultLoadouts[0].PrimaryAmmoSlotsCount;
            if (NativeDefault > 0) { Count = NativeDefault; bRepairedEmptyAmmoSlots = true; }
        }
        Slots.SetNum(FMath::Clamp(Count, 0, 255));
        for (FName& Ammo : Slots) if (!UsableAmmo.Contains(Ammo)) Ammo = UsableAmmo[0];
    };
    NormalizeAmmoSlots(false, true);
    NormalizeAmmoSlots(true, true);
    // Preserve the host game's armour/carrying-capacity constraints, then make
    // row-array lengths agree with any slot counts chosen by its sanitizer.
    UBpGameplayHelperLib::SanitizeLoadout(Loadout);
    NormalizeAmmoSlots(false, false);
    NormalizeAmmoSlots(true, false);
    if ((bSecondary ? Loadout.SecondaryAmmoSlotsCount : Loadout.PrimaryAmmoSlotsCount) <= 0)
    {
        SelectionRouteReason = TEXT("native_loadout_has_no_ammo_capacity_after_sanitize");
        WriteReceipt(TEXT("loadout_rejected"), TEXT("Native armour/default loadout could not provide ammunition slots; previous equipment retained"));
        return false;
    }
    if (bRepairedEmptyAmmoSlots) SelectionRouteReason += TEXT("; restored_empty_slots_from_native_defaults");
    // Invalidate our old references before the native helper destroys their actors.
    CurrentWeapon = nullptr;
    PreviousWeapon = nullptr;
    ActiveWeaponIndex = INDEX_NONE;
    Inventory->DestroyAllEquippedItems();
    if (!UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, Player, Options))
    {
        WriteReceipt(TEXT("loadout_failed"), TEXT("Native EquipLoadoutOnPlayer failed"));
        return false;
    }
    // Match the runtime PlayerState with the runtime inventory. This RPC does not
    // save a preset/profile; native ammo stations read this in-memory loadout.
    if (AReadyOrNotPlayerState* PS = Player->GetPlayerState<AReadyOrNotPlayerState>())
        PS->Server_SetLoadout(Inventory->GetLastEquippedLoadout());
    CurrentWeapon = Cast<ABaseMagazineWeapon>(bSecondary ? Inventory->GetSpawnedGear().Secondary : Inventory->GetSpawnedGear().Primary);
    if (!IsValid(CurrentWeapon) || CurrentWeapon->GetClass() != WeaponClass)
    {
        CurrentWeapon = nullptr;
        WriteReceipt(TEXT("loadout_identity_mismatch"), TEXT("Native loadout slot did not contain the requested class"));
        return false;
    }
    bLastNativeLoadout = true;
    bLastInstantFallback = false;
    bPendingEquip = true;
    PendingSeconds = 0;
    StatusText = FString::Printf(TEXT("Native %s loadout: %s | 1/2 keep both slots | ronlab loadout"),
        bSecondary ? TEXT("secondary") : TEXT("primary"), *CurrentWeapon->ItemName.ToString());
    return true;
}

void ARonGunLab::ReconcileNativeSelection()
{
    if (bPendingEquip || bSmoke || ProbeStage || bProbeOnEquip) return;
    APlayerCharacter* Player = GetNativePlayer();
    if (!Player || Player->IsAnimationBlocking()) return;
    ABaseMagazineWeapon* Held = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem());
    if (!IsValid(Held)) { CurrentWeapon = nullptr; ActiveWeaponIndex = INDEX_NONE; return; }
    if (Held == CurrentWeapon && IsValid(CurrentWeapon)) return;
    if (IsValid(CurrentWeapon) && !IsNativeLoadoutWeapon(CurrentWeapon))
        Player->GetInventoryComponent()->DestroyInventoryItem(CurrentWeapon);
    CurrentWeapon = Held;
    ActiveWeaponIndex = INDEX_NONE;
    for (int32 Index = 0; Index < WeaponClasses.Num(); ++Index)
    {
        if (WeaponClasses[Index].ToString() == Held->GetClass()->GetPathName())
        {
            CurrentIndex = ActiveWeaponIndex = Index;
            break;
        }
    }
    bLastNativeLoadout = IsNativeLoadoutWeapon(Held);
    SelectionRouteReason = bLastNativeLoadout ? TEXT("equipped_existing_native_inventory_slot") : TEXT("equipped_existing_catalog_asset");
    bLastInstantFallback = false;
    StatusText = TEXT("Native inventory: ") + Held->ItemName.ToString();
}

void ARonGunLab::AppendNativeReadiness(const TSharedPtr<FJsonObject>& Row) const
{
    APlayerCharacter* Player = GetNativePlayer();
    if (!Player) return;
    UInventoryComponent* Inventory = Player->GetInventoryComponent();
    const FSpawnedGear& Gear = Inventory->GetSpawnedGear();
    Row->SetStringField(TEXT("selection_route"), bLastNativeLoadout ? TEXT("native_loadout") : TEXT("catalog_asset"));
    Row->SetStringField(TEXT("selection_route_reason"), SelectionRouteReason);
    const FSavedLoadout& LastLoadout = Inventory->GetLastEquippedLoadout();
    Row->SetNumberField(TEXT("native_primary_ammo_slot_count"), LastLoadout.PrimaryAmmoSlotsCount);
    Row->SetNumberField(TEXT("native_secondary_ammo_slot_count"), LastLoadout.SecondaryAmmoSlotsCount);
    Row->SetStringField(TEXT("native_primary"), ObjectPath(Gear.Primary));
    Row->SetStringField(TEXT("native_secondary"), ObjectPath(Gear.Secondary));
    Row->SetStringField(TEXT("native_armor"), ObjectPath(Gear.Armor));
    Row->SetStringField(TEXT("native_helmet"), ObjectPath(Gear.Helmet));
    Row->SetStringField(TEXT("native_long_tactical"), ObjectPath(Gear.LongTactical));
    Row->SetNumberField(TEXT("native_tactical_device_count"), Gear.TacticalDevices.Num());
    Row->SetNumberField(TEXT("native_grenade_count"), Gear.Grenades.Num());
    Row->SetNumberField(TEXT("native_inventory_count"), Inventory->GetInventoryItems().Num());
    Row->SetStringField(TEXT("pawn_class"), Player->GetClass()->GetPathName());
    Row->SetStringField(TEXT("controller_class"), ObjectPath(Player->GetController()));
    Row->SetStringField(TEXT("player_state_class"), ObjectPath(Player->GetPlayerState()));
    Row->SetBoolField(TEXT("native_hud_widget"), Player->HumanCharacterWidget_V2 != nullptr);
    UClass* CrosshairClass = UBpGameplayHelperLib::GetWidgetDataFromLookupData(TEXT("CrossHairOverlay"), false).WidgetClass;
    Row->SetBoolField(TEXT("native_crosshair_overlay_present"), HasNativeWidget(GetWorld(), CrosshairClass));
    Row->SetStringField(TEXT("native_crosshair_overlay_class"), CrosshairClass ? CrosshairClass->GetPathName() : TEXT(""));
    USkeletalMeshComponent* FP = Player->GetMesh1P();
    Row->SetStringField(TEXT("fp_anim_instance"), FP ? ObjectPath(FP->GetAnimInstance()) : TEXT(""));
    Row->SetStringField(TEXT("fp_skeletal_mesh"), FP && FP->GetSkeletalMeshAsset() ? FP->GetSkeletalMeshAsset()->GetPathName() : TEXT(""));
    UCameraComponent* Camera = Player->GetFirstPersonCameraComponent();
    Row->SetBoolField(TEXT("native_fp_camera_active"), Camera && Camera->IsActive());
    Row->SetNumberField(TEXT("native_fp_camera_fov"), Camera ? Camera->FieldOfView : 0);
    if (ABaseMagazineWeapon* Held = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem()))
    {
        Row->SetStringField(TEXT("held_class"), Held->GetClass()->GetPathName());
        Row->SetBoolField(TEXT("held_is_native_loadout_slot"), IsNativeLoadoutWeapon(Held));
        Row->SetStringField(TEXT("ammunition_row"), Held->GetCurrentAmmoTypeRowName().ToString());
        Row->SetNumberField(TEXT("authored_ammunition_type_count"), Held->GetAmmunitionTypes().Num());
        Row->SetNumberField(TEXT("native_player_usable_ammunition_type_count"), SessionLoadoutManager ? SessionLoadoutManager->GetUsableAmmoTypes(Held).Num() : -1);
        Row->SetBoolField(TEXT("animation_data_present"), Held->AnimationData != nullptr);
        Row->SetBoolField(TEXT("sound_data_present"), Held->SoundData != nullptr);
        Row->SetNumberField(TEXT("fire_mode"), static_cast<int32>(Held->CurrentFireMode));
        Row->SetNumberField(TEXT("available_fire_mode_count"), Held->AvailableFireModes.Num());
        TArray<UWeaponAttachment*> Attachments;
        Held->GetComponents(Attachments);
        TArray<TSharedPtr<FJsonValue>> AttachmentPaths;
        for (const UWeaponAttachment* Attachment : Attachments)
            if (IsValid(Attachment)) AttachmentPaths.Add(MakeShared<FJsonValueString>(Attachment->GetClass()->GetPathName()));
        Row->SetArrayField(TEXT("native_attachment_classes"), AttachmentPaths);
    }
}

bool ARonGunLab::HandleLoadoutCommand(const TArray<FString>& Args)
{
    const FString Verb = Args[0].ToLower();
    if (Verb != TEXT("primary") && Verb != TEXT("secondary") && Verb != TEXT("loadout") && Verb != TEXT("status") && Verb != TEXT("crosshair")
        && Verb != TEXT("ammo") && Verb != TEXT("attachment") && Verb != TEXT("attachments")) return false;
    APlayerCharacter* Player = GetNativePlayer();
    if (!Player || bPendingEquip || Player->IsAnimationBlocking())
    { StatusText = TEXT("Wait for native player/equip/animation before changing loadout"); return true; }
    if (Verb == TEXT("primary")) { Player->EquipPrimaryItem(); return true; }
    if (Verb == TEXT("secondary")) { Player->EquipSecondaryItem(); return true; }
    if (Verb == TEXT("crosshair"))
    {
        UClass* WidgetClass = UBpGameplayHelperLib::GetWidgetDataFromLookupData(TEXT("CrossHairOverlay"), false).WidgetClass;
        if (HasNativeWidget(GetWorld(), WidgetClass)) Player->ToggleCrosshairOverlay();
        else EnsureNativeCrosshair();
        return true;
    }
    if (Verb == TEXT("status")) { WriteReceipt(TEXT("readiness"), TEXT("Native runtime state snapshot; presence is not audiovisual/feel verification")); return true; }
    if (Verb == TEXT("loadout"))
    {
        AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>();
        if (!GS || GS->SubPreMissionPlanningLevel.IsNull() || GS->PreMissionStreamedLevel || GS->Loadout_V2)
        { StatusText = TEXT("Native loadout scene unavailable or already open"); return true; }
        for (TActorIterator<ALoadoutPortal> It(GetWorld()); It; ++It)
        {
            Player->EndPrimaryUse(); Player->EndSecondaryUse();
            StatusText = TEXT("Opening original ReadyOrNot loadout UI (native UI saves your selections when closed)");
            It->LoadLoadout(); return true;
        }
        StatusText = TEXT("Place the native LoadoutPortal station in this map first");
        return true;
    }
    ABaseMagazineWeapon* Held = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem());
    if (!IsValid(Held) || !Player->HasAuthority()) { StatusText = TEXT("Hold a native weapon on the authority to configure it"); return true; }
    UInventoryComponent* Inventory = Player->GetInventoryComponent();
    UReadyOrNotLoadoutManager* Manager = GetSessionLoadoutManager();
    Manager->SetActiveLoadout(Inventory->GetLastEquippedLoadout());
    const bool bPrimary = Held == Inventory->GetSpawnedGear().Primary;
    const bool bSecondary = Held == Inventory->GetSpawnedGear().Secondary;
    if (Verb == TEXT("ammo"))
    {
        const TArray<FName> Choices = Manager->GetUsableAmmoTypes(Held);
        if (Args.Num() < 2)
        {
            StatusText = TEXT("ronlab ammo N supplies authored ammo: ");
            for (int32 i = 0; i < Choices.Num(); ++i) StatusText += FString::Printf(TEXT("%d=%s  "), i + 1, *Choices[i].ToString());
            UE_LOG(LogRonGunLabLoadout, Display, TEXT("%s"), *StatusText);
            return true;
        }
        const int32 Index = FCString::Atoi(*Args[1]) - 1;
        if (!Choices.IsValidIndex(Index)) { StatusText = TEXT("Unknown ammo option; use ronlab ammo"); return true; }
        FSavedLoadout Loadout = Manager->GetActiveLoadout();
        const int32 Count = FMath::Max(1, bPrimary ? Loadout.PrimaryAmmoSlotsCount : bSecondary ? Loadout.SecondaryAmmoSlotsCount : SuppliedMagazines);
        TArray<FName> Ammo; Ammo.Init(Choices[Index], Count);
        Held->SetMagazineCount(Count, Ammo);
        if (bPrimary) Loadout.PrimaryAmmoSlots = Ammo;
        if (bSecondary) Loadout.SecondaryAmmoSlots = Ammo;
        Inventory->SetLastEquippedLoadout(Loadout);
        if (AReadyOrNotPlayerState* PS = Player->GetPlayerState<AReadyOrNotPlayerState>()) PS->Server_SetLoadout(Loadout);
        StatusText = TEXT("Supplied native ammo: ") + Choices[Index].ToString() + TEXT(" (session only)");
        return true;
    }
    EWeaponAttachmentType Type;
    if (Args.Num() < 2 || !ParseAttachmentType(Args[1], Type))
    { StatusText = TEXT("Types: optics, muzzle, underbarrel, overbarrel, stock, grip, illuminator, ammunition"); return true; }
    const TArray<TSubclassOf<UWeaponAttachment>> Choices = Manager->GetAttachmentByWeaponAndType(Held, Type);
    if (Verb == TEXT("attachments") || Args.Num() < 3)
    {
        StatusText = TEXT("ronlab attachment ") + Args[1] + TEXT(" N | 0=remove | ");
        for (int32 i = 0; i < Choices.Num(); ++i)
            if (Choices[i]) StatusText += FString::Printf(TEXT("%d=%s  "), i + 1, *Choices[i].GetDefaultObject()->ItemName.ToString());
        UE_LOG(LogRonGunLabLoadout, Display, TEXT("%s"), *StatusText);
        return true;
    }
    if (!Args[2].IsNumeric()) { StatusText = TEXT("Attachment option must be a number"); return true; }
    const int32 Index = FCString::Atoi(*Args[2]) - 1;
    TSubclassOf<UWeaponAttachment> Choice;
    if (Index != INDEX_NONE)
    {
        if (!Choices.IsValidIndex(Index) || !Choices[Index] || !Held->CanAddAttachment(Choices[Index]))
        { StatusText = TEXT("Native weapon rejected attachment option/socket"); return true; }
        Choice = Choices[Index];
        Held->AddAttachment(Choice);
    }
    else RemoveNativeAttachment(Held, Type);
    if (bPrimary) Manager->SetPrimaryAttachment(Choice, Type);
    if (bSecondary) Manager->SetSecondaryAttachment(Choice, Type);
    Inventory->SetLastEquippedLoadout(Manager->GetActiveLoadout());
    if (AReadyOrNotPlayerState* PS = Player->GetPlayerState<AReadyOrNotPlayerState>()) PS->Server_SetLoadout(Manager->GetActiveLoadout());
    StatusText = TEXT("Native attachment applied to current weapon (session only)");
    return true;
}
