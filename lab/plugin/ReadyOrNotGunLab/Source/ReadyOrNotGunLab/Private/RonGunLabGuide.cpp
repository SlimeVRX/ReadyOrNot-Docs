#include "RonGunLabGuide.h"
#include "Characters/PlayerCharacter.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Engine/Engine.h"
#include "CoreGlobals.h"
#include "ReadyOrNotGameState.h"
#include "lib/BpGameplayHelperLib.h"
#include "UObject/UObjectIterator.h"
namespace
{
FString ActionKeys(APlayerController* PC, const FName Action)
{
    TArray<FString> Keys;
    if (PC && PC->PlayerInput)
        for (const FInputActionKeyMapping& Binding : PC->PlayerInput->GetKeysForAction(Action))
            if (Binding.Key.IsValid() && !Binding.Key.IsGamepadKey()) Keys.AddUnique(Binding.Key.GetDisplayName().ToString());
    return Keys.Num() ? FString::Join(Keys, TEXT(" / ")) : TEXT("chưa gán phím");
}
FString ModeName(EFireMode Mode)
{
    FString Name = StaticEnum<EFireMode>()->GetNameStringByValue(static_cast<int64>(Mode));
    Name.RemoveFromStart(TEXT("FM_")); return Name;
}
}
void URonGunLabGuideWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;
    auto Panel = [&](UTextBlock*& Text, FVector2D Position, FVector2D Size)
    {
        UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
        Border->SetBrushColor(FLinearColor(0.012f, 0.025f, 0.033f, 0.91f));
        Border->SetPadding(FMargin(16, 12));
        UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Border);
        Slot->SetPosition(Position); Slot->SetSize(Size);
        Text = WidgetTree->ConstructWidget<UTextBlock>();
        FSlateFontInfo Font = Text->GetFont(); Font.Size = 16; Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.94f, 0.93f, 1)));
        Text->SetAutoWrapText(true); Border->SetContent(Text); return Border;
    };
    Panel(Status, FVector2D(22, 28), FVector2D(690, 110));
    HelpPanel = Panel(Help, FVector2D(22, 150), FVector2D(840, 730));
    FSlateFontInfo HelpFont = Help->GetFont(); HelpFont.Size = 14; Help->SetFont(HelpFont);
    HelpPanel->SetVisibility(ESlateVisibility::Collapsed);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
void URonGunLabGuideWidget::Refresh(APlayerCharacter* Player, bool bExpanded)
{
    if (!Status || !Player) return;
    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    FString Summary = TEXT("READY OR NOT · GUN LAB     [F1] Hướng dẫn / checklist\n");
    if (ABaseMagazineWeapon* Weapon = Cast<ABaseMagazineWeapon>(Player->GetEquippedItem()))
    {
        TArray<FString> Modes;
        for (EFireMode Mode : Weapon->AvailableFireModes) Modes.Add(ModeName(Mode));
        Summary += FString::Printf(TEXT("%s  |  %s  |  Có: %s\n%s: nhấn rồi nhả nhanh để đổi chế độ · F5/F6: đổi loadout"),
            *Weapon->ItemName.ToString(), *ModeName(Weapon->CurrentFireMode), *FString::Join(Modes, TEXT(" / ")), *ActionKeys(PC, TEXT("FireSelect")));
    }
    else Summary += TEXT("Đang dùng trang bị native. Phím 1/2 trở lại primary / secondary.");
    Status->SetText(FText::FromString(Summary));
    HelpPanel->SetVisibility(bExpanded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (!bExpanded) return;
    FString Body = TEXT("ĐIỀU KHIỂN NATIVE — phím đọc từ PlayerInput của phiên này\n\n");
    auto Line = [&](const TCHAR* Action, const TCHAR* Description) { Body += ActionKeys(PC, Action) + TEXT("  —  ") + Description + TEXT("\n"); };
    Line(TEXT("Fire"), TEXT("Bắn; Auto/Burst chỉ có trên các khẩu hỗ trợ"));
    Line(TEXT("SecondaryUse"), TEXT("Ngắm ADS"));
    Line(TEXT("FireSelect"), TEXT("Đổi chế độ: nhấn + nhả nhanh, không giữ"));
    Line(TEXT("Reload/MagCheck"), TEXT("Chạm: tactical reload · nhấn đúp: speed reload · giữ: kiểm tra đạn"));
    Line(TEXT("DrawPrimary"), TEXT("Primary — giữ ammo và attachment trong slot"));
    Line(TEXT("DrawSecondary"), TEXT("Secondary — đổi lại bằng phím primary"));
    Line(TEXT("Walk"), TEXT("Giữ: đi chậm (96 so với 240 cm/s); snapshot không có sprint"));
    Line(TEXT("Crouch"), TEXT("Giữ: ngồi; thả: đứng. Snapshot này không có prone"));
    Line(TEXT("LowReadyToggle"), TEXT("Hạ / nâng súng"));
    Line(TEXT("FreeLean"), TEXT("Giữ + WASD: free lean; Q/E mặc định nghiêng trái/phải"));
    Line(TEXT("FreeLook"), TEXT("Nhìn tự do"));
    Line(TEXT("ToggleCantedSight"), TEXT("Canted aim"));
    Line(TEXT("ToggleSecondarySight"), TEXT("Kính phụ nếu attachment có hỗ trợ"));
    Line(TEXT("ToggleUnderbarrel"), TEXT("Bật attachment phù hợp (đèn/laser tùy cấu hình)"));
    Body += TEXT("\nWORKBENCH & CHECKLIST\nF5/F6: thay súng · F7: cấp đạn · F8: về vạch bắn\nTủ loadout: UI native; UI này lưu preset như game.\nConsole: ronlab attachments optics | ronlab attachment optics 1\nronlab ammo | ronlab ammo 1 | ronlab crosshair | ronlab_targets reset\n\nThử: semi/burst/auto → hip/ADS/canted → recoil khi di chuyển →\nmag check / tactical / speed / empty reload → primary/secondary →\nattachment → vật liệu / target có health / cửa → âm thanh.\nMục không hỗ trợ trên súng hiện tại không được coi là lỗi lab.");
    Help->SetText(FText::FromString(Body));
}
ARonGunLabGuide::ARonGunLabGuide() { PrimaryActorTick.bCanEverTick = true; }
void ARonGunLabGuide::SetReticleHiddenForLoadout(bool bHide)
{
    if (!bHide)
    {
        // Restore the same widget's prior visibility; never recreate a reticle
        // the player may have disabled while the menu was open.
        if (UUserWidget* Reticle = HiddenReticle.Get()) Reticle->SetVisibility(PreviousReticleVisibility);
        HiddenReticle.Reset();
        return;
    }
    if (HiddenReticle.IsValid() && HiddenReticle->IsInViewport())
    {
        HiddenReticle->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    HiddenReticle.Reset();
    UClass* ReticleClass = UBpGameplayHelperLib::GetWidgetDataFromLookupData(TEXT("CrossHairOverlay"), false).WidgetClass;
    if (!ReticleClass) return;
    for (TObjectIterator<UUserWidget> It; It; ++It)
    {
        if (It->GetWorld() != GetWorld() || It->GetClass() != ReticleClass || !It->IsInViewport()) continue;
        PreviousReticleVisibility = It->GetVisibility();
        HiddenReticle = *It;
        It->SetVisibility(ESlateVisibility::Collapsed);
        break;
    }
}
void ARonGunLabGuide::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    APlayerCharacter* Player = PC ? Cast<APlayerCharacter>(PC->GetPawn()) : nullptr;
    if (!Player || !PC->IsLocalController()) return;
    if (!Guide)
    {
        // UE development builds bind F1/F5 to debug view modes. Reserve lab shortcuts
        // for this session only; never write the user's Input.ini or gameplay bindings.
        SessionInput = PC->PlayerInput;
        if (SessionInput)
            for (int32 Index = SessionInput->DebugExecBindings.Num() - 1; Index >= 0; --Index)
            {
                const FKeyBind& Binding = SessionInput->DebugExecBindings[Index];
                if (Binding.Key == EKeys::F1 || Binding.Key == EKeys::F5 || Binding.Key == EKeys::F6 || Binding.Key == EKeys::F7 || Binding.Key == EKeys::F8)
                { RemovedDebugBindings.Add(Binding); SessionInput->DebugExecBindings.RemoveAt(Index); }
            }
        if (!bScreenMessageStateSaved)
        {
            // DrawMapWarnings uses the same global as DisableAllScreenMessages,
            // independently of AddOnScreenDebugMessage. Keep both session-only.
            bPreviousScreenMessages = GAreScreenMessagesEnabled;
            GAreScreenMessagesEnabled = false;
            if (GEngine) { bPreviousDebugMessages = GEngine->bEnableOnScreenDebugMessages; GEngine->bEnableOnScreenDebugMessages = false; }
            bScreenMessageStateSaved = true;
        }
        Guide = CreateWidget<URonGunLabGuideWidget>(PC, URonGunLabGuideWidget::StaticClass());
        if (Guide) Guide->AddToViewport(50);
    }
    // The original menu draws a translucent background; a lab overlay behind it
    // would still obscure its controls. Let that native UI own the screen.
    AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>();
    const bool bNativeLoadoutOpen = GS && (GS->Loadout_V2 || GS->PreMissionStreamedLevel);
    SetReticleHiddenForLoadout(bNativeLoadoutOpen);
    if (bNativeLoadoutOpen)
    { if (Guide) Guide->SetVisibility(ESlateVisibility::Collapsed); return; }
    if (Guide) Guide->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (PC->WasInputKeyJustPressed(EKeys::F1)) { bExpanded = !bExpanded; RefreshTime = 0; }
    RefreshTime -= DeltaSeconds;
    if (Guide && RefreshTime <= 0) { Guide->Refresh(Player, bExpanded); RefreshTime = 0.15f; }
}
void ARonGunLabGuide::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetReticleHiddenForLoadout(false);
    if (Guide) Guide->RemoveFromParent();
    if (IsValid(SessionInput)) SessionInput->DebugExecBindings.Append(RemovedDebugBindings);
    if (bScreenMessageStateSaved)
    {
        GAreScreenMessagesEnabled = bPreviousScreenMessages;
        if (GEngine) GEngine->bEnableOnScreenDebugMessages = bPreviousDebugMessages;
    }
    Super::EndPlay(EndPlayReason);
}
