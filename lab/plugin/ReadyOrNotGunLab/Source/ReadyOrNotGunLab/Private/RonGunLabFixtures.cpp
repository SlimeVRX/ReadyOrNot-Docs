#include "RonGunLabFixtures.h"

#include "Actors/Gameplay/AISpawn.h"
#include "Characters/CyberneticCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Volume.h"
#include "Actors/BaseMagazineWeapon.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#if WITH_EDITOR
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#endif

ARonGunLabFixtures::ARonGunLabFixtures()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f;
}

void ARonGunLabFixtures::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority() && !IConsoleManager::Get().FindConsoleObject(TEXT("ronlab_targets")))
    {
        RegisteredCommand = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("ronlab_targets"), TEXT("ronlab_targets reset: respawn only the lab's native AI damage fixtures"),
            FConsoleCommandWithArgsDelegate::CreateUObject(this, &ARonGunLabFixtures::Command), ECVF_Default);
    }
    if (HasAuthority())
    {
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            if (It->ActorHasTag(TEXT("GunLabProceduralGlass")))
                It->OnTakePointDamage.AddUniqueDynamic(this, &ARonGunLabFixtures::OnNativeGlassPointDamage);
        }
    }
}

void ARonGunLabFixtures::OnNativeGlassPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
    FVector HitLocation, UPrimitiveComponent* HitComponent, FName BoneName,
    FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
    // The original procedural-glass asset listens to physical hit and radial
    // damage events. Native hitscan delivers point damage instead. Adapt only
    // that notification; the original Blueprint owns slicing, forces and sound.
    if (!HasAuthority() || Damage <= 0.0f || !IsValid(DamagedActor) || !IsValid(HitComponent) ||
        !Cast<ABaseMagazineWeapon>(DamageCauser) || !DamagedActor->ActorHasTag(TEXT("GunLabProceduralGlass")))
        return;
    UFunction* Function = DamagedActor->FindFunction(TEXT("OnGlassHit"));
    FStructProperty* HitProperty = Function ? FindFProperty<FStructProperty>(Function, TEXT("HitResult")) : nullptr;
    FObjectPropertyBase* CauserProperty = Function ? FindFProperty<FObjectPropertyBase>(Function, TEXT("DamageCauser")) : nullptr;
    if (!HitProperty || HitProperty->Struct != FHitResult::StaticStruct() || !CauserProperty || Function->NumParms != 2)
    {
        UE_LOG(LogTemp, Error, TEXT("GUNLAB_GLASS_ADAPTER_REJECTED unexpected original OnGlassHit signature"));
        return;
    }
    FHitResult NativeImpact(DamagedActor, HitComponent, HitLocation, ShotFromDirection);
    NativeImpact.bBlockingHit = true;
    NativeImpact.BoneName = BoneName;
    FStructOnScope Parameters(Function);
    *HitProperty->ContainerPtrToValuePtr<FHitResult>(Parameters.GetStructMemory()) = NativeImpact;
    CauserProperty->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), DamageCauser);
    DamagedActor->ProcessEvent(Function, Parameters.GetStructMemory());
    UE_LOG(LogTemp, Display, TEXT("GUNLAB_GLASS_NATIVE_POINT_FORWARDED actor=%s component=%s damage=%.3f"),
        *DamagedActor->GetName(), *HitComponent->GetName(), Damage);
}

void ARonGunLabFixtures::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || bInitialized)
        return;
    StartupSeconds += DeltaSeconds;
    if (StartupSeconds >= 5.0f)
    {
        bInitialized = true;
        EnsureTargets(false);
    }
}

void ARonGunLabFixtures::EnsureTargets(bool bReplaceExisting)
{
    if (!HasAuthority() || !GetWorld())
        return;
    int32 Ready = 0;
    int32 Failed = 0;
    for (TActorIterator<AAISpawn> It(GetWorld()); It; ++It)
    {
        AAISpawn* Spawner = *It;
        if (!Spawner->ActorHasTag(TEXT("GunLabDamageSpawner")))
            continue;
        if ((bReplaceExisting || !IsValid(Spawner->SpawnedCharacter)) && !Spawner->DoSpawn())
        {
            ++Failed;
            UE_LOG(LogTemp, Error, TEXT("GUNLAB_FIXTURE_FAILED spawner=%s"), *Spawner->GetName());
            continue;
        }
        if (ACyberneticCharacter* Character = Spawner->SpawnedCharacter)
        {
            Character->Tags.AddUnique(TEXT("GunLabDamageTarget"));
            for (const FName& Tag : Spawner->Tags)
            {
                if (Tag.ToString().StartsWith(TEXT("GunLabTarget")))
                    Character->Tags.AddUnique(Tag);
            }
            ++Ready;
            UE_LOG(LogTemp, Display, TEXT("GUNLAB_FIXTURE_READY spawner=%s character=%s class=%s row=%s armor=%s unarmed=%d deactivated=%d"),
                *Spawner->GetName(), *Character->GetName(), *Character->GetClass()->GetPathName(),
                *Spawner->SpawnData.SpawnedAI.RowName.ToString(), *Spawner->SpawnData.ForceBodyArmourOverride.ToString(),
                Spawner->SpawnData.bForceNoWeapon, Spawner->SpawnData.bDeactivated);
        }
        else
        {
            ++Failed;
        }
    }
    UE_LOG(LogTemp, Display, TEXT("GUNLAB_FIXTURES_READY ready=%d failed=%d native_spawners=1"), Ready, Failed);
}

void ARonGunLabFixtures::ResetTargets()
{
    EnsureTargets(true);
}

bool ARonGunLabFixtures::BuildEditorBoxVolume(AVolume* Volume, FVector FullSize)
{
#if WITH_EDITOR
    if (!Volume || !Volume->GetWorld() || Volume->GetWorld()->WorldType != EWorldType::Editor ||
        FullSize.X <= 0 || FullSize.Y <= 0 || FullSize.Z <= 0)
        return false;
    UCubeBuilder* Builder = NewObject<UCubeBuilder>();
    Builder->X = FullSize.X;
    Builder->Y = FullSize.Y;
    Builder->Z = FullSize.Z;
    UActorFactory::CreateBrushForVolumeActor(Volume, Builder);
    return Volume->GetBounds().BoxExtent.GetMin() > 0;
#else
    return false;
#endif
}

FString ARonGunLabFixtures::RestoreOriginalGlassEvents(UBlueprint* Blueprint)
{
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("success"), false);
    auto Finish = [&Report](const FString& Error) {
        if (!Error.IsEmpty()) Report->SetStringField(TEXT("error"), Error);
        FString Json; auto Writer = TJsonWriterFactory<>::Create(&Json);
        FJsonSerializer::Serialize(Report, Writer); return Json;
    };
#if WITH_EDITOR
    // Source assets are never accepted here. The Python authoring script first
    // duplicates the original Blueprint into this exact lab-owned package.
    if (!Blueprint || Blueprint->GetOutermost()->GetName() != TEXT("/Game/ReadyOrNot/Level/Study/GlassRecovery/BP_GunLab_ProceduralGlass"))
        return Finish(TEXT("Only the designated lab-owned duplicate may be repaired"));
    UEdGraph* Graph = nullptr;
    for (UEdGraph* Candidate : Blueprint->UbergraphPages)
        if (Candidate && Candidate->GetFName() == TEXT("EventGraph")) Graph = Candidate;
    if (!Graph) return Finish(TEXT("Original EventGraph missing"));
    auto Node = [Graph](const FName Name) -> UEdGraphNode* {
        for (UEdGraphNode* Candidate : Graph->Nodes)
            if (Candidate && Candidate->GetFName() == Name) return Candidate;
        return nullptr;
    };
    auto NamedProperty = [](UObject* Object, const FName Property, const FName Nested = NAME_None) -> FName {
        if (!Object) return NAME_None;
        if (Nested.IsNone()) {
            const FNameProperty* P = FindFProperty<FNameProperty>(Object->GetClass(), Property);
            return P ? P->GetPropertyValue_InContainer(Object) : NAME_None;
        }
        const FStructProperty* P = FindFProperty<FStructProperty>(Object->GetClass(), Property);
        const FNameProperty* N = P ? FindFProperty<FNameProperty>(P->Struct, Nested) : nullptr;
        return N ? N->GetPropertyValue_InContainer(P->ContainerPtrToValuePtr<void>(Object)) : NAME_None;
    };
    if (NamedProperty(Node(TEXT("K2Node_CustomEvent_0")), TEXT("CustomFunctionName")) != TEXT("OnGlassHit") ||
        NamedProperty(Node(TEXT("K2Node_VariableSet_2")), TEXT("VariableReference"), TEXT("MemberName")) != TEXT("LastHitData") ||
        NamedProperty(Node(TEXT("K2Node_ComponentBoundEvent_0")), TEXT("DelegatePropertyName")) != TEXT("OnComponentHit") ||
        NamedProperty(Node(TEXT("K2Node_ComponentBoundEvent_0")), TEXT("ComponentPropertyName")) != TEXT("ProceduralMesh") ||
        NamedProperty(Node(TEXT("K2Node_Event_1")), TEXT("EventReference"), TEXT("MemberName")) != TEXT("ReceiveRadialDamage") ||
        NamedProperty(Node(TEXT("K2Node_CallFunction_7")), TEXT("FunctionReference"), TEXT("MemberName")) != TEXT("OnGlassHit") ||
        NamedProperty(Node(TEXT("K2Node_CallFunction_5")), TEXT("FunctionReference"), TEXT("MemberName")) != TEXT("OnGlassHit"))
        return Finish(TEXT("Original event/function/property semantics changed; refusing repair"));
    struct FWire { FName From, To; FGuid OutId, InId; };
    const FWire Wires[] = {
        {TEXT("K2Node_CustomEvent_0"), TEXT("K2Node_VariableSet_2"), FGuid(0x6ba0f219,0x477c46f8,0xc3e39290,0xb315f4cc), FGuid(0xeeddc0e5,0x491fdf9e,0x7c2b1aba,0x9bbab1cf)},
        {TEXT("K2Node_ComponentBoundEvent_0"), TEXT("K2Node_CallFunction_7"), FGuid(0xc3792a4d,0x4389b10e,0xdea9fa84,0x7e07d290), FGuid(0x7086b1df,0x4424ad3c,0x9873c381,0x0a68412d)},
        {TEXT("K2Node_Event_1"), TEXT("K2Node_CallFunction_5"), FGuid(0xa47e041e,0x4fd43248,0xa545a78e,0xeb5e2396), FGuid(0x7086b1df,0x4424ad3c,0x9873c381,0x0a68412d)}
    };
    TArray<TPair<UEdGraphPin*, UEdGraphPin*>> Connections;
    TArray<TSharedPtr<FJsonValue>> Details;
    for (const FWire& Wire : Wires) {
        UEdGraphNode* From = Node(Wire.From); UEdGraphNode* To = Node(Wire.To);
        UEdGraphPin* Out = From ? From->FindPin(TEXT("then"), EGPD_Output) : nullptr;
        UEdGraphPin* In = To ? To->FindPin(TEXT("execute"), EGPD_Input) : nullptr;
        if (!Out || !In || Out->PinId != Wire.OutId || In->PinId != Wire.InId ||
            Out->PinType.PinCategory != TEXT("exec") || In->PinType.PinCategory != TEXT("exec"))
            return Finish(TEXT("Original execution pin identity/type changed; refusing repair"));
        const bool Empty = Out->LinkedTo.IsEmpty() && In->LinkedTo.IsEmpty();
        const bool AlreadyConnected = Out->LinkedTo.Num() == 1 && In->LinkedTo.Num() == 1 &&
            Out->LinkedTo[0] == In && In->LinkedTo[0] == Out;
        if (!Empty && !AlreadyConnected) return Finish(TEXT("Existing event wiring differs; refusing repair"));
        TSharedRef<FJsonObject> Detail = MakeShared<FJsonObject>();
        Detail->SetStringField(TEXT("from"), Wire.From.ToString() + TEXT(".then"));
        Detail->SetStringField(TEXT("to"), Wire.To.ToString() + TEXT(".execute"));
        Detail->SetStringField(TEXT("output_pin_guid"), Wire.OutId.ToString());
        Detail->SetStringField(TEXT("input_pin_guid"), Wire.InId.ToString());
        Detail->SetNumberField(TEXT("before_output_links"), Out->LinkedTo.Num());
        Detail->SetNumberField(TEXT("before_input_links"), In->LinkedTo.Num());
        Details.Add(MakeShared<FJsonValueObject>(Detail));
        Connections.Emplace(Out, In);
    }
    for (const auto& Connection : Connections)
        if (Connection.Key->LinkedTo.IsEmpty()) Connection.Key->MakeLinkTo(Connection.Value);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Results);
    Report->SetNumberField(TEXT("compiler_errors"), Results.NumErrors);
    Report->SetNumberField(TEXT("compiler_warnings"), Results.NumWarnings);
    Report->SetArrayField(TEXT("wires"), Details);
    if (Results.NumErrors > 0 || Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
        return Finish(TEXT("Duplicate Blueprint compilation failed; do not save or use it"));
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Wires); ++Index) {
        UEdGraphNode* From = Node(Wires[Index].From); UEdGraphNode* To = Node(Wires[Index].To);
        UEdGraphPin* Out = From ? From->FindPin(TEXT("then"), EGPD_Output) : nullptr;
        UEdGraphPin* In = To ? To->FindPin(TEXT("execute"), EGPD_Input) : nullptr;
        if (!Out || !In || Out->LinkedTo.Num() != 1 || In->LinkedTo.Num() != 1 || Out->LinkedTo[0] != In || In->LinkedTo[0] != Out)
            return Finish(TEXT("Restored connection failed readback; do not save or use duplicate"));
        Details[Index]->AsObject()->SetNumberField(TEXT("after_output_links"), 1);
        Details[Index]->AsObject()->SetNumberField(TEXT("after_input_links"), 1);
    }
    Report->SetBoolField(TEXT("success"), true);
    return Finish(TEXT(""));
#else
    return Finish(TEXT("Editor authoring helper unavailable in runtime builds"));
#endif
}

void ARonGunLabFixtures::Command(const TArray<FString>& Args)
{
    if (Args.Num() == 1 && Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
        ResetTargets();
    else
        UE_LOG(LogTemp, Display, TEXT("Usage: ronlab_targets reset"));
}

void ARonGunLabFixtures::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (RegisteredCommand)
    {
        IConsoleManager::Get().UnregisterConsoleObject(RegisteredCommand, false);
        RegisteredCommand = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
