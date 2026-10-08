#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerInput.h"
#include "RonGunLabGuide.generated.h"
class UTextBlock;
class UBorder;
class APlayerCharacter;
/** Read-only help and native state display. No weapon mechanics. */
UCLASS()
class READYORNOTGUNLAB_API URonGunLabGuideWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeOnInitialized() override;
    void Refresh(APlayerCharacter* Player, bool bExpanded);
private:
    UPROPERTY() UTextBlock* Status = nullptr;
    UPROPERTY() UTextBlock* Help = nullptr;
    UPROPERTY() UBorder* HelpPanel = nullptr;
};
UCLASS()
class READYORNOTGUNLAB_API ARonGunLabGuide : public AActor
{
    GENERATED_BODY()
public:
    ARonGunLabGuide();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void SetReticleHiddenForLoadout(bool bHide);
    UPROPERTY() URonGunLabGuideWidget* Guide = nullptr;
    UPROPERTY() UPlayerInput* SessionInput = nullptr;
    TWeakObjectPtr<UUserWidget> HiddenReticle;
    ESlateVisibility PreviousReticleVisibility = ESlateVisibility::HitTestInvisible;
    TArray<FKeyBind> RemovedDebugBindings;
    bool bPreviousDebugMessages = true;
    bool bPreviousScreenMessages = true;
    bool bScreenMessageStateSaved = false;
    bool bExpanded = false;
    float RefreshTime = 0;
};
