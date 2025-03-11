#include "TargetingComponent.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

UTargetingComponent::UTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    TargetingRange = 1000.0f; // Default range, adjust as needed
}

void UTargetingComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTargetingComponent::CycleTarget()
{
    if (!SelectNewTarget())
    {
        // No valid target found, clear current target
        ClearCurrentTarget();
        return;
    }

    // Start or restart the validation timer (1s interval)
    GetWorld()->GetTimerManager().SetTimer(TargetValidationTimer, this, &UTargetingComponent::ValidateCurrentTarget, 1.0f, true);
}

bool UTargetingComponent::SelectNewTarget()
{
    AActor* NewTarget = FindBestTarget();
    
    if (!NewTarget)
    {
        return false;
    }

    if (CurrentTarget == NewTarget)
    {
        return true; // Already targeting this actor
    }

    // Set the new target
    CurrentTarget = NewTarget;

    // Attach or move Niagara effect
    if (!TargetEffect)
    {
        TargetEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
            LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/YourNiagaraEffect")), // Assign VFX location Here
            CurrentTarget->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeTransform,
            true
        );
    }
    else
    {
        TargetEffect->AttachToComponent(CurrentTarget->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    }

    return true;
}

void UTargetingComponent::ValidateCurrentTarget()
{
    if (!IsValidTarget(CurrentTarget))
    {
        ClearCurrentTarget();

        // Try selecting a new target
        if (!SelectNewTarget())
        {
            GetWorld()->GetTimerManager().ClearTimer(TargetValidationTimer);
        }
    }
}

AActor* UTargetingComponent::FindBestTarget()
{
    TArray<AActor*> OverlappingActors;
    FVector PlayerLocation = GetOwner()->GetActorLocation();
    FVector ForwardVector = GetOwner()->GetActorForwardVector();

    // Perform a sphere overlap to find potential targets
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape CollisionShape = FCollisionShape::MakeSphere(TargetingRange);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());

    bool bHasHit = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        PlayerLocation,
        FQuat::Identity,
        ECC_Pawn, // Adjust if necessary
        CollisionShape,
        QueryParams
    );

    if (!bHasHit)
    {
        return nullptr;
    }

    // Store valid targets
    TArray<AActor*> ValidTargets;
    for (const FOverlapResult& Result : OverlapResults)
    {
        AActor* Candidate = Result.GetActor();
        if (Candidate && Candidate->IsA(TargetClass) && IsValidTarget(Candidate))
        {
            ValidTargets.Add(Candidate);
        }
    }

    if (ValidTargets.Num() == 0)
    {
        return nullptr;
    }

    // If there's a current target, find the next one in the list
    if (CurrentTarget)
    {
        int32 Index = ValidTargets.Find(CurrentTarget);
        if (Index != INDEX_NONE && Index + 1 < ValidTargets.Num())
        {
            return ValidTargets[Index + 1]; // Cycle to the next target
        }
    }

    // If no valid cycle target, pick the best one (most in front)
    AActor* BestTarget = nullptr;
    float BestScore = -1.0f;

    for (AActor* Candidate : ValidTargets)
    {
        FVector DirectionToTarget = (Candidate->GetActorLocation() - PlayerLocation).GetSafeNormal();
        float DotProduct = FVector::DotProduct(ForwardVector, DirectionToTarget); // Higher = more in front

        if (DotProduct > BestScore)
        {
            BestScore = DotProduct;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

bool UTargetingComponent::IsValidTarget(AActor* Target) const
{
    if (!Target || !Target->IsValidLowLevel() || Target->IsPendingKill())
    {
        return false;
    }

    // Additional checks (e.g., is enemy, is alive, etc.)
    return true;
}

void UTargetingComponent::ClearCurrentTarget()
{
    if (TargetEffect)
    {
        TargetEffect->DestroyComponent();
        TargetEffect = nullptr;
    }

    CurrentTarget = nullptr;
    
    // Stop validation timer
    GetWorld()->GetTimerManager().ClearTimer(TargetValidationTimer);
}
